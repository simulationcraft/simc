// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <cerrno>

#include "simulationcraft.hpp"

#include "dbc/azerite.hpp"

namespace
{
bool prune_specialized_execute_actions_internal( std::vector<action_t*>& apl, execute_type e,
  std::vector<std::vector<action_t*>*>& visited )
{
  if ( range::find( visited, &apl ) != visited.end() )
    return true;
  else
    visited.push_back( &apl );

  auto it = apl.begin();
  // Prune out any call/run/swap_action list call from the list of actions, if the called apl has no
  // actions of the required type.
  while ( it != apl.end() )
  {
    action_priority_list_t* alist = nullptr;
    action_t* action = *it;

    if ( action->type == ACTION_CALL )
    {
      alist = static_cast<const call_action_list_t*>( action )->alist;
    }
    else if ( util::str_compare_ci( action->name_str, "run_action_list" ) ||
              util::str_compare_ci( action->name_str, "swap_action_list" ) )
    {
      alist = static_cast<const swap_action_list_t*>( action )->alist;
    }

    if ( alist )
    {
      bool state;
      switch ( e )
      {
        case execute_type::OFF_GCD:
          alist->player->sim->print_debug( "{} traversing APL {}, n_actions={} ({})",
            alist->player->name(), alist->name_str, alist->off_gcd_actions.size(),
            e == execute_type::OFF_GCD ? "off-gcd" : "cast-while-casting" );
          state = prune_specialized_execute_actions_internal( alist->off_gcd_actions, e, visited );
          break;
        case execute_type::CAST_WHILE_CASTING:
          alist->player->sim->print_debug( "{} traversing APL {}, n_actions={} ({})",
            alist->player->name(), alist->name_str, alist->cast_while_casting_actions.size(),
            e == execute_type::OFF_GCD ? "off-gcd" : "cast-while-casting" );
          state = prune_specialized_execute_actions_internal( alist->cast_while_casting_actions, e, visited );
          break;
        default:
          state = false;
          break;
      }

      if ( !state )
      {
        alist->player->sim->print_debug( "{} pruning {} from APL ({})",
          alist->player->name(), (*it)->signature_str,
          e == execute_type::OFF_GCD ? "off-gcd" : "cast-while-casting" );
        it = apl.erase( it );
      }
      else
      {
        ++it;
      }
    }
    else
    {
      ++it;
    }
  }

  return apl.size() > 0;
}

bool prune_specialized_execute_actions( std::vector<action_t*>& apl, execute_type e )
{
  std::vector<std::vector<action_t*>*> visited;
  return prune_specialized_execute_actions_internal( apl, e, visited );
}

// Specialized execute for off-gcd and cast-while-casting execution of abilities. Both cases need to
// perform a subset of the normal foreground execution process, and also have to take into account
// execution-type specific bookkeeping on the actor level.
template <typename T>
struct special_execute_event_t : public player_event_t
{
  special_execute_event_t( player_t& p, timespan_t delta_time ) :
    player_event_t( p, delta_time )
  { }

  void execute() override
  {
    ptr() = nullptr;

    execute_action();

    // Create a new specialized execute check event only in the case we didnt find anything to queue
    // (could use an ability right away) and the action we executed was not a run_action_list.
    //
    // Note also that a new specialized execute check event may have been created in do_execute() if
    // this event execution found an off gcd action to execute, in this case, do not create new
    // event.
    reschedule();
  }

  // Select and execute a specialized action based on the execution type (currently off-gcd or cast
  // while casting)
  virtual void execute_action()
  {
    // Dynamically regenerating actors must be regenerated before selecting an action, otherwise
    // resource-specific expressions may not function properly.
    if ( p()->regen_type == REGEN_DYNAMIC )
    {
      p()->do_dynamic_regen();
    }

    p()->visited_apls_ = 0;

    action_t* a = p()->select_action( apl(), type() );

    if ( p()->restore_action_list )
    {
      p()->activate_action_list( p()->restore_action_list, type() );
      p()->restore_action_list = nullptr;
    }

    if ( a == nullptr )
    {
      return;
    }

    // Don't attempt to execute an action that's already being queued (this should not happen
    // anyhow)
    if ( p()->queueing && p()->queueing->internal_id == a->internal_id )
    {
      return;
    }

    // Don't queue the action if the specialized execution interval would elapse before the action
    // is usable again
    if ( usable( a ) )
    {
      // If we're queueing something, it's something different from what we are about to do.
      // Cancel existing queued action, and queue the new one.
      if ( p()->queueing )
      {
        event_t::cancel( p()->queueing->queue_event );
        p()->queueing = nullptr;
      }

      a->queue_execute( type() );
    }
  }

  // Type of specialized action execute
  virtual execute_type type() const = 0;
  // Base list of actions that is being run for the action execute
  virtual action_priority_list_t& apl() const = 0;
  // Pointer to the player-object event tracking member variable (holds this event object)
  virtual event_t*& ptr() = 0;
  // Is a selected action usable during the specialized execute interval (a GCD, or a cast/channel
  // time)
  virtual bool usable( const action_t* a ) const = 0;
  // Perform a reschedule of the polling event if applicable
  virtual void reschedule() = 0;
};

struct player_gcd_event_t : public special_execute_event_t<player_gcd_event_t>
{
  player_gcd_event_t( player_t& p, timespan_t delta_time ) :
    special_execute_event_t( p, delta_time )
  {
    sim().print_debug( "New Player-Ready-GCD Event: {}", p.name() );
  }

  const char* name() const override
  { return "Player-Ready-GCD"; }

  execute_type type() const override
  { return execute_type::OFF_GCD; }

  action_priority_list_t& apl() const override
  { return *p()->active_off_gcd_list; }

  event_t*& ptr() override
  { return p()->off_gcd; }

  void reschedule() override
  { p()->schedule_off_gcd_ready(); }

  // Action is only usable while a GCD is elapsing, and the action is queueable before the ready
  // event occurs
  bool usable( const action_t* a ) const override
  { return a->usable_during_current_gcd(); }

  void execute_action() override
  {
    // It is possible to orchestrate events such that an action is currently executing when an
    // off-gcd event occurs, if this is the case, don't do anything
    if ( p()->executing )
    {
      return;
    }

    special_execute_event_t<player_gcd_event_t>::execute_action();
  }
};

struct player_cwc_event_t : public special_execute_event_t<player_cwc_event_t>
{
  player_cwc_event_t( player_t& p, timespan_t delta_time ) :
    special_execute_event_t( p, delta_time )
  {
    sim().print_debug( "New Player-Ready-Cast-While-Casting Event: {}", p.name() );
  }

  const char* name() const override
  { return "Player-Ready-Cast-While-Casting"; }

  execute_type type() const override
  { return execute_type::CAST_WHILE_CASTING; }

  action_priority_list_t& apl() const override
  { return *p()->active_cast_while_casting_list; }

  event_t*& ptr() override
  { return p()->cast_while_casting_poll_event; }

  bool usable( const action_t* a ) const override
  { return a->usable_during_current_cast(); }

  void reschedule() override
  { p()->schedule_cwc_ready(); }

};

// Player Ready Event =======================================================

struct player_ready_event_t : public player_event_t
{
  player_ready_event_t( player_t& p, timespan_t delta_time ) : player_event_t( p, delta_time )
  {
    if ( sim().debug )
      sim().out_debug.printf( "New Player-Ready Event: %s", p.name() );
  }
  virtual const char* name() const override
  {
    return "Player-Ready";
  }
  virtual void execute() override
  {
    p()->readying = nullptr;
    p()->current_execute_type = execute_type::FOREGROUND;

    // There are certain chains of events where an off-gcd ability can be queued such that the queue
    // time for the action exceeds Player-Ready event (essentially end of GCD). In this case, the
    // simple solution is to just cancel the queue execute and let the actor select an action from
    // the action list as normal.
    if ( p()->queueing )
    {
      event_t::cancel( p()->queueing->queue_event );
      p()->queueing = nullptr;
    }
    // Player that's checking for off gcd actions to use, cancels that checking when there's a ready event firing.
    event_t::cancel( p()->off_gcd );
    // Also cancel checks during casting, since player-ready event by definition means that the
    // actor is not going to be casting.
    event_t::cancel( p()->cast_while_casting_poll_event );

    if ( !p()->execute_action() )
    {
      if ( p()->ready_type == READY_POLL )
      {
        timespan_t x = p()->available();

        p()->schedule_ready( x, true );

        // Waiting Debug
        sim().print_debug( "{} is waiting for {} resource={}", p()->name(), x,
                                  p()->resources.current[ p()->primary_resource() ] );
      }
      else
      {
        p()->started_waiting = sim().current_time();
        p()->min_threshold_trigger();
      }
    }
  }
};

// Trigger a wakeup based on a resource treshold. Only used by trigger_ready=1 actors
struct resource_threshold_event_t : public event_t
{
  player_t* player;

  resource_threshold_event_t( player_t* p, const timespan_t& delay ) : event_t( *p, delay ), player( p )
  {
  }

  const char* name() const override
  {
    return "Resource-Threshold";
  }

  void execute() override
  {
    player->trigger_ready();
    player->resource_threshold_trigger = nullptr;
  }
};

// Execute Pet Action =======================================================

struct execute_pet_action_t : public action_t
{
  action_t* pet_action;
  pet_t* pet;
  std::string action_str;

  std::string pet_name;

  execute_pet_action_t( player_t* player, const std::string& name, const std::string& as,
                        const std::string& options_str ) :
    action_t( ACTION_OTHER, "execute_" + name + "_" + as, player ),
    pet_action( nullptr ),
    pet( nullptr ),
    action_str( as ),
    pet_name( name )
  {
    parse_options( options_str );
    trigger_gcd = timespan_t::zero();
  }

  void init_finished() override
  {
    pet = player->find_pet( pet_name );
    // No pet found, finish init early, the action will never be ready() and never executed.
    if ( !pet )
    {
      return;
    }

    for ( auto& action : pet->action_list )
    {
      if ( action->name_str == action_str )
      {
        action->background = true;
        pet_action    = action;
      }
    }

    if ( !pet_action )
    {

      throw std::invalid_argument(fmt::format("Player {} refers to unknown action {} for pet {}.", player->name(), action_str.c_str(),
          pet->name()));
    }

    action_t::init_finished();
  }

  virtual void execute() override
  {
    assert(pet_action);
    pet_action->execute();
  }

  virtual bool ready() override
  {
    if ( !pet )
    {
      return false;
    }

    if ( !pet_action )
      return false;

    if ( !action_t::ready() )
      return false;

    if ( pet_action->player->is_sleeping() )
      return false;

    return pet_action->action_ready();
  }
};

struct override_talent_action_t : action_t
{
  override_talent_action_t( player_t* player ) : action_t( ACTION_OTHER, "override_talent", player )
  {
    background = true;
  }
};

struct leech_t : public heal_t
{
  leech_t( player_t* player ) : heal_t( "leech", player, player->find_spell( 143924 ) )
  {
    background = proc = true;
    callbacks = may_crit = may_miss = may_dodge = may_parry = may_block = false;
  }

  void init() override
  {
    heal_t::init();

    snapshot_flags = update_flags = STATE_MUL_DA | STATE_TGT_MUL_DA | STATE_VERSATILITY | STATE_MUL_PERSISTENT;
  }
};

/**
 * Get sorted list of action priorirty lists
 *
 * APLs need to always be initialized in the same order, otherwise copy= profiles may break in some cases.
 * Order will be: precombat -> default -> alphabetical list of custom apls
 */
std::vector<action_priority_list_t*> sorted_action_priority_lists( const player_t* p )
{
  std::vector<action_priority_list_t*> apls = p->action_priority_list;
  range::sort( apls, []( const action_priority_list_t* l, const action_priority_list_t* r ) {
    if ( l->name_str == "precombat" && r->name_str != "precombat" )
    {
      return true;
    }
    else if ( l->name_str != "precombat" && r->name_str == "precombat" )
    {
      return false;
    }
    else if ( l->name_str == "default" && r->name_str != "default" )
    {
      return true;
    }
    else if ( l->name_str != "default" && r->name_str == "default" )
    {
      return false;
    }
    else
    {
      return l->name_str < r->name_str;
    }
  } );

  return apls;
}

bool has_foreground_actions( const player_t& p )
{
  return ( p.active_action_list && !p.active_action_list->foreground_action_list.empty() );
}

// parse_talent_url =========================================================

bool parse_talent_url( sim_t* sim, const std::string& name, const std::string& url )
{
  assert( name == "talents" );
  (void)name;

  player_t* p = sim->active_player;

  p->talents_str = url;

  std::string::size_type cut_pt = url.find( '#' );

  if ( cut_pt != url.npos )
  {
    ++cut_pt;
    if ( url.find( ".battle.net" ) != url.npos || url.find( ".battlenet.com" ) != url.npos )
    {
      if ( sim->talent_format == TALENT_FORMAT_UNCHANGED )
        sim->talent_format = TALENT_FORMAT_ARMORY;
      return p->parse_talents_armory( url.substr( cut_pt ) );
    }
    else if ( url.find( "worldofwarcraft.com" ) != url.npos || url.find( "www.wowchina.com" ) != url.npos )
    {
      if ( sim->talent_format == TALENT_FORMAT_UNCHANGED )
        sim->talent_format = TALENT_FORMAT_ARMORY;
      return p->parse_talents_armory2( url );
    }
    else if ( url.find( ".wowhead.com" ) != url.npos )
    {
      if ( sim->talent_format == TALENT_FORMAT_UNCHANGED )
        sim->talent_format = TALENT_FORMAT_WOWHEAD;
      std::string::size_type end = url.find( '|', cut_pt );
      return p->parse_talents_wowhead( url.substr( cut_pt, end - cut_pt ) );
    }
  }
  else
  {
    bool all_digits = true;
    for ( size_t i = 0; i < url.size() && all_digits; i++ )
      if ( !std::isdigit( url[ i ] ) )
        all_digits = false;

    if ( all_digits )
    {
      if ( sim->talent_format == TALENT_FORMAT_UNCHANGED )
        sim->talent_format = TALENT_FORMAT_NUMBERS;
      p->parse_talents_numbers( url );
      return true;
    }
  }

  sim->errorf( "Unable to decode talent string '%s' for player '%s'.\n", url.c_str(), p->name() );

  return false;
}

// parse_talent_override ====================================================

bool parse_talent_override( sim_t* sim, const std::string& name, const std::string& override_str )
{
  assert( name == "talent_override" );
  (void)name;

  player_t* p = sim->active_player;

  if ( !p->talent_overrides_str.empty() )
    p->talent_overrides_str += "/";
  p->talent_overrides_str += override_str;

  return true;
}

// parse_timeofday ====================================================

bool parse_timeofday( sim_t* sim, const std::string& name, const std::string& override_str )
{
  assert( name == "timeofday" );
  (void)name;

  player_t* p = sim->active_player;

  if ( util::str_compare_ci( override_str, "night" ) || util::str_compare_ci( override_str, "nighttime" ) )
  {
    p->timeofday = player_t::NIGHT_TIME;
  }
  else if ( util::str_compare_ci( override_str, "day" ) || util::str_compare_ci( override_str, "daytime" ) )
  {
    p->timeofday = player_t::DAY_TIME;
  }
  else
  {
    sim->errorf( "\n%s timeofday string \"%s\" not valid.\n", sim->active_player->name(), override_str.c_str() );
    return false;
  }

  return true;
}

// parse_loa ====================================================

bool parse_loa( sim_t* sim, const std::string& name, const std::string& override_str )
{
  assert( name == "zandalari_loa" );
  ( void )name;

  player_t* p = sim->active_player;

  if ( util::str_compare_ci( override_str, "akunda" ) || util::str_compare_ci( override_str, "embrace_of_akunda" ) )
  {
    p->zandalari_loa = player_t::AKUNDA;
  }
  else if ( util::str_compare_ci( override_str, "bwonsamdi" ) || util::str_compare_ci( override_str, "embrace_of_bwonsamdi" ) )
  {
    p->zandalari_loa = player_t::BWONSAMDI;
  }
  else if ( util::str_compare_ci( override_str, "gonk" ) || util::str_compare_ci( override_str, "embrace_of_gonk" ) )
  {
    p->zandalari_loa = player_t::GONK;
  }
  else if ( util::str_compare_ci( override_str, "kimbul" ) || util::str_compare_ci( override_str, "embrace_of_kimbul" ) )
  {
    p->zandalari_loa = player_t::KIMBUL;
  }
  else if ( util::str_compare_ci( override_str, "kragwa" ) || util::str_compare_ci( override_str, "embrace_of_kragwa" ) )
  {
    p->zandalari_loa = player_t::KRAGWA;
  }
  else if ( util::str_compare_ci( override_str, "paku" ) || util::str_compare_ci( override_str, "embrace_of_paku" ) )
  {
    p->zandalari_loa = player_t::PAKU;
  }
  else
  {
    sim->errorf( "\n%s zandalari_loa string \"%s\" not valid.\n", sim->active_player->name(), override_str.c_str() );
    return false;
  }

  return true;
}

// parse_role_string ========================================================

bool parse_role_string( sim_t* sim, const std::string& name, const std::string& value )
{
  assert( name == "role" );
  (void)name;

  sim->active_player->role = util::parse_role_type( value );

  return true;
}

// parse_world_lag ==========================================================

bool parse_world_lag( sim_t* sim, const std::string& name, const std::string& value )
{
  assert( name == "world_lag" );
  (void)name;

  sim->active_player->world_lag = timespan_t::from_seconds( std::stod( value ) );

  if ( sim->active_player->world_lag < timespan_t::zero() )
  {
    sim->active_player->world_lag = timespan_t::zero();
  }

  sim->active_player->world_lag_override = true;

  return true;
}

// parse_world_lag ==========================================================

bool parse_world_lag_stddev( sim_t* sim, const std::string& name, const std::string& value )
{
  assert( name == "world_lag_stddev" );
  (void)name;

  sim->active_player->world_lag_stddev = timespan_t::from_seconds( std::stod( value ) );

  if ( sim->active_player->world_lag_stddev < timespan_t::zero() )
  {
    sim->active_player->world_lag_stddev = timespan_t::zero();
  }

  sim->active_player->world_lag_stddev_override = true;

  return true;
}

// parse_brain_lag ==========================================================

bool parse_brain_lag( sim_t* sim, const std::string& name, const std::string& value )
{
  assert( name == "brain_lag" );
  (void)name;

  sim->active_player->brain_lag = timespan_t::from_seconds( std::stod( value ) );

  if ( sim->active_player->brain_lag < timespan_t::zero() )
  {
    sim->active_player->brain_lag = timespan_t::zero();
  }

  return true;
}

// parse_brain_lag_stddev ===================================================

bool parse_brain_lag_stddev( sim_t* sim, const std::string& name, const std::string& value )
{
  assert( name == "brain_lag_stddev" );
  (void)name;

  sim->active_player->brain_lag_stddev = timespan_t::from_seconds( std::stod( value ) );

  if ( sim->active_player->brain_lag_stddev < timespan_t::zero() )
  {
    sim->active_player->brain_lag_stddev = timespan_t::zero();
  }

  return true;
}

// parse_specialization =====================================================

bool parse_specialization( sim_t* sim, const std::string&, const std::string& value )
{
  sim->active_player->_spec = dbc::translate_spec_str( sim->active_player->type, value );

  if ( sim->active_player->_spec == SPEC_NONE )
  {
    sim->errorf( "\n%s specialization string \"%s\" not valid.\n", sim->active_player->name(), value.c_str() );
    return false;
  }

  return true;
}

// parse stat timelines =====================================================

bool parse_stat_timelines( sim_t* sim, const std::string& name, const std::string& value )
{
  assert( name == "stat_timelines" );
  (void)name;

  std::vector<std::string> stats = util::string_split( value, "," );

  for ( auto& stat_type : stats )
  {
    stat_e st = util::parse_stat_type( stat_type );
    if ( st == STAT_NONE )
    {
      sim->errorf( "'%s' could not parse timeline stat '%s' (%s).\n", sim->active_player->name(), stat_type.c_str(), value.c_str() );
      return false;
    }

    sim->active_player->stat_timelines.push_back( st );
  }

  return true;
}

// parse_origin =============================================================

bool parse_origin( sim_t* sim, const std::string&, const std::string& origin )
{
  player_t& p = *sim->active_player;

  p.origin_str = origin;

  std::string region, server, name;
  if ( util::parse_origin( region, server, name, origin ) )
  {
    p.region_str = region;
    p.server_str = server;
  }

  return true;
}

// parse_source ===============================================================

bool parse_source( sim_t* sim, const std::string&, const std::string& value )
{
  player_t& p = *sim->active_player;

  p.profile_source_ = util::parse_profile_source( value );

  return true;
}

bool parse_set_bonus( sim_t* sim, const std::string&, const std::string& value )
{
  static const char* error_str = "%s invalid 'set_bonus' option value '%s' given, available options: %s";

  player_t* p = sim->active_player;

  std::vector<std::string> set_bonus_split = util::string_split( value, "=" );

  if ( set_bonus_split.size() != 2 )
  {
    sim->errorf( error_str, p->name(), value.c_str(), p->sets->generate_set_bonus_options().c_str() );
    return false;
  }

  int opt_val = std::stoi( set_bonus_split[ 1 ] );
  if ( opt_val != 0 && opt_val != 1 )
  {
    sim->errorf( error_str, p->name(), value.c_str(), p->sets->generate_set_bonus_options().c_str() );
    return false;
  }

  set_bonus_type_e set_bonus = SET_BONUS_NONE;
  set_bonus_e bonus          = B_NONE;

  if ( !p->sets->parse_set_bonus_option( set_bonus_split[ 0 ], set_bonus, bonus ) )
  {
    sim->errorf( error_str, p->name(), value.c_str(), p->sets->generate_set_bonus_options().c_str() );
    return false;
  }

  p->sets->set_bonus_spec_data[ set_bonus ][ specdata::spec_idx( p->specialization() ) ][ bonus ].overridden = opt_val;

  return true;
}

bool parse_initial_resource( sim_t* sim, const std::string&, const std::string& value )
{
  player_t* player = sim->active_player;
  auto opts        = util::string_split( value, ":/" );
  for ( const auto& opt_str : opts )
  {
    auto resource_split = util::string_split( opt_str, "=" );
    if ( resource_split.size() != 2 )
    {
      sim->errorf( "%s unknown initial_resources option '%s'", player->name(), opt_str.c_str() );
      return false;
    }

    resource_e resource = util::parse_resource_type( resource_split[ 0 ] );
    if ( resource == RESOURCE_NONE )
    {
      sim->errorf( "%s could not parse valid resource from '%s'", player->name(), resource_split[ 0 ].c_str() );
      return false;
    }

    double amount = std::stod( resource_split[ 1 ] );
    if ( amount < 0 )
    {
      sim->errorf( "%s too low initial_resources option '%s'", player->name(), opt_str.c_str() );
      return false;
    }

    player->resources.initial_opt[ resource ] = amount;
  }

  return true;
}

}  // namespace

/**
 * This is a template for Ignite like mechanics, like of course Ignite, Hunter Piercing Shots, Priest Echo of Light,
 * etc.
 *
 * It should get specialized in the class module.
 * Detailed MoP Ignite Mechanic description at http://us.battle.net/wow/en/forum/topic/5889309137?page=40#787
 * There is still a delay between the impact of the triggering spell and the dot application/refresh and damage
 * calculation.
 */
void residual_action::trigger( action_t* residual_action, player_t* t, double amount )
{
  struct delay_event_t : public event_t
  {
    double additional_residual_amount;
    player_t* target;
    action_t* action;

    static timespan_t delay_duration( player_t* p )
    {
      // Use same delay as in buff application
      return p->sim->rng().gauss( p->sim->default_aura_delay, p->sim->default_aura_delay_stddev );
    }

    delay_event_t( player_t* t, action_t* a, double amount ) :
      event_t( *a->player, delay_duration( a->player ) ),
      additional_residual_amount( amount ),
      target( t ),
      action( a )
    {
      if ( sim().debug )
        sim().out_debug.printf( "%s %s residual_action delay_event_start amount=%f", a->player->name(), action->name(),
                                amount );
    }

    virtual const char* name() const override
    {
      return "residual_action_delay_event";
    }

    virtual void execute() override
    {
      // Don't ignite on targets that are not active
      if ( target->is_sleeping() )
        return;

      dot_t* dot     = action->get_dot( target );
      auto dot_state = debug_cast<residual_periodic_state_t*>( dot->state );

      assert( action->dot_duration > timespan_t::zero() );

      if ( sim().debug )
      {
        if ( dot->is_ticking() )
        {
          sim().out_debug.printf(
              "%s %s residual_action delay_event_execute target=%s amount=%f current_ticks=%d current_tick=%f",
              action->player->name(), action->name(), target->name(), additional_residual_amount, dot->ticks_left(),
              dot_state->tick_amount );
        }
        else
        {
          sim().out_debug.printf( "%s %s residual_action delay_event_execute target=%s amount=%f",
                                  action->player->name(), action->name(), target->name(), additional_residual_amount );
        }
      }

      // Pass total amount of damage to the ignite action, and let it divide it by the correct number of ticks!
      action_state_t* s = action->get_state();
      s->target         = target;
      s->result         = RESULT_HIT;
      action->snapshot_state( s, action->type == ACTION_HEAL ? HEAL_OVER_TIME : DMG_OVER_TIME );
      s->result_amount = additional_residual_amount;
      action->schedule_travel( s );
      if ( !action->dual )
        action->stats->add_execute( timespan_t::zero(), s->target );
    }
  };

  assert( residual_action );

  make_event<delay_event_t>( *residual_action->sim, t, residual_action, amount );
}

player_t::player_t( sim_t* s, player_e t, const std::string& n, race_e r ):
  actor_t( s, n ),
  type( t ),
  parent( nullptr ),
  index( -1 ),
  creation_iteration( sim -> current_iteration ),
  actor_spawn_index( -1 ),
  race( r ),
  role( ROLE_NONE ),
  true_level( default_level ),
  party( 0 ),
  ready_type( READY_POLL ),
  _spec( SPEC_NONE ),
  bugs( true ),
  disable_hotfixes( 0 ),
  scale_player( true ),
  death_pct( 0.0 ),
  height( 0 ),
  combat_reach( 1.0 ),
  profile_source_( profile_source::DEFAULT ),
  default_target( nullptr ),
  target( 0 ),
  initialized( false ),
  potion_used( false ),
  region_str( s->default_region_str ),
  server_str( s->default_server_str ),
  origin_str(),
  timeofday( DAY_TIME ),  // Set to Day by Default since in raid it always switches to Day, user can override.
  zandalari_loa( PAKU ), //Set loa to paku by default (as it has some gain for any role if not optimal), user can override
  gcd_ready( timespan_t::zero() ),
  base_gcd( timespan_t::from_seconds( 1.5 ) ),
  min_gcd( timespan_t::from_millis( 750 ) ),
  gcd_haste_type( HASTE_NONE ),
  gcd_current_haste_value( 1.0 ),
  started_waiting( timespan_t::min() ),
  pet_list(),
  active_pets(),
  invert_scaling( 0 ),
  // Reaction
  reaction_offset( timespan_t::from_seconds( 0.1 ) ),
  reaction_max( timespan_t::from_seconds( 1.4 ) ),
  reaction_mean( timespan_t::from_seconds( 0.3 ) ),
  reaction_stddev( timespan_t::zero() ),
  reaction_nu( timespan_t::from_seconds( 0.25 ) ),
  // Latency
  world_lag( timespan_t::from_seconds( 0.1 ) ),
  world_lag_stddev( timespan_t::min() ),
  brain_lag( timespan_t::zero() ),
  brain_lag_stddev( timespan_t::min() ),
  world_lag_override( false ),
  world_lag_stddev_override( false ),
  cooldown_tolerance_( timespan_t::min() ),
  dbc( s->dbc ),
  talent_points(),
  profession(),
  artifact( nullptr ),
  azerite( nullptr ),
  base(),
  initial(),
  current(),
  last_cast( timespan_t::zero() ),
  // Defense Mechanics
  def_dr( diminishing_returns_constants_t() ),
  // Attacks
  main_hand_attack( nullptr ),
  off_hand_attack( nullptr ),
  current_attack_speed( 1.0 ),
  // Resources
  resources(),
  // Consumables
  // Events
  executing( 0 ),
  queueing( 0 ),
  channeling( 0 ),
  strict_sequence( 0 ),
  readying( 0 ),
  off_gcd( 0 ),
  cast_while_casting_poll_event(),
  off_gcd_ready( timespan_t::min() ),
  cast_while_casting_ready( timespan_t::min() ),
  in_combat( false ),
  action_queued( false ),
  first_cast( true ),
  last_foreground_action( 0 ),
  prev_gcd_actions( 0 ),
  off_gcdactions(),
  cast_delay_reaction( timespan_t::zero() ),
  cast_delay_occurred( timespan_t::zero() ),
  callbacks( s ),
  use_apl( "" ),
  // Actions
  use_default_action_list( 0 ),
  precombat_action_list( 0 ),
  active_action_list(),
  default_action_list(),
  active_off_gcd_list(),
  active_cast_while_casting_list(),
  restore_action_list(),
  no_action_list_provided(),
  // Reporting
  quiet( false ),
  report_extension( new player_report_extension_t() ),
  arise_time( timespan_t::min() ),
  iteration_fight_length(),
  iteration_waiting_time(),
  iteration_pooling_time(),
  iteration_executed_foreground_actions( 0 ),
  iteration_resource_lost(),
  iteration_resource_gained(),
  rps_gain( 0 ),
  rps_loss( 0 ),
  tmi_window( 6.0 ),
  collected_data( this ),
  // Damage
  iteration_dmg( 0 ),
  priority_iteration_dmg( 0 ),
  iteration_dmg_taken( 0 ),
  dpr( 0 ),
  // Heal
  iteration_heal( 0 ),
  iteration_heal_taken( 0 ),
  iteration_absorb(),
  iteration_absorb_taken(),
  hpr( 0 ),
  report_information(),
  // Gear
  sets( ( !is_pet() && !is_enemy() ) ? new set_bonus_t( this ) : nullptr ),
  meta_gem( META_GEM_NONE ),
  matching_gear( false ),
  karazhan_trinkets_paired( false ),
  item_cooldown( "item_cd", *this ),
  legendary_tank_cloak_cd( nullptr ),
  warlords_unseeing_eye( 0.0 ),
  warlords_unseeing_eye_stats(),
  auto_attack_multiplier( 1.0 ),
  insignia_of_the_grand_army_multiplier( 1.0 ),
  scaling( ( !is_pet() || sim->report_pets_separately ) ? new player_scaling_t() : nullptr ),
  // Movement & Position
  base_movement_speed( 7.0 ),
  passive_modifier( 0 ),
  x_position( 0.0 ),
  y_position( 0.0 ),
  default_x_position( 0.0 ),
  default_y_position( 0.0 ),
  consumables(),
  buffs(),
  debuffs(),
  gains(),
  spells(),
  procs(),
  uptimes(),
  racials(),
  passive_values(),
  active_during_iteration( false ),
  _mastery( spelleffect_data_t::nil() ),
  cache( this ),
  regen_type( REGEN_STATIC ),
  last_regen( timespan_t::zero() ),
  regen_caches( CACHE_MAX ),
  dynamic_regen_pets( false ),
  visited_apls_( 0 ),
  action_list_id_( 0 ),
  current_execute_type( execute_type::FOREGROUND ),
  has_active_resource_callbacks( false ),
  resource_threshold_trigger()
{
  actor_index = sim->actor_list.size();
  sim->actor_list.push_back( this );

  if ( !is_enemy() && !is_pet() && type != HEALING_ENEMY )
  {
    artifact = artifact::player_artifact_data_t::create( this );
  }

  if ( ! is_enemy() && ! is_pet() )
  {
    azerite = azerite::create_state( this );
  }

  // Set the gear object to a special default value, so we can support gear_x=0 properly.
  // player_t::init_items will replace the defaulted gear_stats_t object with a computed one once
  // the item stats have been computed.
  gear.initialize( std::numeric_limits<double>::lowest() );

  base.skill              = sim->default_skill;
  base.mastery            = 8.0;
  base.movement_direction = MOVEMENT_NONE;

  if ( !is_enemy() && type != HEALING_ENEMY )
  {
    sim->print_debug( "Creating Player {}.", name_str );
    sim->player_list.push_back( this );
    if ( !is_pet() )
    {
      sim->player_no_pet_list.push_back( this );
    }
    index = ++( sim->num_players );
  }
  else
  {
    if ( type != HEALING_ENEMY )  // Not actually a enemy target.
    {
      ++( sim->enemy_targets );
    }
    index = -( ++( sim->num_enemies ) );
  }

  // Fill healing lists with all non-enemy players.
  if ( !is_enemy() )
  {
    if ( !is_pet() )
    {
      sim->healing_no_pet_list.push_back( this );
    }
    else
    {
      sim->healing_pet_list.push_back( this );
    }
  }

  if ( !is_pet() && sim->stat_cache != -1 )
  {
    cache.active = sim->stat_cache != 0;
  }
  if ( is_pet() )
    current.skill = 1.0;

  resources.infinite_resource[ RESOURCE_HEALTH ] = true;

  if ( !is_pet() )
  {
    items.resize( SLOT_MAX );
    for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
    {
      items[ i ].slot   = i;
      items[ i ].sim    = sim;
      items[ i ].player = this;
    }
  }

  main_hand_weapon.slot = SLOT_MAIN_HAND;
  off_hand_weapon.slot  = SLOT_OFF_HAND;

  if ( reaction_stddev == timespan_t::zero() )
    reaction_stddev = reaction_mean * 0.2;

  action_list_information =
      "\n"
      "# This default action priority list is automatically created based on your character.\n"
      "# It is a attempt to provide you with a action list that is both simple and practicable,\n"
      "# while resulting in a meaningful and good simulation. It may not result in the absolutely highest possible "
      "dps.\n"
      "# Feel free to edit, adapt and improve it to your own needs.\n"
      "# SimulationCraft is always looking for updates and improvements to the default action lists.\n";
}

player_t::base_initial_current_t::base_initial_current_t() :
  stats(),
  spell_power_per_intellect( 0 ),
  spell_power_per_attack_power( 0 ),
  spell_crit_per_intellect( 0 ),
  attack_power_per_strength( 0 ),
  attack_power_per_agility( 0 ),
  attack_crit_per_agility( 0 ),
  dodge_per_agility( 0 ),
  parry_per_strength( 0 ),
  health_per_stamina( 0 ),
  resource_reduction(),
  miss( 0 ),
  dodge( 0 ),
  parry( 0 ),
  block( 0 ),
  hit( 0 ),
  expertise( 0 ),
  spell_crit_chance(),
  attack_crit_chance(),
  block_reduction(),
  mastery(),
  skill( 1.0 ),
  skill_debuff( 0.0 ),
  distance( 0 ),
  distance_to_move( 0 ),
  moving_away( 0 ),
  movement_direction(),
  armor_coeff( 0 ),
  sleeping( false ),
  rating(),
  attribute_multiplier(),
  spell_power_multiplier( 1.0 ),
  attack_power_multiplier( 1.0 ),
  base_armor_multiplier( 1.0 ),
  armor_multiplier( 1.0 ),
  position( POSITION_BACK )
{
  range::fill( attribute_multiplier, 1.0 );
}

std::string player_t::base_initial_current_t::to_string()
{
  std::ostringstream s;

  s << stats.to_string();
  s << " spell_power_per_intellect=" << spell_power_per_intellect;
  s << " spell_power_per_attack_power=" << spell_power_per_attack_power;
  s << " spell_crit_per_intellect=" << spell_crit_per_intellect;
  s << " attack_power_per_strength=" << attack_power_per_strength;
  s << " attack_power_per_agility=" << attack_power_per_agility;
  s << " attack_crit_per_agility=" << attack_crit_per_agility;
  s << " dodge_per_agility=" << dodge_per_agility;
  s << " parry_per_strength=" << parry_per_strength;
  s << " health_per_stamina=" << health_per_stamina;
  // resource_reduction
  s << " miss=" << miss;
  s << " dodge=" << dodge;
  s << " parry=" << parry;
  s << " block=" << block;
  s << " spell_crit_chance=" << spell_crit_chance;
  s << " attack_crit_chance=" << attack_crit_chance;
  s << " block_reduction=" << block_reduction;
  s << " mastery=" << mastery;
  s << " skill=" << skill;
  s << " distance=" << distance;
  s << " armor_coeff=" << armor_coeff;
  s << " sleeping=" << sleeping;
  // attribute_multiplier
  s << " spell_power_multiplier=" << spell_power_multiplier;
  s << " attack_power_multiplier=" << attack_power_multiplier;
  s << " base_armor_multiplier=" << base_armor_multiplier;
  s << " armor_multiplier=" << armor_multiplier;
  s << " position=" << util::position_type_string( position );
  return s.str();
}

void player_t::init()
{
  sim->print_debug( "Initializing {}.", *this );

  // Ensure the precombat and default lists are the first listed
  auto pre_combat = get_action_priority_list( "precombat", "Executed before combat begins. Accepts non-harmful actions only." );
  pre_combat->used = true;
  get_action_priority_list( "default", "Executed every time the actor is available." );

  for ( auto& elem : alist_map )
  {
    if ( elem.first == "default" )
      sim->error( "Ignoring action list named default." );
    else
      get_action_priority_list( elem.first )->action_list_str = elem.second;
  }

  // If the owner is regenerating using dynamic resource regen, we need to
  // ensure that pets that regen dynamically also get updated correctly. Thus,
  // we copy any CACHE_x enum values from pets to the owner. Also, if we have
  // no dynamically regenerating pets, we do not need to go through extra work
  // in do_dynamic_regen() to call the pets do_dynamic_regen(), saving some cpu
  // cycles.
  if ( regen_type == REGEN_DYNAMIC )
  {
    for ( auto pet : pet_list )
    {
      if ( pet->regen_type != REGEN_DYNAMIC )
        continue;

      for ( cache_e c = CACHE_NONE; c < CACHE_MAX; c++ )
      {
        if ( pet->regen_caches[ c ] )
          regen_caches[ c ] = true;
      }

      dynamic_regen_pets = true;
    }
  }

  // If Single-actor batch mode is used, player_collected_data_t::fight_length has to be
  // unconditionally made to record full data on actor fight lengths. This is to get proper
  // information on timelines in reports, since the sim-wide fight lengths ar no longer usable as
  // the "timeline adjustor" during analysis phase. This operation is done here so the
  // single_actor_batch does not become a positional parameter, since relying on it's state in
  // player_collected_data_t constructor would require it to be parsed before any actors are
  // defined.
  if ( sim->single_actor_batch )
  {
    collected_data.fight_length.change_mode( false );  // Not simple
  }
}

/**
 * Initialize race, specialization, talents, professions
 */
void player_t::init_character_properties()
{
  init_race();
  init_talents();
  replace_spells();
  init_position();
  init_professions();

  if ( sim->tmi_window_global > 0 )
    tmi_window = sim->tmi_window_global;
}

/**
 * Initialize base variables from database or other sources
 *
 * After player_t::init_base is executed, you can modify the base member until init_initial is called
 */
void player_t::init_base_stats()
{
  sim->print_debug( "Initializing base stats for {}.", *this );

  // Ensure base stats have not been tampered with.
  for ( stat_e stat = STAT_NONE; stat < STAT_MAX; ++stat )
  {
    double stat_value = base.stats.get_stat( stat );
    if ( stat_value > 0 )
    {
      sim->error( "{} stat {} is {} != 0.0", name(), util::stat_type_string( stat ), stat_value );
      sim->error( " Please do not modify player_t::base before init_base_stats is called\n" );
    }
  }

  if ( !is_enemy() )
    base.rating.init( dbc, level() );

  sim->print_debug( "{} base ratings initialized: {}", *this, base.rating.to_string() );

  if ( !is_enemy() )
  {
    base.stats.attribute[ STAT_STRENGTH ] =
        dbc.race_base( race ).strength + dbc.attribute_base( type, level() ).strength;
    base.stats.attribute[ STAT_AGILITY ] = dbc.race_base( race ).agility + dbc.attribute_base( type, level() ).agility;
    base.stats.attribute[ STAT_STAMINA ] = dbc.race_base( race ).stamina + dbc.attribute_base( type, level() ).stamina;
    base.stats.attribute[ STAT_INTELLECT ] =
        dbc.race_base( race ).intellect + dbc.attribute_base( type, level() ).intellect;
    base.stats.attribute[ STAT_SPIRIT ] = dbc.race_base( race ).spirit + dbc.attribute_base( type, level() ).spirit;

    // heroic presence is treated like base stats, floored before adding in; tested 2014-07-20
    base.stats.attribute[ STAT_STRENGTH ] += util::floor( racials.heroic_presence->effectN( 1 ).average( this ) );
    base.stats.attribute[ STAT_AGILITY ] += util::floor( racials.heroic_presence->effectN( 2 ).average( this ) );
    base.stats.attribute[ STAT_INTELLECT ] += util::floor( racials.heroic_presence->effectN( 3 ).average( this ) );
    // so is endurance. Can't tell if this is floored, ends in 0.055 @ L100. Assuming based on symmetry w/ heroic pres.
    base.stats.attribute[ STAT_STAMINA ] += util::floor( racials.endurance->effectN( 1 ).average( this ) );

    base.spell_crit_chance        = dbc.spell_crit_base( type, level() );
    base.attack_crit_chance       = dbc.melee_crit_base( type, level() );
    base.spell_crit_per_intellect = dbc.spell_crit_scaling( type, level() );
    base.attack_crit_per_agility  = dbc.melee_crit_scaling( type, level() );
    base.mastery                  = 8.0;

    resources.base[ RESOURCE_HEALTH ] = dbc.health_base( type, level() );
    resources.base[ RESOURCE_MANA ]   = dbc.resource_base( type, level() );

    // 1% of base mana as mana regen per second for all classes.
    resources.base_regen_per_second[ RESOURCE_MANA ] = dbc.resource_base( type, level() ) * 0.01;

    // Automatically parse mana regen and max mana modifiers from class passives.
    for ( auto spell : dbc::class_passives( this ) )
    {
      if ( !spell->_effects)
      {
        continue;
      }
      for ( auto effect : *spell->_effects )
      {
        if ( effect->subtype() == A_MOD_MANA_REGEN_PCT )
        {
          resources.base_regen_per_second[ RESOURCE_MANA ] *= 1.0 + effect->percent();
        }
        if ( effect->subtype() == A_MOD_MAX_MANA_PCT || effect->subtype() == A_MOD_MANA_POOL_PCT )
        {
          resources.base[ RESOURCE_MANA ] *= 1.0 + effect->percent();
        }
      }
    }

    base.health_per_stamina = dbc.health_per_stamina( level() );

    // players have a base 7.5% hit/exp
    base.hit       = 0.075;
    base.expertise = 0.075;

    if ( base.distance < 1 )
      base.distance = 5;
  }

  // only certain classes get Agi->Dodge conversions, dodge_per_agility defaults to 0.00
  if ( type == MONK || type == DRUID || type == ROGUE || type == HUNTER || type == SHAMAN || type == DEMON_HUNTER )
    base.dodge_per_agility =
        dbc.avoid_per_str_agi_by_level( level() ) / 100.0;  // exact values given by Blizzard, only have L90-L100 data

  // only certain classes get Str->Parry conversions, dodge_per_agility defaults to 0.00
  if ( type == PALADIN || type == WARRIOR || type == DEATH_KNIGHT )
    base.parry_per_strength =
        dbc.avoid_per_str_agi_by_level( level() ) / 100.0;  // exact values given by Blizzard, only have L90-L100 data

  // All classes get 3% dodge and miss
  base.dodge = 0.03;
  base.miss = 0.03;
  // Dodge from base agillity isn't affected by diminishing returns and is added here
  base.dodge += racials.quickness->effectN( 1 ).percent() +
      ( dbc.race_base( race ).agility + dbc.attribute_base( type, level() ).agility ) * base.dodge_per_agility;

  // Only Warriors and Paladins (and enemies) can block, defaults to 0
  if ( type == WARRIOR || type == PALADIN || type == ENEMY || type == TMI_BOSS || type == TANK_DUMMY )
  {
    // Set block reduction to 0 for warrior/paladin because it's computed in composite_block_reduction()
    base.block_reduction = 0;

    // Base block chance is 10% for players, warriors have a bonus 8% in their spec aura
    switch ( type )
    {
    case WARRIOR:
    case PALADIN:
      base.block = 0.10;
      break;
    default:
      base.block = 0.03;
      base.block_reduction = 0.30;
      break;
    }
  }

  // Only certain classes can parry, and get 3% base parry, default is 0
  // Parry from base strength isn't affected by diminishing returns and is added here
  if ( type == WARRIOR || type == PALADIN || type == ROGUE || type == DEATH_KNIGHT || type == MONK ||
       type == DEMON_HUNTER || specialization() == SHAMAN_ENHANCEMENT || type == ENEMY || type == TMI_BOSS ||
       type == TANK_DUMMY )
  {
    base.parry = 0.03 + ( dbc.race_base( race ).strength + dbc.attribute_base( type, level() ).strength ) * base.parry_per_strength;
  }

  // Extract avoidance DR values from table in sc_extra_data.inc
  def_dr.horizontal_shift       = dbc.horizontal_shift( type );
  def_dr.block_vertical_stretch = dbc.block_vertical_stretch( type );
  def_dr.vertical_stretch       = dbc.vertical_stretch( type );
  def_dr.dodge_factor           = dbc.dodge_factor( type );
  def_dr.parry_factor           = dbc.parry_factor( type );
  def_dr.miss_factor            = dbc.miss_factor( type );
  def_dr.block_factor           = dbc.block_factor( type );

  if ( ( meta_gem == META_EMBER_PRIMAL ) || ( meta_gem == META_EMBER_SHADOWSPIRIT ) ||
       ( meta_gem == META_EMBER_SKYFIRE ) || ( meta_gem == META_EMBER_SKYFLARE ) )
  {
    resources.base_multiplier[ RESOURCE_MANA ] *= 1.02;
  }

  // Expansive Mind
  for ( auto& r : { RESOURCE_MANA, RESOURCE_RAGE, RESOURCE_ENERGY, RESOURCE_RUNIC_POWER, RESOURCE_FOCUS} )
  {
    resources.base_multiplier[ r ] *= 1.0 + racials.expansive_mind->effectN( 1 ).percent();
  }

  if ( world_lag_stddev < timespan_t::zero() )
    world_lag_stddev = world_lag * 0.1;
  if ( brain_lag_stddev < timespan_t::zero() )
    brain_lag_stddev = brain_lag * 0.1;

  if ( primary_role() == ROLE_TANK )
  {
    // Collect DTPS data for tanks even for statistics_level == 1
    if ( sim->statistics_level >= 1 )
      collected_data.dtps.change_mode( false );
  }

  sim->print_debug( "{} generic base stats: {}", *this, base.to_string() );
}

/**
 * Initialize initial stats from base + gear
 *
 * After player_t::init_initial is executed, you can modify the initial member until combat starts
 */
void player_t::init_initial_stats()
{
  sim->print_debug( "Initializing initial stats for {}.", *this );

  // Ensure initial stats have not been tampered with.
  for ( stat_e stat = STAT_NONE; stat < STAT_MAX; ++stat )
  {
    double stat_value = initial.stats.get_stat( stat );
    if ( stat_value > 0 )
    {
      sim->error( "{} initial stat {} is {} != 0", *this, util::stat_type_string( stat ), stat_value );
      sim->error( " Please do not modify player_t::initial before init_initial_stats is called\n" );
    }
  }

  initial = base;
  initial.stats += passive;

  // Compute current "total from gear" into total gear. Per stat, this is either the amount of stats
  // the items for the actor gives, or the overridden value (player_t::gear + player_t::enchant +
  // sim_t::enchant).
  if ( !is_pet() && !is_enemy() )
  {
    gear_stats_t item_stats =
        std::accumulate( items.begin(), items.end(), gear_stats_t(),
                         []( const gear_stats_t& t, const item_t& i ) { return t + i.total_stats(); } );

    for ( stat_e stat = STAT_NONE; stat < STAT_MAX; ++stat )
    {
      if ( gear.get_stat( stat ) < 0 )
        total_gear.add_stat( stat, item_stats.get_stat( stat ) );
      else
        total_gear.add_stat( stat, gear.get_stat( stat ) );
    }

    sim->print_debug( "{} total gear stats: {}", *this, total_gear.to_string() );

    initial.stats += enchant;
    initial.stats += sim->enchant;
  }

  initial.stats += total_gear;

  sim->print_debug( "{} generic initial stats: %s", *this, initial.to_string() );
}

void player_t::init_items()
{
  sim->print_debug( "Initializing items for {}.", *this );

  // Create items
  for ( const std::string& split : util::string_split( items_str, "/" ) )
  {
    if ( find_item_by_name( split ) )
    {
      sim->error( "{} has multiple %s equipped.\n", *this, split );
    }
    items.emplace_back( this, split );
  }

  std::array<bool, SLOT_MAX> matching_gear_slots;  // true if the given item is equal to the highest armor type the player can wear
  for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
    matching_gear_slots[ i ] = !util::is_match_slot( i );

  // We need to simple-parse the items first, this will set up some base information, and parse out
  // simple options
  for ( auto& item : items )
  {
    try
    {
      // If the item has been specified in options we want to start from scratch, forgetting about
      // lingering stuff from profile copy
      if ( !item.options_str.empty() )
      {
        auto slot = item.slot;
        item      = item_t( this, item.options_str );
        item.slot = slot;
      }

      item.parse_options();

      if ( !item.initialize_data() )
      {
        throw std::invalid_argument("Cannot initialize data");
      }
    }
    catch (const std::exception& e)
    {
      std::throw_with_nested(std::runtime_error(fmt::format("Item '{}' Slot '{}'", item.name(), item.slot_name() )));
    }
  }

  // Once item data is initialized, initialize the parent - child relationships of each item
  range::for_each( items, [this]( item_t& i ) {
    i.parent_slot = parent_item_slot( i );

    // Set the primary artifact slot for this player, if the item is (after data download)
    // determined to be the primary artifact slot.
    if ( i.parsed.data.id_artifact > 0 && i.parent_slot == SLOT_INVALID )
    {
      assert( artifact->slot() == SLOT_INVALID );
      artifact->set_artifact_slot( i.slot );
    }
  } );

  // Slot initialization order vector. Needed to ensure parents of children get initialized first
  std::vector<slot_e> init_slots;
  range::for_each( items, [&init_slots]( const item_t& i ) { init_slots.push_back( i.slot ); } );

  // Sort items with children before items without children
  range::sort( init_slots, [this]( slot_e first, slot_e second ) {
    const item_t &fi = items[ first ], si = items[ second ];
    if ( fi.parent_slot != SLOT_INVALID && si.parent_slot == SLOT_INVALID )
    {
      return false;
    }
    else if ( fi.parent_slot == SLOT_INVALID && si.parent_slot != SLOT_INVALID )
    {
      return true;
    }

    return false;
  } );

  for ( slot_e slot : init_slots )
  {
    item_t& item = items[ slot ];

    try
    {
      item.init();
      if ( !item.is_valid_type() )
      {
        throw std::invalid_argument("Invalid type");
      }
    }
    catch (const std::exception& e)
    {
      std::throw_with_nested(std::runtime_error(fmt::format("Item '{}' Slot '{}'", item.name(), item.slot_name() )));
    }

    matching_gear_slots[ item.slot ] = item.is_matching_type();
  }

  matching_gear = true;
  for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
  {
    if ( !matching_gear_slots[ i ] )
    {
      matching_gear = false;
      break;
    }
  }

  init_meta_gem();

  // Needs to be initialized after old set bonus system
  if ( sets != nullptr )
  {
    sets->initialize();
  }

  // these initialize the weapons, but don't have a return value (yet?)
  init_weapon( main_hand_weapon );
  init_weapon( off_hand_weapon );
}

/**
 * Initializes the Azerite-related support structures for an actor if there are any.
 *
 * Since multiple instances of the same azerite power can be worn by an actor, we need to ensure
 * that only one instance of the azerite power gets initialized. Ensure this by building a simple
 * map that keeps initialization status for all azerite powers defined for the actor from different
 * sources (currently only items). Initialization status changes automatically when an
 * azerite_power_t object is created for the actor.
 *
 * Note, guards against invocation from non-player actors (enemies, adds, pets ...)
 */
void player_t::init_azerite()
{
  if ( is_enemy() || is_pet() )
  {
    return;
  }

  sim->print_debug( "Initializing Azerite sub-system for {}.", *this );

  azerite -> initialize();
}

void player_t::init_meta_gem()
{
  sim->print_debug( "Initializing meta-gem for {}.", *this );

  if ( !meta_gem_str.empty() )
  {
    meta_gem = util::parse_meta_gem_type( meta_gem_str );
    if ( meta_gem == META_GEM_NONE )
    {
      throw std::invalid_argument(fmt::format( "Invalid meta gem '{}'.", meta_gem_str ));
    }
  }


  if ( ( meta_gem == META_AUSTERE_EARTHSIEGE ) || ( meta_gem == META_AUSTERE_SHADOWSPIRIT ) )
  {
    initial.base_armor_multiplier *= 1.02;
  }
}

void player_t::init_position()
{
  sim->print_debug( "Initializing position for {}.", *this );

  if ( !position_str.empty() )
  {
    base.position = util::parse_position_type( position_str );
    if ( base.position == POSITION_NONE )
    {
      throw std::invalid_argument(fmt::format( "Invalid position '{}'.", position_str ));
    }
    else
    {
      initial.position = base.position;
      change_position( base.position );
    }
  }
  // if the position string is empty and we're a tank, assume they want to be in front
  else if ( primary_role() == ROLE_TANK )
  {
    // set position to front
    base.position    = POSITION_FRONT;
    initial.position = POSITION_FRONT;
    change_position( POSITION_FRONT );
  }

  // default to back when we have an invalid position
  if ( base.position == POSITION_NONE )
    base.position = POSITION_BACK;

  position_str = util::position_type_string( base.position );

  sim->print_debug( "{} position set to {}.", *this, position_str );
}

void player_t::init_race()
{
  sim->print_debug( "Initializing race for {}.", *this );

  if ( race_str.empty() )
  {
    race_str = util::race_type_string( race );
  }
  else
  {
    race = util::parse_race_type( race_str );
    if ( race == RACE_UNKNOWN)
    {
      throw std::invalid_argument(fmt::format("Unknown race '{}'.", race_str ));
    }
  }
}

/**
 * Initialize defensive properties (role, position, data collection, etc.)
 */
void player_t::init_defense()
{
  sim->print_debug( "Initializing defense for {}.", *this );

  if ( primary_role() == ROLE_TANK )
  {
    initial.position = POSITION_FRONT;
    sim->print_debug( "{} initial position set to front because primary_role() == ROLE_TANK", *this );
  }

  if ( !is_pet() && primary_role() == ROLE_TANK )
  {
    collected_data.health_changes.collect = true;
    collected_data.health_changes.set_bin_size( sim->tmi_bin_size );
    collected_data.health_changes_tmi.collect = true;
    collected_data.health_changes_tmi.set_bin_size( sim->tmi_bin_size );
  }

  // Armor Coefficient
  initial.armor_coeff = dbc.armor_mitigation_constant( level() );
  sim->print_debug( "{} initial armor coefficient set to {}.", *this, initial.armor_coeff );
}

void player_t::init_weapon( weapon_t& w )
{
  sim->print_debug( "Initializing weapon ( type {} ) for {}.", util::weapon_type_string( w.type ),
                           *this );

  if ( w.type == WEAPON_NONE )
    return;

  if ( w.slot == SLOT_MAIN_HAND )
    assert( w.type >= WEAPON_NONE && w.type < WEAPON_RANGED );
  if ( w.slot == SLOT_OFF_HAND )
    assert( w.type >= WEAPON_NONE && w.type < WEAPON_2H );
}

void player_t::create_special_effects()
{
  if ( is_pet() || is_enemy() )
  {
    return;
  }

  sim->print_debug( "Creating special effects for {}.", *this );

  // Initialize all item-based special effects. This includes any DBC-backed enchants, gems, as well
  // as inherent item effects that use a spell
  for ( auto& item : items )
  {
    item.init_special_effects();
  }

  // Set bonus initialization. Note that we err on the side of caution here and
  // require that the set bonus is "custom" (and as such, specified in the
  // master list of custom special effect in unique gear). This is to avoid
  // false positives with class-specific set bonuses that have to always be
  // implemented inside the class module anyhow.
  for ( const auto& set_bonus : sets->enabled_set_bonus_data() )
  {
    special_effect_t effect( this );
    unique_gear::initialize_special_effect( effect, set_bonus->spell_id );

    if ( effect.custom_init_object.size() == 0 )
    {
      continue;
    }

    special_effects.push_back( new special_effect_t( effect ) );
  }

  unique_gear::initialize_racial_effects( this );

  // Initialize generic azerite powers. Note that this occurs later in the process than the class
  // module spell initialization (init_spells()), which is where the core presumes that each class
  // module gets the state their azerite powers (through the invocation of find_azerite_spells).
  // This means that any enabled azerite power that is not referenced in a class module will be
  // initialized here.
  azerite::initialize_azerite_powers( this );

  // Once all special effects are first-phase initialized, do a pass to first-phase initialize any
  // potential fallback special effects for the actor.
  unique_gear::initialize_special_effect_fallbacks( this );
}

void player_t::init_special_effects()
{
  if ( is_pet() || is_enemy() )
  {
    return;
  }

  sim->print_debug( "Initializing special effects for {}.", *this );

  // ..and then move on to second phase initialization of all special effects.
  unique_gear::init( this );

  for ( auto& elem : callbacks.all_callbacks )
  {
    elem->initialize();
  }
}

namespace
{
/**
 * Compute max resource r for an actor, based on their set base resources
 */
double compute_max_resource( player_t* p, resource_e r )
{
  double value = p->resources.base[ r ];
  value *= p->resources.base_multiplier[ r ];
  value += p->total_gear.resource[ r ];

  // re-ordered 2016-06-19 by Theck - initial_multiplier should do something for RESOURCE_HEALTH
  if ( r == RESOURCE_HEALTH )
    value += std::floor( p->stamina() ) * p->current.health_per_stamina;

  value *= p->resources.initial_multiplier[ r ];
  value = std::floor( value );

  return value;
}
}  // namespace

void player_t::init_resources( bool force )
{
  sim->print_debug( "Initializing resources for {}.", *this );

  for ( resource_e resource = RESOURCE_NONE; resource < RESOURCE_MAX; resource++ )
  {
    // Don't reset non-forced and already-reset initial resources
    if ( !force && resources.initial[ resource ] != 0 )
    {
      continue;
    }

    double max_resource = compute_max_resource( this, resource );

    resources.initial[ resource ] = max_resource;
    resources.max[ resource ]     = max_resource;

    if ( resources.initial_opt[ resource ] != -1.0 )
    {
      double actual_resource = std::min( max_resource, resources.initial_opt[ resource ] );

      sim->print_debug( "{} resource {} overridden to {}", *this, util::resource_type_string( resource ),
                               actual_resource );

      resources.current[ resource ] = actual_resource;
    }
    else
    {
      // Actual "current" resource can never exceed the computed maximum resource for the actor
      resources.current[ resource ] = std::min( max_resource, resources.start_at[ resource ] );
    }
  }

  // Only collect pet resource timelines if they get reported separately
  if ( !is_pet() || sim->report_pets_separately )
  {
    if ( collected_data.resource_timelines.size() == 0 )
    {
      for ( resource_e resource = RESOURCE_NONE; resource < RESOURCE_MAX; ++resource )
      {
        if ( resources.max[ resource ] > 0 )
        {
          collected_data.resource_timelines.emplace_back( resource );
        }
      }
    }
  }
}

void player_t::init_professions()
{
  if ( professions_str.empty() )
    return;

  sim->print_debug( "Initializing professions for {}.", *this );

  std::vector<std::string> splits = util::string_split( professions_str, ",/" );

  for ( auto& split : splits )
  {
    std::string prof_name;
    int prof_value = 0;

    auto subsplit = util::string_split( split, "=" );
    if ( subsplit.size() == 2 )
    {
      prof_name = subsplit[ 0 ];
      try
      {
        prof_value = std::stoi( subsplit[ 1 ] );
      }
      catch ( const std::exception& e )
      {
        std::throw_with_nested(std::runtime_error(fmt::format("Could not parse profession level '{}' for profession '{}'",
            subsplit[ 1 ], prof_name)));
      }
    }
    else
    {
      prof_name  = split;
      prof_value = true_level > 85 ? 600 : 525;
    }

    auto prof_type = util::parse_profession_type( prof_name );
    if ( prof_type == PROFESSION_NONE )
    {
      throw std::invalid_argument(fmt::format("Invalid profession '{}'.", prof_name ));
    }

    profession[ prof_type ] = prof_value;
  }
}

void player_t::init_target()
{
  if ( !target_str.empty() )
  {
    target = sim->find_player( target_str );
  }
  if ( !target )
  {
    target = sim->target;
  }

  default_target = target;
}

std::string player_t::init_use_item_actions( const std::string& append )
{
  std::string buffer;

  for ( auto& item : items )
  {
    if ( item.slot == SLOT_HANDS )
      continue;

    if ( item.has_use_special_effect() )
    {
      buffer += "/use_item,slot=";
      buffer += item.slot_name();
      if ( !append.empty() )
      {
        buffer += append;
      }
    }
  }
  if ( items[ SLOT_HANDS ].has_use_special_effect() )
  {
    buffer += "/use_item,slot=";
    buffer += items[ SLOT_HANDS ].slot_name();
    if ( !append.empty() )
    {
      buffer += append;
    }
  }

  return buffer;
}

std::vector<std::string> player_t::get_item_actions( const std::string& options )
{
  std::vector<std::string> actions;

  for ( const auto& item : items )
  {
    // This will skip Addon and Enchant-based on-use effects. Addons especially are important to
    // skip from the default APLs since they will interfere with the potion timer, which is almost
    // always preferred over an Addon.
    if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
    {
      std::string action_string = "use_item,slot=";
      action_string += item.slot_name();
      if ( !options.empty() )
      {
        if ( options[ 0 ] != ',' )
        {
          action_string += ',';
        }
        action_string += options;
      }

      actions.push_back( action_string );
    }
  }

  return actions;
}

std::string player_t::init_use_profession_actions( const std::string& /* append */ )
{
  std::string buffer;

  return buffer;
}

std::vector<std::string> player_t::get_profession_actions()
{
  std::vector<std::string> actions;

  return actions;
}

std::string player_t::init_use_racial_actions( const std::string& append )
{
  std::string buffer;
  bool race_action_found = false;

  switch ( race )
  {
    case RACE_ORC:
      buffer += "/blood_fury";
      race_action_found = true;
      break;
    case RACE_TROLL:
      buffer += "/berserking";
      race_action_found = true;
      break;
    case RACE_BLOOD_ELF:
      buffer += "/arcane_torrent";
      race_action_found = true;
      break;
    case RACE_LIGHTFORGED_DRAENEI:
      buffer += "/lights_judgment";
      race_action_found = true;
      break;
    default:
      break;
  }

  if ( race_action_found && !append.empty() )
  {
    buffer += append;
  }

  return buffer;
}

std::vector<std::string> player_t::get_racial_actions()
{
  std::vector<std::string> actions;

  actions.push_back( "blood_fury" );
  actions.push_back( "berserking" );
  actions.push_back( "arcane_torrent" );
  actions.push_back( "lights_judgment" );
  actions.push_back( "fireblood" );
  actions.push_back( "ancestral_call" );

  return actions;
}

/**
 * Add action to action list.
 *
 * Helper function to add actions with spell data of the same name to the action list,
 * and check if that spell data is ok()
 * returns true if spell data is ok(), false otherwise
 */
bool player_t::add_action( std::string action, std::string options, std::string alist )
{
  return add_action( find_class_spell( action ), options, alist );
}

/**
 * Add action to action list
 *
 * Helper function to add actions to the action list if given spell data is ok()
 * returns true if spell data is ok(), false otherwise
 */
bool player_t::add_action( const spell_data_t* s, std::string options, std::string alist )
{
  if ( s->ok() )
  {
    std::string& str =
        ( alist == "default" ) ? action_list_str : ( get_action_priority_list( alist )->action_list_str );
    std::string name = s->name_cstr();
    util::tokenize( name );
    str += "/" + name;
    if ( !options.empty() )
    {
      str += "," + options;
    }
    return true;
  }
  return false;
}

/**
 * Adds all on use item actions for all items with their on use effect not excluded in the exclude_effects string.
 */
void player_t::activate_action_list( action_priority_list_t* a, execute_type type )
{
  if ( type == execute_type::OFF_GCD )
    active_off_gcd_list = a;
  else if ( type == execute_type::CAST_WHILE_CASTING )
    active_cast_while_casting_list = a;
  else
    active_action_list = a;
  a->used = true;
}

void player_t::override_talent( std::string& override_str )
{
  std::string::size_type cut_pt = override_str.find( ',' );

  if ( cut_pt != override_str.npos && override_str.substr( cut_pt + 1, 3 ) == "if=" )
  {
    override_talent_action_t* dummy_action = new override_talent_action_t( this );
    expr_t* expr                           = expr_t::parse( dummy_action, override_str.substr( cut_pt + 4 ) );
    if ( !expr )
      return;
    bool success = expr->success();
    delete expr;
    if ( !success )
      return;
    override_str = override_str.substr( 0, cut_pt );
  }

  util::tokenize( override_str );

  // Support disable_row:<row> syntax
  std::string::size_type pos = override_str.find( "disable_row" );
  if ( pos != std::string::npos )
  {
    std::string row_str = override_str.substr( 11 );
    if ( !row_str.empty() )
    {
      unsigned row = util::to_unsigned( override_str.substr( 11 ) );
      if ( row == 0 || row > MAX_TALENT_ROWS )
      {
        throw std::invalid_argument(fmt::format("talent_override: Invalid talent row {} in '{}'.", row, override_str ));
      }

      talent_points.clear( row - 1 );
      if ( sim->num_players == 1 )
      {
        sim->error( "talent_override: Talent row {} for {} disabled.\n", row, *this );
      }
      return;
    }
  }

  unsigned spell_id = dbc.talent_ability_id( type, specialization(), override_str.c_str(), true );

  if ( !spell_id || dbc.spell( spell_id )->id() != spell_id )
  {
    throw std::invalid_argument(fmt::format("talent_override: Override talent '{}' not found.\n", override_str ));
  }

  for ( int j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    for ( int i = 0; i < MAX_TALENT_COLS; i++ )
    {
      talent_data_t* t = talent_data_t::find( type, j, i, specialization(), dbc.ptr );
      if ( t && ( t->spell_id() == spell_id ) )
      {
        if ( true_level < std::min( ( j + 1 ) * 15, 100 ) )
        {
          throw std::invalid_argument(fmt::format("talent_override: Override talent '{}' is too high level.\n",
              override_str ));
        }

        if ( talent_points.has_row_col( j, i ) )
        {
          sim->print_debug( "talent_override: talent {} for {} is already enabled\n",
                                 override_str, *this );
        }

        if ( sim->num_players == 1 )
        {  // To prevent spamming up raid reports, only do this with 1 player sims.
          sim->error( "talent_override: talent '{}' for {}s replaced talent {} in tier {}.\n", override_str,
                       *this, talent_points.choice( j ) + 1, j + 1 );
        }
        talent_points.select_row_col( j, i );
      }
    }
  }
}

void player_t::init_talents()
{
  sim->print_debug( "Initializing talents for {}.", *this );

  if ( !talent_overrides_str.empty() )
  {
    for ( auto& split : util::string_split( talent_overrides_str, "/" ) )
    {
      override_talent( split );
    }
  }
}

void player_t::init_spells()
{
  sim->print_debug( "Initializing spells for {}.", *this );

  racials.quickness             = find_racial_spell( "Quickness" );
  racials.command               = find_racial_spell( "Command" );
  racials.arcane_acuity         = find_racial_spell( "Arcane Acuity" );
  racials.heroic_presence       = find_racial_spell( "Heroic Presence" );
  racials.might_of_the_mountain = find_racial_spell( "Might of the Mountain" );
  racials.expansive_mind        = find_racial_spell( "Expansive Mind" );
  racials.nimble_fingers        = find_racial_spell( "Nimble Fingers" );
  racials.time_is_money         = find_racial_spell( "Time is Money" );
  racials.the_human_spirit      = find_racial_spell( "The Human Spirit" );
  racials.touch_of_elune        = find_racial_spell( "Touch of Elune" );
  racials.brawn                 = find_racial_spell( "Brawn" );
  racials.endurance             = find_racial_spell( "Endurance" );
  racials.viciousness           = find_racial_spell( "Viciousness" );
  racials.magical_affinity      = find_racial_spell( "Magical Affinity" );
  racials.mountaineer           = find_racial_spell( "Mountaineer" );
  racials.brush_it_off          = find_racial_spell( "Brush It Off" );

  if ( !is_enemy() )
  {
    const spell_data_t* s = find_mastery_spell( specialization() );
    if ( s->ok() )
      _mastery = &( s->effectN( 1 ) );
  }

  if ( record_healing() )
  {
    spells.leech = new leech_t( this );
  }
}

void player_t::init_gains()
{
  sim->print_debug( "Initializing gains for {}.", *this );

  gains.arcane_torrent      = get_gain( "arcane_torrent" );
  gains.endurance_of_niuzao = get_gain( "endurance_of_niuzao" );
  for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
  {
    std::string name = util::resource_type_string( r );
    name += "_regen";
    gains.resource_regen[ r ] = get_gain( name );
  }
  gains.health             = get_gain( "external_healing" );
  gains.mana_potion        = get_gain( "mana_potion" );
  gains.restore_mana       = get_gain( "restore_mana" );
  gains.touch_of_the_grave = get_gain( "touch_of_the_grave" );
  gains.vampiric_embrace   = get_gain( "vampiric_embrace" );
  gains.leech              = get_gain( "leech" );
  gains.embrace_of_bwonsamdi = get_gain( "embrace_of_bwonsamdi" );
}

void player_t::init_procs()
{
  sim->print_debug( "Initializing procs for {}.", *this );

  procs.parry_haste = get_proc( "parry_haste" );
}

void player_t::init_uptimes()
{
  sim->print_debug( "Initializing uptimes for {}.", *this );

  uptimes.primary_resource_cap =
      get_uptime( util::inverse_tokenize( util::resource_type_string( primary_resource() ) ) + " Cap" );
}

void player_t::init_benefits()
{
  sim->print_debug( "Initializing benefits for {}.", *this );
}

void player_t::init_rng()
{
  sim->print_debug( "Initializing random number generators for {}.", *this );
}

void player_t::init_stats()
{
  sim->print_debug( "Initializing stats for {}.", *this );

  if ( sim->maximize_reporting )
  {
    for ( stat_e s = STAT_NONE; s < STAT_MAX; ++s )
    {
      stat_timelines.push_back( s );
    }
  }
  if ( !is_pet() || sim->report_pets_separately )
  {
    if ( collected_data.stat_timelines.size() == 0 )
    {
      for ( stat_e stat : stat_timelines )
      {
        collected_data.stat_timelines.emplace_back( stat );
      }
    }
  }

  collected_data.reserve_memory( *this );
}

/**
 * Define absorb priority.
 *
 * Absorbs with the high priority flag will follow the order in absorb_priority vector when resolving.
 * This method should be overwritten in class modules to declare the order of spec-specific high priority absorbs.
 */
void player_t::init_absorb_priority()
{
}

void player_t::init_scaling()
{
  sim->print_debug( "Initializing scaling for {}.", *this );

  if ( !is_pet() && !is_enemy() )
  {
    invert_scaling = 0;

    role_e role = primary_role();

    bool attack = ( role == ROLE_ATTACK || role == ROLE_HYBRID || role == ROLE_TANK || role == ROLE_DPS );
    bool spell  = ( role == ROLE_SPELL || role == ROLE_HYBRID || role == ROLE_HEAL || role == ROLE_DPS );
    bool tank   = ( role == ROLE_TANK );
    bool heal   = ( role == ROLE_HEAL );

    scaling->set( STAT_STRENGTH, attack );
    scaling->set( STAT_AGILITY, attack );
    scaling->set( STAT_STAMINA, tank );
    scaling->set( STAT_INTELLECT, spell );
    scaling->set( STAT_SPIRIT, heal );

    scaling->set( STAT_SPELL_POWER, spell );
    scaling->set( STAT_ATTACK_POWER, attack );
    scaling->enable( STAT_CRIT_RATING );
    scaling->enable( STAT_HASTE_RATING );
    scaling->enable( STAT_MASTERY_RATING );
    scaling->enable( STAT_VERSATILITY_RATING );

    scaling->set( STAT_SPEED_RATING, sim->has_raid_event( "movement" ) );
    // scaling -> set( STAT_AVOIDANCE_RATING          ] = tank; // Waste of sim time vast majority of the time. Can be
    // enabled manually.
    scaling->set( STAT_LEECH_RATING, tank );

    scaling->set( STAT_WEAPON_DPS, attack );

    scaling->set( STAT_ARMOR, tank );

    auto add_stat = []( double& to, double value, double lower_limit )
      {
      if ( to + value < lower_limit )
        to = lower_limit;
      else
        to += value;
      };

    if ( sim->scaling->scale_stat != STAT_NONE && scale_player )
    {
      double v = sim->scaling->scale_value;

      switch ( sim->scaling->scale_stat )
      {
        case STAT_STRENGTH:
          add_stat( initial.stats.attribute[ ATTR_STRENGTH ], v, 0 );
          break;
        case STAT_AGILITY:
          add_stat( initial.stats.attribute[ ATTR_AGILITY ], v, 0 );
          break;
        case STAT_STAMINA:
          add_stat( initial.stats.attribute[ ATTR_STAMINA ], v, 0 );
          break;
        case STAT_INTELLECT:
          add_stat( initial.stats.attribute[ ATTR_INTELLECT ], v, 0 );
          break;
        case STAT_SPIRIT:
          add_stat( initial.stats.attribute[ ATTR_SPIRIT ], v, 0 );
          break;

        case STAT_SPELL_POWER:
          add_stat( initial.stats.spell_power, v, 0 );
          break;

        case STAT_ATTACK_POWER:
          add_stat( initial.stats.attack_power, v, 0 );
          break;

        case STAT_CRIT_RATING:
          add_stat( initial.stats.crit_rating, v, 0 );
          break;

        case STAT_HASTE_RATING:
          add_stat( initial.stats.haste_rating, v, 0 );
          break;

        case STAT_MASTERY_RATING:
          add_stat( initial.stats.mastery_rating, v, 0 );
          break;

        case STAT_VERSATILITY_RATING:
          add_stat( initial.stats.versatility_rating, v, 0 );
          break;

        case STAT_DODGE_RATING:
          add_stat( initial.stats.dodge_rating, v, 0 );
          break;

        case STAT_PARRY_RATING:
          add_stat( initial.stats.parry_rating, v, 0 );
          break;

        case STAT_SPEED_RATING:
          add_stat( initial.stats.speed_rating, v, 0 );
          break;

        case STAT_AVOIDANCE_RATING:
          add_stat( initial.stats.avoidance_rating, v, 0 );
          break;

        case STAT_LEECH_RATING:
          add_stat( initial.stats.leech_rating, v, 0 );
          break;

        case STAT_WEAPON_DPS:
          if ( main_hand_weapon.damage > 0 )
          {
            add_stat( main_hand_weapon.damage, main_hand_weapon.swing_time.total_seconds() * v, 1 );
            add_stat( main_hand_weapon.dps, v, 1 );
            add_stat( main_hand_weapon.min_dmg, main_hand_weapon.swing_time.total_seconds() * v, 1 );
            add_stat( main_hand_weapon.max_dmg, main_hand_weapon.swing_time.total_seconds() * v, 1 );
          }
          break;

        case STAT_WEAPON_OFFHAND_DPS:
          if ( off_hand_weapon.damage > 0 )
          {
            add_stat( off_hand_weapon.damage, off_hand_weapon.swing_time.total_seconds() * v, 1 );
            add_stat( off_hand_weapon.dps, v, 1 );
            add_stat( off_hand_weapon.min_dmg, off_hand_weapon.swing_time.total_seconds() * v, 1 );
            add_stat( off_hand_weapon.max_dmg, off_hand_weapon.swing_time.total_seconds() * v, 1 );
          }
          break;

        case STAT_ARMOR:
          add_stat( initial.stats.armor, v, 0 );
          break;

        case STAT_BONUS_ARMOR:
          add_stat( initial.stats.bonus_armor, v, 0 );
          break;

        case STAT_BLOCK_RATING:
          add_stat( initial.stats.block_rating, v, 0 );
          break;

        case STAT_MAX:
          break;

        default:
          assert( false );
          break;
      }
    }
  }
}

void player_t::create_actions()
{
  if ( action_list_str.empty() )
    no_action_list_provided = true;

  init_action_list();  // virtual function which creates the action list string

  std::string modify_action_options;

  if ( !modify_action.empty() )
  {
    std::string::size_type cut_pt = modify_action.find( ',' );

    if ( cut_pt != modify_action.npos )
    {
      modify_action_options = modify_action.substr( cut_pt + 1 );
      modify_action         = modify_action.substr( 0, cut_pt );
    }
  }
  util::tokenize( modify_action );

  std::vector<std::string> skip_actions;
  if ( !action_list_skip.empty() )
  {
    sim->print_debug( "{} action_list_skip={}", *this, action_list_skip );

    skip_actions = util::string_split( action_list_skip, "/" );
  }

  if ( !use_apl.empty() )
    copy_action_priority_list( "default", use_apl );

  if ( !action_list_str.empty() )
    get_action_priority_list( "default" )->action_list_str = action_list_str;

  int j = 0;

  auto apls = sorted_action_priority_lists( this );
  for ( auto apl : apls )
  {
    assert( !( !apl->action_list_str.empty() && !apl->action_list.empty() ) );

    // Convert old style action list to new style, all lines are without comments
    if ( !apl->action_list_str.empty() )
    {
      for ( auto& split : util::string_split( apl->action_list_str, "/" ) )
        apl->action_list.emplace_back( split, "" );
    }

    sim->print_debug( "{} actions.{}={}", *this, apl->name_str, apl->action_list_str );

    for ( auto& action_priority : apl->action_list )
    {
      std::string& action_str = action_priority.action_;

      std::string::size_type cut_pt = action_str.find( ',' );
      std::string action_name       = action_str.substr( 0, cut_pt );
      std::string action_options;
      if ( cut_pt != std::string::npos )
        action_options = action_str.substr( cut_pt + 1 );

      action_t* a = nullptr;

      cut_pt = action_name.find( ':' );
      if ( cut_pt != std::string::npos )
      {
        std::string pet_name   = action_name.substr( 0, cut_pt );
        std::string pet_action = action_name.substr( cut_pt + 1 );

        util::tokenize( pet_action );

        a = new execute_pet_action_t( this, pet_name, pet_action, action_options );
      }
      else
      {
        util::tokenize( action_name );
        if ( action_name == modify_action )
        {
          sim->print_debug( "{}: modify_action={}", *this, modify_action );

          action_options = modify_action_options;
          action_str     = modify_action + "," + modify_action_options;
        }
        a = create_action( action_name, action_options );
      }

      if ( !a )
      {
        throw std::invalid_argument(
            fmt::format("{} unable to create action: {}", *this, action_str));
      }

      bool skip = false;
      for ( auto& skip_action : skip_actions )
      {
        if ( skip_action == a->name_str )
        {
          skip = true;
          break;
        }
      }

      if ( skip )
      {
        a->background = true;
      }
      else
      {
        // a -> action_list = action_priority_list[ alist ] -> name_str;
        a->action_list = apl;

        a->signature_str = action_str;
        a->signature     = &( action_priority );

        if ( sim->separate_stats_by_actions > 0 && !is_pet() )
        {
          a->marker =
              (char)( ( j < 10 ) ? ( '0' + j )
                                 : ( j < 36 ) ? ( 'A' + j - 10 )
                                              : ( j < 66 ) ? ( 'a' + j - 36 )
                                                           : ( j < 79 ) ? ( '!' + j - 66 )
                                                                        : ( j < 86 ) ? ( ':' + j - 79 ) : '.' );

          a->stats = get_stats( a->name_str + "__" + a->marker, a );
        }
        j++;
      }
    }
  }

  if ( !is_add() && ( !is_pet() || sim->report_pets_separately ) )
  {
    int capacity = std::max( 1200, static_cast<int>( sim->max_time.total_seconds() / 2.0 ) );
    collected_data.action_sequence.reserve( capacity );
    collected_data.action_sequence.clear();
  }
}

namespace
{
// Only "real" off gcd actions should trigger the off gcd apl behavior since it has a performance
// impact. Some meta-actions (call_action_list, variable, run_action_list, swap_action_list) alone
// cannot trigger off-gcd apl behavior.
bool is_real_action( const action_t* a )
{
  return a -> type != ACTION_CALL && a -> type != ACTION_VARIABLE &&
         ! util::str_compare_ci( a->name_str, "run_action_list" ) &&
         ! util::str_compare_ci( a->name_str, "swap_action_list" );
}
} // Anonymous namespace ends

void player_t::init_actions()
{
  for ( size_t i = 0; i < action_list.size(); ++i )
  {
    action_t* action = action_list[ i ];
    try
    {
      action -> init();
    }
    catch (const std::exception& e)
    {
      std::throw_with_nested(std::runtime_error(fmt::format("Action '{}'", action->name())));
    }
  }

  bool have_off_gcd_actions = false;
  bool have_cast_while_casting_actions = false;
  for ( auto action : action_list )
  {
    if ( action->action_list && action->trigger_gcd == timespan_t::zero() &&
         !action->background && action->use_off_gcd )
    {
      action->action_list->off_gcd_actions.push_back( action );
      // Optimization: We don't need to do off gcd stuff when there are no other off gcd actions
      // than "meta actions"
      if ( is_real_action( action ) )
      {
        have_off_gcd_actions = true;
        auto it = range::find( off_gcd_cd, action -> cooldown );
        if ( it == off_gcd_cd.end() )
        {
          off_gcd_cd.push_back( action -> cooldown );
        }

        if ( range::find( off_gcd_icd, action->internal_cooldown ) == off_gcd_icd.end() )
        {
          off_gcd_icd.push_back( action->internal_cooldown );
        }
      }
    }

    if ( action->action_list && !action->background && action->use_while_casting &&
         action->usable_while_casting )
    {
      action->action_list->cast_while_casting_actions.push_back( action );

      if ( is_real_action( action ) )
      {
        have_cast_while_casting_actions = true;
        auto it = range::find( cast_while_casting_cd, action->cooldown );
        if ( it == cast_while_casting_cd.end() )
        {
          cast_while_casting_cd.push_back( action->cooldown );
        }

        auto icd_it = range::find( cast_while_casting_icd, action->internal_cooldown );
        if ( icd_it == cast_while_casting_icd.end() )
        {
          cast_while_casting_icd.push_back( action->internal_cooldown );
        }
      }
    }
  }

  if ( choose_action_list.empty() )
    choose_action_list = "default";

  default_action_list = find_action_priority_list( choose_action_list );

  if ( !default_action_list && choose_action_list != "default" )
  {
    sim->error( "Action List '{}' not found, using default action list.", choose_action_list );
    default_action_list = find_action_priority_list( "default" );
  }

  if ( default_action_list )
  {
    prune_specialized_execute_actions( default_action_list->off_gcd_actions, execute_type::OFF_GCD );
    prune_specialized_execute_actions( default_action_list->cast_while_casting_actions,
        execute_type::CAST_WHILE_CASTING );

    activate_action_list( default_action_list );
    if ( have_off_gcd_actions )
      activate_action_list( default_action_list, execute_type::OFF_GCD );

    if ( have_cast_while_casting_actions )
      activate_action_list( default_action_list, execute_type::CAST_WHILE_CASTING );
  }
  else
  {
    sim->error( "No Default Action List available." );
  }
}

void player_t::init_assessors()
{
  // Target related mitigation
  assessor_out_damage.add( assessor::TARGET_MITIGATION, []( dmg_e dmg_type, action_state_t* state ) {
    state->target->assess_damage( state->action->get_school(), dmg_type, state );
    return assessor::CONTINUE;
  } );

  // Target damage
  assessor_out_damage.add( assessor::TARGET_DAMAGE, []( dmg_e, action_state_t* state ) {
    state->target->do_damage( state );
    return assessor::CONTINUE;
  } );

  // Logging and debug .. Technically, this should probably be in action_t::assess_damage, but we
  // don't need this piece of code for the vast majority of sims, so it makes sense to yank it out
  // completely from there, and only conditionally include it if logging/debugging is enabled.
  if ( sim->log || sim->debug || sim->debug_seed.size() > 0 )
  {
    assessor_out_damage.add( assessor::LOG, [this]( dmg_e type, action_state_t* state ) {
      if ( sim->debug )
      {
        state->debug();
      }

      if ( sim->log )
      {
        if ( type == DMG_DIRECT )
        {
          sim->print_log( "{} {} hits {} for {} {} damage ({})", *this, state->action->name(),
                               *state->target, state->result_amount,
                               util::school_type_string( state->action->get_school() ),
                               util::result_type_string( state->result ) );
        }
        else  // DMG_OVER_TIME
        {
          dot_t* dot = state->action->get_dot( state->target );
          sim->print_log( "{} {} ticks ({} of {}) on {} for {} {} damage ({})", *this, state->action->name(),
                               dot->current_tick, dot->num_ticks, *state->target, state->result_amount,
                               util::school_type_string( state->action->get_school() ),
                               util::result_type_string( state->result ) );
        }
      }
      return assessor::CONTINUE;
    } );
  }

  // Leech, if the player has leeching enabled (disabled by default)
  if ( spells.leech )
  {
    assessor_out_damage.add( assessor::LEECH, [this]( dmg_e, action_state_t* state ) {
      // Leeching .. sanity check that the result type is a damaging one, so things hopefully don't
      // break in the future if we ever decide to not separate heal and damage assessing.
      double leech_pct = 0;
      if ( ( state->result_type == DMG_DIRECT || state->result_type == DMG_OVER_TIME ) && state->result_amount > 0 &&
           ( leech_pct = state->action->composite_leech( state ) ) > 0 )
      {
        double leech_amount       = leech_pct * state->result_amount;
        spells.leech->base_dd_min = spells.leech->base_dd_max = leech_amount;
        spells.leech->schedule_execute();
      }
      return assessor::CONTINUE;
    } );
  }

  // Generic actor callbacks
  assessor_out_damage.add( assessor::CALLBACKS, [this]( dmg_e, action_state_t* state ) {
    if ( !state->action->callbacks )
    {
      return assessor::CONTINUE;
    }

    proc_types pt   = state->proc_type();
    proc_types2 pt2 = state->impact_proc_type2();
    if ( pt != PROC1_INVALID && pt2 != PROC2_INVALID )
      action_callback_t::trigger( callbacks.procs[ pt ][ pt2 ], state->action, state );

    return assessor::CONTINUE;
  } );
}

void player_t::init_finished()
{
  for ( auto action : action_list )
  {
    try
    {
      action->init_finished();
    }
    catch (const std::exception& e)
    {
      std::throw_with_nested(std::runtime_error(fmt::format("Action '{}'", action->name())));
    }
  }

  // Naive recording of minimum energy thresholds for the actor.
  // TODO: Energy pooling, and energy-based expressions (energy>=10) are not included yet
  for ( auto action : action_list )
  {
    if ( !action->background && action->base_costs[ primary_resource() ] > 0 )
    {
      if ( std::find( resource_thresholds.begin(), resource_thresholds.end(),
                      action->base_costs[ primary_resource() ] ) == resource_thresholds.end() )
      {
        resource_thresholds.push_back( action->base_costs[ primary_resource() ] );
      }
    }
  }

  range::sort( resource_thresholds );

  range::for_each( cooldown_list, [this]( cooldown_t* c ) {
    if ( c->hasted )
    {
      dynamic_cooldown_list.push_back( c );
    }
  } );

  // Sort outbound assessors
  assessor_out_damage.sort();

  // Print items to debug log
  if ( sim->debug )
  {
    range::for_each( items, [this]( const item_t& item ) {
      if ( item.active() )
      {
        sim->out_debug << item.to_string();
      }
    } );
  }
}

/**
 * Add a wake-up call at the next resource threshold level, compared to the current resource status of the actor.
 */
void player_t::min_threshold_trigger()
{
  if ( ready_type == READY_POLL )
  {
    return;
  }

  if ( resource_thresholds.empty() )
  {
    return;
  }

  resource_e pres = primary_resource();
  if ( pres != RESOURCE_MANA && pres != RESOURCE_ENERGY && pres != RESOURCE_FOCUS )
  {
    return;
  }

  size_t i = 0, end = resource_thresholds.size();
  double threshold = 0;
  for ( ; i < end; ++i )
  {
    if ( resources.current[ pres ] < resource_thresholds[ i ] )
    {
      threshold = resource_thresholds[ i ];
      break;
    }
  }

  timespan_t time_to_threshold = timespan_t::zero();
  if ( i < resource_thresholds.size() )
  {
    double rps = resources.base_regen_per_second[ pres ];

    if ( rps > 0 )
    {
      double diff       = threshold - resources.current[ pres ];
      time_to_threshold = timespan_t::from_seconds( diff / rps );
    }
  }

  // If time to threshold is zero, there's no need to reset anything
  if ( time_to_threshold <= timespan_t::zero() )
  {
    return;
  }

  timespan_t occurs = sim->current_time() + time_to_threshold;
  if ( resource_threshold_trigger )
  {
    // We should never ever be doing threshold-based wake up calls if there already is a
    // Player-ready event.
    assert( !readying );

    if ( occurs > resource_threshold_trigger->occurs() )
    {
      resource_threshold_trigger->reschedule( time_to_threshold );
      sim->print_debug( "{} rescheduling Resource-Threshold event: threshold={} delay={}", *this,
                               threshold, time_to_threshold );
    }
    else if ( occurs < resource_threshold_trigger->occurs() )
    {
      event_t::cancel( resource_threshold_trigger );
      resource_threshold_trigger = make_event<resource_threshold_event_t>( *sim, this, time_to_threshold );

      sim->print_debug( "{} recreating Resource-Threshold event: threshold={} delay={}", *this,
                               threshold, time_to_threshold );
    }
  }
  else
  {
    // We should never ever be doing threshold-based wake up calls if there already is a
    // Player-ready event.
    assert( !readying );

    resource_threshold_trigger = make_event<resource_threshold_event_t>( *sim, this, time_to_threshold );

    sim->print_debug( "{} scheduling new Resource-Threshold event: threshold={} delay={}", *this,
                             threshold, time_to_threshold );
  }
}

/**
 * Create player buffs.
 *
 * Note, these are player and enemy buffs/debuffs. Pet buffs and debuffs are in pet_t::create_buffs
 */
void player_t::create_buffs()
{
  sim->print_debug( "Creating Auras, Buffs, and Debuffs for {}.", *this );

  struct norgannons_foresight_buff_t : public buff_t
  {
    norgannons_foresight_buff_t( player_t* p ) :
      buff_t( p, "norgannons_foresight", p->find_spell( 234797 ) )
    {
      set_chance( 0 );
    }

    void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
    {
      buff_t::expire_override( expiration_stacks, remaining_duration );
      player->buffs.norgannons_foresight_ready->trigger();
    }
  };

  // Infinite-Stacking Buffs and De-Buffs for everyone
  buffs.stunned   = make_buff( this, "stunned" )->set_max_stack( 1 );
  debuffs.casting = make_buff( this, "casting" )->set_max_stack( 1 )->set_quiet( 1 );

  // .. for players
  if ( !is_enemy() )
  {
    // Racials
    buffs.berserking = make_buff( this, "berserking", find_spell( 26297 ) )->add_invalidate( CACHE_HASTE );
    buffs.stoneform  = make_buff( this, "stoneform", find_spell( 65116 ) );
    buffs.blood_fury = make_buff<stat_buff_t>( this, "blood_fury", find_racial_spell( "Blood Fury" ) )
                           ->add_invalidate( CACHE_SPELL_POWER )
                           ->add_invalidate( CACHE_ATTACK_POWER );
    buffs.fortitude  = make_buff( this, "fortitude", find_spell( 137593 ) )->set_activated( false );
    buffs.shadowmeld = make_buff( this, "shadowmeld", find_spell( 58984 ) )->set_cooldown( timespan_t::zero() );

    buffs.ancestral_call[ 0 ] = make_buff<stat_buff_t>( this, "rictus_of_the_laughing_skull", find_spell( 274739 ) );
    buffs.ancestral_call[ 1 ] = make_buff<stat_buff_t>( this, "zeal_of_the_burning_blade", find_spell( 274740 ) );
    buffs.ancestral_call[ 2 ] = make_buff<stat_buff_t>( this, "ferocity_of_the_frostwolf", find_spell( 274741 ) );
    buffs.ancestral_call[ 3 ] = make_buff<stat_buff_t>( this, "might_of_the_blackrock", find_spell( 274742 ) );

    buffs.fireblood = make_buff<stat_buff_t>( this, "fireblood", find_spell( 265226 ) )
                        ->add_stat( convert_hybrid_stat( STAT_STR_AGI_INT ),
                          util::round( find_spell( 265226 ) -> effectN( 1 ).average( this, level() ) ) * 3 );

    buffs.archmages_greater_incandescence_agi =
        make_buff( this, "archmages_greater_incandescence_agi", find_spell( 177172 ) )
            ->add_invalidate( CACHE_AGILITY );
    buffs.archmages_greater_incandescence_str =
        make_buff( this, "archmages_greater_incandescence_str", find_spell( 177175 ) )
            ->add_invalidate( CACHE_STRENGTH );
    buffs.archmages_greater_incandescence_int =
        make_buff( this, "archmages_greater_incandescence_int", find_spell( 177176 ) )
            ->add_invalidate( CACHE_INTELLECT );

    buffs.archmages_incandescence_agi =
        make_buff( this, "archmages_incandescence_agi", find_spell( 177161 ) )->add_invalidate( CACHE_AGILITY );
    buffs.archmages_incandescence_str =
        make_buff( this, "archmages_incandescence_str", find_spell( 177160 ) )->add_invalidate( CACHE_STRENGTH );
    buffs.archmages_incandescence_int =
        make_buff( this, "archmages_incandescence_int", find_spell( 177159 ) )->add_invalidate( CACHE_INTELLECT );

    // Legendary meta haste buff
    buffs.tempus_repit = make_buff( this, "tempus_repit", find_spell( 137590 ) )
        ->add_invalidate( CACHE_SPELL_SPEED )->set_activated( false );

    buffs.darkflight = make_buff( this, "darkflight", find_racial_spell( "darkflight" ) );

    buffs.nitro_boosts = make_buff( this, "nitro_boosts", find_spell( 54861 ) );

    buffs.amplification = make_buff( this, "amplification", find_spell( 146051 ) )
                              ->add_invalidate( CACHE_MASTERY )
                              ->add_invalidate( CACHE_HASTE )
                              ->add_invalidate( CACHE_SPIRIT )
                              ->set_chance( 0 );
    buffs.amplification_2 = make_buff( this, "amplification_2", find_spell( 146051 ) )
                                ->add_invalidate( CACHE_MASTERY )
                                ->add_invalidate( CACHE_HASTE )
                                ->add_invalidate( CACHE_SPIRIT )
                                ->set_chance( 0 );

    buffs.temptation = make_buff( this, "temptation", find_spell( 234143 ) )
                           ->set_cooldown( timespan_t::zero() )
                           ->set_chance( 1 )
                           ->set_default_value( 0.1 );  // Not in spelldata

    buffs.norgannons_foresight_ready =
        make_buff( this, "norgannons_foresight_ready", find_spell( 236380 ) )->set_chance( 0 );

    buffs.norgannons_foresight = new norgannons_foresight_buff_t( this );

    buffs.courageous_primal_diamond_lucidity = make_buff( this, "lucidity", find_spell( 137288 ) );

    buffs.body_and_soul = make_buff( this, "body_and_soul", find_spell( 64129 ) )
                              ->set_max_stack( 1 )
                              ->set_duration( timespan_t::from_seconds( 4.0 ) );

    buffs.angelic_feather = make_buff( this, "angelic_feather", find_spell( 121557 ) )
                                ->set_max_stack( 1 )
                                ->set_duration( timespan_t::from_seconds( 6.0 ) );

    buffs.movement = new movement_buff_t( this );
  }
  // .. for enemies
  else
  {
    debuffs.bleeding      = make_buff( this, "bleeding" )->set_max_stack( 1 );
    debuffs.invulnerable  = new invulnerable_debuff_t( this );
    debuffs.vulnerable    = make_buff( this, "vulnerable" )->set_max_stack( 1 );
    debuffs.flying        = make_buff( this, "flying" )->set_max_stack( 1 );
    debuffs.mortal_wounds = make_buff( this, "mortal_wounds", find_spell( 115804 ) )
        ->set_default_value( std::fabs( find_spell( 115804 )->effectN( 1 ).percent() ) );

    // BfA Raid Damage Modifier Debuffs
    debuffs.chaos_brand = make_buff( this, "chaos_brand", find_spell( 1490 ) )
        ->set_default_value( find_spell( 1490 )->effectN( 1 ).percent() )
        ->set_cooldown( timespan_t::from_seconds( 5.0 ) );
    debuffs.mystic_touch = make_buff( this, "mystic_touch", find_spell( 113746 ) )
        ->set_default_value( find_spell( 113746 )->effectN( 1 ).percent() )
        ->set_cooldown( timespan_t::from_seconds( 5.0 ) );
  }

  // set up always since this can be applied by enemy actions and raid events.
  debuffs.damage_taken =
        make_buff( this, "damage_taken" )->set_duration( timespan_t::from_seconds( 20.0 ) )->set_max_stack( 999 );

  if ( sim->has_raid_event( "damage_done" ) )
  {
    buffs.damage_done =
        make_buff( this, "damage_done" )->set_max_stack( 1 )->add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
}

item_t* player_t::find_item_by_name( const std::string& item_name )
{
  auto it = range::find_if(items, [item_name](const item_t& item) { return item_name == item.name();});

  if ( it != items.end())
  {
    return &(*it);
  }

  return nullptr;
}

item_t* player_t::find_item_by_id( unsigned item_id )
{
  auto it = range::find_if(items, [item_id](const item_t& item) { return item_id == item.parsed.data.id;});

  if ( it != items.end())
  {
    return &(*it);
  }

  return nullptr;
}

bool player_t::has_t18_class_trinket() const
{
  // Class modules should override this with their individual trinket detection
  return false;
}

/**
 * Scaled player level.
 *
 * Options such as timewalking will affect the players true_level.
 */
int player_t::level() const
{
  if ( sim->timewalk > 0 && !is_enemy() )
    return sim->timewalk;
  else
    return true_level;
}

double player_t::resource_regen_per_second( resource_e r ) const
{
  double reg = resources.base_regen_per_second[ r ];

  if ( r == RESOURCE_FOCUS || r == RESOURCE_ENERGY )
  {
    if ( reg )
    {
      reg *= ( 1.0 / cache.attack_haste() );
    }
  }

  if ( r == RESOURCE_ENERGY )
  {
    if ( buffs.surge_of_energy && buffs.surge_of_energy->check() )
    {
      reg *= 1.0 + buffs.surge_of_energy->data().effectN( 1 ).percent();
    }
  }

  return reg;
}

double player_t::composite_melee_haste() const
{
  double h;

  h = std::max( 0.0, composite_melee_haste_rating() ) / current.rating.attack_haste;

  h = 1.0 / ( 1.0 + h );

  if ( !is_pet() && !is_enemy() )
  {
    if ( buffs.bloodlust->check() )
      h *= 1.0 / ( 1.0 + buffs.bloodlust->data().effectN( 1 ).percent() );

    if ( buffs.mongoose_mh && buffs.mongoose_mh->check() )
      h *= 1.0 / ( 1.0 + 30 / current.rating.attack_haste );

    if ( buffs.mongoose_oh && buffs.mongoose_oh->check() )
      h *= 1.0 / ( 1.0 + 30 / current.rating.attack_haste );

    if ( buffs.berserking->up() )
      h *= 1.0 / ( 1.0 + buffs.berserking->data().effectN( 1 ).percent() );

    h *= 1.0 / ( 1.0 + racials.nimble_fingers->effectN( 1 ).percent() );
    h *= 1.0 / ( 1.0 + racials.time_is_money->effectN( 1 ).percent() );

    if ( timeofday == NIGHT_TIME )
      h *= 1.0 / ( 1.0 + racials.touch_of_elune->effectN( 1 ).percent() );
  }

  return h;
}

double player_t::composite_melee_speed() const
{
  double h = composite_melee_haste();

  if ( buffs.fel_winds && buffs.fel_winds->check() )
  {
    h *= 1.0 / ( 1.0 + buffs.fel_winds->check_value() );
  }

  if ( buffs.galeforce_striking && buffs.galeforce_striking->check() )
    h *= 1.0 / ( 1.0 + buffs.galeforce_striking->check_value() );

  return h;
}

double player_t::composite_melee_attack_power() const
{
  double ap = current.stats.attack_power;

  ap += current.attack_power_per_strength * cache.strength();
  ap += current.attack_power_per_agility * cache.agility();

  return ap;
}

double player_t::composite_melee_attack_power( attack_power_e type ) const
{
  double base_ap = cache.attack_power();
  double ap = 0;
  bool has_mh = main_hand_weapon.type != WEAPON_NONE;
  bool has_oh = off_hand_weapon.type != WEAPON_NONE;

  switch ( type )
  {
    case AP_WEAPON_MH:
      if ( has_mh )
      {
        ap = base_ap + main_hand_weapon.dps * WEAPON_POWER_COEFFICIENT;
      }
      // Unarmed is apparently a 0.5 dps weapon, Bruce Lee would be ashamed.
      else
      {
        ap = base_ap + .5 * WEAPON_POWER_COEFFICIENT;
      }
      break;
    case AP_WEAPON_OH:
      if ( has_oh )
      {
        ap = base_ap + off_hand_weapon.dps * WEAPON_POWER_COEFFICIENT;
      }
      else
      {
        ap = base_ap + .5 * WEAPON_POWER_COEFFICIENT;
      }
      break;
    case AP_WEAPON_BOTH:
    {
      // Don't use with weapon = player -> off_hand_weapon or the OH penalty will be applied to the whole spell
      ap = ( has_mh ? main_hand_weapon.dps : .5 ) + ( has_oh ? off_hand_weapon.dps : .5 ) / 2;
      ap *= 2.0 / 3.0 * WEAPON_POWER_COEFFICIENT;
      ap += base_ap;

      break;
    }
    // Nohand, just base AP then
    default:
      ap = base_ap;
      break;
  }

  return ap;
}

double player_t::composite_attack_power_multiplier() const
{
  double m = current.attack_power_multiplier;

  if ( is_pet() || is_enemy() )
  {
    return 1.0;
  }

  m *= 1.0 + sim->auras.battle_shout->check_value();

  return m;
}

double player_t::composite_melee_crit_chance() const
{
  double ac = current.attack_crit_chance + composite_melee_crit_rating() / current.rating.attack_crit;

  if ( current.attack_crit_per_agility )
    ac += ( cache.agility() / current.attack_crit_per_agility / 100.0 );

  ac += racials.viciousness->effectN( 1 ).percent();
  ac += racials.arcane_acuity->effectN( 1 ).percent();
  if(buffs.embrace_of_paku)
    ac += buffs.embrace_of_paku->check_value();

  if ( timeofday == DAY_TIME )
    ac += racials.touch_of_elune->effectN( 1 ).percent();

  return ac;
}

double player_t::composite_melee_expertise( const weapon_t* ) const
{
  double e = current.expertise;

  // e += composite_expertise_rating() / current.rating.expertise;

  return e;
}

double player_t::composite_melee_hit() const
{
  double ah = current.hit;

  // ah += composite_melee_hit_rating() / current.rating.attack_hit;

  return ah;
}

double player_t::composite_armor() const
{
  double a = current.stats.armor;

  a *= composite_base_armor_multiplier();

  a += cache.bonus_armor();

  // "Modify Armor%" effects affect all armor multiplicatively
  // TODO: What constitutes "bonus armor" and "base armor" in terms of client data?
  a *= composite_armor_multiplier();

  return a;
}

double player_t::composite_base_armor_multiplier() const
{
  double a = current.base_armor_multiplier;

  if ( meta_gem == META_AUSTERE_PRIMAL )
  {
    a += 0.02;
  }

  return a;
}

double player_t::composite_armor_multiplier() const
{
  double a = current.armor_multiplier;

  return a;
}

double player_t::composite_miss() const
{
  // this is sort of academic, since we know of no sources of miss that are actually subject to DR.
  // However, we've been told that it follows the same DR formula as the other avoidance stats, and
  // we've been given the relevant constants (def_dr.miss_factor etc.). I've coded this mostly for symmetry,
  // such that it will be fully-functional if in the future we suddenly gain some source of bonus_miss.

  // Start with sources not subject to DR - base miss (stored in current.miss)
  double total_miss = current.miss;

  // bonus_miss is miss from rating or other sources subject to DR
  double bonus_miss = 0.0;

  // if we have any bonus_miss, apply diminishing returns and add it to total_miss
  if ( bonus_miss > 0 )
    total_miss +=
        bonus_miss / ( def_dr.miss_factor * bonus_miss * 100 * def_dr.vertical_stretch + def_dr.horizontal_shift );

  assert( total_miss >= 0.0 && total_miss <= 1.0 );

  return total_miss;
}

/**
 * Composite block chance.
 *
 * Use composite_block_dr function to apply diminishing returns on the part of your block which is affected by it.
 * See paladin_t::composite_block() to see how this works.
 */
double player_t::composite_block() const
{
  return player_t::composite_block_dr( 0.0 );
}

/**
 * Helper function to calculate block value after diminishing returns.
 */
double player_t::composite_block_dr( double extra_block ) const
{
  // Start with sources not subject to DR - base block (stored in current.block)
  double total_block = current.block;

  // bonus_block is block from rating or other sources subject to DR (passed from class module via extra_block)
  double bonus_block = composite_block_rating() / current.rating.block;
  bonus_block += extra_block;

  // bonus_block gets rounded because that's how blizzard rolls...
  bonus_block = util::round( 12800 * bonus_block ) / 12800;

  // if we have any bonus_block, apply diminishing returns and add it to total_block
  if ( bonus_block > 0 )
    total_block +=
        bonus_block / ( def_dr.block_factor * bonus_block * 100 * def_dr.block_vertical_stretch + def_dr.horizontal_shift );

  return total_block;
}

double player_t::composite_dodge() const
{
  // Start with sources not subject to DR - base dodge + dodge from base agility (stored in current.dodge)
  double total_dodge = current.dodge;

  // bonus_dodge is from crit (through dodge rating) and bonus Agility
  double bonus_dodge = composite_dodge_rating() / current.rating.dodge;
  if ( !is_enemy() )
  {
    bonus_dodge += ( cache.agility() - dbc.race_base( race ).agility -
        dbc.attribute_base( type, level() ).agility ) * current.dodge_per_agility;
  }

  // if we have any bonus_dodge, apply diminishing returns and add it to total_dodge.
  if ( bonus_dodge != 0 )
    total_dodge +=
        bonus_dodge / ( def_dr.dodge_factor * bonus_dodge * 100 * def_dr.vertical_stretch + def_dr.horizontal_shift );

  return total_dodge;
}

double player_t::composite_parry() const
{
  // Start with sources not subject to DR - base parry + parry from base strength (stored in current.parry). 
  double total_parry = current.parry;

  // bonus_parry is from rating and bonus Strength
  double bonus_parry = composite_parry_rating() / current.rating.parry;
  if ( !is_enemy() )
  {
    bonus_parry += ( cache.strength() - dbc.race_base( race ).strength -
        dbc.attribute_base( type, level() ).strength ) * current.parry_per_strength;
  }

  // if we have any bonus_parry, apply diminishing returns and add it to total_parry.
  if ( bonus_parry != 0 )
    total_parry +=
    bonus_parry / ( def_dr.parry_factor * bonus_parry * 100 * def_dr.vertical_stretch + def_dr.horizontal_shift );

  return total_parry;
}

double player_t::composite_block_reduction( action_state_t* ) const
{
  double b = current.block_reduction;

  // Players have a base block reduction = 0, enemies have a fixed 0.30 damage reduction on block
  if ( b == 0 )
  {
    // The block reduction rating is equal to 2.5 times the equipped shield's armor rating
    return items[ SLOT_OFF_HAND ].stats.armor * 2.5;
  }

  return b;
}

double player_t::composite_crit_block() const
{
  return 0;
}

double player_t::composite_crit_avoidance() const
{
  return 0;
}

/**
 * This is the subset of the old_spell_haste that applies to RPPM
 */
double player_t::composite_spell_haste() const
{
  double h;

  h = std::max( 0.0, composite_spell_haste_rating() ) / current.rating.spell_haste;

  h = 1.0 / ( 1.0 + h );

  if ( !is_pet() && !is_enemy() )
  {
    if ( buffs.bloodlust->check() )
      h *= 1.0 / ( 1.0 + buffs.bloodlust->data().effectN( 1 ).percent() );

    if ( buffs.berserking->check() )
      h *= 1.0 / ( 1.0 + buffs.berserking->data().effectN( 1 ).percent() );

    h *= 1.0 / ( 1.0 + racials.nimble_fingers->effectN( 1 ).percent() );
    h *= 1.0 / ( 1.0 + racials.time_is_money->effectN( 1 ).percent() );

    if ( timeofday == NIGHT_TIME )
      h *= 1.0 / ( 1.0 + racials.touch_of_elune->effectN( 1 ).percent() );
  }

  return h;
}

/**
 * This is the old spell_haste and incorporates everything that buffs cast speed
 */
double player_t::composite_spell_speed() const
{
  auto speed = cache.spell_haste();

  if ( !is_pet() && !is_enemy() )
  {
    if ( buffs.tempus_repit->check() )
    {
      speed *= 1.0 / ( 1.0 + buffs.tempus_repit->data().effectN( 1 ).percent() );
    }

    if ( buffs.nefarious_pact )
    {
      speed *= 1.0 / ( 1.0 + buffs.nefarious_pact->check_stack_value() );
    }

    if ( buffs.devils_due )
    {
      speed *= 1.0 - buffs.devils_due->check_stack_value();
    }
  }

  return speed;
}

double player_t::composite_spell_power( school_e /* school */ ) const
{
  double sp = current.stats.spell_power;

  sp += current.spell_power_per_intellect * cache.intellect();
  sp += std::floor( current.spell_power_per_attack_power * cache.agility() );

  return sp;
}

double player_t::composite_spell_power_multiplier() const
{
  return current.spell_power_multiplier;
}

double player_t::composite_spell_crit_chance() const
{
  double sc = current.spell_crit_chance + composite_spell_crit_rating() / current.rating.spell_crit;

  if ( current.spell_crit_per_intellect > 0 )
  {
    sc += ( cache.intellect() / current.spell_crit_per_intellect / 100.0 );
  }

  sc += racials.viciousness->effectN( 1 ).percent();
  sc += racials.arcane_acuity->effectN( 1 ).percent();
  if(buffs.embrace_of_paku)
    sc += buffs.embrace_of_paku->check_value();

  if ( timeofday == DAY_TIME )
  {
    sc += racials.touch_of_elune->effectN( 1 ).percent();
  }

  return sc;
}

double player_t::composite_spell_hit() const
{
  double sh = current.hit;

  // sh += composite_spell_hit_rating() / current.rating.spell_hit;

  sh += composite_melee_expertise();

  return sh;
}

double player_t::composite_mastery() const
{
  return current.mastery + composite_mastery_rating() / current.rating.mastery;
}

double player_t::composite_bonus_armor() const
{
  return current.stats.bonus_armor;
}

double player_t::composite_damage_versatility() const
{
  double cdv = composite_damage_versatility_rating() / current.rating.damage_versatility;

  if ( !is_pet() && !is_enemy() )
  {
    if ( buffs.legendary_tank_buff )
      cdv += buffs.legendary_tank_buff->check_value();
  }

  if ( buffs.dmf_well_fed )
  {
    cdv += buffs.dmf_well_fed->check_value();
  }

  cdv += racials.mountaineer->effectN( 1 ).percent();
  cdv += racials.brush_it_off->effectN( 1 ).percent();

  return cdv;
}

double player_t::composite_heal_versatility() const
{
  double chv = composite_heal_versatility_rating() / current.rating.heal_versatility;

  if ( !is_pet() && !is_enemy() )
  {
    if ( buffs.legendary_tank_buff )
      chv += buffs.legendary_tank_buff->check_value();
  }

  if ( buffs.dmf_well_fed )
  {
    chv += buffs.dmf_well_fed->check_value();
  }

  chv += racials.mountaineer->effectN( 1 ).percent();
  chv += racials.brush_it_off->effectN( 1 ).percent();

  return chv;
}

double player_t::composite_mitigation_versatility() const
{
  double cmv = composite_mitigation_versatility_rating() / current.rating.mitigation_versatility;

  if ( !is_pet() && !is_enemy() )
  {
    if ( buffs.legendary_tank_buff )
      cmv += buffs.legendary_tank_buff->check_value() / 2;
  }

  if ( buffs.dmf_well_fed )
  {
    cmv += buffs.dmf_well_fed->check_value() / 2;
  }

  cmv += racials.mountaineer->effectN( 1 ).percent() / 2;
  cmv += racials.brush_it_off->effectN( 1 ).percent() / 2;

  return cmv;
}

double player_t::composite_leech() const
{
  return composite_leech_rating() / current.rating.leech;
}

double player_t::composite_run_speed() const
{
  // speed DRs using the following formula:
  double pct = composite_speed_rating() / current.rating.speed;

  double coefficient = std::exp( -.0003 * composite_speed_rating() );

  return pct * coefficient * .1;
}

double player_t::composite_avoidance() const
{
  return composite_avoidance_rating() / current.rating.avoidance;
}

double player_t::composite_player_pet_damage_multiplier( const action_state_t* ) const
{
  double m = 1.0;

  m *= 1.0 + racials.command->effectN( 1 ).percent();

  return m;
}

double player_t::composite_player_multiplier( school_e school ) const
{
  double m = 1.0;

  if ( buffs.brute_strength && buffs.brute_strength->check() )
  {
    m *= 1.0 + buffs.brute_strength->data().effectN( 1 ).percent();
  }

  if ( buffs.legendary_aoe_ring && buffs.legendary_aoe_ring->check() )
    m *= 1.0 + buffs.legendary_aoe_ring->default_value;

  if ( buffs.taste_of_mana && buffs.taste_of_mana->check() && school != SCHOOL_PHYSICAL )
  {
    m *= 1.0 + buffs.taste_of_mana->default_value;
  }

  if ( buffs.torrent_of_elements && buffs.torrent_of_elements->check() && school != SCHOOL_PHYSICAL )
  {
    m *= 1.0 + buffs.torrent_of_elements->default_value;
  }

  if ( buffs.damage_done && buffs.damage_done->check() )
  {
    m *= 1.0 + buffs.damage_done->check_stack_value();
  }

  if ( school != SCHOOL_PHYSICAL )
    m *= 1.0 + racials.magical_affinity->effectN( 1 ).percent();

  // 8.0 Legendary Insignia of the Grand Army Effect
  m *= insignia_of_the_grand_army_multiplier;

  return m;
}

double player_t::composite_player_td_multiplier( school_e /* school */, const action_t* /* a */ ) const
{
  return 1.0;
}

double player_t::composite_player_target_multiplier( player_t* target, school_e /* school */ ) const
{
  double m = 1.0;

  if ( target->race == RACE_DEMON && buffs.demon_damage_buff && buffs.demon_damage_buff->check() )
  {
    // Bad idea to hardcode the effect number, but it'll work for now. The buffs themselves are
    // stat buffs.
    m *= 1.0 + buffs.demon_damage_buff->data().effectN( 2 ).percent();
  }

  return m;
}

double player_t::composite_player_heal_multiplier( const action_state_t* ) const
{
  return 1.0;
}

double player_t::composite_player_th_multiplier( school_e /* school */ ) const
{
  return 1.0;
}

double player_t::composite_player_absorb_multiplier( const action_state_t* ) const
{
  return 1.0;
}

double player_t::composite_player_critical_damage_multiplier( const action_state_t* /* s */ ) const
{
  double m = 1.0;

  m *= 1.0 + racials.brawn->effectN( 1 ).percent();
  m *= 1.0 + racials.might_of_the_mountain->effectN( 1 ).percent();
  if ( buffs.incensed )
  {
    m *= 1.0 + buffs.incensed->check_value();
  }

  return m;
}

double player_t::composite_player_critical_healing_multiplier() const
{
  double m = 1.0;

  m += racials.brawn->effectN( 1 ).percent();
  m += racials.might_of_the_mountain->effectN( 1 ).percent();

  return m;
}

/**
 * Return the highest-only temporary movement modifier.
 *
 * There are 2 categories of movement speed buffs: Passive and Temporary,
 * both which stack additively. Passive buffs include movement speed enchant, unholy presence, cat form and generally
 * anything that has the ability to be kept active all fight. These permanent buffs do stack with each other.
 * Temporary includes all other speed bonuses, however, only the highest temporary bonus will be added on top.
 */
double player_t::temporary_movement_modifier() const
{
  double temporary = 0;

  if ( !is_enemy() )
  {
    if ( buffs.stampeding_roar->check() )
      temporary = std::max( buffs.stampeding_roar->data().effectN( 1 ).percent(), temporary );
  }

  if ( !is_enemy() && !is_pet() )
  {
    if ( buffs.darkflight->check() )
      temporary = std::max( buffs.darkflight->data().effectN( 1 ).percent(), temporary );

    if ( buffs.nitro_boosts->check() )
      temporary = std::max( buffs.nitro_boosts->data().effectN( 1 ).percent(), temporary );

    if ( buffs.body_and_soul->check() )
      temporary = std::max( buffs.body_and_soul->data().effectN( 1 ).percent(), temporary );

    if ( buffs.angelic_feather->check() )
      temporary = std::max( buffs.angelic_feather->data().effectN( 1 ).percent(), temporary );

    if ( buffs.normalization_increase && buffs.normalization_increase->check() )
    {
      temporary = std::max( buffs.normalization_increase->data().effectN( 3 ).percent(), temporary );
    }
  }

  return temporary;
}

/**
 * Return the sum of all passive movement modifier.
 *
 * There are 2 categories of movement speed buffs: Passive and Temporary,
 * both which stack additively. Passive buffs include movement speed enchant, unholy presence, cat form and generally
 * anything that has the ability to be kept active all fight. These permanent buffs do stack with each other.
 * Temporary includes all other speed bonuses, however, only the highest temporary bonus will be added on top.
 */
double player_t::passive_movement_modifier() const
{
  double passive = passive_modifier;

  if ( race == RACE_ZANDALARI_TROLL && zandalari_loa == GONK )
  {
    passive += find_spell( 292362 )->effectN( 1 ).percent();
  }
  passive += racials.quickness->effectN( 2 ).percent();
  if ( buffs.aggramars_stride )
  {
    passive += ( buffs.aggramars_stride->check_value() *
                 ( ( 1.0 / cache.attack_haste() - 1.0 ) > cache.attack_crit_chance()
                       ? ( 1.0 / cache.attack_haste() - 1.0 )
                       : cache.attack_crit_chance() ) );  // Takes the larger of the two values.
  }
  passive += composite_run_speed();

  return passive;
}

/**
 * Return the combined (passive+temporary) movement speed.
 *
 * Multiplicative movement modifiers like snares can be applied here. They work similar to temporary speed boosts
 * in that only the highest one counts.
 */
double player_t::composite_movement_speed() const
{
  double speed = base_movement_speed;

  double passive = passive_movement_modifier();

  double temporary = temporary_movement_modifier();

  speed *= ( 1 + passive + temporary );

  return speed;
}

double player_t::composite_attribute( attribute_e attr ) const
{
  return current.stats.attribute[ attr ];
}

double player_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = current.attribute_multiplier[ attr ];

  if ( is_pet() || is_enemy() )
    return m;

  if ( ( true_level >= 50 ) && matching_gear )
  {
    m *= 1.0 + matching_gear_multiplier( attr );
  }

  switch ( attr )
  {
    case ATTR_STRENGTH:
      if ( buffs.archmages_greater_incandescence_str->check() )
        m *= 1.0 + buffs.archmages_greater_incandescence_str->data().effectN( 1 ).percent();
      if ( buffs.archmages_incandescence_str->check() )
        m *= 1.0 + buffs.archmages_incandescence_str->data().effectN( 1 ).percent();
      break;
    case ATTR_AGILITY:
      if ( buffs.archmages_greater_incandescence_agi->check() )
        m *= 1.0 + buffs.archmages_greater_incandescence_agi->data().effectN( 1 ).percent();
      if ( buffs.archmages_incandescence_agi->check() )
        m *= 1.0 + buffs.archmages_incandescence_agi->data().effectN( 1 ).percent();
      break;
    case ATTR_INTELLECT:
      if ( buffs.archmages_greater_incandescence_int->check() )
        m *= 1.0 + buffs.archmages_greater_incandescence_int->data().effectN( 1 ).percent();
      if ( buffs.archmages_incandescence_int->check() )
        m *= 1.0 + buffs.archmages_incandescence_int->data().effectN( 1 ).percent();
      if ( sim->auras.arcane_intellect->check() )
        m *= 1.0 + sim->auras.arcane_intellect->value();
      break;
    case ATTR_SPIRIT:
      if ( buffs.amplification )
        m *= 1.0 + passive_values.amplification_1;
      if ( buffs.amplification_2 )
        m *= 1.0 + passive_values.amplification_2;
      break;
    case ATTR_STAMINA:
      if ( sim->auras.power_word_fortitude->check() )
      {
        m *= 1.0 + sim->auras.power_word_fortitude->value();
      }
      break;
    default:
      break;
  }

  return m;
}

double player_t::composite_rating_multiplier( rating_e rating ) const
{
  double v = 1.0;

  switch ( rating )
  {
    case RATING_SPELL_HASTE:
    case RATING_MELEE_HASTE:
    case RATING_RANGED_HASTE:
      if ( buffs.amplification )
        v *= 1.0 + passive_values.amplification_1;
      if ( buffs.amplification_2 )
        v *= 1.0 + passive_values.amplification_2;
      v *= 1.0 + racials.the_human_spirit->effectN( 1 ).percent();
      break;
    case RATING_MASTERY:
      if ( buffs.amplification )
        v *= 1.0 + passive_values.amplification_1;
      if ( buffs.amplification_2 )
        v *= 1.0 + passive_values.amplification_2;
      v *= 1.0 + racials.the_human_spirit->effectN( 1 ).percent();
      break;
    case RATING_SPELL_CRIT:
    case RATING_MELEE_CRIT:
    case RATING_RANGED_CRIT:
      v *= 1.0 + racials.the_human_spirit->effectN( 1 ).percent();
      break;
    case RATING_DAMAGE_VERSATILITY:
    case RATING_HEAL_VERSATILITY:
    case RATING_MITIGATION_VERSATILITY:
      v *= 1.0 + racials.the_human_spirit->effectN( 1 ).percent();
      break;
    default:
      break;
  }

  return v;
}

double player_t::composite_rating( rating_e rating ) const
{
  double v = 0;

  // Internally, we treat all the primary rating types as a single entity;
  // in game, they are actually split into spell/ranged/melee
  switch ( rating )
  {
    case RATING_SPELL_CRIT:
    case RATING_MELEE_CRIT:
    case RATING_RANGED_CRIT:
      v = current.stats.crit_rating;
      break;
    case RATING_SPELL_HASTE:
    case RATING_MELEE_HASTE:
    case RATING_RANGED_HASTE:
      v = current.stats.haste_rating;
      break;
    case RATING_SPELL_HIT:
    case RATING_MELEE_HIT:
    case RATING_RANGED_HIT:
      v = current.stats.hit_rating;
      break;
    case RATING_MASTERY:
      v = current.stats.mastery_rating;
      break;
    case RATING_DAMAGE_VERSATILITY:
    case RATING_HEAL_VERSATILITY:
    case RATING_MITIGATION_VERSATILITY:
      v = current.stats.versatility_rating;
      break;
    case RATING_EXPERTISE:
      v = current.stats.expertise_rating;
      break;
    case RATING_DODGE:
      v = current.stats.dodge_rating;
      break;
    case RATING_PARRY:
      v = current.stats.parry_rating;
      break;
    case RATING_BLOCK:
      v = current.stats.block_rating;
      break;
    case RATING_LEECH:
      v = current.stats.leech_rating;
      break;
    case RATING_SPEED:
      v = current.stats.speed_rating;
      break;
    case RATING_AVOIDANCE:
      v = current.stats.avoidance_rating;
      break;
    default:
      break;
  }

  return util::round( v * composite_rating_multiplier( rating ), 0 );
}

double player_t::composite_player_vulnerability( school_e school ) const
{
  double m = debuffs.invulnerable && debuffs.invulnerable->check() ? 0.0 : 1.0;

  if ( debuffs.vulnerable && debuffs.vulnerable->check() )
    m *= 1.0 + debuffs.vulnerable->value();

  // 1% damage taken per stack, arbitrary because this buff is completely fabricated!
  if ( debuffs.damage_taken && debuffs.damage_taken->check() )
    m *= 1.0 + debuffs.damage_taken->current_stack * 0.01;

  if ( debuffs.mystic_touch &&
       dbc::has_common_school( debuffs.mystic_touch->data().effectN( 1 ).school_type(), school ) )
    m *= 1.0 + debuffs.mystic_touch->value();

  if ( debuffs.chaos_brand && dbc::has_common_school( debuffs.chaos_brand->data().effectN( 1 ).school_type(), school ) )
    m *= 1.0 + debuffs.chaos_brand->value();

  return m;
}

double player_t::composite_mitigation_multiplier( school_e /* school */ ) const
{
  return 1.0;
}

double player_t::composite_mastery_value() const
{
  return composite_mastery() * mastery_coefficient();
}

#if defined( SC_USE_STAT_CACHE )

/**
 * Invalidate a stat cache, resulting in re-calculation of the composite stat value.
 */
void player_t::invalidate_cache( cache_e c )
{
  if ( !cache.active )
    return;

  sim->print_debug( "{} invalidates stat cache for {}.", *this, util::cache_type_string( c ) );

  // Special linked invalidations
  switch ( c )
  {
    case CACHE_STRENGTH:
      if ( current.attack_power_per_strength > 0 )
        invalidate_cache( CACHE_ATTACK_POWER );
      if ( current.parry_per_strength > 0 )
        invalidate_cache( CACHE_PARRY );
      break;
    case CACHE_AGILITY:
      if ( current.attack_power_per_agility > 0 )
        invalidate_cache( CACHE_ATTACK_POWER );
      if ( current.dodge_per_agility > 0 )
        invalidate_cache( CACHE_DODGE );
      if ( current.spell_power_per_attack_power > 0 )
      {
        invalidate_cache( CACHE_SPELL_POWER );
        invalidate_cache( CACHE_ATTACK_POWER );
      }
      break;
    case CACHE_INTELLECT:
      if ( current.spell_power_per_intellect > 0 )
        invalidate_cache( CACHE_SPELL_POWER );
      break;
    case CACHE_ATTACK_HASTE:
      invalidate_cache( CACHE_ATTACK_SPEED );
      invalidate_cache( CACHE_RPPM_HASTE );
      break;
    case CACHE_SPELL_HASTE:
      invalidate_cache( CACHE_SPELL_SPEED );
      invalidate_cache( CACHE_RPPM_HASTE );
      break;
    case CACHE_BONUS_ARMOR:
      invalidate_cache( CACHE_ARMOR );
      break;
    case CACHE_ATTACK_CRIT_CHANCE:
      invalidate_cache( CACHE_RPPM_CRIT );
      break;
    case CACHE_SPELL_CRIT_CHANCE:
      invalidate_cache( CACHE_RPPM_CRIT );
      break;
    default:
      break;
  }

  // Normal invalidation of the corresponding Cache
  switch ( c )
  {
    case CACHE_EXP:
      invalidate_cache( CACHE_ATTACK_EXP );
      invalidate_cache( CACHE_SPELL_HIT );
      break;
    case CACHE_HIT:
      invalidate_cache( CACHE_ATTACK_HIT );
      invalidate_cache( CACHE_SPELL_HIT );
      break;
    case CACHE_CRIT_CHANCE:
      invalidate_cache( CACHE_ATTACK_CRIT_CHANCE );
      invalidate_cache( CACHE_SPELL_CRIT_CHANCE );
      break;
    case CACHE_HASTE:
      invalidate_cache( CACHE_ATTACK_HASTE );
      invalidate_cache( CACHE_SPELL_HASTE );
      break;
    case CACHE_SPEED:
      invalidate_cache( CACHE_ATTACK_SPEED );
      invalidate_cache( CACHE_SPELL_SPEED );
      break;
    case CACHE_VERSATILITY:
      invalidate_cache( CACHE_DAMAGE_VERSATILITY );
      invalidate_cache( CACHE_HEAL_VERSATILITY );
      invalidate_cache( CACHE_MITIGATION_VERSATILITY );
      break;
    default:
      cache.invalidate( c );
      break;
  }
}
#else
void invalidate_cache( cache_e )
{
}
#endif

void player_t::sequence_add_wait( const timespan_t& amount, const timespan_t& ts )
{
  // Collect iteration#1 data, for log/debug/iterations==1 simulation iteration#0 data
  if ( ( sim->iterations <= 1 && sim->current_iteration == 0 ) ||
       ( sim->iterations > 1 && nth_iteration() == 1 ) )
  {
    if ( collected_data.action_sequence.size() <= sim->expected_max_time() * 2.0 + 3.0 )
    {
      if ( in_combat )
      {
        if ( collected_data.action_sequence.size() &&
             collected_data.action_sequence.back().wait_time > timespan_t::zero() )
          collected_data.action_sequence.back().wait_time += amount;
        else
          collected_data.action_sequence.emplace_back( ts, amount, this );
      }
    }
    else
    {
      assert(
          false &&
          "Collected too much action sequence data."
          "This means there is a serious overflow of executed actions in the first iteration, which should be fixed." );
    }
  }
}

void player_t::sequence_add( const action_t* a, const player_t* target, const timespan_t& ts )
{
  // Collect iteration#1 data, for log/debug/iterations==1 simulation iteration#0 data
  if ( ( a->sim->iterations <= 1 && a->sim->current_iteration == 0 ) ||
       ( a->sim->iterations > 1 && nth_iteration() == 1 ) )
  {
    if ( collected_data.action_sequence.size() <= sim->expected_max_time() * 2.0 + 3.0 )
    {
      if ( in_combat )
        collected_data.action_sequence.emplace_back( a, target, ts, this );
      else
        collected_data.action_sequence_precombat.emplace_back( a, target, ts, this );
    }
    else
    {
      assert(
          false &&
          "Collected too much action sequence data."
          "This means there is a serious overflow of executed actions in the first iteration, which should be fixed." );
    }
  }
}

void player_t::combat_begin()
{
  sim->print_debug( "Combat begins for {}.", *this );

  if ( !is_pet() && !is_add() )
  {
    arise();
  }

  init_resources( true );

  // Execute pre-combat actions
  if ( !is_pet() && !is_add() )
  {
    for ( auto& action : precombat_action_list )
    {
      if ( action->action_ready() )
      {
        if ( action->harmful )
        {
          if ( first_cast )
          {
            if ( !is_enemy() )
            {
              sequence_add( action, action->target, sim->current_time() );
            }
            action->execute();
            first_cast = false;
          }
          else
          {
            sim->print_debug( "{} attempting to cast multiple harmful spells during pre-combat.", *this );
          }
        }
        else
        {
          if ( !is_enemy() )
          {
            sequence_add( action, action->target, sim->current_time() );
          }
          action->execute();
        }
      }
    }
  }
  first_cast = false;

  if ( !precombat_action_list.empty() )
    in_combat = true;

  // re-initialize collected_data.health_changes.previous_*_level
  // necessary because food/flask are counted as resource gains, and thus provide phantom
  // gains on the timeline if not corrected
  collected_data.health_changes.previous_gain_level     = 0.0;
  collected_data.health_changes_tmi.previous_gain_level = 0.0;
  // forcing a resource timeline data collection in combat_end() seems to have rendered this next line unnecessary
  collected_data.health_changes.previous_loss_level     = 0.0;
  collected_data.health_changes_tmi.previous_gain_level = 0.0;

  if ( buffs.cooldown_reduction )
    buffs.cooldown_reduction->trigger();

  if ( buffs.amplification )
    buffs.amplification->trigger();

  if ( buffs.amplification_2 )
    buffs.amplification_2->trigger();

  if ( buffs.aggramars_stride )
    buffs.aggramars_stride->trigger();

  if ( buffs.norgannons_foresight_ready )
    buffs.norgannons_foresight_ready->trigger();

  if ( buffs.tyrants_decree_driver )
  {  // Assume actor has stacked the buff to max stack precombat.
    buffs.tyrants_decree_driver->trigger();
    buffs.tyrants_immortality->trigger( buffs.tyrants_immortality->max_stack() );
  }

  // Trigger registered combat-begin functions
  for ( const auto& f : combat_begin_functions)
  {
    f( this );
  }
}

void player_t::combat_end()
{
  for ( size_t i = 0; i < pet_list.size(); ++i )
  {
    pet_list[ i ]->combat_end();
  }

  if ( !is_pet() )
  {
    demise();
  }
  else
    cast_pet()->dismiss();

  sim->print_debug( "Combat ends for {} at time {} fight_length={}", *this,
                           sim->current_time(), iteration_fight_length );

  // Defer parent actor find to combat end, and ensure it is only performed if the parent sim is
  // initialized. This will avoid a data race in case the main thread for some reason is in init
  // phase, while a child thread is already done (their init). Note that this may mean that with
  // target_error option, some data to estimate the target error can be missed (in the main thread).
  // In turn, lazily finding the parent actor here ensures that the performance hit on the init
  // process is minimal (no need for locks).
  if ( parent == nullptr && !is_pet() && !is_enemy() && sim->parent != nullptr &&
      sim->parent->initialized == true && sim->thread_index > 0 )
  {
    // NOTE NOTE NOTE: This search can no longer be run based on find_player() because it uses
    // actor_list. Ever since pet_spawner support, the actor_list of the parent sim can (and will)
    // change during run-time. This can cause iterator breakage (vector capacity increase), causing
    // a crash.
    //
    // Since the parent actor is only necessary for real player characters, we can instead look at
    // player_no_pet_list, which is guaranteed to be static after initialization (for now ...).
    auto it = range::find_if( sim->parent->player_no_pet_list, [ this ]( const player_t* p ) {
      return name_str == p->name_str;
    } );

    if ( it != sim->parent->player_no_pet_list.end() )
    {
      parent = *it;
    }
  }
}

/**
 * Begin data collection for statistics.
 */
void player_t::datacollection_begin()
{
  // Check whether the actor was arisen at least once during the _previous_ iteration
  // Note that this check is dependant on sim_t::combat_begin() having
  // sim_t::datacollection_begin() call before the player_t::combat_begin() calls.
  if ( !active_during_iteration )
    return;

  sim->print_debug( "Data collection begins for {} (id={})", *this, index );

  iteration_fight_length                = timespan_t::zero();
  iteration_waiting_time                = timespan_t::zero();
  iteration_pooling_time                = timespan_t::zero();
  iteration_executed_foreground_actions = 0;
  iteration_dmg                         = 0;
  priority_iteration_dmg                = 0;
  iteration_heal                        = 0;
  iteration_absorb                      = 0.0;
  iteration_absorb_taken                = 0.0;
  iteration_dmg_taken                   = 0;
  iteration_heal_taken                  = 0;
  active_during_iteration               = false;

  range::fill( iteration_resource_lost, 0.0 );
  range::fill( iteration_resource_gained, 0.0 );

  if ( collected_data.health_changes.collect )
  {
    collected_data.health_changes.timeline.clear();  // Drop Data
    collected_data.health_changes.timeline_normalized.clear();
  }

  if ( collected_data.health_changes_tmi.collect )
  {
    collected_data.health_changes_tmi.timeline.clear();  // Drop Data
    collected_data.health_changes_tmi.timeline_normalized.clear();
  }

  range::for_each( buff_list, std::mem_fn( &buff_t::datacollection_begin ) );
  range::for_each( stats_list, std::mem_fn( &stats_t::datacollection_begin ) );
  range::for_each( uptime_list, std::mem_fn( &uptime_t::datacollection_begin ) );
  range::for_each( benefit_list, std::mem_fn( &benefit_t::datacollection_begin ) );
  range::for_each( proc_list, std::mem_fn( &proc_t::datacollection_begin ) );
  range::for_each( pet_list, std::mem_fn( &pet_t::datacollection_begin ) );
  range::for_each( sample_data_list, std::mem_fn( &luxurious_sample_data_t::datacollection_begin ) );
}

/**
 * End data collection for statistics.
 */
void player_t::datacollection_end()
{
  // This checks if the actor was arisen at least once during this iteration.
  if ( !requires_data_collection() )
    return;

  for ( size_t i = 0; i < pet_list.size(); ++i )
    pet_list[ i ]->datacollection_end();

  if ( arise_time >= timespan_t::zero() )
  {
    // If we collect data while the player is still alive, capture active time up to now
    assert( sim->current_time() >= arise_time );
    iteration_fight_length += sim->current_time() - arise_time;
    arise_time = sim->current_time();
  }

  range::for_each( spawners, []( spawner::base_actor_spawner_t* spawner ) {
    spawner->datacollection_end();
  } );

  sim->print_debug( "Data collection ends for {} (id={}) at time {} fight_length={}", *this, index,
                           sim->current_time(), iteration_fight_length );

  for ( auto& stats : stats_list )
  {
    stats->datacollection_end();
  }

  if ( !is_enemy() && !is_add() )
  {
    sim->iteration_dmg += iteration_dmg;
    sim->priority_iteration_dmg += priority_iteration_dmg;
    sim->iteration_heal += iteration_heal;
    sim->iteration_absorb += iteration_absorb;
  }

  // make sure TMI-relevant timeline lengths all match for tanks
  if ( !is_enemy() && !is_pet() && primary_role() == ROLE_TANK )
  {
    collected_data.timeline_healing_taken.add( sim->current_time(), 0.0 );
    collected_data.timeline_dmg_taken.add( sim->current_time(), 0.0 );
    collected_data.health_changes.timeline.add( sim->current_time(), 0.0 );
    collected_data.health_changes.timeline_normalized.add( sim->current_time(), 0.0 );
    collected_data.health_changes_tmi.timeline.add( sim->current_time(), 0.0 );
    collected_data.health_changes_tmi.timeline_normalized.add( sim->current_time(), 0.0 );
  }
  collected_data.collect_data( *this );

  range::for_each( buff_list, std::mem_fn( &buff_t::datacollection_end ) );

  for ( auto& uptime : uptime_list )
  {
    uptime->datacollection_end( iteration_fight_length );
  }

  range::for_each( benefit_list, std::mem_fn( &benefit_t::datacollection_end ) );
  range::for_each( proc_list, std::mem_fn( &proc_t::datacollection_end ) );
  range::for_each( sample_data_list, std::mem_fn( &luxurious_sample_data_t::datacollection_end ) );
}

// player_t::merge ==========================================================

namespace
{
namespace buff_merge
{
// a < b iff ( a.name < b.name || ( a.name == b.name && a.source < b.source ) )
bool compare( const buff_t* a, const buff_t* b )
{
  assert( a );
  assert( b );

  if ( a->name_str < b->name_str )
    return true;
  if ( b->name_str < a->name_str )
    return false;

  // NULL and player are identically considered "bottom" for source comparison
  bool a_is_bottom = ( !a->source || a->source == a->player );
  bool b_is_bottom = ( !b->source || b->source == b->player );

  if ( a_is_bottom )
    return !b_is_bottom;

  if ( b_is_bottom )
    return false;

  // If neither source is bottom, order by source index
  return a->source->index < b->source->index;
}

// Sort buff_list and check for uniqueness
void prepare( player_t& p )
{
  range::sort( p.buff_list, compare );
  // For all i, p.buff_list[ i ] <= p.buff_list[ i + 1 ]

#ifndef NDEBUG
  if ( p.buff_list.empty() )
    return;

  for ( size_t i = 0, last = p.buff_list.size() - 1; i < last; ++i )
  {
    // We know [ i ] <= [ i + 1 ] due to sorting; if also
    // ! ( [ i ] < [ i + 1 ] ), then [ i ] == [ i + 1 ].
    if ( !compare( p.buff_list[ i ], p.buff_list[ i + 1 ] ) )
    {
      p.sim->error( "{} has duplicate buffs named '{}' with source '{}' - the end is near.", p,
                     p.buff_list[ i ]->name(), p.buff_list[ i ]->source_name() );
    }
  }
#endif
}

void report_unmatched( const buff_t& b )
{
#ifndef NDEBUG
  /* Don't complain about targetdata buffs, since it is perfectly viable that the buff
   * is not created in another thread, because of our on-demand targetdata creation
   */
  if ( !b.source || b.source == b.player )
  {
    b.sim->error( "{} can't merge buff '{}' with source '{}'.", *b.player, b.name(), b.source_name() );
  }
#else
  // "Use" the parameters to silence compiler warnings.
  (void)b;
#endif
}

void check_tail( player_t& p, size_t first )
{
#ifndef NDEBUG
  for ( size_t last = p.buff_list.size(); first < last; ++first )
    report_unmatched( *p.buff_list[ first ] );
#else
  // "Use" the parameters to silence compiler warnings.
  (void)p;
  (void)first;
#endif
}

// Merge stats of matching buffs from right into left.
void merge( player_t& left, player_t& right )
{
  prepare( left );
  prepare( right );

  // Both buff_lists are sorted, join them mergesort-style.
  size_t i = 0, j = 0;
  while ( i < left.buff_list.size() && j < right.buff_list.size() )
  {
    if ( compare( left.buff_list[ i ], right.buff_list[ j ] ) )
    {
      // [ i ] < [ j ]
      report_unmatched( *left.buff_list[ i ] );
      ++i;
    }
    else if ( compare( right.buff_list[ j ], left.buff_list[ i ] ) )
    {
      // [ i ] > [ j ]
      report_unmatched( *right.buff_list[ j ] );
      ++j;
    }
    else
    {
      // [ i ] == [ j ]
      left.buff_list[ i ]->merge( *right.buff_list[ j ] );
      ++i, ++j;
    }
  }

  check_tail( left, i );
  check_tail( right, j );
}

}  // namespace buff_merge
}  // namespace

/**
 * Merge player data with player from other thread.
 *
 * After simulating the same setup in multiple sim/threads, this function will merge the same player (and its collected
 * data) from two simulations together, aggregating the collected data.
 */
void player_t::merge( player_t& other )
{
  collected_data.merge( other );

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; ++i )
  {
    iteration_resource_lost[ i ] += other.iteration_resource_lost[ i ];
    iteration_resource_gained[ i ] += other.iteration_resource_gained[ i ];
  }

  buff_merge::merge( *this, other );

  // Procs
  for ( size_t i = 0; i < proc_list.size(); ++i )
  {
    proc_t& proc = *proc_list[ i ];
    if ( proc_t* other_proc = other.find_proc( proc.name_str ) )
      proc.merge( *other_proc );
    else
    {
#ifndef NDEBUG
      sim->error( "{} can't merge proc '{}'.", *this, proc.name() );
#endif
    }
  }

  // Gains
  for ( size_t i = 0; i < gain_list.size(); ++i )
  {
    gain_t& gain = *gain_list[ i ];
    if ( gain_t* other_gain = other.find_gain( gain.name_str ) )
      gain.merge( *other_gain );
    else
    {
#ifndef NDEBUG
      sim->error( "{} can't merge gain '{}'.", *this, gain.name() );
#endif
    }
  }

  // Stats
  for ( size_t i = 0; i < stats_list.size(); ++i )
  {
    stats_t& stats = *stats_list[ i ];
    if ( stats_t* other_stats = other.find_stats( stats.name_str ) )
      stats.merge( *other_stats );
    else
    {
#ifndef NDEBUG
      sim->error( "{} can't merge stats '{}'", *this, stats.name() );
#endif
    }
  }

  // Uptimes
  for ( size_t i = 0; i < uptime_list.size(); ++i )
  {
    uptime_t& uptime = *uptime_list[ i ];
    if ( uptime_t* other_uptime = other.find_uptime( uptime.name_str ) )
      uptime.merge( *other_uptime );
    else
    {
#ifndef NDEBUG
      sim->error( "{} can't merge uptime '{}'.", *this, uptime.name() );
#endif
    }
  }

  // Benefits
  for ( size_t i = 0; i < benefit_list.size(); ++i )
  {
    benefit_t& benefit = *benefit_list[ i ];
    if ( benefit_t* other_benefit = other.find_benefit( benefit.name_str ) )
      benefit.merge( *other_benefit );
    else
    {
#ifndef NDEBUG
      sim->error( "{} can't merge benefit '{}'.", *this, benefit.name() );
#endif
    }
  }

  // Sample Data
  for ( size_t i = 0; i < sample_data_list.size(); ++i )
  {
    luxurious_sample_data_t& sd = *sample_data_list[ i ];
    if ( luxurious_sample_data_t* other_sd = other.find_sample_data( sd.name_str ) )
      sd.merge( *other_sd );
    else
    {
#ifndef NDEBUG
      sim->error("{} can't merge proc '{}'.", *this, sd.name_str );
#endif
    }
  }

  // Action Map
  size_t n_entries = std::min( action_list.size(), other.action_list.size() );
  if ( action_list.size() != other.action_list.size() )
  {
    sim->error( "{} can't merge action lists: size differs (other={}, size={}, other.size={})!", *this,
                 other.name(), action_list.size(), other.action_list.size() );
  }

  for ( size_t i = 0; i < n_entries; ++i )
  {
    if ( action_list[ i ]->internal_id == other.action_list[ i ]->internal_id )
    {
      action_list[ i ]->total_executions += other.action_list[ i ]->total_executions;
    }
    else
    {
      sim->error(
          "{} can't merge action {}::{} with {}::{} because ids do not match.", *this,
          action_list[ i ]->action_list ? action_list[ i ]->action_list->name_str.c_str() : "(none)",
          action_list[ i ]->signature_str,
          other.action_list[ i ]->action_list ? other.action_list[ i ]->action_list->name_str.c_str() : "(none)",
          other.action_list[ i ]->signature_str );
    }
  }
}

/**
 * Reset player. Called after each iteration to reset the player to its initial state.
 */
void player_t::reset()
{
  sim->print_debug( "Resetting {}.", *this );

  last_cast = timespan_t::zero();
  gcd_ready = timespan_t::zero();
  off_gcd_ready = timespan_t::min();
  cast_while_casting_ready = timespan_t::min();

  cache.invalidate_all();

  // Reset current stats to initial stats
  current = initial;

  current.sleeping = true;

  change_position( initial.position );

  // Restore default target
  target = default_target;

  sim->print_debug( "{} resets current stats ( reset to initial ): {}", *this, current.to_string() );

  for ( auto& buff : buff_list )
    buff->reset();

  last_foreground_action = nullptr;
  prev_gcd_actions.clear();
  off_gcdactions.clear();

  first_cast = true;

  executing       = nullptr;
  queueing        = nullptr;
  channeling      = nullptr;
  readying        = nullptr;
  strict_sequence = 0;
  off_gcd         = nullptr;
  cast_while_casting_poll_event = nullptr;
  in_combat       = false;

  current_execute_type = execute_type::FOREGROUND;

  current_attack_speed    = 1.0;
  gcd_current_haste_value = 1.0;
  gcd_haste_type          = HASTE_NONE;

  cast_delay_reaction = timespan_t::zero();
  cast_delay_occurred = timespan_t::zero();

  main_hand_weapon.buff_type  = 0;
  main_hand_weapon.buff_value = 0;
  main_hand_weapon.bonus_dmg  = 0;

  off_hand_weapon.buff_type  = 0;
  off_hand_weapon.buff_value = 0;
  off_hand_weapon.bonus_dmg  = 0;

  x_position = default_x_position;
  y_position = default_y_position;

  callbacks.reset();

  init_resources( true );

  // Variable optimization must be done before action expression optimization (occurs in
  // action_t::reset) since action expression optimization may reference the variables.
  if ( sim->optimize_expressions && nth_iteration() == 1 )
  {
    range::for_each( variables, []( action_variable_t* var ) { var->optimize(); } );
  }

  range::for_each( action_list, []( action_t* action ) { action->reset(); } );

  range::for_each( cooldown_list, []( cooldown_t* cooldown ) { cooldown->reset_init(); } );

  range::for_each( dot_list, []( dot_t* dot ) { dot->reset(); } );

  range::for_each( stats_list, []( stats_t* stat ) { stat->reset(); } );

  range::for_each( uptime_list, []( uptime_t* uptime ) { uptime->reset(); } );

  range::for_each( proc_list, []( proc_t* proc ) { proc->reset(); } );

  range::for_each( rppm_list, []( real_ppm_t* rppm ) { rppm->reset(); } );

  range::for_each( shuffled_rng_list, []( shuffled_rng_t* shuffled_rng ) { shuffled_rng->reset(); } );

  range::for_each( spawners, []( spawner::base_actor_spawner_t* obj ) { obj->reset(); } );

  potion_used = 0;

  item_cooldown.reset( false );

  incoming_damage.clear();

  resource_threshold_trigger = 0;

  for ( auto& elem : variables )
    elem->reset();

#ifndef NDEBUG
  for ( auto& elem : active_dots )
  {
    assert( elem == 0 );
  }
#endif

  if ( active_action_list != default_action_list )
  {
    active_action_list = default_action_list;
  }

  if ( active_off_gcd_list && active_off_gcd_list != default_action_list )
  {
    active_off_gcd_list = default_action_list;
  }

  if ( active_cast_while_casting_list && active_cast_while_casting_list != default_action_list )
  {
    active_cast_while_casting_list = default_action_list;
  }

  reset_resource_callbacks();
}

void player_t::trigger_ready()
{
  if ( ready_type == READY_POLL )
    return;

  if ( readying )
    return;
  if ( executing )
    return;
  if ( queueing )
    return;
  if ( channeling )
    return;
  if ( started_waiting < timespan_t::zero() )
    return;
  if ( current.sleeping )
    return;

  if ( buffs.stunned->check() )
    return;

  sim->print_debug( "{} is triggering ready, interval={}", *this,
                           ( sim->current_time() - started_waiting ) );

  iteration_waiting_time += sim->current_time() - started_waiting;
  started_waiting = timespan_t::min();

  schedule_ready( available() );
}

void player_t::schedule_off_gcd_ready( timespan_t delta_time )
{
  if ( active_off_gcd_list == nullptr )
  {
    return;
  }

  if ( current_execute_type != execute_type::OFF_GCD )
  {
    return;
  }

  // Already polling, do nothing
  if ( off_gcd )
  {
    return;
  }

  // Casting or channeling something
  if ( executing || channeling )
  {
    return;
  }

  if ( !readying )
  {
    return;
  }

  // No time left to perform a poll event before GCD elapses
  if ( sim->current_time() + delta_time >= readying->occurs() )
  {
    return;
  }

  // No off-gcd cooldown will be ready during this GCD interval
  if ( off_gcd_ready >= readying->occurs() )
  {
    return;
  }

  assert( cast_while_casting_poll_event == nullptr );

  off_gcd = make_event<player_gcd_event_t>( *sim, *this, delta_time );
}

void player_t::schedule_cwc_ready( timespan_t delta_time )
{
  if ( active_cast_while_casting_list == nullptr )
  {
    return;
  }

  if ( current_execute_type != execute_type::CAST_WHILE_CASTING )
  {
    return;
  }

  // Already polling, do nothing
  if ( cast_while_casting_poll_event )
  {
    return;
  }

  // Not casting, or channeling anything, nor trying to find something to do
  if ( ( !executing || !executing->execute_event ) && !channeling && !readying )
  {
    return;
  }

  // Figure out a safe threshold where the poller should not trigger any more, based on the next
  // ready event (actor finding something to do), when the channel ends, or when the current cast
  // ends.
  timespan_t threshold;
  if ( readying )
  {
    threshold = readying->occurs();
  }
  else if ( channeling )
  {
    threshold = channeling->get_dot()->end_event->occurs();
    threshold += sim->channel_lag + 4 * sim->channel_lag_stddev;
  }
  else
  {
    threshold = executing->execute_event->occurs();
  }

  // No cast while casting ability cd ready before the threshold finishes
  if ( cast_while_casting_ready >= threshold )
  {
    return;
  }

  // No time left to perform a poll event before the threshold finishes
  if ( sim->current_time() + delta_time >= threshold )
  {
    return;
  }

  assert( off_gcd == nullptr );

  cast_while_casting_poll_event = make_event<player_cwc_event_t>( *sim, *this, delta_time );
}

/**
 * Player is ready to perform another action.
 */
void player_t::schedule_ready( timespan_t delta_time, bool waiting )
{
  if ( readying )
  {
    std::runtime_error(fmt::format("{} scheduled ready while already ready.", *this ));
  }
  action_t* was_executing = ( channeling ? channeling : executing );

  if ( queueing )
  {
    sim->print_debug( "{} canceling queued action '{}' at {}", *this, queueing->name(),
                             queueing->queue_event->occurs() );
    event_t::cancel( queueing->queue_event );
    queueing = nullptr;
  }

  if ( current.sleeping )
    return;

  executing     = nullptr;
  queueing      = nullptr;
  channeling    = nullptr;
  action_queued = false;

  started_waiting = timespan_t::min();

  timespan_t gcd_adjust = gcd_ready - ( sim->current_time() + delta_time );
  if ( gcd_adjust > timespan_t::zero() )
    delta_time += gcd_adjust;

  if ( waiting )
  {
    if ( !is_enemy() )
    {
      sequence_add_wait( delta_time, sim->current_time() );
    }
    iteration_waiting_time += delta_time;
  }
  else
  {
    timespan_t lag = timespan_t::zero();

    if ( last_foreground_action )
    {
      if ( last_foreground_action->ability_lag > timespan_t::zero() )
      {
        timespan_t ability_lag =
            rng().gauss( last_foreground_action->ability_lag, last_foreground_action->ability_lag_stddev );
        timespan_t gcd_lag = rng().gauss( sim->gcd_lag, sim->gcd_lag_stddev );
        timespan_t diff    = ( gcd_ready + gcd_lag ) - ( sim->current_time() + ability_lag );
        if ( diff > timespan_t::zero() && sim->strict_gcd_queue )
        {
          lag = gcd_lag;
        }
        else
        {
          lag           = ability_lag;
          action_queued = true;
        }
      }
      else if ( last_foreground_action->gcd() == timespan_t::zero() )
      {
        lag = timespan_t::zero();
      }
      else if ( last_foreground_action->channeled && !last_foreground_action->interrupt_immediate_occurred )
      {
        lag = rng().gauss( sim->channel_lag, sim->channel_lag_stddev );
      }
      else
      {
        timespan_t gcd_lag   = rng().gauss( sim->gcd_lag, sim->gcd_lag_stddev );
        timespan_t queue_lag = rng().gauss( sim->queue_lag, sim->queue_lag_stddev );

        timespan_t diff = ( gcd_ready + gcd_lag ) - ( sim->current_time() + queue_lag );

        if ( diff > timespan_t::zero() && sim->strict_gcd_queue )
        {
          lag = gcd_lag;
        }
        else
        {
          lag           = queue_lag;
          action_queued = true;
        }
      }
    }

    if ( lag < timespan_t::zero() )
      lag = timespan_t::zero();

    if ( type == PLAYER_GUARDIAN )
      lag = timespan_t::zero();  // Guardians do not seem to feel the effects of queue/gcd lag in WoD.

    delta_time += lag;
  }

  if ( last_foreground_action )
  {
    // This is why "total_execute_time" is not tracked per-target!
    last_foreground_action->stats->iteration_total_execute_time += delta_time;
  }

  readying = make_event<player_ready_event_t>( *sim, *this, delta_time );

  if ( was_executing && was_executing->gcd() > timespan_t::zero() && !was_executing->background &&
       !was_executing->proc && !was_executing->repeating )
  {
    // Record the last ability use time for cast_react
    cast_delay_occurred = readying->occurs();
    if ( !is_pet() )
    {
      cast_delay_reaction = rng().gauss( brain_lag, brain_lag_stddev );
    }
    else
    {
      cast_delay_reaction = timespan_t::zero();
    }

    sim->print_debug( "{} {} schedule_ready(): cast_finishes={} cast_delay={}", *this,
                             was_executing->name_str, readying->occurs(),
                             cast_delay_reaction );
  }
}

/**
 * Player arises from the dead.
 */
void player_t::arise()
{
  sim->print_log( "{} tries to arise.", *this );

  if ( !initial.sleeping )
    current.sleeping = false;

  if ( current.sleeping )
    return;

  actor_spawn_index = sim->global_spawn_index++;

  sim->print_log( "{} arises. Spawn Index={}", *this, actor_spawn_index );

  init_resources( true );

  cache.invalidate_all();

  readying = nullptr;
  off_gcd  = nullptr;
  cast_while_casting_poll_event = nullptr;

  arise_time = sim->current_time();
  last_regen = sim->current_time();

  if ( is_enemy() )
  {
    sim->active_enemies++;
    sim->target_non_sleeping_list.push_back( this );

    // When an enemy arises, trigger players to potentially acquire a new target
    range::for_each( sim->player_non_sleeping_list, [this]( player_t* p ) { p->acquire_target( ACTOR_ARISE, this ); } );

    if ( sim->overrides.chaos_brand && debuffs.chaos_brand )
      debuffs.chaos_brand->override_buff();
    if ( sim->overrides.mystic_touch && debuffs.mystic_touch )
      debuffs.mystic_touch->override_buff();
    if ( sim->overrides.bleeding && debuffs.bleeding )
      debuffs.bleeding->override_buff( 1, 1.0 );
    if ( sim->overrides.mortal_wounds && debuffs.mortal_wounds )
      debuffs.mortal_wounds->override_buff();
  }
  else
  {
    sim->active_allies++;
    sim->player_non_sleeping_list.push_back( this );

    // Find a target to shoot on. This is to ensure that transient friendly allies (such as dynamic
    // pets) can cope with a situation, where the primary target for example is invulnerable, so
    // they need to figure out a (more valid) target to shoot spells on.
    acquire_target( SELF_ARISE );
  }

  if ( has_foreground_actions( *this ) )
    schedule_ready();

  active_during_iteration = true;

  for ( auto callback : callbacks.all_callbacks )
  {
    dbc_proc_callback_t* cb = debug_cast<dbc_proc_callback_t*>( callback );

    if ( cb->cooldown && cb->effect.item && cb->effect.item->parsed.initial_cd > timespan_t::zero() )
    {
      timespan_t initial_cd = std::min( cb->effect.cooldown(), cb->effect.item->parsed.initial_cd );
      cb->cooldown->start( initial_cd );

      if ( sim->log )
      {
        sim->out_log.printf( "%s sets initial cooldown for %s to %.2f seconds.", name(), cb->effect.name().c_str(),
                             initial_cd.total_seconds() );
      }
    }
  }

  current_attack_speed = cache.attack_speed();

  range::for_each( callbacks_on_arise, []( const std::function<void( void )>& fn ) { fn(); } );
}

/**
 * Player dies.
 */
void player_t::demise()
{
  // No point in demising anything if we're not even active
  if ( current.sleeping )
    return;

  current.sleeping = true;

  if ( sim->log )
    sim->out_log.printf( "%s demises.. Spawn Index=%u", name(), actor_spawn_index );

  /* Do not reset spawn index, because the player can still have damaging events ( dots ) which
   * need to be associated with eg. resolve Diminishing Return list.
   */

  if (arise_time >= timespan_t::zero())
  {
    iteration_fight_length += sim->current_time() - arise_time;
  }
  // Arise time has to be set to default value before actions are canceled.
  arise_time               = timespan_t::min();
  current.distance_to_move = 0;

  event_t::cancel( readying );
  event_t::cancel( off_gcd );
  event_t::cancel( cast_while_casting_poll_event );

  range::for_each( callbacks_on_demise, [this]( const std::function<void( player_t* )>& fn ) { fn( this ); } );

  for ( size_t i = 0; i < buff_list.size(); ++i )
  {
    buff_t* b = buff_list[ i ];
    b->expire();
    // Dead actors speak no lies .. or proc aura delayed buffs
    event_t::cancel( b->delay );
    event_t::cancel( b->expiration_delay );
  }
  for ( size_t i = 0; i < action_list.size(); ++i )
    action_list[ i ]->cancel();

  // sim -> cancel_events( this );

  for ( size_t i = 0; i < pet_list.size(); ++i )
  {
    pet_list[ i ]->demise();
  }

  for ( size_t i = 0; i < dot_list.size(); ++i )
    dot_list[ i ]->cancel();

  if ( is_enemy() )
  {
    sim->active_enemies--;
    sim->target_non_sleeping_list.find_and_erase_unordered( this );

    // When an enemy dies, trigger players to acquire a new target
    range::for_each( sim->player_non_sleeping_list,
                     [this]( player_t* p ) { p->acquire_target( ACTOR_DEMISE, this ); } );
  }
  else
  {
    sim->active_allies--;
    sim->player_non_sleeping_list.find_and_erase_unordered( this );
  }
}

/**
 * Player is being interrupted.
 */
void player_t::interrupt()
{
  // FIXME! Players will need to override this to handle background repeating actions.

  if ( sim->log )
    sim->out_log.printf( "%s is interrupted", name() );

  if ( executing )
    executing->interrupt_action();
  if ( queueing )
    queueing->interrupt_action();
  if ( channeling )
    channeling->interrupt_action();

  event_t::cancel( cast_while_casting_poll_event );

  if ( strict_sequence )
  {
    strict_sequence->cancel();
    strict_sequence = 0;
  }
  if ( buffs.stunned->check() )
  {
    if ( readying )
      event_t::cancel( readying );
    if ( off_gcd )
      event_t::cancel( off_gcd );

    current_execute_type = execute_type::FOREGROUND;
  }
  else
  {
    if ( !readying && !current.sleeping )
      schedule_ready();

    if ( current_execute_type == execute_type::CAST_WHILE_CASTING )
    {
      current_execute_type = execute_type::OFF_GCD;
      schedule_off_gcd_ready( timespan_t::zero() );
    }
  }
}

void player_t::halt()
{
  if ( sim->log )
    sim->out_log.printf( "%s is halted", name() );

  interrupt();

  if ( main_hand_attack )
    main_hand_attack->cancel();
  if ( off_hand_attack )
    off_hand_attack->cancel();
}

void player_t::stun()
{
  halt();
}

void player_t::moving()
{
  if ( !buffs.norgannons_foresight_ready || !buffs.norgannons_foresight_ready->check() )
  {
    halt();
  }
}

void player_t::finish_moving()
{
  if ( buffs.norgannons_foresight )
  {
    buffs.norgannons_foresight->trigger();
  }
}

void player_t::clear_debuffs()
{
  if ( sim->log )
    sim->out_log.printf( "%s clears debuffs", name() );

  // Clear Dots
  for ( size_t i = 0; i < dot_list.size(); ++i )
  {
    dot_t* dot = dot_list[ i ];
    dot->cancel();
  }

  // Clear all debuffs
  for ( buff_t* buff : buff_list )
  {
    if ( buff->source != this )
    {
      buff->expire();
    }
  }
}

action_t* player_t::execute_action()
{
  assert( !readying );

  action_t* action = 0;

  if ( regen_type == REGEN_DYNAMIC )
    do_dynamic_regen();

  if ( !strict_sequence )
  {
    visited_apls_ = 0;  // Reset visited apl list
    action = select_action( *active_action_list, execute_type::FOREGROUND );
  }
  // Committed to a strict sequence of actions, just perform them instead of a priority list
  else
    action = strict_sequence;

  last_foreground_action = action;

  if ( restore_action_list != 0 )
  {
    activate_action_list( restore_action_list );
    restore_action_list = 0;
  }

  if ( action )
  {
    action->line_cooldown.start();
    action->queue_execute( execute_type::FOREGROUND );
    if ( !action->quiet )
    {
      iteration_executed_foreground_actions++;
      action->total_executions++;
      if ( action->trigger_gcd > timespan_t::zero() )
      {
        prev_gcd_actions.push_back( action );
      }
      else
        off_gcdactions.push_back( action );

      if ( !is_enemy() )
      {
        sequence_add( action, action->target, sim->current_time() );
      }
    }
  }

  return action;
}

void player_t::regen( timespan_t periodicity )
{
  if ( regen_type == REGEN_DYNAMIC && sim->debug )
    sim->out_debug.printf( "%s dynamic regen, last=%.3f interval=%.3f", name(), last_regen.total_seconds(),
                           periodicity.total_seconds() );

  for ( resource_e r = RESOURCE_HEALTH; r < RESOURCE_MAX; r++ )
  {
    if ( resources.is_active( r ) )
    {
      double base  = resource_regen_per_second( r );
      gain_t* gain = gains.resource_regen[ r ];

      if ( gain && base )
        resource_gain( r, base * periodicity.total_seconds(), gain );
    }
  }
}

void player_t::collect_resource_timeline_information()
{
  for ( auto& elem : collected_data.resource_timelines )
  {
    elem.timeline.add( sim->current_time(), resources.current[ elem.type ] );
  }

  for ( auto& elem : collected_data.stat_timelines )
  {
    switch ( elem.type )
    {
      case STAT_STRENGTH:
        elem.timeline.add( sim->current_time(), cache.strength() );
        break;
      case STAT_AGILITY:
        elem.timeline.add( sim->current_time(), cache.agility() );
        break;
      case STAT_INTELLECT:
        elem.timeline.add( sim->current_time(), cache.intellect() );
        break;
      case STAT_SPELL_POWER:
        elem.timeline.add( sim->current_time(), cache.spell_power( SCHOOL_NONE ) );
        break;
      case STAT_ATTACK_POWER:
        elem.timeline.add( sim->current_time(), cache.attack_power() );
        break;
      default:
        elem.timeline.add( sim->current_time(), 0 );
        break;
    }
  }
}

double player_t::resource_loss( resource_e resource_type, double amount, gain_t* source, action_t* )
{
  if ( amount == 0 )
    return 0.0;

  if ( current.sleeping )
    return 0.0;

  if ( resource_type == primary_resource() )
    uptimes.primary_resource_cap->update( false, sim->current_time() );

  double actual_amount;
  double previous_amount;
  double previous_pct_points;
  bool has_active_resource_callbacks = this -> has_active_resource_callbacks;
  if (has_active_resource_callbacks)
  {
    previous_amount = resources.current[ resource_type ];
    previous_pct_points = resources.current[ resource_type ] / resources.max[ resource_type ] * 100.0;
  }

  if ( !resources.is_infinite( resource_type ) || is_enemy() )
  {
    actual_amount = std::min( amount, resources.current[ resource_type ] );
    resources.current[ resource_type ] -= actual_amount;
    iteration_resource_lost[ resource_type ] += actual_amount;
  }
  else
  {
    actual_amount = amount;
    resources.current[ resource_type ] -= actual_amount;
    iteration_resource_lost[ resource_type ] += actual_amount;
  }

  if ( source )
  {
    source->add( resource_type, actual_amount * -1, ( amount - actual_amount ) * -1 );
  }

  if ( resource_type == RESOURCE_MANA )
  {
    last_cast = sim->current_time();
  }

  if (has_active_resource_callbacks)
  {
    check_resource_change_for_callback(resource_type, previous_amount, previous_pct_points);
  }

  if ( sim->debug )
    sim->out_debug.printf( "Player %s loses %.2f (%.2f) %s. pct=%.2f%% (%.0f/%.0f)", name(), actual_amount, amount,
                           util::resource_type_string( resource_type ),
                           resources.max[ resource_type ] ? resources.pct( resource_type ) * 100 : 0,
                           resources.current[ resource_type ], resources.max[ resource_type ] );

  return actual_amount;
}

double player_t::resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* action )
{
  if ( current.sleeping || amount == 0.0 )
    return 0.0;

  double actual_amount = std::min( amount, resources.max[ resource_type ] - resources.current[ resource_type ] );

  if ( actual_amount > 0.0 )
  {
    resources.current[ resource_type ] += actual_amount;
    iteration_resource_gained[ resource_type ] += actual_amount;
  }

  if ( resource_type == primary_resource() && resources.max[ resource_type ] <= resources.current[ resource_type ] )
    uptimes.primary_resource_cap->update( true, sim->current_time() );

  if ( source )
  {
    source->add( resource_type, actual_amount, amount - actual_amount );
  }

  if ( sim->log )
  {
    sim->out_log.printf( "%s gains %.2f (%.2f) %s from %s (%.2f/%.2f)", name(), actual_amount, amount,
                         util::resource_type_string( resource_type ),
                         source ? source->name() : action ? action->name() : "unknown",
                         resources.current[ resource_type ], resources.max[ resource_type ] );
  }

  return actual_amount;
}

bool player_t::resource_available( resource_e resource_type, double cost ) const
{
  if ( resource_type == RESOURCE_NONE || cost <= 0 || resources.is_infinite( resource_type ) )
  {
    return true;
  }

  bool available = resources.current[ resource_type ] >= cost;

#ifndef NDEBUG
  if ( !resources.active_resource[ resource_type ] )
  {
    assert( available && "Insufficient inactive resource to cast!" );
  }
#endif

  return available;
}

void player_t::recalculate_resource_max( resource_e resource_type )
{
  resources.max[ resource_type ] = resources.base[ resource_type ];
  resources.max[ resource_type ] *= resources.base_multiplier[ resource_type ];
  resources.max[ resource_type ] += total_gear.resource[ resource_type ];

  switch ( resource_type )
  {
    case RESOURCE_HEALTH:
    {
      // Calculate & set maximum health
      resources.max[ resource_type ] += floor( stamina() ) * current.health_per_stamina;

      // Make sure the player starts combat with full health
      if ( !in_combat )
        resources.current[ resource_type ] = resources.max[ resource_type ];
      break;
    }
    default:
      break;
  }
  resources.max[ resource_type ] += resources.temporary[ resource_type ];

  resources.max[ resource_type ] *= resources.initial_multiplier[ resource_type ];
  // Sanity check on current values
  resources.current[ resource_type ] = std::min( resources.current[ resource_type ], resources.max[ resource_type ] );
}

role_e player_t::primary_role() const
{
  return role;
}

const char* player_t::primary_tree_name() const
{
  return dbc::specialization_string( specialization() ).c_str();
}

/**
 * Scale factor normalization stat.
 *
 * Determine the stat by which scale factors are normalized, so that this stat will get a value of 1.0.
 */
stat_e player_t::normalize_by() const
{
  if ( sim->normalized_stat != STAT_NONE )
    return sim->normalized_stat;

  const scale_metric_e sm = this->sim->scaling->scaling_metric;

  role_e role = primary_role();
  if ( role == ROLE_SPELL || role == ROLE_HEAL )
    return STAT_INTELLECT;
  else if ( role == ROLE_TANK && ( sm == SCALE_METRIC_TMI || sm == SCALE_METRIC_DEATHS ) &&
            scaling->scaling[ sm ].get_stat( STAT_STAMINA ) != 0.0 )
    return STAT_STAMINA;
  else if ( type == DRUID || type == HUNTER || type == SHAMAN || type == ROGUE || type == MONK || type == DEMON_HUNTER )
    return STAT_AGILITY;
  else if ( type == DEATH_KNIGHT || type == PALADIN || type == WARRIOR )
    return STAT_STRENGTH;

  return STAT_ATTACK_POWER;
}

double player_t::health_percentage() const
{
  return resources.pct( RESOURCE_HEALTH ) * 100;
}

double player_t::max_health() const
{
  return resources.max[ RESOURCE_HEALTH ];
}

double player_t::current_health() const
{
  return resources.current[ RESOURCE_HEALTH ];
}

timespan_t player_t::time_to_percent( double percent ) const
{
  timespan_t time_to_percent;
  double ttp;

  if ( iteration_dmg_taken > 0.0 && resources.base[ RESOURCE_HEALTH ] > 0 &&
       sim->current_time() >= timespan_t::from_seconds( 1.0 ) && !sim->fixed_time )
    ttp = ( resources.current[ RESOURCE_HEALTH ] - ( percent * 0.01 * resources.base[ RESOURCE_HEALTH ] ) ) /
          ( iteration_dmg_taken / sim->current_time().total_seconds() );
  else
    ttp = ( sim->expected_iteration_time * ( 1.0 - percent * 0.01 ) - sim->current_time() ).total_seconds();

  time_to_percent = timespan_t::from_seconds( ttp );

  if ( time_to_percent < timespan_t::zero() )
    return timespan_t::zero();
  else
    return time_to_percent;
}

timespan_t player_t::total_reaction_time()
{
  return std::min( reaction_max, reaction_offset + rng().exgauss( reaction_mean, reaction_stddev, reaction_nu ) );
}

void player_t::stat_gain( stat_e stat, double amount, gain_t* gain, action_t* action, bool temporary_stat )
{
  if ( amount <= 0 )
    return;

  // bail out if this is a stat that doesn't work for this class
  if ( convert_hybrid_stat( stat ) == STAT_NONE )
    return;

  int temp_value = temporary_stat ? 1 : 0;

  cache_e cache_type = cache_from_stat( stat );
  if ( regen_type == REGEN_DYNAMIC && regen_caches[ cache_type ] )
    do_dynamic_regen();

  if ( sim->log )
    sim->out_log.printf( "%s gains %.2f %s%s", name(), amount, util::stat_type_string( stat ),
                         temporary_stat ? " (temporary)" : "" );

  switch ( stat )
  {
    case STAT_STAMINA:
    case STAT_STRENGTH:
    case STAT_AGILITY:
    case STAT_INTELLECT:
    case STAT_SPIRIT:
    case STAT_SPELL_POWER:
    case STAT_ATTACK_POWER:
    case STAT_EXPERTISE_RATING:
    case STAT_HIT_RATING:
    case STAT_CRIT_RATING:
    case STAT_ARMOR:
    case STAT_BONUS_ARMOR:
    case STAT_DODGE_RATING:
    case STAT_PARRY_RATING:
    case STAT_BLOCK_RATING:
    case STAT_MASTERY_RATING:
    case STAT_VERSATILITY_RATING:
    case STAT_LEECH_RATING:
    case STAT_AVOIDANCE_RATING:
    case STAT_SPEED_RATING:
      current.stats.add_stat( stat, amount );
      invalidate_cache( cache_type );
      break;

    case STAT_HASTE_RATING:
    {
      current.stats.add_stat( stat, amount );
      invalidate_cache( cache_type );

      adjust_dynamic_cooldowns();
      // adjust_global_cooldown( HASTE_ANY );
      adjust_auto_attack( HASTE_ANY );
      break;
    }

    case STAT_ALL:
      for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; i++ )
      {
        current.stats.attribute[ i ] += amount;
        invalidate_cache( static_cast<cache_e>( i ) );
      }
      break;

    case STAT_HEALTH:
    case STAT_MANA:
    case STAT_RAGE:
    case STAT_ENERGY:
    case STAT_FOCUS:
    case STAT_RUNIC:
    {
      resource_e r =
          ( ( stat == STAT_HEALTH )
                ? RESOURCE_HEALTH
                : ( stat == STAT_MANA )
                      ? RESOURCE_MANA
                      : ( stat == STAT_RAGE ) ? RESOURCE_RAGE
                                              : ( stat == STAT_ENERGY )
                                                    ? RESOURCE_ENERGY
                                                    : ( stat == STAT_FOCUS ) ? RESOURCE_FOCUS : RESOURCE_RUNIC_POWER );
      resource_gain( r, amount, gain, action );
    }
    break;

    case STAT_MAX_HEALTH:
    case STAT_MAX_MANA:
    case STAT_MAX_RAGE:
    case STAT_MAX_ENERGY:
    case STAT_MAX_FOCUS:
    case STAT_MAX_RUNIC:
    {
      resource_e r = ( ( stat == STAT_MAX_HEALTH )
                           ? RESOURCE_HEALTH
                           : ( stat == STAT_MAX_MANA )
                                 ? RESOURCE_MANA
                                 : ( stat == STAT_MAX_RAGE )
                                       ? RESOURCE_RAGE
                                       : ( stat == STAT_MAX_ENERGY )
                                             ? RESOURCE_ENERGY
                                             : ( stat == STAT_MAX_FOCUS ) ? RESOURCE_FOCUS : RESOURCE_RUNIC_POWER );

      resources.temporary[ r ] += temp_value * amount;
      recalculate_resource_max( r );
      resource_gain( r, amount, gain, action );
    }
    break;

    // Unused but known stats to prevent asserting
    case STAT_RESILIENCE_RATING:
      break;

    default:
      assert( false );
      break;
  }

  switch ( stat )
  {
    case STAT_STAMINA:
    case STAT_ALL:
    {
      recalculate_resource_max( RESOURCE_HEALTH );
      // Adjust current health to new max on stamina gains, if the actor is not in combat
      if ( !in_combat )
      {
        double delta = resources.max[ RESOURCE_HEALTH ] - resources.current[ RESOURCE_HEALTH ];
        resource_gain( RESOURCE_HEALTH, delta );
      }
      break;
    }
    default:
      break;
  }
}

void player_t::stat_loss( stat_e stat, double amount, gain_t* gain, action_t* action, bool temporary_buff )
{
  if ( amount <= 0 )
    return;

  // bail out if this is a stat that doesn't work for this class
  if ( convert_hybrid_stat( stat ) == STAT_NONE )
    return;

  cache_e cache_type = cache_from_stat( stat );
  if ( regen_type == REGEN_DYNAMIC && regen_caches[ cache_type ] )
    do_dynamic_regen();

  if ( sim->log )
    sim->out_log.printf( "%s loses %.2f %s%s", name(), amount, util::stat_type_string( stat ),
                         ( temporary_buff ) ? " (temporary)" : "" );

  int temp_value = temporary_buff ? 1 : 0;
  switch ( stat )
  {
    case STAT_STAMINA:
    case STAT_STRENGTH:
    case STAT_AGILITY:
    case STAT_INTELLECT:
    case STAT_SPIRIT:
    case STAT_SPELL_POWER:
    case STAT_ATTACK_POWER:
    case STAT_EXPERTISE_RATING:
    case STAT_HIT_RATING:
    case STAT_CRIT_RATING:
    case STAT_ARMOR:
    case STAT_BONUS_ARMOR:
    case STAT_DODGE_RATING:
    case STAT_PARRY_RATING:
    case STAT_BLOCK_RATING:
    case STAT_MASTERY_RATING:
    case STAT_VERSATILITY_RATING:
    case STAT_LEECH_RATING:
    case STAT_AVOIDANCE_RATING:
    case STAT_SPEED_RATING:
      current.stats.add_stat( stat, -amount );
      invalidate_cache( cache_type );
      break;

    case STAT_ALL:
      for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; i++ )
      {
        current.stats.attribute[ i ] -= amount;
        invalidate_cache( (cache_e)i );
      }
      break;

    case STAT_HEALTH:
    case STAT_MANA:
    case STAT_RAGE:
    case STAT_ENERGY:
    case STAT_FOCUS:
    case STAT_RUNIC:
    {
      resource_e r =
          ( ( stat == STAT_HEALTH )
                ? RESOURCE_HEALTH
                : ( stat == STAT_MANA )
                      ? RESOURCE_MANA
                      : ( stat == STAT_RAGE ) ? RESOURCE_RAGE
                                              : ( stat == STAT_ENERGY )
                                                    ? RESOURCE_ENERGY
                                                    : ( stat == STAT_FOCUS ) ? RESOURCE_FOCUS : RESOURCE_RUNIC_POWER );
      resource_loss( r, amount, gain, action );
    }
    break;

    case STAT_MAX_HEALTH:
    case STAT_MAX_MANA:
    case STAT_MAX_RAGE:
    case STAT_MAX_ENERGY:
    case STAT_MAX_FOCUS:
    case STAT_MAX_RUNIC:
    {
      resource_e r = ( ( stat == STAT_MAX_HEALTH )
                           ? RESOURCE_HEALTH
                           : ( stat == STAT_MAX_MANA )
                                 ? RESOURCE_MANA
                                 : ( stat == STAT_MAX_RAGE )
                                       ? RESOURCE_RAGE
                                       : ( stat == STAT_MAX_ENERGY )
                                             ? RESOURCE_ENERGY
                                             : ( stat == STAT_MAX_FOCUS ) ? RESOURCE_FOCUS : RESOURCE_RUNIC_POWER );

      resources.temporary[ r ] -= temp_value * amount;
      recalculate_resource_max( r );
      double delta = resources.current[ r ] - resources.max[ r ];
      if ( delta > 0 )
        resource_loss( r, delta, gain, action );
    }
    break;

    case STAT_HASTE_RATING:
    {
      current.stats.haste_rating -= amount;
      invalidate_cache( cache_type );

      adjust_dynamic_cooldowns();
      // adjust_global_cooldown( HASTE_ANY );
      adjust_auto_attack( HASTE_ANY );
      break;
    }

    // Unused but known stats to prevent asserting
    case STAT_RESILIENCE_RATING:
      break;

    default:
      assert( false );
      break;
  }

  switch ( stat )
  {
    case STAT_STAMINA:
    case STAT_ALL:
    {
      recalculate_resource_max( RESOURCE_HEALTH );
      // Adjust current health to new max on stamina gains
      double delta = resources.current[ RESOURCE_HEALTH ] - resources.max[ RESOURCE_HEALTH ];
      if ( delta > 0 )
        resource_loss( RESOURCE_HEALTH, delta, gain, action );
      break;
    }
    default:
      break;
  }
}

/**
 * Adjust the current rating -> % modifer of a stat.
 *
 * Invalidates corresponding cache
 */
void player_t::modify_current_rating( rating_e r, double amount )
{
  current.rating.get( r ) += amount;
  invalidate_cache( cache_from_rating( r ) );
}

void player_t::cost_reduction_gain( school_e school, double amount, gain_t* /* gain */, action_t* /* action */ )
{
  if ( amount <= 0 )
    return;

  if ( sim->log )
    sim->out_log.printf( "%s gains a cost reduction of %.0f on abilities of school %s", name(), amount,
                         util::school_type_string( school ) );

  if ( school > SCHOOL_MAX_PRIMARY )
  {
    for ( school_e i = SCHOOL_NONE; ++i < SCHOOL_MAX_PRIMARY; )
    {
      if ( util::school_type_component( school, i ) )
      {
        current.resource_reduction[ i ] += amount;
      }
    }
  }
  else
  {
    current.resource_reduction[ school ] += amount;
  }
}

void player_t::cost_reduction_loss( school_e school, double amount, action_t* /* action */ )
{
  if ( amount <= 0 )
    return;

  if ( sim->log )
    sim->out_log.printf( "%s loses a cost reduction %.0f on abilities of school %s", name(), amount,
                         util::school_type_string( school ) );

  if ( school > SCHOOL_MAX_PRIMARY )
  {
    for ( school_e i = SCHOOL_NONE; ++i < SCHOOL_MAX_PRIMARY; )
    {
      if ( util::school_type_component( school, i ) )
      {
        current.resource_reduction[ i ] -= amount;
      }
    }
  }
  else
  {
    current.resource_reduction[ school ] -= amount;
  }
}

namespace {
namespace assess_dmg_helper_functions
{
void account_parry_haste( player_t& p, action_state_t* s )
{
  if ( !p.is_enemy() )
  {
    // Parry Haste accounting
    if ( s->result == RESULT_PARRY )
    {
      if ( p.main_hand_attack && p.main_hand_attack->execute_event )
      {
        // Parry haste mechanics:  When parrying an attack, the game subtracts 40% of the player's base swing timer
        // from the time remaining on the current swing timer.  However, this effect cannot reduce the current swing
        // timer to less than 20% of the base value.  The game uses hasted values.  To illustrate that, two examples:
        // base weapon speed: 2.6, 30% haste, thus base swing timer is 2.6/1.3=2.0 seconds
        // 1) if we parry when the current swing timer has 1.8 seconds remaining, then it gets reduced by 40% of 2.0, or
        // 0.8 seconds,
        //    and the current swing timer becomes 1.0 seconds.
        // 2) if we parry when the current swing timer has 1.0 second remaining the game tries to subtract 0.8 seconds,
        // but hits the
        //    minimum value (20% of 2.0, or 0.4 seconds.  The current swing timer becomes 0.4 seconds.
        // Thus, the result is that the current swing timer becomes
        // max(current_swing_timer-0.4*base_swing_timer,0.2*base_swing_timer)

        // the reschedule_execute(x) function we call to perform this tries to reschedule the effect such that it occurs
        // at (sim->current_time() + x).  Thus we need to give it the difference between sim->current_time() and the new
        // target of execute_event->occurs(). That value is simply the remaining time on the current swing timer.

        // first, we need the hasted base swing timer, swing_time
        timespan_t swing_time = p.main_hand_attack->time_to_execute;

        // and we also need the time remaining on the current swing timer
        timespan_t current_swing_timer = p.main_hand_attack->execute_event->occurs() - p.sim->current_time();

        // next, check that the current swing timer is longer than 0.2*swing_time - if not we do nothing
        if ( current_swing_timer > 0.20 * swing_time )
        {
          // now we apply parry-hasting.  Subtract 40% of base swing timer from current swing timer
          current_swing_timer -= 0.40 * swing_time;

          // enforce 20% base swing timer minimum
          current_swing_timer = std::max( current_swing_timer, 0.20 * swing_time );

          // now reschedule the event and log a parry haste
          p.main_hand_attack->reschedule_execute( current_swing_timer );
          p.procs.parry_haste->occur();
        }
      }
    }
  }
}

void account_blessing_of_sacrifice( player_t& p, action_state_t* s )
{
  if ( p.buffs.blessing_of_sacrifice->check() )
  {
    // figure out how much damage gets redirected
    double redirected_damage = s->result_amount * ( p.buffs.blessing_of_sacrifice->data().effectN( 1 ).percent() );

    // apply that damage to the source paladin
    p.buffs.blessing_of_sacrifice->trigger( s->action, 0, redirected_damage, timespan_t::zero() );

    // mitigate that amount from the target.
    // Slight inaccuracy: We do not get a feedback of paladin health buffer expiration here.
    s->result_amount -= redirected_damage;

    if ( p.sim->debug && s->action && !s->target->is_enemy() && !s->target->is_add() )
      p.sim->out_debug.printf( "Damage to %s after Blessing of Sacrifice is %f", s->target->name(), s->result_amount );
  }
}

bool absorb_sort( absorb_buff_t* a, absorb_buff_t* b )
{
  double lv = a->current_value, rv = a->current_value;
  if ( lv == rv )
  {
    return a->name_str < b->name_str;
  }

  return lv < rv;
}

void account_absorb_buffs( player_t& p, action_state_t* s, school_e school )
{
  /* ABSORB BUFFS

     std::vector<absorb_buff_t*> absorb_buff_list; is a dynamic vector, which contains
     the currently active absorb buffs of a player.

     std:map<int,instant_absorb_t*> instant_absorb_list; is a map by spell id that contains
     the data for instant absorb effects.

     std::vector<int> absorb_priority; is a vector that contains the sequencing for
     high priority (tank) absorbs and instant absorbs, by spell ID. High priority and
     instant absorbs MUST be pushed into this array inside init_absorb_priority() to
     function.

  */

  if ( !( p.absorb_buff_list.empty() && p.instant_absorb_list.empty() ) )
  {
    /* First, handle high priority absorbs and instant absorbs. These absorbs should
       obey the sequence laid out in absorb_priority. To achieve this, we loop through
       absorb_priority in order, then find and apply the appropriate high priority
       or instant absorb. */
    for ( size_t i = 0; i < p.absorb_priority.size(); i++ )
    {
      // Check for the absorb ID in instantaneous absorbs first, since its low cost.
      auto it = p.instant_absorb_list.find( p.absorb_priority[ i ] );
      if ( it != p.instant_absorb_list.end() )
      {
        instant_absorb_t& ab = it->second;

        // eligibility is handled in the instant absorb's handler
        double absorbed = ab.consume( s );

        s->result_amount -= absorbed;
        s->self_absorb_amount += absorbed;

        if ( p.sim->debug && s->action && !s->target->is_enemy() && !s->target->is_add() && absorbed != 0 )
          p.sim->out_debug.printf( "Damage to %s after %s is %f", s->target->name(), ab.name.c_str(),
                                   s->result_amount );
      }
      else
      {
        // Check for the absorb ID in high priority absorbs.
        for ( size_t j = 0; j < p.absorb_buff_list.size(); j++ )
        {
          if ( p.absorb_buff_list[ j ]->data().id() == p.absorb_priority[ i ] )
          {
            absorb_buff_t* ab = p.absorb_buff_list[ j ];

            assert( ab->high_priority && "Absorb buff with set priority is not flagged for high priority." );

            if ( ( ( ab->eligibility && ab->eligibility( s ) )  // Use the eligibility function if there is one
                   || ( school == SCHOOL_NONE ||
                        dbc::is_school( ab->absorb_school, school ) ) )  // Otherwise check by school
                 && ab->up() )
            {
              double absorbed = ab->consume( s->result_amount );

              s->result_amount -= absorbed;

              // track result using only self-absorbs separately
              if ( ab->source == &p || p.is_my_pet( ab->source ) )
                s->self_absorb_amount += absorbed;

              if ( p.sim->debug && s->action && !s->target->is_enemy() && !s->target->is_add() && absorbed != 0 )
                p.sim->out_debug.printf( "Damage to %s after %s is %f", s->target->name(), ab->name(),
                                         s->result_amount );
            }

            if ( ab->current_value <= 0 )
              ab->expire();

            break;
          }
        }
      }

      if ( s->result_amount <= 0 )
      {
        assert( s->result_amount == 0 );
        break;
      }
    }

    // Second, we handle any low priority absorbs. These absorbs obey the rule of "smallest first".

    // Sort the absorbs by size, then we can loop through them in order.
    range::sort( p.absorb_buff_list, absorb_sort );
    size_t offset = 0;

    while ( offset < p.absorb_buff_list.size() && s->result_amount > 0 && !p.absorb_buff_list.empty() )
    {
      absorb_buff_t* ab = p.absorb_buff_list[ offset ];

      /* Check absorb eligbility by school and custom eligibility function, skipping high priority
         absorbs since those have already been processed above. */
      if ( ab->high_priority ||
           ( ab->eligibility && !ab->eligibility( s ) )  // Use the eligibility function if there is one
           || ( school != SCHOOL_NONE && !dbc::is_school( ab->absorb_school, school ) ) )  // Otherwise check by school
      {
        offset++;
        continue;
      }

      // Don't be too paranoid about inactive absorb buffs in the list. Just expire them
      if ( ab->up() )
      {
        // Consume the absorb and grab the effective amount consumed.
        double absorbed = ab->consume( s->result_amount );

        s->result_amount -= absorbed;

        // track result using only self-absorbs separately
        if ( ab->source == &p || p.is_my_pet( ab->source ) )
          s->self_absorb_amount += absorbed;

        if ( p.sim->debug && s->action && !s->target->is_enemy() && !s->target->is_add() )
          p.sim->out_debug.printf( "Damage to %s after %s is %f", s->target->name(), ab->name(), s->result_amount );

        if ( s->result_amount <= 0 )
        {
          // Buff is not fully consumed
          assert( s->result_amount == 0 );
          break;
        }
      }

      // So, it turns out it's possible to have absorb buff behavior, where
      // there's a "minimum value" for the absorb buff, even after absorbing
      // damage more than its current value. In this case, the absorb buff should
      // not be expired, as the current_value still has something left.
      if ( ab->current_value <= 0 )
      {
        ab->expire();
        assert( p.absorb_buff_list.empty() || p.absorb_buff_list[ 0 ] != ab );
      }
      else
        offset++;
    }  // end of absorb list loop
  }

  p.iteration_absorb_taken += s->self_absorb_amount;

  s->result_absorbed = s->result_amount;
}

void account_legendary_tank_cloak( player_t& p, action_state_t* s )
{
  // Legendary Tank Cloak Proc - max absorb of 1e7 hardcoded (in spellid 146193, effect 1)
  if ( p.legendary_tank_cloak_cd && p.legendary_tank_cloak_cd->up()    // and the cloak's cooldown is up
       && s->result_amount > p.resources.current[ RESOURCE_HEALTH ] )  // attack exceeds player health
  {
    if ( s->result_amount > 1e7 )
    {
      p.gains.endurance_of_niuzao->add( RESOURCE_HEALTH, 1e7, 0 );
      s->result_amount -= 1e7;
      s->result_absorbed += 1e7;
    }
    else
    {
      p.gains.endurance_of_niuzao->add( RESOURCE_HEALTH, s->result_amount, 0 );
      s->result_absorbed += s->result_amount;
      s->result_amount = 0;
    }
    p.legendary_tank_cloak_cd->start();
  }
}

/**
 * Statistical data collection for damage taken.
 */
void collect_dmg_taken_data( player_t& p, const action_state_t* s, double result_ignoring_external_absorbs )
{
  p.iteration_dmg_taken += s->result_amount;

  // collect data for timelines
  p.collected_data.timeline_dmg_taken.add( p.sim->current_time(), s->result_amount );

  // tank-specific data storage
  if ( p.collected_data.health_changes.collect )
  {
    // health_changes covers everything, used for ETMI and other things
    p.collected_data.health_changes.timeline.add( p.sim->current_time(), s->result_amount );
    p.collected_data.health_changes.timeline_normalized.add( p.sim->current_time(),
                                                             s->result_amount / p.resources.max[ RESOURCE_HEALTH ] );

    // store value in incoming damage array for conditionals
    p.incoming_damage.push_back( {p.sim->current_time(), s->result_amount, s->action->get_school()} );
  }
  if ( p.collected_data.health_changes_tmi.collect )
  {
    // health_changes_tmi ignores external effects (e.g. external absorbs), used for raw TMI
    p.collected_data.health_changes_tmi.timeline.add( p.sim->current_time(), result_ignoring_external_absorbs );
    p.collected_data.health_changes_tmi.timeline_normalized.add(
        p.sim->current_time(), result_ignoring_external_absorbs / p.resources.max[ RESOURCE_HEALTH ] );
  }
}

/**
 * Check if Guardian spirit saves the life of the player.
 */
bool try_guardian_spirit( player_t& p, double actual_amount )
{
  // This can only save the target, if the damage is less than 200% of the target's health as of 4.0.6
  if ( !p.is_enemy() && p.buffs.guardian_spirit->check() &&
       actual_amount <= ( p.resources.max[ RESOURCE_HEALTH ] * 2 ) )
  {
    // Just assume that this is used so rarely that a strcmp hack will do
    // stats_t* stat = buffs.guardian_spirit -> source ? buffs.guardian_spirit -> source -> get_stats( "guardian_spirit"
    // ) : 0; double gs_amount = resources.max[ RESOURCE_HEALTH ] * buffs.guardian_spirit -> data().effectN( 2
    // ).percent(); resource_gain( RESOURCE_HEALTH, s -> result_amount ); if ( stat ) stat -> add_result( gs_amount,
    // gs_amount, HEAL_DIRECT, RESULT_HIT );
    p.buffs.guardian_spirit->expire();
    return true;
  }

  return false;
}

}  // namespace assess_dmg_helper_functions

}

void player_t::assess_damage( school_e school, dmg_e type, action_state_t* s )
{
  using namespace assess_dmg_helper_functions;

  account_parry_haste( *this, s );

  if ( s->result_amount <= 0.0 )
    return;

  target_mitigation( school, type, s );

  // store post-mitigation, pre-absorb value
  s->result_mitigated = s->result_amount;

  if ( sim->debug && s->action && !s->target->is_enemy() && !s->target->is_add() )
    sim->out_debug.printf( "Damage to %s after all mitigation is %f", s->target->name(), s->result_amount );

  account_blessing_of_sacrifice( *this, s );

  assess_damage_imminent_pre_absorb( school, type, s );

  account_absorb_buffs( *this, s, school );

  assess_damage_imminent( school, type, s );

  account_legendary_tank_cloak( *this, s );
}

void player_t::do_damage( action_state_t* incoming_state )
{
  using namespace assess_dmg_helper_functions;

  double actual_amount = 0.0;
  collect_dmg_taken_data( *this, incoming_state,
                          incoming_state->result_mitigated - incoming_state->self_absorb_amount );

  if ( incoming_state->result_amount > 0.0 )
  {
    actual_amount = resource_loss( RESOURCE_HEALTH, incoming_state->result_amount, nullptr, incoming_state->action );
  }

  // New callback system; proc abilities on incoming events.
  // TODO: How to express action causing/not causing incoming callbacks?
  if ( incoming_state->action && incoming_state->action->callbacks )
  {
    proc_types pt   = incoming_state->proc_type();
    proc_types2 pt2 = incoming_state->execute_proc_type2();
    // For incoming landed abilities, get the impact type for the proc.
    // if ( pt2 == PROC2_LANDED )
    //  pt2 = s -> impact_proc_type2();

    // On damage/heal in. Proc flags are arranged as such that the "incoming"
    // version of the primary proc flag is always follows the outgoing version.
    if ( pt != PROC1_INVALID && pt2 != PROC2_INVALID )
      action_callback_t::trigger( callbacks.procs[ pt + 1 ][ pt2 ], incoming_state->action, incoming_state );
  }

  // Check if target is dying
  if ( health_percentage() <= death_pct && !resources.is_infinite( RESOURCE_HEALTH ) )
  {
    if ( !try_guardian_spirit( *this, actual_amount ) )
    {  // Player was not saved by guardian spirit, kill him
      if ( !current.sleeping )
      {
        collected_data.deaths.add( sim->current_time().total_seconds() );
      }
      if ( sim->log )
        sim->out_log.printf( "%s has died.", name() );
      make_event<player_demise_event_t>( *sim, *this );
    }
  }
}

void player_t::assess_damage_imminent_pre_absorb( school_e, dmg_e, action_state_t* )
{
}

void player_t::assess_damage_imminent( school_e, dmg_e, action_state_t* )
{
}

void player_t::target_mitigation( school_e school, dmg_e dmg_type, action_state_t* s )
{
  if ( s->result_amount == 0 )
    return;

  if ( buffs.pain_supression && buffs.pain_supression->up() )
    s->result_amount *= 1.0 + buffs.pain_supression->data().effectN( 1 ).percent();

  if ( buffs.naarus_discipline && buffs.naarus_discipline->check() )
    s->result_amount *= 1.0 + buffs.naarus_discipline->stack_value();

  if ( buffs.stoneform && buffs.stoneform->up() )
    s->result_amount *= 1.0 + buffs.stoneform->data().effectN( 1 ).percent();

  if ( buffs.fortitude && buffs.fortitude->up() )
    s->result_amount *= 1.0 + buffs.fortitude->data().effectN( 1 ).percent();

  if ( s->action->is_aoe() )
    s->result_amount *= 1.0 - cache.avoidance();

  // TODO-WOD: Where should this be? Or does it matter?
  s->result_amount *= 1.0 - cache.mitigation_versatility();

  if ( debuffs.invulnerable && debuffs.invulnerable->check() )
  {
    s->result_amount = 0;
  }

  if ( school == SCHOOL_PHYSICAL && dmg_type == DMG_DIRECT )
  {
    if ( sim->debug && s->action && !s->target->is_enemy() && !s->target->is_add() )
      sim->out_debug.printf( "Damage to %s before armor mitigation is %f", s->target->name(), s->result_amount );

    // Maximum amount of damage reduced by armor
    double armor_cap = 0.85;

    // Armor
    if ( s->action )
    {
      double armor  = s->target_armor;
      double resist = armor / ( armor + s -> action -> player -> current.armor_coeff );
      resist        = clamp( resist, 0.0, armor_cap );
      s -> result_amount *= 1.0 - resist;
    }

    if ( sim->debug && s->action && !s->target->is_enemy() && !s->target->is_add() )
      sim->out_debug.printf( "Damage to %s after armor mitigation is %f (%f armor)", s->target->name(), s->result_amount, s -> target_armor );

    double pre_block_amount = s->result_amount;

    // In BfA, Block and Crit Block work in the same manner as armor and are affected by the same cap
    if ( s -> block_result == BLOCK_RESULT_BLOCKED || s -> block_result == BLOCK_RESULT_CRIT_BLOCKED )
    {
      double block_reduction = composite_block_reduction( s );

      double block_resist = block_reduction / ( block_reduction + s -> action -> player -> current.armor_coeff );
     
      if ( s -> block_result == BLOCK_RESULT_CRIT_BLOCKED )
        block_resist *= 2.0;
      
      block_resist = clamp( block_resist, 0.0, armor_cap );
      s -> result_amount *= 1.0 - block_resist;

      if ( s -> result_amount <= 0 )
        return;
    }

    s->blocked_amount = pre_block_amount - s->result_amount;

    if ( sim->debug && s->action && !s->target->is_enemy() && !s->target->is_add() && s->blocked_amount > 0.0 )
      sim->out_debug.printf( "Damage to %s after blocking is %f", s->target->name(), s->result_amount );
  }
}

void player_t::assess_heal( school_e, dmg_e, action_state_t* s )
{
  // Increases to healing taken should modify result_total in order to correctly calculate overhealing
  // and other effects based on raw healing.
  if ( buffs.guardian_spirit->up() )
    s->result_total *= 1.0 + buffs.guardian_spirit->data().effectN( 1 ).percent();

  // process heal
  s->result_amount = resource_gain( RESOURCE_HEALTH, s->result_total, 0, s->action );

  // if the target is a tank record this event on damage timeline
  if ( !is_pet() && primary_role() == ROLE_TANK )
  {
    // health_changes and timeline_healing_taken record everything, accounting for overheal and so on
    collected_data.timeline_healing_taken.add( sim->current_time(), -( s->result_amount ) );
    collected_data.health_changes.timeline.add( sim->current_time(), -( s->result_amount ) );
    double normalized =
        resources.max[ RESOURCE_HEALTH ] ? -( s->result_amount ) / resources.max[ RESOURCE_HEALTH ] : 0.0;
    collected_data.health_changes.timeline_normalized.add( sim->current_time(), normalized );

    // health_changes_tmi ignores external healing - use result_total to count player overhealing as effective healing
    if ( s->action->player == this || is_my_pet( s->action->player ) )
    {
      collected_data.health_changes_tmi.timeline.add( sim->current_time(), -( s->result_total ) );
      collected_data.health_changes_tmi.timeline_normalized.add(
          sim->current_time(), -( s->result_total ) / resources.max[ RESOURCE_HEALTH ] );
    }
  }

  // store iteration heal taken
  iteration_heal_taken += s->result_amount;
}

void player_t::summon_pet( const std::string& pet_name, const timespan_t duration )
{
  if ( pet_t* p = find_pet( pet_name ) )
    p->summon( duration );
  else
    sim->errorf( "Player %s is unable to summon pet '%s'\n", name(), pet_name.c_str() );
}

void player_t::dismiss_pet( const std::string& pet_name )
{
  pet_t* p = find_pet( pet_name );
  if ( !p )
  {
    sim->errorf( "Player %s: Could not find pet with name '%s' to dismiss.", name(), pet_name.c_str() );
    return;
  }
  p->dismiss();
}

bool player_t::recent_cast() const
{
  return ( last_cast > timespan_t::zero() ) &&
         ( ( last_cast + timespan_t::from_seconds( 5.0 ) ) > sim->current_time() );
}

dot_t* player_t::find_dot( const std::string& name, player_t* source ) const
{
  for ( size_t i = 0; i < dot_list.size(); ++i )
  {
    dot_t* d = dot_list[ i ];
    if ( d->source == source && d->name_str == name )
      return d;
  }
  return nullptr;
}

void player_t::clear_action_priority_lists() const
{
  for ( size_t i = 0; i < action_priority_list.size(); i++ )
  {
    action_priority_list_t* a = action_priority_list[ i ];
    a->action_list_str.clear();
    a->action_list.clear();
  }
}

/**
 * Replaces "old_list" action_priority_list data with "new_list" action_priority_list data
 */
void player_t::copy_action_priority_list( const std::string& old_list, const std::string& new_list )
{
  action_priority_list_t* ol = find_action_priority_list( old_list );
  action_priority_list_t* nl = find_action_priority_list( new_list );

  if ( ol && nl )
  {
    ol->action_list            = nl->action_list;
    ol->action_list_str        = nl->action_list_str;
    ol->foreground_action_list = nl->foreground_action_list;
    ol->off_gcd_actions        = nl->off_gcd_actions;
    ol->random                 = nl->random;
  }
}

template <typename T>
T* find_vector_member( const std::vector<T*>& list, const std::string& name )
{
  for ( auto t : list )
  {
    if ( t->name_str == name )
      return t;
  }
  return nullptr;
}

// player_t::find_action_priority_list( const std::string& name ) ===========

action_priority_list_t* player_t::find_action_priority_list( const std::string& name ) const
{
  return find_vector_member( action_priority_list, name );
}

pet_t* player_t::find_pet( const std::string& name ) const
{
  return find_vector_member( pet_list, name );
}

stats_t* player_t::find_stats( const std::string& name ) const
{
  return find_vector_member( stats_list, name );
}

gain_t* player_t::find_gain( const std::string& name ) const
{
  return find_vector_member( gain_list, name );
}

proc_t* player_t::find_proc( const std::string& name ) const
{
  return find_vector_member( proc_list, name );
}

luxurious_sample_data_t* player_t::find_sample_data( const std::string& name ) const
{
  return find_vector_member( sample_data_list, name );
}

benefit_t* player_t::find_benefit( const std::string& name ) const
{
  return find_vector_member( benefit_list, name );
}

uptime_t* player_t::find_uptime( const std::string& name ) const
{
  return find_vector_member( uptime_list, name );
}

cooldown_t* player_t::find_cooldown( const std::string& name ) const
{
  return find_vector_member( cooldown_list, name );
}

action_t* player_t::find_action( const std::string& name ) const
{
  return find_vector_member( action_list, name );
}

cooldown_t* player_t::get_cooldown( const std::string& name )
{
  cooldown_t* c = find_cooldown( name );

  if ( !c )
  {
    c = new cooldown_t( name, *this );

    cooldown_list.push_back( c );
  }

  return c;
}

real_ppm_t* player_t::get_rppm( const std::string& name, const spell_data_t* data, const item_t* item )
{
  auto it = range::find_if( rppm_list,
                            [&name]( const real_ppm_t* rppm ) { return util::str_compare_ci( rppm->name(), name ); } );

  if ( it != rppm_list.end() )
  {
    return *it;
  }

  real_ppm_t* new_rppm = new real_ppm_t( name, this, data, item );
  rppm_list.push_back( new_rppm );

  return new_rppm;
}

real_ppm_t* player_t::get_rppm( const std::string& name, double freq, double mod, unsigned s )
{
  auto it = range::find_if( rppm_list,
                            [&name]( const real_ppm_t* rppm ) { return util::str_compare_ci( rppm->name(), name ); } );

  if ( it != rppm_list.end() )
  {
    return *it;
  }

  real_ppm_t* new_rppm = new real_ppm_t( name, this, freq, mod, s );
  rppm_list.push_back( new_rppm );

  return new_rppm;
}

shuffled_rng_t* player_t::get_shuffled_rng( const std::string& name, int success_entries, int total_entries )
{
  auto it = range::find_if( shuffled_rng_list, [&name]( const shuffled_rng_t* shuffled_rng ) {
    return util::str_compare_ci( shuffled_rng->name(), name );
  } );

  if ( it != shuffled_rng_list.end() )
  {
    return *it;
  }

  shuffled_rng_t* new_shuffled_rng = new shuffled_rng_t( name, this, success_entries, total_entries );
  shuffled_rng_list.push_back( new_shuffled_rng );

  return new_shuffled_rng;
}

dot_t* player_t::get_dot( const std::string& name, player_t* source )
{
  dot_t* d = find_dot( name, source );

  if ( !d )
  {
    d = new dot_t( name, this, source );
    dot_list.push_back( d );
  }

  return d;
}

gain_t* player_t::get_gain( const std::string& name )
{
  gain_t* g = find_gain( name );

  if ( !g )
  {
    g = new gain_t( name );

    gain_list.push_back( g );
  }

  return g;
}

proc_t* player_t::get_proc( const std::string& name )
{
  proc_t* p = find_proc( name );

  if ( !p )
  {
    p = new proc_t( *sim, name );

    proc_list.push_back( p );
  }

  return p;
}

luxurious_sample_data_t* player_t::get_sample_data( const std::string& name )
{
  luxurious_sample_data_t* sd = find_sample_data( name );

  if ( !sd )
  {
    sd = new luxurious_sample_data_t( *this, name );

    sample_data_list.push_back( sd );
  }

  return sd;
}

stats_t* player_t::get_stats( const std::string& n, action_t* a )
{
  stats_t* stats = find_stats( n );

  if ( !stats )
  {
    stats = new stats_t( n, this );

    stats_list.push_back( stats );
  }

  assert( stats->player == this );

  if ( a )
    stats->action_list.push_back( a );

  return stats;
}

benefit_t* player_t::get_benefit( const std::string& name )
{
  benefit_t* u = find_benefit( name );

  if ( !u )
  {
    u = new benefit_t( name );

    benefit_list.push_back( u );
  }

  return u;
}

uptime_t* player_t::get_uptime( const std::string& name )
{
  uptime_t* u = find_uptime( name );

  if ( !u )
  {
    u = new uptime_t( name );

    uptime_list.push_back( u );
  }

  return u;
}

action_priority_list_t* player_t::get_action_priority_list( const std::string& name, const std::string& comment )
{
  action_priority_list_t* a = find_action_priority_list( name );
  if ( !a )
  {
    if ( action_list_id_ == 64 )
    {
      throw std::invalid_argument("Maximum number of action lists is 64");
    }

    a                          = new action_priority_list_t( name, this );
    a->action_list_comment_str = comment;
    a->internal_id             = action_list_id_++;
    a->internal_id_mask        = 1ULL << ( a->internal_id );

    action_priority_list.push_back( a );
  }
  return a;
}

int player_t::find_action_id( const std::string& name ) const
{
  for ( size_t i = 0; i < action_map.size(); i++ )
  {
    if ( util::str_compare_ci( name, action_map[ i ] ) )
      return static_cast<int>( i );
  }

  return -1;
}

int player_t::get_action_id( const std::string& name )
{
  auto id = find_action_id( name );
  if ( id != -1 )
  {
    return id;
  }

  action_map.push_back( name );
  return static_cast<int>(action_map.size() - 1);
}

wait_for_cooldown_t::wait_for_cooldown_t( player_t* player, const std::string& cd_name ) :
  wait_action_base_t( player, "wait_for_" + cd_name ),
  wait_cd( player->get_cooldown( cd_name ) ),
  a( player->find_action( cd_name ) )
{
  assert( a );
  interrupt_auto_attack = false;
  quiet                 = true;
}

timespan_t wait_for_cooldown_t::execute_time() const
{
  assert( wait_cd->duration > timespan_t::zero() );
  return wait_cd->remains();
}

snapshot_stats_t::snapshot_stats_t( player_t* player, const std::string& options_str ) :
  action_t( ACTION_OTHER, "snapshot_stats", player ),
  completed( false ),
  proxy_spell( 0 ),
  proxy_attack( 0 ),
  role( player->primary_role() )
{
  parse_options( options_str );
  trigger_gcd = timespan_t::zero();
  harmful     = false;

  if ( role == ROLE_SPELL || role == ROLE_HYBRID || role == ROLE_HEAL )
  {
    proxy_spell             = new spell_t( "snapshot_spell", player );
    proxy_spell->background = true;
    proxy_spell->callbacks  = false;
  }

  if ( role == ROLE_ATTACK || role == ROLE_HYBRID || role == ROLE_TANK )
  {
    proxy_attack             = new melee_attack_t( "snapshot_attack", player );
    proxy_attack->background = true;
    proxy_attack->callbacks  = false;
  }
}

void snapshot_stats_t::init_finished()
{
  player_t* p = player;
  for ( size_t i = 0; sim->report_pets_separately && i < p->pet_list.size(); ++i )
  {
    pet_t* pet             = p->pet_list[ i ];
    action_t* pet_snapshot = pet->find_action( "snapshot_stats" );
    if ( !pet_snapshot )
    {
      pet_snapshot = pet->create_action( "snapshot_stats", "" );
      pet_snapshot->init();
    }
  }

  action_t::init_finished();
}

void snapshot_stats_t::execute()
{
  player_t* p = player;

  if ( completed )
    return;

  completed = true;

  if ( p -> nth_iteration() > 0 )
    return;

  if ( sim->log )
    sim->out_log.printf( "%s performs %s", p->name(), name() );

  player_collected_data_t::buffed_stats_t& buffed_stats = p->collected_data.buffed_stats_snapshot;

  for ( attribute_e i = ATTRIBUTE_NONE; i < ATTRIBUTE_MAX; ++i )
    buffed_stats.attribute[ i ] = floor( p->get_attribute( i ) );

  buffed_stats.resource = p->resources.max;

  buffed_stats.spell_haste            = p->cache.spell_haste();
  buffed_stats.spell_speed            = p->cache.spell_speed();
  buffed_stats.attack_haste           = p->cache.attack_haste();
  buffed_stats.attack_speed           = p->cache.attack_speed();
  buffed_stats.mastery_value          = p->cache.mastery_value();
  buffed_stats.bonus_armor            = p->composite_bonus_armor();
  buffed_stats.damage_versatility     = p->cache.damage_versatility();
  buffed_stats.heal_versatility       = p->cache.heal_versatility();
  buffed_stats.mitigation_versatility = p->cache.mitigation_versatility();
  buffed_stats.run_speed              = p->cache.run_speed();
  buffed_stats.avoidance              = p->cache.avoidance();
  buffed_stats.leech                  = p->cache.leech();

  buffed_stats.spell_power =
      util::round( p->cache.spell_power( SCHOOL_MAX ) * p->composite_spell_power_multiplier() );
  buffed_stats.spell_hit          = p->cache.spell_hit();
  buffed_stats.spell_crit_chance  = p->cache.spell_crit_chance();
  buffed_stats.manareg_per_second = p->resource_regen_per_second( RESOURCE_MANA );

  buffed_stats.attack_power        = p->cache.attack_power() * p->composite_attack_power_multiplier();
  buffed_stats.attack_hit          = p->cache.attack_hit();
  buffed_stats.mh_attack_expertise = p->composite_melee_expertise( &( p->main_hand_weapon ) );
  buffed_stats.oh_attack_expertise = p->composite_melee_expertise( &( p->off_hand_weapon ) );
  buffed_stats.attack_crit_chance  = p->cache.attack_crit_chance();

  buffed_stats.armor = p->composite_armor();
  buffed_stats.miss  = p->composite_miss();
  buffed_stats.dodge = p->cache.dodge();
  buffed_stats.parry = p->cache.parry();
  buffed_stats.block = p->cache.block();
  buffed_stats.crit  = p->cache.crit_avoidance();

  double spell_hit_extra = 0, attack_hit_extra = 0, expertise_extra = 0;

  // The code below is not properly handling the case where the player has
  // so much Hit Rating or Expertise Rating that the extra amount of stat
  // they have is higher than the delta.

  // In this case, the following line in sc_scaling.cpp
  //     if ( divisor < 0.0 ) divisor += ref_p -> over_cap[ i ];
  // would give divisor a positive value (whereas it is expected to always
  // remain negative).
  // Also, if a player has an extra amount of Hit Rating or Expertise Rating
  // that is equal to the ``delta'', the following line in sc_scaling.cpp
  //     double score = ( delta_score - ref_score ) / divisor;
  // will cause a division by 0 error.
  // We would need to increase the delta before starting the scaling sim to remove this error

  if ( role == ROLE_SPELL || role == ROLE_HYBRID || role == ROLE_HEAL )
  {
    double chance = proxy_spell->miss_chance( proxy_spell->composite_hit(), sim->target );
    if ( chance < 0 )
      spell_hit_extra = -chance * p->current.rating.spell_hit;
  }

  if ( role == ROLE_ATTACK || role == ROLE_HYBRID || role == ROLE_TANK )
  {
    double chance = proxy_attack->miss_chance( proxy_attack->composite_hit(), sim->target );
    if ( p->dual_wield() )
      chance += 0.19;
    if ( chance < 0 )
      attack_hit_extra = -chance * p->current.rating.attack_hit;
    if ( p->position() != POSITION_FRONT )
    {
      chance = proxy_attack->dodge_chance( p->cache.attack_expertise(), sim->target );
      if ( chance < 0 )
        expertise_extra = -chance * p->current.rating.expertise;
    }
    else if ( p->position() == POSITION_FRONT )
    {
      chance = proxy_attack->parry_chance( p->cache.attack_expertise(), sim->target );
      if ( chance < 0 )
        expertise_extra = -chance * p->current.rating.expertise;
    }
  }

  if ( p->scaling )
  {
    p->scaling->over_cap[ STAT_HIT_RATING ]       = std::max( spell_hit_extra, attack_hit_extra );
    p->scaling->over_cap[ STAT_EXPERTISE_RATING ] = expertise_extra;
  }

  for ( size_t i = 0; i < p->pet_list.size(); ++i )
  {
    pet_t* pet             = p->pet_list[ i ];
    action_t* pet_snapshot = pet->find_action( "snapshot_stats" );
    if ( pet_snapshot )
    {
      pet_snapshot->execute();
    }
  }
}

void snapshot_stats_t::reset()
{
  action_t::reset();

  completed = false;
}

bool snapshot_stats_t::ready()
{
  if ( completed || player -> nth_iteration() > 0 )
    return false;
  return action_t::ready();
}

namespace
{  // ANONYMOUS

// Chosen Movement Actions ==================================================

struct start_moving_t : public action_t
{
  start_moving_t( player_t* player, const std::string& options_str ) : action_t( ACTION_OTHER, "start_moving", player )
  {
    parse_options( options_str );
    trigger_gcd           = timespan_t::zero();
    cooldown->duration    = timespan_t::from_seconds( 0.5 );
    harmful               = false;
    ignore_false_positive = true;
  }

  virtual void execute() override
  {
    player->buffs.movement->trigger();

    if ( sim->log )
      sim->out_log.printf( "%s starts moving.", player->name() );

    update_ready();
  }

  virtual bool ready() override
  {
    if ( player->buffs.movement->check() )
      return false;

    return action_t::ready();
  }
};

struct stop_moving_t : public action_t
{
  stop_moving_t( player_t* player, const std::string& options_str ) : action_t( ACTION_OTHER, "stop_moving", player )
  {
    parse_options( options_str );
    trigger_gcd           = timespan_t::zero();
    cooldown->duration    = timespan_t::from_seconds( 0.5 );
    harmful               = false;
    ignore_false_positive = true;
  }

  virtual void execute() override
  {
    player->buffs.movement->expire();

    if ( sim->log )
      sim->out_log.printf( "%s stops moving.", player->name() );
    update_ready();
  }

  virtual bool ready() override
  {
    if ( !player->buffs.movement->check() )
      return false;

    return action_t::ready();
  }
};

struct variable_t : public action_t
{
  action_var_e operation;
  action_variable_t* var;
  std::string value_str, value_else_str, var_name_str, condition_str;
  expr_t* value_expression;
  expr_t* condition_expression;
  expr_t* value_else_expression;

  variable_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_VARIABLE, "variable", player ),
    operation( OPERATION_SET ),
    var( nullptr ),
    value_expression( nullptr ),
    condition_expression( nullptr ),
    value_else_expression( nullptr )
  {
    quiet   = true;
    harmful = proc = callbacks = may_miss = may_crit = may_block = may_parry = may_dodge = false;
    trigger_gcd = timespan_t::zero();

    std::string operation_;
    double default_   = 0;
    timespan_t delay_ = timespan_t::zero();

    add_option( opt_string( "name", name_str ) );
    add_option( opt_string( "value", value_str ) );
    add_option( opt_string( "op", operation_ ) );
    add_option( opt_float( "default", default_ ) );
    add_option( opt_timespan( "delay", delay_ ) );
    add_option( opt_string( "condition", condition_str ) );
    add_option( opt_string( "value_else", value_else_str ) );
    parse_options( options_str );

    if ( name_str.empty() )
    {
      sim->errorf( "Player %s unnamed 'variable' action used", player->name() );
      background = true;
      return;
    }

    // Figure out operation
    if ( !operation_.empty() )
    {
      if ( util::str_compare_ci( operation_, "set" ) )
        operation = OPERATION_SET;
      else if ( util::str_compare_ci( operation_, "print" ) )
        operation = OPERATION_PRINT;
      else if ( util::str_compare_ci( operation_, "reset" ) )
        operation = OPERATION_RESET;
      else if ( util::str_compare_ci( operation_, "add" ) )
        operation = OPERATION_ADD;
      else if ( util::str_compare_ci( operation_, "sub" ) )
        operation = OPERATION_SUB;
      else if ( util::str_compare_ci( operation_, "mul" ) )
        operation = OPERATION_MUL;
      else if ( util::str_compare_ci( operation_, "div" ) )
        operation = OPERATION_DIV;
      else if ( util::str_compare_ci( operation_, "pow" ) )
        operation = OPERATION_POW;
      else if ( util::str_compare_ci( operation_, "mod" ) )
        operation = OPERATION_MOD;
      else if ( util::str_compare_ci( operation_, "min" ) )
        operation = OPERATION_MIN;
      else if ( util::str_compare_ci( operation_, "max" ) )
        operation = OPERATION_MAX;
      else if ( util::str_compare_ci( operation_, "floor" ) )
        operation = OPERATION_FLOOR;
      else if ( util::str_compare_ci( operation_, "ceil" ) )
        operation = OPERATION_CEIL;
      else if ( util::str_compare_ci( operation_, "setif" ) )
        operation = OPERATION_SETIF;
      else
      {
        sim->errorf(
            "Player %s unknown operation '%s' given for variable, valid values are 'set', 'print', and 'reset'.",
            player->name(), operation_.c_str() );
        background = true;
        return;
      }
    }

    // Printing needs a delay, otherwise the action list will not progress
    if ( operation == OPERATION_PRINT && delay_ == timespan_t::zero() )
      delay_ = timespan_t::from_seconds( 1.0 );

    if ( operation != OPERATION_FLOOR && operation != OPERATION_CEIL && operation != OPERATION_RESET &&
         operation != OPERATION_PRINT )
    {
      if ( value_str.empty() )
      {
        sim->errorf( "Player %s no value expression given for variable '%s'", player->name(), name_str.c_str() );
        background = true;
        return;
      }
      if ( operation == OPERATION_SETIF )
      {
        if ( condition_str.empty() )
        {
          sim->errorf( "Player %s no condition expression given for variable '%s'", player->name(), name_str.c_str() );
          background = true;
          return;
        }
        if ( value_else_str.empty() )
        {
          sim->errorf( "Player %s no value_else expression given for variable '%s'", player->name(), name_str.c_str() );
          background = true;
          return;
        }
      }
    }

    // Add a delay
    if ( delay_ > timespan_t::zero() )
    {
      std::string cooldown_name = "variable_actor";
      cooldown_name += util::to_string( player->index );
      cooldown_name += "_";
      cooldown_name += name_str;

      cooldown           = player->get_cooldown( cooldown_name );
      cooldown->duration = delay_;
    }

    // Find the variable
    for ( auto& elem : player->variables )
    {
      if ( util::str_compare_ci( elem->name_, name_str ) )
      {
        var = elem;
        break;
      }
    }

    if ( !var )
    {
      player->variables.push_back( new action_variable_t( name_str, default_ ) );
      var = player->variables.back();
    }
  }

  void init_finished() override
  {
    action_t::init_finished();

    if ( !background && operation != OPERATION_FLOOR && operation != OPERATION_CEIL && operation != OPERATION_RESET &&
         operation != OPERATION_PRINT )
    {
      value_expression = expr_t::parse( this, value_str, sim->optimize_expressions );
      if ( !value_expression )
      {
        sim->errorf( "Player %s unable to parse 'variable' value '%s'", player->name(), value_str.c_str() );
        background = true;
      }
      if ( operation == OPERATION_SETIF )
      {
        condition_expression = expr_t::parse( this, condition_str, sim->optimize_expressions );
        if ( !condition_expression )
        {
          sim->errorf( "Player %s unable to parse 'condition' value '%s'", player->name(), condition_str.c_str() );
          background = true;
        }
        value_else_expression = expr_t::parse( this, value_else_str, sim->optimize_expressions );
        if ( !value_else_expression )
        {
          sim->errorf( "Player %s unable to parse 'value_else' value '%s'", player->name(), value_else_str.c_str() );
          background = true;
        }
      }
    }

    if ( !background )
    {
      var->variable_actions.push_back( this );
    }
  }

  void reset() override
  {
    action_t::reset();

    double cv = 0;
    // In addition to if= expression removing the variable from the APLs, if the the variable value
    // is constant, we can remove any variable action referencing it from the APL
    if ( action_list && sim->optimize_expressions && player->nth_iteration() == 1 &&
         var->is_constant( &cv ) && ( ! if_expr || ( if_expr && if_expr->is_constant( &cv ) ) ) )
    {
      auto it = range::find( action_list->foreground_action_list, this );
      if ( it != action_list->foreground_action_list.end() )
      {
        sim->print_debug( "{} removing variable action {} from APL because the variable value is "
                          "constant (value={})",
            player->name(), signature_str, var->current_value_ );

        action_list->foreground_action_list.erase( it );
      }
    }
  }

  // A variable action is constant if
  // 1) The operation is not SETIF and the value expression is constant
  // 2) The operation is SETIF and both the condition expression and the value (or value expression)
  //    are both constant
  // 3) The operation is reset/floor/ceil and all of the other actions manipulating the variable are
  //    constant
  bool is_constant() const
  {
    double const_value = 0;
    // If the variable action is conditionally executed, and the conditional execution is not
    // constant, the variable cannot be constant.
    if ( if_expr && !if_expr->is_constant( &const_value ) )
    {
      return false;
    }

    // Special casing, some actions are only constant, if all of the other action variables in the
    // set (that manipulates a variable) are constant
    if ( operation == OPERATION_RESET || operation == OPERATION_FLOOR ||
         operation == OPERATION_CEIL )
    {
      auto it = range::find_if( var->variable_actions, [this]( const action_t* action ) {
        return action != this && !debug_cast<const variable_t*>( action )->is_constant();
      } );

      return it == var->variable_actions.end();
    }
    else if ( operation != OPERATION_SETIF )
    {
      return value_expression ? value_expression->is_constant( &const_value ) : true;
    }
    else
    {
      bool constant = condition_expression->is_constant( &const_value );
      if ( !constant )
      {
        return false;
      }

      if ( const_value != 0 )
      {
        return value_expression->is_constant( &const_value );
      }
      else
      {
        return value_else_expression->is_constant( &const_value );
      }
    }
  }

  // Variable action expressions have to do an optimization pass before other actions, so that
  // actions with variable expressions can know if the associated variable is constant
  void optimize_expressions()
  {
    if ( if_expr )
    {
      if_expr = if_expr->optimize();
    }

    if ( value_expression )
    {
      value_expression = value_expression->optimize();
    }

    if ( condition_expression )
    {
      value_expression = value_expression->optimize();
    }

    if ( value_else_expression )
    {
      value_else_expression = value_else_expression->optimize();
    }
  }

  ~variable_t()
  {
    delete value_expression;
    delete condition_expression;
    delete value_else_expression;
  }

  // Note note note, doesn't do anything that a real action does
  void execute() override
  {
    if ( sim->debug && operation != OPERATION_PRINT )
    {
      sim->out_debug.printf( "%s variable name=%s op=%d value=%f default=%f sig=%s", player->name(), var->name_.c_str(),
                             operation, var->current_value_, var->default_, signature_str.c_str() );
    }

    switch ( operation )
    {
      case OPERATION_SET:
        var->current_value_ = value_expression->eval();
        break;
      case OPERATION_ADD:
        var->current_value_ += value_expression->eval();
        break;
      case OPERATION_SUB:
        var->current_value_ -= value_expression->eval();
        break;
      case OPERATION_MUL:
        var->current_value_ *= value_expression->eval();
        break;
      case OPERATION_DIV:
      {
        auto v = value_expression->eval();
        // Disallow division by zero, set value to zero
        if ( v == 0 )
        {
          var->current_value_ = 0;
        }
        else
        {
          var->current_value_ /= v;
        }
        break;
      }
      case OPERATION_POW:
        var->current_value_ = std::pow( var->current_value_, value_expression->eval() );
        break;
      case OPERATION_MOD:
      {
        // Disallow division by zero, set value to zero
        auto v = value_expression->eval();
        if ( v == 0 )
        {
          var->current_value_ = 0;
        }
        else
        {
          var->current_value_ = std::fmod( var->current_value_, value_expression->eval() );
        }
        break;
      }
      case OPERATION_MIN:
        var->current_value_ = std::min( var->current_value_, value_expression->eval() );
        break;
      case OPERATION_MAX:
        var->current_value_ = std::max( var->current_value_, value_expression->eval() );
        break;
      case OPERATION_FLOOR:
        var->current_value_ = util::floor( var->current_value_ );
        break;
      case OPERATION_CEIL:
        var->current_value_ = util::ceil( var->current_value_ );
        break;
      case OPERATION_PRINT:
        // Only spit out prints in main thread
        if ( sim->parent == 0 )
          std::cout << "actor=" << player->name_str << " time=" << sim->current_time().total_seconds()
                    << " iteration=" << sim->current_iteration << " variable=" << var->name_.c_str()
                    << " value=" << var->current_value_ << std::endl;
        break;
      case OPERATION_RESET:
        var->reset();
        break;
      case OPERATION_SETIF:
        if ( condition_expression->eval() != 0 )
          var->current_value_ = value_expression->eval();
        else
          var->current_value_ = value_else_expression->eval();
        break;
      default:
        assert( 0 );
        break;
    }
  }
};

// ===== Racial Abilities ===================================================

struct racial_spell_t : public spell_t
{
  racial_spell_t( player_t* p, const std::string& token, const spell_data_t* spell, const std::string& options ) :
    spell_t( token, p, spell )
  {
    parse_options( options );
  }

  void init() override
  {
    spell_t::init();

    if ( &data() == &spell_data_not_found_t::singleton )
      background = true;
  }
};

// Shadowmeld ===============================================================

struct shadowmeld_t : public racial_spell_t
{
  shadowmeld_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "shadowmeld", p->find_racial_spell( "Shadowmeld" ), options_str )
  {
  }

  void execute() override
  {
    racial_spell_t::execute();

    player->buffs.shadowmeld->trigger();

    // Shadowmeld stops autoattacks
    if ( player->main_hand_attack && player->main_hand_attack->execute_event )
      event_t::cancel( player->main_hand_attack->execute_event );

    if ( player->off_hand_attack && player->off_hand_attack->execute_event )
      event_t::cancel( player->off_hand_attack->execute_event );
  }
};

// Arcane Torrent ===========================================================

struct arcane_torrent_t : public racial_spell_t
{
  double gain_pct;
  double gain_energy;

  arcane_torrent_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "arcane_torrent", p->find_racial_spell( "Arcane Torrent" ), options_str ),
    gain_pct( 0 ),
    gain_energy( 0 )
  {
    harmful = false;
    energize_type = ENERGIZE_ON_CAST;
    // Some specs need special handling here
    switch ( p->specialization() )
    {
      case MAGE_FROST:
      case MAGE_FIRE:
      case MAGE_ARCANE:
      case WARLOCK_AFFLICTION:
      case WARLOCK_DEMONOLOGY:
      case WARLOCK_DESTRUCTION:
        gain_pct = data().effectN( 2 ).percent();
        break;
      case MONK_MISTWEAVER:
        gain_pct = data().effectN( 3 ).percent();
        break;
      case MONK_WINDWALKER:
      {
        parse_effect_data( data().effectN( 2 ) );  // Chi
        break;
      }
      case MONK_BREWMASTER:
        gain_energy = data().effectN( 4 ).base_value();
        break;
      case PALADIN_HOLY:
        gain_pct = data().effectN( 3 ).percent();
        break;
      default:
        break;
    }
  }

  void execute() override
  {
    racial_spell_t::execute();

    if ( gain_pct > 0 )
    {
      double gain = player->resources.max[ RESOURCE_MANA ] * gain_pct;
      player->resource_gain( RESOURCE_MANA, gain, player->gains.arcane_torrent );
    }

    if ( gain_energy > 0 )
      player->resource_gain( RESOURCE_ENERGY, gain_energy, player->gains.arcane_torrent );
  }
};

// Berserking ===============================================================

struct berserking_t : public racial_spell_t
{
  berserking_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "berserking", p->find_racial_spell( "Berserking" ), options_str )
  {
    harmful = false;
  }

  virtual void execute() override
  {
    racial_spell_t::execute();

    player->buffs.berserking->trigger();
  }
};

// Blood Fury ===============================================================

struct blood_fury_t : public racial_spell_t
{
  blood_fury_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "blood_fury", p->find_racial_spell( "Blood Fury" ), options_str )
  {
    harmful = false;
  }

  virtual void execute() override
  {
    racial_spell_t::execute();

    player->buffs.blood_fury->trigger();
  }
};

// Darkflight ==============================================================

struct darkflight_t : public racial_spell_t
{
  darkflight_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "darkflight", p->find_racial_spell( "Darkflight" ), options_str )
  {
    parse_options( options_str );
  }

  virtual void execute() override
  {
    racial_spell_t::execute();

    player->buffs.darkflight->trigger();
  }
};

// Rocket Barrage ===========================================================

struct rocket_barrage_t : public racial_spell_t
{
  rocket_barrage_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "rocket_barrage", p->find_racial_spell( "Rocket Barrage" ), options_str )
  {
    parse_options( options_str );
  }
};

// Stoneform ================================================================

struct stoneform_t : public racial_spell_t
{
  stoneform_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "stoneform", p->find_racial_spell( "Stoneform" ), options_str )
  {
    harmful = false;
  }

  virtual void execute() override
  {
    racial_spell_t::execute();

    player->buffs.stoneform->trigger();
  }
};

// Light's Judgment =========================================================

struct lights_judgment_t : public racial_spell_t
{
  struct lights_judgment_damage_t : public spell_t
  {
    lights_judgment_damage_t( player_t* p ) : spell_t( "lights_judgment_damage", p, p->find_spell( 256893 ) )
    {
      background = may_crit = true;
      aoe                   = -1;
      // these are sadly hardcoded in the tooltip
      attack_power_mod.direct = 3.0;
      spell_power_mod.direct = 3.0;
    }

    double attack_direct_power_coefficient( const action_state_t* s ) const override
    {
      auto ap = composite_attack_power() * player->composite_attack_power_multiplier();
      auto sp = composite_spell_power() * player->composite_spell_power_multiplier();

      if ( ap <= sp )
        return 0;
      return spell_t::attack_direct_power_coefficient( s );
    }

    double spell_direct_power_coefficient( const action_state_t* s ) const override
    {
      auto ap = composite_attack_power() * player->composite_attack_power_multiplier();
      auto sp = composite_spell_power() * player->composite_spell_power_multiplier();

      if ( ap > sp )
        return 0;
      return spell_t::spell_direct_power_coefficient( s );
    }
  };

  action_t* damage;

  lights_judgment_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "lights_judgment", p->find_racial_spell( "Light's Judgment" ), options_str )
  {
    // The cast doesn't trigger combat
    may_miss = callbacks = harmful = false;

    damage = p->find_action( "lights_judgment_damage" );
    if ( damage == nullptr )
    {
      damage = new lights_judgment_damage_t( p );
    }

    add_child( damage );
  }

  void execute() override
  {
    racial_spell_t::execute();
    // Reduce the cooldown by 1.5s when used during precombat
    if ( ! player -> in_combat )
    {
      cooldown -> adjust( timespan_t::from_seconds( -1.5 ) );
    }
  }

  // Missile travels for 3 seconds, stored in missile speed
  timespan_t travel_time() const override
  {
    // Reduce the delay before the hit by 1.5s when used during precombat
    if ( ! player -> in_combat )
    {
      return timespan_t::from_seconds( travel_speed - 1.5 );
    }
    return timespan_t::from_seconds( travel_speed );
  }

  void impact( action_state_t* state ) override
  {
    racial_spell_t::impact( state );

    damage->set_target( state->target );
    damage->execute();
  }
};

// Arcane Pulse =============================================================

struct arcane_pulse_t : public racial_spell_t
{
  arcane_pulse_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "arcane_pulse", p->find_racial_spell( "Arcane Pulse" ), options_str )
  {
    may_crit = true;
    aoe      = -1;
    // these are sadly hardcoded in the tooltip
    attack_power_mod.direct = 0.5;
    spell_power_mod.direct = 0.25;
  }

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    auto ap = composite_attack_power() * player->composite_attack_power_multiplier();
    auto sp = composite_spell_power() * player->composite_spell_power_multiplier();

    if ( ap <= sp )
      return 0;
    return racial_spell_t::attack_direct_power_coefficient( s );
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    auto ap = composite_attack_power() * player->composite_attack_power_multiplier();
    auto sp = composite_spell_power() * player->composite_spell_power_multiplier();

    if ( ap > sp )
      return 0;
    return racial_spell_t::spell_direct_power_coefficient( s );
  }
};

// Ancestral Call ===========================================================

struct ancestral_call_t : public racial_spell_t
{
  ancestral_call_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "ancestral_call", p->find_racial_spell( "Ancestral Call" ), options_str )
  {
    harmful = false;
  }

  void execute() override
  {
    racial_spell_t::execute();

    auto& buffs = player->buffs.ancestral_call;
    buffs[ rng().range( buffs.size() ) ] -> trigger();
  }
};

// Fireblood ================================================================

struct fireblood_t : public racial_spell_t
{
  fireblood_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "fireblood", p->find_racial_spell( "Fireblood" ), options_str )
  {
    harmful = false;
  }

  void execute() override
  {
    racial_spell_t::execute();

    player->buffs.fireblood -> trigger();
  }
};

// Haymaker =================================================================

struct haymaker_t : public racial_spell_t
{
  haymaker_t( player_t* p, const std::string& options_str ) :
    racial_spell_t( p, "haymaker", p->find_racial_spell( "Haymaker" ), options_str )
  {
    may_crit = true;
    // Hardcoded in the tooltip.
    attack_power_mod.direct = 0.75;
    spell_power_mod.direct = 0.75;
  }

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    auto ap = composite_attack_power() * player->composite_attack_power_multiplier();
    auto sp = composite_spell_power() * player->composite_spell_power_multiplier();

    if ( ap <= sp )
      return 0;

    return racial_spell_t::attack_direct_power_coefficient( s );
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    auto ap = composite_attack_power() * player->composite_attack_power_multiplier();
    auto sp = composite_spell_power() * player->composite_spell_power_multiplier();

    if ( ap > sp )
      return 0;

    return racial_spell_t::spell_direct_power_coefficient( s );
  }
};

// Restart Sequence Action ==================================================

struct restart_sequence_t : public action_t
{
  sequence_t* seq;
  std::string seq_name_str;

  restart_sequence_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "restart_sequence", player ),
    seq( 0 ),
    seq_name_str( "default" )  // matches default name for sequences
  {
    add_option( opt_string( "name", seq_name_str ) );
    parse_options( options_str );
    ignore_false_positive = true;
    trigger_gcd           = timespan_t::zero();
  }

  virtual void init() override
  {
    action_t::init();

    if ( !seq )
    {
      for ( size_t i = 0; i < player->action_list.size() && !seq; ++i )
      {
        action_t* a = player->action_list[ i ];
        if ( a && a->type != ACTION_SEQUENCE )
          continue;

        if ( !seq_name_str.empty() )
          if ( a && seq_name_str != a->name_str )
            continue;

        seq = dynamic_cast<sequence_t*>( a );
      }

      if ( !seq )
      {
        throw std::invalid_argument(fmt::format("Can't find sequence '{}'.",
            seq_name_str.empty() ? "(default)" : seq_name_str.c_str() ));
      }
    }
  }

  virtual void execute() override
  {
    if ( sim->debug )
      sim->out_debug.printf( "%s restarting sequence %s", player->name(), seq_name_str.c_str() );
    seq->restart();
  }

  virtual bool ready() override
  {
    bool ret = seq && seq->can_restart();
    if ( ret )
      return action_t::ready();

    return ret;
  }
};

// Restore Mana Action ======================================================

struct restore_mana_t : public action_t
{
  double mana;

  restore_mana_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "restore_mana", player ),
    mana( 0 )
  {
    add_option( opt_float( "mana", mana ) );
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    double mana_missing = player->resources.max[ RESOURCE_MANA ] - player->resources.current[ RESOURCE_MANA ];
    double mana_gain    = mana;

    if ( mana_gain == 0 || mana_gain > mana_missing )
      mana_gain = mana_missing;

    if ( mana_gain > 0 )
    {
      player->resource_gain( RESOURCE_MANA, mana_gain, player->gains.restore_mana );
    }
  }
};

// Wait Fixed Action ========================================================

struct wait_fixed_t : public wait_action_base_t
{
  std::unique_ptr<expr_t> time_expr;

  wait_fixed_t( player_t* player, const std::string& options_str ) : wait_action_base_t( player, "wait" ), time_expr()
  {
    std::string sec_str = "1.0";

    add_option( opt_string( "sec", sec_str ) );
    parse_options( options_str );
    interrupt_auto_attack = false;  // Probably shouldn't interrupt autoattacks while waiting.
    quiet                 = true;

    time_expr = std::unique_ptr<expr_t>( expr_t::parse( this, sec_str ) );
    if ( !time_expr )
    {
      sim->errorf( "%s: Unable to generate wait expression from '%s'", player->name(), options_str.c_str() );
      background = true;
    }
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t wait = timespan_t::from_seconds( time_expr->eval() );
    if ( wait <= timespan_t::zero() )
      wait = player->available();
    return wait;
  }
};

// Wait Until Ready Action ==================================================

// wait until actions *before* this wait are ready.
struct wait_until_ready_t : public wait_fixed_t
{
  wait_until_ready_t( player_t* player, const std::string& options_str ) : wait_fixed_t( player, options_str )
  {
    interrupt_auto_attack = false;
    quiet                 = true;
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t wait    = wait_fixed_t::execute_time();
    timespan_t remains = timespan_t::zero();

    for ( size_t i = 0; i < player->action_list.size(); ++i )
    {
      action_t* a = player->action_list[ i ];
      assert( a );
      if ( a == NULL )  // For release builds.
        break;
      if ( a == this )
        break;
      if ( a->background )
        continue;

      remains = a->cooldown->remains();
      if ( remains > timespan_t::zero() && remains < wait )
        wait = remains;

      remains = a->get_dot()->remains();
      if ( remains > timespan_t::zero() && remains < wait )
        wait = remains;
    }

    if ( wait <= timespan_t::zero() )
      wait = player->available();

    return wait;
  }
};

// Use Item Action ==========================================================

struct use_item_t : public action_t
{
  item_t* item;
  std::string use_name;
  action_t* action;
  buff_t* buff;
  cooldown_t* cooldown_group;
  timespan_t cooldown_group_duration;
  std::string item_name, item_slot;

  use_item_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "use_item", player ),
    item( nullptr ),
    action( nullptr ),
    buff( nullptr ),
    cooldown_group( nullptr ),
    cooldown_group_duration( timespan_t::zero() )
  {
    add_option( opt_string( "name", item_name ) );
    add_option( opt_string( "slot", item_slot ) );
    parse_options( options_str );

    if ( !item_name.empty() )
    {
      item = player->find_item_by_name( item_name );
      if ( !item )
      {
        if ( sim->debug )
        {
          sim->errorf( "Player %s attempting 'use_item' action with item '%s' which is not currently equipped.\n",
                       player->name(), item_name.c_str() );
        }
        background = true;
        return;
      }

      name_str = name_str + "_" + item_name;
    }
    else if ( !item_slot.empty() )
    {
      slot_e s = util::parse_slot_type( item_slot );
      if ( s == SLOT_INVALID )
      {
        sim->errorf( "Player %s attempting 'use_item' action with invalid slot name '%s'.", player->name(),
                     item_slot.c_str() );
        background = true;
        return;
      }

      item = &( player->items[ s ] );

      if ( !item || !item->active() )
      {
        if ( sim->debug )
        {
          sim->errorf( "Player %s attempting 'use_item' action with invalid item '%s' in slot '%s'.", player->name(),
                       item->name(), item_slot.c_str() );
        }
        item       = 0;
        background = true;
      }
      else
      {
        name_str = name_str + "_" + item->name();
      }
    }
    else
    {
      sim->errorf( "Player %s has 'use_item' action with no 'name=' or 'slot=' option.\n", player->name() );
      background = true;
    }
  }

  void erase_action( action_priority_list_t* apl )
  {
    if ( apl == nullptr )
    {
      return;
    }

    auto it = range::find( apl->foreground_action_list, this );

    if ( it != apl->foreground_action_list.end() )
    {
      apl->foreground_action_list.erase( it );
    }
  }

  void init() override
  {
    action_t::init();

    action_priority_list_t* apl = nullptr;
    if ( action_list )
    {
      apl = player->find_action_priority_list( action_list->name_str );
    }

    if ( !item )
    {
      erase_action( apl );
      return;
    }

    // Parse Special Effect
    const special_effect_t* e = item->special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE );
    if ( e && e->type == SPECIAL_EFFECT_USE )
    {
      // Create a buff
      if ( e->buff_type() != SPECIAL_EFFECT_BUFF_NONE )
      {
        buff = buff_t::find( player, e->name() );
        if ( !buff )
        {
          buff = e->create_buff();
        }

        // On-use buff cooldowns are unconditionally handled by the action, so as a precaution,
        // reset any cooldown associated with the buff itself
        buff->set_cooldown( timespan_t::zero() );
      }

      // Create an action
      if ( e->action_type() != SPECIAL_EFFECT_ACTION_NONE )
      {
        action = e->create_action();
      }

      stats = player->get_stats( name_str, this );

      // Setup the long-duration cooldown for this item effect
      cooldown           = player->get_cooldown( e->cooldown_name() );
      cooldown->duration = e->cooldown();
      trigger_gcd        = timespan_t::zero();

      // Use DBC-backed item cooldown system for any item data coming from our local database that
      // has no user-given 'use' option on items.
      if ( e->item && util::str_compare_ci( e->item->source_str, "local" ) && e->item->option_use_str.empty() )
      {
        std::string cdgrp = e->cooldown_group_name();
        // No cooldown group will not trigger a shared cooldown
        if ( !cdgrp.empty() )
        {
          cooldown_group          = player->get_cooldown( cdgrp );
          cooldown_group_duration = e->cooldown_group_duration();
        }
        else
        {
          cooldown_group = &( player->item_cooldown );
        }
      }
      else
      {
        cooldown_group = &( player->item_cooldown );
        // Presumes on-use items will always have static durations. Considering the client data
        // system hardcodes the cooldown group durations in the DBC files, this is a reasonably safe
        // bet for now.
        if ( buff )
        {
          cooldown_group_duration = buff->buff_duration;
        }
      }
    }

    if ( !buff && !action )
    {
      if ( sim->debug )
      {
        sim->errorf( "Player %s has 'use_item' action with no custom buff or action setup.\n", player->name() );
      }
      background = true;

      erase_action( apl );
    }
  }

  void execute() override
  {
    bool triggered = buff == 0;
    if ( buff )
      triggered = buff->trigger();

    if ( triggered && action && ( !buff || buff->check() == buff->max_stack() ) )
    {
      action->target = target;
      action->schedule_execute();

      // Decide whether to expire the buff even with 1 max stack
      if ( buff && buff->max_stack() > 1 )
      {
        buff->expire();
      }
    }

    // Enable to report use_item ability
    // if ( ! dual ) stats -> add_execute( time_to_execute );

    update_ready();

    // Start the shared cooldown
    if ( cooldown_group_duration > timespan_t::zero() )
    {
      cooldown_group->start( cooldown_group_duration );
      if ( sim->debug )
      {
        sim->out_debug.printf( "%s starts shared cooldown for %s (%s). Will be ready at %.4f", player->name(), name(),
                               cooldown_group->name(), cooldown_group->ready.total_seconds() );
      }
    }
  }

  bool ready() override
  {
    if ( !item )
      return false;

    if ( cooldown_group && cooldown_group->remains() > timespan_t::zero() )
    {
      return false;
    }

    if ( action && !action->ready() )
    {
      return false;
    }

    return action_t::ready();
  }

  expr_t* create_special_effect_expr( const std::vector<std::string>& data_str_split )
  {
    struct use_item_buff_type_expr_t : public expr_t
    {
      double v;

      use_item_buff_type_expr_t( bool state ) : expr_t( "use_item_buff_type" ), v( state ? 1.0 : 0 )
      {
      }

      bool is_constant( double* ) override
      {
        return true;
      }

      double evaluate() override
      {
        return v;
      }
    };

    if ( data_str_split.size() != 2 || !util::str_compare_ci( data_str_split[ 0 ], "use_buff" ) )
    {
      return 0;
    }

    stat_e stat = util::parse_stat_type( data_str_split[ 1 ] );
    if ( stat == STAT_NONE )
    {
      return 0;
    }

    const special_effect_t* e = item->special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE );
    if ( !e )
    {
      return 0;
    }

    return new use_item_buff_type_expr_t( e->stat_type() == stat );
  }

  expr_t* create_expression( const std::string& name_str ) override
  {
    std::vector<std::string> split = util::string_split( name_str, "." );
    if ( expr_t* e = create_special_effect_expr( split ) )
    {
      return e;
    }

    return action_t::create_expression( name_str );
  }
};

// Use Items Action =========================================================

struct use_items_t : public action_t
{
  std::vector<use_item_t*> use_actions;  // List of proxy use_item_t actions to execute
  std::vector<slot_e> priority_slots;    // Slot priority, or custom slots to check
  bool custom_slots;                     // Custom slots= parameter passed. Only check priority_slots.

  use_items_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_USE, "use_items", player ),
    // Ensure trinkets are checked before all other items by default
    priority_slots( {SLOT_TRINKET_1, SLOT_TRINKET_2} ),
    custom_slots( false )
  {
    callbacks = may_miss = may_crit = may_block = may_parry = false;

    trigger_gcd = timespan_t::zero();

    add_option( opt_func( "slots", std::bind( &use_items_t::parse_slots, this, std::placeholders::_1,
                                              std::placeholders::_2, std::placeholders::_3 ) ) );

    parse_options( options_str );
  }

  result_e calculate_result( action_state_t* ) const override
  {
    return RESULT_HIT;
  }

  void execute() override
  {
    // Execute first ready sub-action. Note that technically an on-use action can go "unavailable"
    // between the ready check and the execution, meaning this action actually executes no
    // sub-action
    for ( auto action : use_actions )
    {
      if ( action->ready() )
      {
        action->execute();
        break;
      }
    }

    update_ready();
  }

  // Init creates the sub-actions (use_item_t). The sub-action creation is deferred to here (instead
  // of the constructor) to be able to skip actor slots that already have an use_item action defiend
  // in the action list.
  void init() override
  {
    create_use_subactions();

    // No use_item sub-actions created here, so this action does not need to execute ever. The
    // parent init() call below will filter it out from the "foreground action list".
    if ( use_actions.size() == 0 )
    {
      background = true;
    }

    action_t::init();
  }

  bool ready() override
  {
    // use_items action itself is not ready
    if ( !action_t::ready() )
    {
      return false;
    }

    // Check all use_item actions, if at least one of them is ready, this use_items action is ready
    for ( const auto action : use_actions )
    {
      if ( action->ready() )
      {
        return true;
      }
    }

    return false;
  }

  bool parse_slots( sim_t* /* sim */, const std::string& /* opt_name */, const std::string& opt_value )
  {
    // Empty out default priority slots. slots= option will change the use_items action logic so
    // that only the designated slots will be checked/executed for a special effect
    priority_slots.clear();

    auto split = util::string_split( ":|", opt_value );
    range::for_each( split, [this]( const std::string& slot_str ) {
      auto slot = util::parse_slot_type( slot_str );
      if ( slot != SLOT_INVALID )
      {
        priority_slots.push_back( slot );
      }
    } );

    custom_slots = true;

    // If slots= option is given, presume that at aleast one of those slots is a valid slot name
    return priority_slots.size() > 0;
  }

  // Creates "use_item" actions for all usable special effects on the actor. Note that the creation
  // is done in a specified slot order (by default trinkets before other slots).
  void create_use_subactions()
  {
    std::vector<const use_item_t*> use_item_actions;
    // Collect a list of existing use_item actions in the APL.
    range::for_each( player->action_list, [&use_item_actions]( const action_t* action ) {
      if ( const use_item_t* use_item = dynamic_cast<const use_item_t*>( action ) )
      {
        use_item_actions.push_back( use_item );
      }
    } );

    std::vector<slot_e> slot_order = priority_slots;
    // Add in the rest of the slots, if no slots= parameter is given. If a slots= parameter is
    // given, only the slots in that parameter value will be checked.
    for ( auto slot = SLOT_MIN; !custom_slots && slot < SLOT_MAX; ++slot )
    {
      if ( range::find( slot_order, slot ) == slot_order.end() )
      {
        slot_order.push_back( slot );
      }
    }

    // Remove any slots from the slot list that have custom use item actions. Note that this search
    // looks for use_item,slot=X.
    range::for_each( use_item_actions, [&slot_order]( const use_item_t* action ) {
      slot_e slot = util::parse_slot_type( action->item_slot );
      if ( slot == SLOT_INVALID )
      {
        return;
      }

      auto it = range::find( slot_order, slot );
      if ( it != slot_order.end() )
      {
        slot_order.erase( it );
      }
    } );

    // Remove any slots from the list, where the actor has an item equipped, and corresponding a
    // use_item,name=X action for the item.
    range::for_each( use_item_actions, [this, &slot_order]( const use_item_t* action ) {
      if ( action->item_name.empty() )
      {
        return;
      }

      // Find out if the item is worn
      auto it = range::find_if( player->items, [action]( const item_t& item ) {
        return util::str_compare_ci( item.name(), action->item_name );
      } );

      // Worn item, remove slot if necessary
      if ( it != player->items.end() )
      {
        auto slot_it = range::find( slot_order, it->slot );
        if ( slot_it != slot_order.end() )
        {
          slot_order.erase( slot_it );
        }
      }
    } );

    // Create use_item actions for each remaining slot, if the user has an on-use item in that slot.
    // Note that this only looks at item-sourced on-use actions (e.g., no engineering addons).
    range::for_each( slot_order, [this]( slot_e slot ) {
      const auto& item     = player->items[ slot ];
      const auto effect_it = range::find_if( item.parsed.special_effects, []( const special_effect_t* e ) {
        return e->source == SPECIAL_EFFECT_SOURCE_ITEM && e->type == SPECIAL_EFFECT_USE;
      } );

      // No item-based on-use effect in the slot, skip
      if ( effect_it == item.parsed.special_effects.end() )
      {
        return;
      }

      if ( sim->debug )
      {
        sim->out_debug.printf( "%s use_items creating proxy action for %s (slot=%s)", player->name(),
                               item.full_name().c_str(), item.slot_name() );
      }

      use_actions.push_back( new use_item_t( player, std::string( "slot=" ) + item.slot_name() ) );

      auto action = use_actions.back();
      // The use_item action is not triggered by the actor (through the APL), so background it
      action->background = true;
    } );
  }
};

// Cancel Buff ==============================================================

struct cancel_buff_t : public action_t
{
  buff_t* buff;

  cancel_buff_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "cancel_buff", player ),
    buff( 0 )
  {
    std::string buff_name;
    add_option( opt_string( "name", buff_name ) );
    parse_options( options_str );
    ignore_false_positive = true;
    if ( buff_name.empty() )
    {
      sim->errorf( "Player %s uses cancel_buff without specifying the name of the buff\n", player->name() );
      sim->cancel();
    }

    buff = buff_t::find( player, buff_name );

    // if the buff isn't in the player_t -> buff_list, try again in the player_td_t -> target -> buff_list
    if ( !buff )
    {
      buff = buff_t::find( player->get_target_data( player )->target, buff_name );
    }

    if ( !buff )
    {
      sim->errorf( "Player %s uses cancel_buff with unknown buff %s\n", player->name(), buff_name.c_str() );
      sim->cancel();
    }
    else if ( !buff->can_cancel )
    {
      sim->errorf( "Player %s uses cancel_buff on %s, which cannot be cancelled in game\n", player->name(),
                   buff_name.c_str() );
      sim->cancel();
    }

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute() override
  {
    if ( sim->log )
      sim->out_log.printf( "%s cancels buff %s", player->name(), buff->name() );
    buff->expire();
  }

  virtual bool ready() override
  {
    if ( !buff || !buff->check() )
      return false;

    return action_t::ready();
  }
};

struct cancel_action_t : public action_t
{
  cancel_action_t( player_t* player, const std::string& options_str ) :
    action_t( ACTION_OTHER, "cancel_action", player )
  {
    parse_options( options_str );
    harmful = false;
    usable_while_casting = use_while_casting = ignore_false_positive = true;
    trigger_gcd = timespan_t::zero();
    action_skill = 1;
  }

  virtual void execute() override
  {
    sim->print_log( "{} performs {}", player->name(), name() );
    player->interrupt();
  }

  virtual bool ready() override
  {
    if ( !player->executing && !player->channeling )
      return false;

    return action_t::ready();
  }
};

/**
 * Pool Resource
 *
 * If the next action in the list would be "ready" if it was not constrained by resource,
 * then this command will pool resource until we have enough.
 */

struct pool_resource_t : public action_t
{
  resource_e resource;
  std::string resource_str;
  timespan_t wait;
  int for_next;
  action_t* next_action;
  std::string amount_str;
  expr_t* amount_expr;

  pool_resource_t( player_t* p, const std::string& options_str, resource_e r = RESOURCE_NONE ) :
    action_t( ACTION_OTHER, "pool_resource", p ),
    resource( r != RESOURCE_NONE ? r : p->primary_resource() ),
    wait( timespan_t::from_seconds( 0.251 ) ),
    for_next( 0 ),
    next_action( 0 ),
    amount_expr( nullptr )
  {
    quiet = true;
    add_option( opt_timespan( "wait", wait ) );
    add_option( opt_bool( "for_next", for_next ) );
    add_option( opt_string( "resource", resource_str ) );
    add_option( opt_string( "extra_amount", amount_str ) );
    parse_options( options_str );

    if ( !resource_str.empty() )
    {
      resource_e res = util::parse_resource_type( resource_str );
      if ( res != RESOURCE_NONE )
        resource = res;
    }
  }

  ~pool_resource_t()
  {
    delete amount_expr;
  }

  void init_finished() override
  {
    action_t::init_finished();

    if ( !amount_str.empty() )
    {
      amount_expr = expr_t::parse( this, amount_str, sim->optimize_expressions );
      if (amount_expr == nullptr)
      {
        throw std::invalid_argument(fmt::format("Could not parse amount if expression from '{}'", amount_str));
      }

    }
  }

  virtual void reset() override
  {
    action_t::reset();

    if ( !next_action && for_next )
    {
      for ( size_t i = 0; i < player->action_priority_list.size(); i++ )
      {
        for ( size_t j = 0; j < player->action_priority_list[ i ]->foreground_action_list.size(); j++ )
        {
          if ( player->action_priority_list[ i ]->foreground_action_list[ j ] != this )
            continue;

          if ( ++j != player->action_priority_list[ i ]->foreground_action_list.size() )
          {
            next_action = player->action_priority_list[ i ]->foreground_action_list[ j ];
            break;
          }
        }
      }

      if ( !next_action )
      {
        sim->errorf( "%s: can't find next action.\n", __FUNCTION__ );
        background = true;
      }
    }
  }

  virtual void execute() override
  {
    if ( sim->log )
      sim->out_log.printf( "%s performs %s", player->name(), name() );

    player->iteration_pooling_time += wait;
  }

  virtual timespan_t gcd() const override
  {
    return wait;
  }

  virtual bool ready() override
  {
    bool rd = action_t::ready();
    if ( !rd )
      return rd;

    if ( next_action )
    {
      if ( next_action->action_ready() )
        return false;

      // If the next action in the list would be "ready" if it was not constrained by energy,
      // then this command will pool energy until we have enough.

      double theoretical_cost = next_action->cost() + ( amount_expr ? amount_expr->eval() : 0 );
      player->resources.current[ resource ] += theoretical_cost;

      bool resource_limited = next_action->action_ready();

      player->resources.current[ resource ] -= theoretical_cost;

      if ( !resource_limited )
        return false;
    }

    return rd;
  }
};

}  // UNNAMED NAMESPACE

action_t* player_t::create_action( const std::string& name, const std::string& options_str )
{
  if ( name == "ancestral_call" )
    return new ancestral_call_t( this, options_str );
  if ( name == "arcane_pulse" )
    return new arcane_pulse_t( this, options_str );
  if ( name == "arcane_torrent" )
    return new arcane_torrent_t( this, options_str );
  if ( name == "berserking" )
    return new berserking_t( this, options_str );
  if ( name == "blood_fury" )
    return new blood_fury_t( this, options_str );
  if ( name == "darkflight" )
    return new darkflight_t( this, options_str );
  if ( name == "fireblood" )
    return new fireblood_t( this, options_str );
  if ( name == "haymaker" )
    return new haymaker_t( this, options_str );
  if ( name == "lights_judgment" )
    return new lights_judgment_t( this, options_str );
  if ( name == "rocket_barrage" )
    return new rocket_barrage_t( this, options_str );
  if ( name == "shadowmeld" )
    return new shadowmeld_t( this, options_str );
  if ( name == "stoneform" )
    return new stoneform_t( this, options_str );

  if ( name == "cancel_action" )
    return new cancel_action_t( this, options_str );
  if ( name == "cancel_buff" )
    return new cancel_buff_t( this, options_str );
  if ( name == "swap_action_list" )
    return new swap_action_list_t( this, options_str );
  if ( name == "run_action_list" )
    return new run_action_list_t( this, options_str );
  if ( name == "call_action_list" )
    return new call_action_list_t( this, options_str );
  if ( name == "restart_sequence" )
    return new restart_sequence_t( this, options_str );
  if ( name == "restore_mana" )
    return new restore_mana_t( this, options_str );
  if ( name == "sequence" )
    return new sequence_t( this, options_str );
  if ( name == "strict_sequence" )
    return new strict_sequence_t( this, options_str );
  if ( name == "snapshot_stats" )
    return new snapshot_stats_t( this, options_str );
  if ( name == "start_moving" )
    return new start_moving_t( this, options_str );
  if ( name == "stop_moving" )
    return new stop_moving_t( this, options_str );
  if ( name == "use_item" )
    return new use_item_t( this, options_str );
  if ( name == "use_items" )
    return new use_items_t( this, options_str );
  if ( name == "wait" )
    return new wait_fixed_t( this, options_str );
  if ( name == "wait_until_ready" )
    return new wait_until_ready_t( this, options_str );
  if ( name == "pool_resource" )
    return new pool_resource_t( this, options_str );
  if ( name == "variable" )
    return new variable_t( this, options_str );

  return consumable::create_action( this, name, options_str );
}

void player_t::parse_talents_numbers( const std::string& talent_string )
{
  talent_points.clear();

  int i_max = std::min( static_cast<int>( talent_string.size() ), MAX_TALENT_ROWS );

  for ( int i = 0; i < i_max; ++i )
  {
    char c = talent_string[ i ];
    if ( c < '0' || c > ( '0' + MAX_TALENT_COLS ) )
    {
      std::runtime_error(fmt::format("Illegal character '{}' in talent encoding.", c ));
    }
    if ( c > '0' )
      talent_points.select_row_col( i, c - '1' );
  }

  create_talents_numbers();
}

/**
 * Old-style armory format for xx.battle.net / xx.battlenet.com
 */
bool player_t::parse_talents_armory( const std::string& talent_string )
{
  talent_points.clear();

  if ( talent_string.size() < 2 )
  {
    sim->errorf( "Player %s has malformed MoP battle.net talent string. Empty or too short string.\n", name() );
    return false;
  }

  // Verify class
  {
    player_e w_class = PLAYER_NONE;
    switch ( talent_string[ 0 ] )
    {
      case 'd':
        w_class = DEATH_KNIGHT;
        break;
      case 'g':
        w_class = DEMON_HUNTER;
        break;
      case 'U':
        w_class = DRUID;
        break;
      case 'Y':
        w_class = HUNTER;
        break;
      case 'e':
        w_class = MAGE;
        break;
      case 'f':
        w_class = MONK;
        break;
      case 'b':
        w_class = PALADIN;
        break;
      case 'X':
        w_class = PRIEST;
        break;
      case 'c':
        w_class = ROGUE;
        break;
      case 'W':
        w_class = SHAMAN;
        break;
      case 'V':
        w_class = WARLOCK;
        break;
      case 'Z':
        w_class = WARRIOR;
        break;
      default:
        sim->errorf( "Player %s has malformed talent string '%s': invalid class character '%c'.\n", name(),
                     talent_string.c_str(), talent_string[ 0 ] );
        return false;
    }

    if ( w_class != type )
    {
      sim->errorf( "Player %s has malformed talent string '%s': specified class %s does not match player class %s.\n",
                   name(), talent_string.c_str(), util::player_type_string( w_class ),
                   util::player_type_string( type ) );
      return false;
    }
  }

  std::string::size_type cut_pt = talent_string.find( '!' );
  if ( cut_pt == talent_string.npos )
  {
    sim->errorf( "Player %s has malformed talent string '%s'.\n", name(), talent_string.c_str() );
    return false;
  }

  std::string spec_string = talent_string.substr( 1, cut_pt - 1 );
  if ( !spec_string.empty() )
  {
    unsigned specidx = 0;
    // A spec was specified
    switch ( spec_string[ 0 ] )
    {
      case 'a':
        specidx = 0;
        break;
      case 'Z':
        specidx = 1;
        break;
      case 'b':
        specidx = 2;
        break;
      case 'Y':
        specidx = 3;
        break;
      default:
        sim->errorf( "Player %s has malformed talent string '%s': invalid spec character '%c'.\n", name(),
                     talent_string.c_str(), spec_string[ 0 ] );
        return false;
    }

    _spec = dbc.spec_by_idx( type, specidx );
  }

  std::string t_str = talent_string.substr( cut_pt + 1 );
  if ( t_str.empty() )
  {
    // No talents picked.
    return true;
  }

  //  if ( t_str.size() < MAX_TALENT_ROWS )
  //  {
  //    sim -> errorf( "Player %s has malformed talent string '%s': talent list too short.\n",
  //                   name(), talent_string.c_str() );
  //    return false;
  //  }

  for ( size_t i = 0; i < std::min( t_str.size(), (size_t)MAX_TALENT_ROWS ); ++i )
  {
    switch ( t_str[ i ] )
    {
      case '.':
        break;
      case '0':
      case '1':
      case '2':
        talent_points.select_row_col( static_cast<int>( i ), t_str[ i ] - '0' );
        break;
      default:
        sim->errorf( "Player %s has malformed talent string '%s': talent list has invalid character '%c'.\n", name(),
                     talent_string.c_str(), t_str[ i ] );
        return false;
    }
  }

  create_talents_armory();

  return true;
}

namespace
{
std::string armory2_class_name( const std::string& tokenized_class, const std::string& tokenized_spec )
{
  auto class_str = tokenized_class;
  auto spec_str  = tokenized_spec;

  util::replace_all( class_str, "-", " " );
  util::replace_all( spec_str, "-", " " );
  return util::inverse_tokenize( spec_str ) + " " + util::inverse_tokenize( class_str );
}

player_e armory2_parse_player_type( const std::string& class_name )
{
  if ( util::str_compare_ci( class_name, "death-knight" ) )
  {
    return DEATH_KNIGHT;
  }
  else if ( util::str_compare_ci( class_name, "demon-hunter" ) )
  {
    return DEMON_HUNTER;
  }
  else
  {
    return util::parse_player_type( class_name );
  }
}
}  // namespace

/**
 * New armory format used in worldofwarcraft.com / www.wowchina.com
 */
bool player_t::parse_talents_armory2( const std::string& talents_url )
{
  auto split = util::string_split( talents_url, "#/=" );
  if ( split.size() < 5 )
  {
    sim->errorf( "Player %s has malformed talent url '%s'", name(), talents_url.c_str() );
    return false;
  }

  // Sanity check that second to last split is "talents"
  if ( !util::str_compare_ci( split[ split.size() - 2 ], "talents" ) )
  {
    sim->errorf( "Player %s has malformed talent url '%s'", name(), talents_url.c_str() );
    return false;
  }

  const size_t OFFSET_CLASS   = split.size() - 4;
  const size_t OFFSET_SPEC    = split.size() - 3;
  const size_t OFFSET_TALENTS = split.size() - 1;

  auto spec_name_str = armory2_class_name( split[ OFFSET_CLASS ], split[ OFFSET_SPEC ] );
  auto player_type   = armory2_parse_player_type( split[ OFFSET_CLASS ] );
  auto spec_type     = util::parse_specialization_type( spec_name_str );

  if ( player_type == PLAYER_NONE || type != player_type )
  {
    sim->errorf( "Player %s has malformed talent url '%s': expected class '%s', got '%s'", name(), talents_url.c_str(),
                 util::player_type_string( type ), split[ OFFSET_CLASS ].c_str() );
    return false;
  }

  if ( spec_type == SPEC_NONE || specialization() != spec_type )
  {
    sim->errorf( "Player %s has malformed talent url '%s': expected specialization '%s', got '%s'", name(),
                 talents_url.c_str(), dbc::specialization_string( specialization() ).c_str(),
                 split[ OFFSET_SPEC ].c_str() );
    return false;
  }

  talent_points.clear();

  auto idx_max = std::min( as<int>( split[ OFFSET_TALENTS ].size() ), MAX_TALENT_ROWS );

  for ( auto talent_idx = 0; talent_idx < idx_max; ++talent_idx )
  {
    auto c = split[ OFFSET_TALENTS ][ talent_idx ];
    if ( c < '0' || c > ( '0' + MAX_TALENT_COLS ) )
    {
      sim->errorf( "Player %s has illegal character '%c' in talent encoding.\n", name(), c );
      return false;
    }

    if ( c > '0' )
    {
      talent_points.select_row_col( talent_idx, c - '1' );
    }
  }

  create_talents_armory();

  return true;
}

void player_t::create_talents_wowhead()
{
  // The Wowhead talent scheme encodes three talent selections per character
  // in at most two characters to represent all six talent choices. Basically,
  // each "row" of choices is numbered from 0 (no choice) to 3 (rightmost
  // talent). For each set of 3 rows, total together
  //   <first row choice> + (4 * <second row>) + (16 * <third row>)
  // If that total is zero, the encoding character is omitted. If non-zero,
  // add the total to the ascii code for '/' to obtain the encoding
  // character.
  //
  // Decoding is pretty simple, subtract '/' from the encoded character to
  // obtain the original total, the row choices are then total % 4,
  // (total / 4) % 4, and (total / 16) % 4 respectively.

  talents_str.clear();
  std::string result = "https://www.wowhead.com/talent#";

  // Class Type
  {
    char c;
    switch ( type )
    {
      case DEATH_KNIGHT:
        c = 'k';
        break;
      case DRUID:
        c = 'd';
        break;
      case HUNTER:
        c = 'h';
        break;
      case MAGE:
        c = 'm';
        break;
      case MONK:
        c = 'n';
        break;
      case PALADIN:
        c = 'l';
        break;
      case PRIEST:
        c = 'p';
        break;
      case ROGUE:
        c = 'r';
        break;
      case SHAMAN:
        c = 's';
        break;
      case WARLOCK:
        c = 'o';
        break;
      case WARRIOR:
        c = 'w';
        break;
      default:
        return;
    }
    result += c;
  }

  // Spec if specified
  {
    uint32_t idx = 0;
    uint32_t cid = 0;
    if ( dbc.spec_idx( _spec, cid, idx ) && ( (int)cid == util::class_id( type ) ) )
    {
      switch ( idx )
      {
        case 0:
          result += '!';
          break;
        case 1:
          result += 'x';
          break;
        case 2:
          result += 'y';
          break;
        case 3:
          result += 'z';
          break;
        default:
          break;
      }
    }
  }

  int encoding[ 2 ] = {0, 0};

  for ( int tier = 0; tier < 2; ++tier )
  {
    for ( int row = 0, multiplier = 1; row < 3; ++row, multiplier *= 4 )
    {
      for ( int col = 0; col < MAX_TALENT_COLS; ++col )
      {
        if ( talent_points.has_row_col( ( tier * 3 ) + row, col ) )
        {
          encoding[ tier ] += ( col + 1 ) * multiplier;
          break;
        }
      }
    }
  }

  if ( encoding[ 0 ] == 0 )
  {
    if ( encoding[ 1 ] == 0 )
      return;
    // The representation for NO talent selected in all 3 rows is to omit the
    // character; astute observers will see right away that talent specs with
    // no selections in the first three rows but with talents in the second 3
    // rows will break the encoding scheme.
    //
    // Select the first talent in the first tier as a workaround.
    encoding[ 0 ] = 1;
  }

  result += encoding[ 0 ] + '/';
  if ( encoding[ 1 ] != 0 )
    result += encoding[ 1 ] + '/';

  talents_str = result;
}

/**
 * Get average item level the player is wearing
 */
double player_t::avg_item_level() const
{
  double avg_ilvl    = 0.0;
  int num_ilvl_items = 0;
  for ( const auto& item : items )
  {
    if ( item.slot != SLOT_SHIRT && item.slot != SLOT_TABARD && item.slot != SLOT_RANGED && item.active() )
    {
      avg_ilvl += item.item_level();
      num_ilvl_items++;
    }
  }

  if ( num_ilvl_items > 1 )
    avg_ilvl /= num_ilvl_items;

  return avg_ilvl;
}

void player_t::create_talents_armory()
{
  if ( is_enemy() )
    return;

  talents_str = util::create_blizzard_talent_url( *this );
}

void player_t::create_talents_numbers()
{
  talents_str.clear();

  for ( int j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    talents_str += util::to_string( talent_points.choice( j ) + 1 );
  }
}

bool player_t::parse_talents_wowhead( const std::string& talent_string )
{
  talent_points.clear();

  if ( talent_string.empty() )
  {
    sim->errorf( "Player %s has empty wowhead talent string.\n", name() );
    return false;
  }

  // Verify class
  {
    player_e w_class;
    switch ( talent_string[ 0 ] )
    {
      case 'k':
        w_class = DEATH_KNIGHT;
        break;
      case 'd':
        w_class = DRUID;
        break;
      case 'h':
        w_class = HUNTER;
        break;
      case 'm':
        w_class = MAGE;
        break;
      case 'n':
        w_class = MONK;
        break;
      case 'l':
        w_class = PALADIN;
        break;
      case 'p':
        w_class = PRIEST;
        break;
      case 'r':
        w_class = ROGUE;
        break;
      case 's':
        w_class = SHAMAN;
        break;
      case 'o':
        w_class = WARLOCK;
        break;
      case 'w':
        w_class = WARRIOR;
        break;
      default:
        sim->errorf( "Player %s has malformed wowhead talent string '%s': invalid class character '%c'.\n", name(),
                     talent_string.c_str(), talent_string[ 0 ] );
        return false;
    }

    if ( w_class != type )
    {
      sim->errorf(
          "Player %s has malformed wowhead talent string '%s': specified class %s does not match player class %s.\n",
          name(), talent_string.c_str(), util::player_type_string( w_class ), util::player_type_string( type ) );
      return false;
    }
  }

  std::string::size_type idx = 1;

  // Parse spec if specified
  {
    int w_spec_idx;
    switch ( talent_string[ idx++ ] )
    {
      case '!':
        w_spec_idx = 0;
        break;
      case 'x':
        w_spec_idx = 1;
        break;
      case 'y':
        w_spec_idx = 2;
        break;
      case 'z':
        w_spec_idx = 3;
        break;
      default:
        w_spec_idx = -1;
        --idx;
        break;
    }

    if ( w_spec_idx >= 0 )
      _spec = dbc.spec_by_idx( type, w_spec_idx );
  }

  if ( talent_string.size() > idx + 2 )
  {
    sim->errorf( "Player %s has malformed wowhead talent string '%s': too long.\n", name(), talent_string.c_str() );
    return false;
  }

  for ( int tier = 0; tier < 2 && idx + tier < talent_string.size(); ++tier )
  {
    // Process 3 rows of talents per encoded character.
    int total = static_cast<unsigned char>( talent_string[ idx + tier ] );

    if ( ( total < '0' ) || ( total > '/' + 3 * ( 1 + 4 + 16 ) ) )
    {
      sim->errorf(
          "Player %s has malformed wowhead talent string '%s': encoded character '%c' in position %d is invalid.\n",
          name(), talent_string.c_str(), total, (int)idx );
      return false;
    }

    total -= '/';

    for ( int row = 0; row < 3; ++row, total /= 4 )
    {
      int selection = total % 4;
      if ( selection )
        talent_points.select_row_col( 3 * tier + row, selection - 1 );
    }
  }

  if ( sim->debug )
  {
    sim->out_debug.printf( "Player %s wowhead talent string translation: '%s'\n", name(),
                           talent_points.to_string().c_str() );
  }

  create_talents_wowhead();

  return true;
}

static bool parse_min_gcd( sim_t* sim, const std::string& name, const std::string& value )
{
  if ( name != "min_gcd" )
    return false;

  double v = std::strtod( value.c_str(), nullptr );
  if ( v <= 0 )
  {
    sim->errorf( " %s: Invalid value '%s' for global minimum cooldown.", sim->active_player->name(), value.c_str() );
    return false;
  }

  sim->active_player->min_gcd = timespan_t::from_seconds( v );
  return true;
}

// TODO: HOTFIX handling
void player_t::replace_spells()
{
  uint32_t class_idx, spec_index;

  if ( !dbc.spec_idx( _spec, class_idx, spec_index ) )
    return;

  // Search spec spells for spells to replace.
  if ( _spec != SPEC_NONE )
  {
    for ( unsigned int i = 0; i < dbc.specialization_ability_size(); i++ )
    {
      unsigned id = dbc.specialization_ability( class_idx, spec_index, i );
      if ( id == 0 )
      {
        break;
      }
      const spell_data_t* s = dbc.spell( id );
      if ( s->replace_spell_id() && ( (int)s->level() <= true_level ) )
      {
        // Found a spell we should replace
        dbc.replace_id( s->replace_spell_id(), id );
      }
    }
  }

  // Search talents for spells to replace.
  for ( int j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    for ( int i = 0; i < MAX_TALENT_COLS; i++ )
    {
      if ( talent_points.has_row_col( j, i ) && true_level < std::min( ( j + 1 ) * 15, 100 ) )
      {
        talent_data_t* td = talent_data_t::find( type, j, i, specialization(), dbc.ptr );
        if ( td && td->replace_id() )
        {
          dbc.replace_id( td->replace_id(), td->spell_id() );
          break;
        }
      }
    }
  }

  // Search general spells for spells to replace (a spell you learn earlier might be replaced by one you learn later)
  if ( _spec != SPEC_NONE )
  {
    for ( unsigned int i = 0; i < dbc.class_ability_size(); i++ )
    {
      unsigned id = dbc.class_ability( class_idx, 0, i );
      if ( id == 0 )
      {
        break;
      }
      const spell_data_t* s = dbc.spell( id );
      if ( s->replace_spell_id() && ( (int)s->level() <= true_level ) )
      {
        // Found a spell we should replace
        dbc.replace_id( s->replace_spell_id(), id );
      }
    }
  }
}

/**
 * Retrieves the Spell Data Associated with a given talent.
 *
 * If the player does not have have the talent activated, or the talent is not found,
 * spell_data_t::not_found() is returned.
 * The talent search by name is case sensitive, including all special characters!
 */
const spell_data_t* player_t::find_talent_spell( const std::string& n, specialization_e s, bool name_tokenized,
                                                 bool check_validity ) const
{
  if ( s == SPEC_NONE )
  {
    s = specialization();
  }

  // Get a talent's spell id for a given talent name
  unsigned spell_id = dbc.talent_ability_id( type, s, n.c_str(), name_tokenized );

  if ( !spell_id )
  {
    sim->print_debug( "Player {}: Can't find talent with name '{}'.\n", name(), n );
    return spell_data_t::nil();
  }

  for ( int j = 0; j < MAX_TALENT_ROWS; j++ )
  {
    for ( int i = 0; i < MAX_TALENT_COLS; i++ )
    {
      auto td = talent_data_t::find( type, j, i, s, dbc.ptr );
      if ( !td )
        continue;
      auto spell = dbc::find_spell( this, td->spell_id() );

      // Loop through all our classes talents, and check if their spell's id match the one we maped to the given talent
      // name
      if ( td && ( td->spell_id() == spell_id ) )
      {
        // check if we have the talent enabled or not
        // std::min( 100, x ) dirty fix so that we can access tier 7 talents at level 100 and not level 105
        if ( check_validity &&
             ( !talent_points.validate( spell, j, i ) || true_level < std::min( ( j + 1 ) * 15, 100 ) ) )
          return spell_data_t::not_found();

        return spell;
      }
    }
  }

  /* Talent not enabled */
  return spell_data_t::not_found();
}

const spell_data_t* player_t::find_specialization_spell( const std::string& name, specialization_e s ) const
{
  if ( s == SPEC_NONE || s == _spec )
  {
    if ( unsigned spell_id = dbc.specialization_ability_id( _spec, name.c_str() ) )
    {
      auto spell = dbc::find_spell( this, spell_id );

      if ( ( as<int>( spell->level() ) <= true_level ) )
      {
        return spell;
      }
    }
  }

  return spell_data_t::not_found();
}

const spell_data_t* player_t::find_specialization_spell( unsigned spell_id, specialization_e s ) const
{
  if ( s == SPEC_NONE || s == _spec )
  {
    auto spell = dbc::find_spell( this, spell_id );
    if ( dbc.is_specialization_ability( _spec, spell_id ) && ( as<int>( spell->level() ) <= true_level ) )
    {
      return spell;
    }
  }

  return spell_data_t::not_found();
}

const spell_data_t* player_t::find_mastery_spell( specialization_e s, uint32_t idx ) const
{
  if ( s != SPEC_NONE && s == _spec )
  {
    if ( unsigned spell_id = dbc.mastery_ability_id( s, idx ) )
    {
      const spell_data_t* spell = dbc::find_spell( this, spell_id );
      if ( as<int>( spell->level() ) <= true_level )
      {
        return spell;
      }
    }
  }

  return spell_data_t::not_found();
}

azerite_power_t player_t::find_azerite_spell( unsigned id ) const
{
  if ( ! azerite )
  {
    return {};
  }

  return azerite -> get_power( id );
}

azerite_power_t player_t::find_azerite_spell( const std::string& name, bool tokenized ) const
{
  if ( ! azerite )
  {
    return {};
  }

  // Note, no const propagation here, so this works
  return azerite -> get_power( name, tokenized );
}

/**
 * Tries to find spell data by name.
 *
 * It does this by going through various spell lists in following order:
 * class spell, specialization spell, mastery spell, talent spell, racial spell, pet_spell
 */
const spell_data_t* player_t::find_spell( const std::string& name, specialization_e s ) const
{
  const spell_data_t* sp = find_class_spell( name, s );
  assert( sp );
  if ( sp->ok() )
    return sp;

  sp = find_specialization_spell( name );
  assert( sp );
  if ( sp->ok() )
    return sp;

  if ( s != SPEC_NONE )
  {
    sp = find_mastery_spell( s, 0 );
    assert( sp );
    if ( sp->ok() )
      return sp;
  }

  sp = find_talent_spell( name );
  assert( sp );
  if ( sp->ok() )
    return sp;

  sp = find_racial_spell( name );
  assert( sp );
  if ( sp->ok() )
    return sp;

  sp = find_pet_spell( name );
  assert( sp );
  if ( sp->ok() )
    return sp;

  return spell_data_t::not_found();
}

const spell_data_t* player_t::find_racial_spell( const std::string& name, race_e r ) const
{
  if ( unsigned spell_id = dbc.race_ability_id( type, ( r != RACE_NONE ) ? r : race, name.c_str() ) )
  {
    const spell_data_t* s = dbc.spell( spell_id );
    if ( s->id() == spell_id )
    {
      return dbc::find_spell( this, s );
    }
  }

  return spell_data_t::not_found();
}

const spell_data_t* player_t::find_class_spell( const std::string& name, specialization_e s ) const
{
  if ( s == SPEC_NONE || s == _spec )
  {
    if ( unsigned spell_id = dbc.class_ability_id( type, _spec, name.c_str() ) )
    {
      const spell_data_t* spell = dbc.spell( spell_id );
      if ( spell->id() == spell_id && (int)spell->level() <= true_level )
      {
        return dbc::find_spell( this, spell );
      }
    }
  }

  return spell_data_t::not_found();
}

const spell_data_t* player_t::find_pet_spell( const std::string& name ) const
{
  if ( unsigned spell_id = dbc.pet_ability_id( type, name.c_str() ) )
  {
    const spell_data_t* s = dbc.spell( spell_id );
    if ( s->id() == spell_id )
    {
      return dbc::find_spell( this, s );
    }
  }

  return spell_data_t::not_found();
}

const spell_data_t* player_t::find_spell( unsigned int id ) const
{
  if ( id )
  {
    auto spell = dbc::find_spell( this, id );
    if ( spell->id() && as<int>( spell->level() ) <= true_level )
    {
      return spell;
    }
  }

  return spell_data_t::not_found();
}

const spell_data_t* player_t::find_spell( unsigned int id, specialization_e s ) const
{
  if ( s == SPEC_NONE || s == _spec )
  {
    return find_spell( id );
  }

  return spell_data_t::not_found();
}

namespace
{
expr_t* deprecate_expression( player_t& p, const std::string& old_name, const std::string& new_name, action_t* a = nullptr  )
{
  p.sim->errorf( "Use of \"%s\" ( action %s ) in action expressions is deprecated: use \"%s\" instead.\n",
                  old_name.c_str(), a?a->name() : "unknown", new_name.c_str() );

  return p.create_expression( new_name );
}

struct player_expr_t : public expr_t
{
  player_t& player;

  player_expr_t( const std::string& n, player_t& p ) : expr_t( n ), player( p )
  {
  }
};

struct position_expr_t : public player_expr_t
{
  int mask;
  position_expr_t( const std::string& n, player_t& p, int m ) : player_expr_t( n, p ), mask( m )
  {
  }
  virtual double evaluate() override
  {
    return ( 1 << player.position() ) & mask;
  }
};

expr_t* deprecated_player_expressions( player_t& player, const std::string& expression_str )
{
  if ( expression_str == "health_pct" )
    return deprecate_expression( player, expression_str, "health.pct" );

  if ( expression_str == "mana_pct" )
    return deprecate_expression( player, expression_str, "mana.pct" );

  if ( expression_str == "energy_regen" )
    return deprecate_expression( player, expression_str, "energy.regen" );

  if ( expression_str == "focus_regen" )
    return deprecate_expression( player, expression_str, "focus.regen" );

  if ( expression_str == "time_to_max_energy" )
    return deprecate_expression( player, expression_str, "energy.time_to_max" );

  if ( expression_str == "time_to_max_focus" )
    return deprecate_expression( player, expression_str, "focus.time_to_max" );

  if ( expression_str == "max_mana_nonproc" )
    return deprecate_expression( player, expression_str, "mana.max_nonproc" );

  return nullptr;
}

}  // namespace

/**
 * Player specific action expressions
 *
 * Use this function for expressions which are bound to some action property (eg. target, cast_time, etc.) and not just
 * to the player itself.
 */
expr_t* player_t::create_action_expression( action_t&, const std::string& name )
{
  return create_expression( name );
}

expr_t* player_t::create_expression( const std::string& expression_str )
{
  if (expr_t* e = deprecated_player_expressions(*this, expression_str))
  {
    return e;
  }

  if ( expression_str == "level" )
    return expr_t::create_constant( "level", true_level );

  if ( expression_str == "name" )
    return expr_t::create_constant( "name", actor_index );

  if ( expression_str == "self" )
    return expr_t::create_constant( "self", actor_index );

  if ( expression_str == "target" )
    return make_fn_expr( expression_str, [ this ] { return target->actor_index; } );

  if ( expression_str == "in_combat" )
    return make_ref_expr( "in_combat", in_combat );

  if ( expression_str == "ptr" )
    return expr_t::create_constant( "ptr", dbc.ptr );

  if ( expression_str == "bugs" )
    return expr_t::create_constant( "bugs", bugs );

  if ( expression_str == "is_add" )
    return expr_t::create_constant("is_add", is_add() );

  if ( expression_str == "is_enemy" )
    return expr_t::create_constant("is_enemy", is_enemy() );

  if ( expression_str == "attack_haste" )
    return make_fn_expr( expression_str, [this] { return cache.attack_haste(); } );

  if ( expression_str == "attack_speed" )
    return make_fn_expr( expression_str, [this] { return cache.attack_speed(); } );

  if ( expression_str == "spell_haste" )
    return make_fn_expr( expression_str, [this] { return cache.spell_haste(); } );

  if ( expression_str == "spell_speed" )
    return make_fn_expr( expression_str, [this] { return cache.spell_speed(); } );

  if ( expression_str == "mastery_value" )
    return make_mem_fn_expr( expression_str, this->cache, &player_stat_cache_t::mastery_value );

  if ( expression_str == "attack_crit" )
    return make_fn_expr( expression_str, [this] { return cache.attack_crit_chance(); } );

  if ( expression_str == "spell_crit" )
    return make_fn_expr( expression_str, [this] { return cache.spell_crit_chance(); } );

  if ( expression_str == "position_front" )
    return new position_expr_t( "position_front", *this, ( 1 << POSITION_FRONT ) | ( 1 << POSITION_RANGED_FRONT ) );
  if ( expression_str == "position_back" )
    return new position_expr_t( "position_back", *this, ( 1 << POSITION_BACK ) | ( 1 << POSITION_RANGED_BACK ) );

  if ( expression_str == "time_to_bloodlust" )
    return make_fn_expr( expression_str, [this] { return calculate_time_to_bloodlust(); } );

  // Get the actor's raw initial haste percent
  if ( expression_str == "raw_haste_pct" )
  {
    return make_fn_expr( expression_str, [this]() {
      return std::max( 0.0, initial.stats.haste_rating ) / initial.rating.spell_haste;
    } );
  }

  // Resource expressions
  if ( expr_t* q = create_resource_expression( expression_str ) )
    return q;

  // time_to_pct expressions
  if ( util::str_begins_with_ci( expression_str, "time_to_" ) )
  {
    std::vector<std::string> parts = util::string_split( expression_str, "_" );
    double percent = -1.0;

    if ( util::str_in_str_ci( parts[ 2 ], "die" ) )
      percent = 0.0;
    else if ( util::str_in_str_ci( parts[ 2 ], "pct" ) )
    {
      if (parts.size() == 4 )
      {
        // eg. time_to_pct_90.1
        percent = std::stod( parts[ 3 ] );
      }
      else
      {
        throw std::invalid_argument(fmt::format("No pct value given for time_to_pct_ expression."));
      }
    }
    else
    {
      throw std::invalid_argument(fmt::format("Unsupported time_to_ expression '{}'.", parts[ 2 ]));
    }

    return make_fn_expr( expression_str, [this, percent] { return time_to_percent( percent ).total_seconds(); } );
  }

  // incoming_damage_X expressions
  if ( util::str_in_str_ci( expression_str, "incoming_damage_" ) || util::str_in_str_ci( expression_str, "incoming_magic_damage_" ))
  {
    bool magic_damage = util::str_in_str_ci( expression_str, "incoming_magic_damage_" );
    std::vector<std::string> parts = util::string_split( expression_str, "_" );
    timespan_t window_duration;

    if ( util::str_in_str_ci( parts.back(), "ms" ) )
      window_duration = timespan_t::from_millis( std::stoi( parts.back() ) );
    else
      window_duration = timespan_t::from_seconds( std::stod( parts.back() ) );

    // skip construction if the duration is nonsensical
    if ( window_duration > timespan_t::zero() )
    {
      if (magic_damage)
      {
        return make_fn_expr(expression_str, [this, window_duration] {return compute_incoming_magic_damage( window_duration );});
      }
      else
      {
        return make_fn_expr(expression_str, [this, window_duration] {return compute_incoming_damage( window_duration );});
      }
    }
    else
    {
      throw std::invalid_argument(fmt::format("Non-positive window duration '{}'.", window_duration));
    }
  }

  // everything from here on requires splits
  std::vector<std::string> splits = util::string_split( expression_str, "." );

  if ( splits.size() == 2 )
  {
    // player variables
    if ( splits[ 0 ] == "variable"  )
    {
      struct variable_expr_t : public expr_t
      {
        const action_variable_t* var_;

        variable_expr_t( player_t* p, const std::string& name ) : expr_t( "variable" ),
          var_( nullptr )
        {
          auto it = range::find_if( p->variables, [&name]( const action_variable_t* var ) {
            return util::str_compare_ci( name, var->name_ );
          } );

          if ( it == p->variables.end() )
          {
            throw std::invalid_argument( fmt::format( "Player {} no variable named '{}' found",
                  p->name(), name ) );
          }
          else
          {
            var_ = *it;
          }
        }

        bool is_constant( double* value ) override
        { return var_->is_constant( value ); }

        double evaluate() override
        { return var_->current_value_; }
      };

      return new variable_expr_t( this, splits[ 1 ] );
    }

    // item equipped by item_id or name
    if ( splits[ 0 ] == "equipped" )
    {
      unsigned item_id = util::to_unsigned( splits[ 1 ] );
      for ( size_t i = 0; i < items.size(); ++i )
      {
        if ( item_id > 0 && items[ i ].parsed.data.id == item_id )
        {
          return expr_t::create_constant( "item_equipped", 1 );
        }
        else if ( util::str_compare_ci( items[ i ].name_str, splits[ 1 ] ) )
        {
          return expr_t::create_constant( "item_equipped", 1 );
        }
      }

      return expr_t::create_constant( "item_equipped", 0 );
    }

    // check weapon type
    if ( ( splits[ 0 ] == "main_hand" || splits[ 0 ] == "off_hand" ) )
    {
      double weapon_status = -1;
      if ( splits[ 0 ] == "main_hand" && util::str_compare_ci( splits[ 1 ], "2h" ) )
      {
        weapon_status = static_cast<double>( main_hand_weapon.group() == WEAPON_2H );
      }
      else if ( splits[ 0 ] == "main_hand" && util::str_compare_ci( splits[ 1 ], "1h" ) )
      {
        weapon_status =
            static_cast<double>( main_hand_weapon.group() == WEAPON_1H || main_hand_weapon.group() == WEAPON_SMALL );
      }
      else if ( splits[ 0 ] == "off_hand" && util::str_compare_ci( splits[ 1 ], "2h" ) )
      {
        weapon_status = static_cast<double>( off_hand_weapon.group() == WEAPON_2H );
      }
      else if ( splits[ 0 ] == "off_hand" && util::str_compare_ci( splits[ 1 ], "1h" ) )
      {
        weapon_status =
            static_cast<double>( off_hand_weapon.group() == WEAPON_1H || off_hand_weapon.group() == WEAPON_SMALL );
      }

      if ( weapon_status > -1 )
      {
        return expr_t::create_constant( "weapon_type_expr", weapon_status );
      }
    }

    // race
    if ( splits[ 0 ] == "race" )
    {
      return expr_t::create_constant(expression_str, race_str == splits[ 1 ]);
    }

    // spec
    if ( splits[ 0 ] == "spec" )
    {

      return expr_t::create_constant( "spec", dbc::translate_spec_str( type, splits[1]) == specialization() );
    }

    // role
    if ( splits[ 0 ] == "role" )
    {
      return expr_t::create_constant(expression_str, util::str_compare_ci( util::role_type_string( primary_role() ), splits[ 1 ] ));
    }

    // stat
    if (splits[ 0 ] == "stat"  )
    {
      if ( util::str_compare_ci( "spell_haste", splits[ 1 ] ) )
        return make_fn_expr(expression_str, [this] {return 1.0 / cache.spell_haste() - 1.0;});

      stat_e stat = util::parse_stat_type( splits[ 1 ] );
      switch ( stat )
      {
        case STAT_STRENGTH:
        case STAT_AGILITY:
        case STAT_STAMINA:
        case STAT_INTELLECT:
        case STAT_SPIRIT:
        {
          return make_fn_expr(expression_str, [this, stat] {return cache.get_attribute( static_cast<attribute_e>( stat ) );});
        }

        case STAT_SPELL_POWER:
        {
          return make_fn_expr(expression_str, [this] {return cache.spell_power( SCHOOL_MAX ) * composite_spell_power_multiplier();});
        }

        case STAT_ATTACK_POWER:
        {
          return make_fn_expr(expression_str, [this] {return cache.attack_power() * composite_attack_power_multiplier();});
        }

        case STAT_EXPERTISE_RATING:
          return make_mem_fn_expr( expression_str, *this, &player_t::composite_expertise_rating );
        case STAT_HIT_RATING:
          return make_mem_fn_expr( expression_str, *this, &player_t::composite_melee_hit_rating );
        case STAT_CRIT_RATING:
          return make_mem_fn_expr( expression_str, *this, &player_t::composite_melee_crit_rating );
        case STAT_HASTE_RATING:
          return make_mem_fn_expr( expression_str, *this, &player_t::composite_melee_haste_rating );
        case STAT_ARMOR:
          return make_ref_expr( expression_str, current.stats.armor );
        case STAT_BONUS_ARMOR:
          return make_ref_expr( expression_str, current.stats.bonus_armor );
        case STAT_DODGE_RATING:
          return make_mem_fn_expr( expression_str, *this, &player_t::composite_dodge_rating );
        case STAT_PARRY_RATING:
          return make_mem_fn_expr( expression_str, *this, &player_t::composite_parry_rating );
        case STAT_BLOCK_RATING:
          return make_mem_fn_expr( expression_str, *this, &player_t::composite_block_rating );
        case STAT_MASTERY_RATING:
          return make_mem_fn_expr( expression_str, *this, &player_t::composite_mastery_rating );
        case STAT_VERSATILITY_RATING:
          return make_mem_fn_expr( expression_str, *this, &player_t::composite_damage_versatility_rating );
        default:
          break;
      }

      throw std::invalid_argument(fmt::format("Cannot build expression from '{}' because stat type '{}' could not be parsed.",
          expression_str, splits[ 1 ]));
    }

    if ( splits[ 0 ] == "using_apl" )
    {
      return expr_t::create_constant( expression_str, util::str_compare_ci( splits[ 1 ], use_apl ) );
    }

    if ( splits[ 0 ] == "set_bonus" )
      return sets->create_expression( this, splits[ 1 ] );

    if ( splits[ 0 ] == "active_dot" )
    {
      int internal_id = find_action_id( splits[ 1 ] );
      if ( internal_id > -1 )
      {
        return make_fn_expr(expression_str, [this, internal_id] {return get_active_dots( internal_id );});
      }
      throw std::invalid_argument(fmt::format("Cannot find action '{}'.", splits[ 1 ]));
    }

    if ( splits[ 0 ] == "movement" )
    {
      if ( splits[ 1 ] == "remains" )
      {
        return make_fn_expr(expression_str, [this] {
          if ( current.distance_to_move > 0 )
            return ( current.distance_to_move / composite_movement_speed() );
          else
            return buffs.movement->remains().total_seconds();
        });
      }
      else if ( splits[ 1 ] == "distance" )
      {
        return make_fn_expr(expression_str, [this] {return current.distance_to_move;});
      }
      else if ( splits[ 1 ] == "speed" )
        return make_mem_fn_expr( splits[ 1 ], *this, &player_t::composite_movement_speed );

      throw std::invalid_argument(fmt::format("Unsupported movement expression '{}'.", splits[ 1 ]));
    }
  } // splits.size() == 2


  if ( splits.size() == 3 )
  {
    if ( splits[ 0 ] == "buff" || splits[ 0 ] == "debuff" )
    {
      // buff.buff_name.buff_property
      get_target_data( this );
      buff_t* buff = buff_t::find_expressable( buff_list, splits[ 1 ], this );
      if ( !buff )
        buff = buff_t::find( this, splits[ 1 ], this );  // Raid debuffs
      if ( buff )
        return buff_t::create_expression( splits[ 1 ], splits[ 2 ], *buff );
      throw std::invalid_argument(fmt::format("Cannot find buff '{}'.", splits[ 1 ]));
    }
    else if ( splits[ 0 ] == "cooldown" )
    {
      if ( cooldown_t* cooldown = get_cooldown( splits[ 1 ] ) )
      {
        return cooldown->create_expression( splits[ 2 ] );
      }
      throw std::invalid_argument(fmt::format("Cannot find any cooldown with name '{}'.", splits[ 1 ]));
    }
    else if ( splits[ 0 ] == "swing" )
    {
      std::string& s = splits[ 1 ];
      slot_e hand    = SLOT_INVALID;
      if ( s == "mh" || s == "mainhand" || s == "main_hand" )
        hand = SLOT_MAIN_HAND;
      if ( s == "oh" || s == "offhand" || s == "off_hand" )
        hand = SLOT_OFF_HAND;
      if ( hand == SLOT_INVALID )
        throw std::invalid_argument(fmt::format("Invalid slot '{}' for swing expression.", splits[ 1 ]));

      if ( splits[ 2 ] == "remains" )
      {
        struct swing_remains_expr_t : public player_expr_t
        {
          slot_e slot;
          swing_remains_expr_t( player_t& p, slot_e s ) : player_expr_t( "swing_remains", p ), slot( s )
          {
          }
          virtual double evaluate() override
          {
            attack_t* attack = ( slot == SLOT_MAIN_HAND ) ? player.main_hand_attack : player.off_hand_attack;
            if ( attack && attack->execute_event )
              return attack->execute_event->remains().total_seconds();
            return 9999;
          }
        };
        return new swing_remains_expr_t( *this, hand );
      }
    }

    if ( splits[ 0 ] == "spell" && splits[ 2 ] == "exists" )
    {
      return expr_t::create_constant(expression_str, find_spell( splits[ 1 ] )->ok());
    }

    if (splits[ 0 ] == "talent" )
    {
      if ( splits[ 2 ] == "enabled" )
      {
        const spell_data_t* s = find_talent_spell( splits[ 1 ], specialization(), true );
        if ( s == spell_data_t::nil() )
        {
          throw std::invalid_argument(fmt::format("Cannot find talent '{}'.", splits[ 1 ]));
        }

        return expr_t::create_constant( expression_str, s->ok() );
      }
      throw std::invalid_argument(fmt::format("Unsupported talent expression '{}'.", splits[ 2 ]));
    }

    if ( splits[ 0 ] == "artifact" )
    {
      artifact_power_t power = find_artifact_spell( splits[ 1 ], true );

      if ( splits[ 2 ] == "enabled" )
      {
        return expr_t::create_constant( expression_str, power.rank() > 0 );
      }
      else if ( splits[ 2 ] == "rank" )
      {
        return expr_t::create_constant( expression_str, power.rank() );
      }
      throw std::invalid_argument(fmt::format("Unsupported artifact expression '{}'.", splits[ 2 ]));
    }
  } // splits.size() == 3


  // *** Variable-Length expressions from here on ***

  // trinkets
  if ( !splits.empty() && splits[ 0 ] == "trinket" )
  {
    if ( expr_t* expr = unique_gear::create_expression( *this, expression_str ) )
      return expr;
  }

  if ( splits[ 0 ] == "legendary_ring" )
  {
    if ( expr_t* expr = unique_gear::create_expression( *this, expression_str ) )
      return expr;
  }

  // pet
  if ( splits.size() >= 2 && splits[ 0 ] == "pet" )
  {
    pet_t* pet = find_pet( splits[ 1 ] );
    spawner::base_actor_spawner_t* spawner = nullptr;

    if ( ! pet )
    {
      spawner = find_spawner( splits[ 1 ] );
    }

    if ( ! pet && ! spawner )
    {
      throw std::invalid_argument(fmt::format("Cannot find pet or pet spawner '{}'.", splits[ 1 ]));
    }

    if ( pet )
    {
      if ( splits.size() == 2 )
      {
        return expr_t::create_constant( "pet_index_expr", static_cast<double>( pet->actor_index ) );
      }
      // pet.foo.blah
      else
      {
        if ( splits[ 2 ] == "active" )
        {
          return make_fn_expr(expression_str, [pet] {return !pet->is_sleeping();});
        }
        else if ( splits[ 2 ] == "remains" )
        {
          return make_fn_expr(expression_str, [pet] {
            if ( pet->expiration && pet->expiration->remains() > timespan_t::zero() )
            {
              return pet->expiration->remains().total_seconds();
            }
            else
            {
              return 0.0;
            };});
        }

        // build player/pet expression from the tail of the expression string.
        std::string tail = expression_str.substr( splits[ 1 ].length() + 5 );
        if ( expr_t* e = pet->create_expression( tail ) )
        {
          return e;
        }

        throw std::invalid_argument(fmt::format("Unsupported pet expression '{}'.", tail));
      }
    }
    // No pet found, but a pet spawner was found. Make a pet-spawner based expression out of the
    // rest of the info
    else
    {
      auto rest = arv::make_view( splits );
      if ( auto expr = spawner -> create_expression( rest.slice( 2, rest.size() - 2 ) ) )
      {
        return expr;
      }
    }
  }

  // owner
  if ( splits.size() > 1 && splits[ 0 ] == "owner" )
  {
    if ( pet_t* pet = dynamic_cast<pet_t*>( this ) )
    {
      if ( pet->owner )
      {
        std::string tail = expression_str.substr( 6 );
        if ( expr_t* e = pet->owner->create_expression( tail ) )
        {
          return e;
        }
        throw std::invalid_argument(fmt::format("Unsupported owner expression '{}'.", tail));
      }
      throw std::invalid_argument(fmt::format("Pet has no owner."));
    }
    else
    {
      throw std::invalid_argument(fmt::format("Cannot use expression '{}' because player is not a pet.",
          expression_str));
    }
  }

  if ( splits[ 0 ] == "azerite" )
  {
    return azerite -> create_expression( splits );
  }

  return sim->create_expression( expression_str );
}

expr_t* player_t::create_resource_expression( const std::string& name_str )
{
  auto splits = util::string_split( name_str, "." );
  if ( splits.empty() )
    return nullptr;

  resource_e r = util::parse_resource_type( splits[ 0 ] );
  if ( r == RESOURCE_NONE )
    return nullptr;

  if ( splits.size() == 1 )
    return make_ref_expr( name_str, resources.current[ r ] );

  if ( splits.size() == 2 )
  {
    if ( splits[ 1 ] == "deficit" )
    {
      return make_fn_expr( name_str, [this, r] { return resources.max[ r ] - resources.current[ r ]; } );
    }

    else if ( splits[ 1 ] == "pct" || splits[ 1 ] == "percent" )
    {
      if ( r == RESOURCE_HEALTH )
      {
        return make_mem_fn_expr( name_str, *this, &player_t::health_percentage );
      }
      else
      {
        return make_fn_expr( name_str, [this, r] { return resources.pct( r ) * 100.0; } );
      }
    }

    else if ( splits[ 1 ] == "max" )
      return make_ref_expr( name_str, resources.max[ r ] );

    else if ( splits[ 1 ] == "max_nonproc" )
      return make_ref_expr( name_str, collected_data.buffed_stats_snapshot.resource[ r ] );

    else if ( splits[ 1 ] == "pct_nonproc" )
    {
      return make_fn_expr( name_str, [this, r] {
        return resources.current[ r ] / collected_data.buffed_stats_snapshot.resource[ r ] * 100.0;
      } );
    }
    else if ( splits[ 1 ] == "net_regen" )
    {
      return make_fn_expr( name_str, [this, r] {
        timespan_t now = sim->current_time();
        if ( now != timespan_t::zero() )
          return ( iteration_resource_gained[ r ] - iteration_resource_lost[ r ] ) / now.total_seconds();
        else
          return 0.0;
      } );
    }
    else if ( splits[ 1 ] == "regen" )
    {
      return make_fn_expr( name_str, [this, r] { return resources.base_regen_per_second[ r ]; } );
    }

    else if ( splits[ 1 ] == "time_to_max" )
    {
      return make_fn_expr( "time_to_max_resource", [this, r] {
        return ( resources.max[ r ] - resources.current[ r ] ) / resources.base_regen_per_second[ r ];
      } );
    }
  }

  std::string tail = name_str.substr(splits[ 0 ].length() + 1);
  throw std::invalid_argument(fmt::format("Unsupported resource expression '{}'.", tail));
}

double player_t::compute_incoming_damage( timespan_t interval ) const
{
  double amount = 0;

  if ( incoming_damage.size() > 0 )
  {
    for ( auto i = incoming_damage.rbegin(), end = incoming_damage.rend(); i != end; ++i )
    {
      if ( sim->current_time() - ( *i ).time > interval )
        break;

      amount += ( *i ).amount;
    }
  }

  return amount;
}

double player_t::compute_incoming_magic_damage( timespan_t interval ) const
{
  double amount = 0;

  if ( incoming_damage.size() > 0 )
  {
    for ( auto i = incoming_damage.rbegin(), end = incoming_damage.rend(); i != end; ++i )
    {
      if ( sim->current_time() - ( *i ).time > interval )
        break;

      if ( (*i).school == SCHOOL_PHYSICAL )
        continue;

      amount += ( *i ).amount;
    }
  }

  return amount;
}

double player_t::calculate_time_to_bloodlust() const
{
  // only bother if the sim is automatically casting bloodlust. Otherwise error out
  if ( sim->overrides.bloodlust )
  {
    timespan_t time_to_bl  = timespan_t::from_seconds( -1 );
    timespan_t bl_pct_time = timespan_t::from_seconds( -1 );

    // first, check bloodlust_time.  If it's >0, we just compare to current_time()
    // if it's <0, then use time_to_die estimate
    if ( sim->bloodlust_time > timespan_t::zero() )
      time_to_bl = sim->bloodlust_time - sim->current_time();
    else if ( sim->bloodlust_time < timespan_t::zero() )
      time_to_bl = target->time_to_percent( 0.0 ) + sim->bloodlust_time;

    // check bloodlust_percent, if >0 then we need to estimate time based on time_to_die and health_percentage
    if ( sim->bloodlust_percent > 0 && target->health_percentage() > 0 )
      bl_pct_time = ( target->health_percentage() - sim->bloodlust_percent ) * target->time_to_percent( 0.0 ) /
                    target->health_percentage();

    // now that we have both times, we want to check for the Exhaustion buff.  If either time is shorter than
    // the remaining duration on Exhaustion, we won't get that bloodlust and should ignore it
    if ( buffs.exhaustion->check() )
    {
      if ( time_to_bl < buffs.exhaustion->remains() )
        time_to_bl = timespan_t::from_seconds( -1 );
      if ( bl_pct_time < buffs.exhaustion->remains() )
        bl_pct_time = timespan_t::from_seconds( -1 );
    }
    else
    {
      // the sim's bloodlust_check event fires every second, so negative times under 1 second should be treated as zero
      // for safety. without this, time_to_bloodlust can spike to the next target value up to a second before bloodlust
      // is actually cast. probably a non-issue since the worst case is likely to be casting something 1 second too
      // early, but we may as well account for it
      if ( time_to_bl < timespan_t::zero() && -time_to_bl < timespan_t::from_seconds( 1.0 ) )
        time_to_bl = timespan_t::zero();
      if ( bl_pct_time < timespan_t::zero() && -bl_pct_time < timespan_t::from_seconds( 1.0 ) )
        bl_pct_time = timespan_t::zero();
    }

    // if both times are non-negative, take the shortest one since that will happen first
    if ( bl_pct_time >= timespan_t::zero() && time_to_bl >= timespan_t::zero() )
      time_to_bl = std::min( bl_pct_time, time_to_bl );
    // otherwise, at least one is negative, so take the larger of the two
    else
      time_to_bl = std::max( bl_pct_time, time_to_bl );

    // if both are negative, time_to_bl will still be negative and we won't be benefitting from another BL cast.
    // Otherwise we return the positive time until BL is being cast.
    if ( time_to_bl >= timespan_t::zero() )
      return time_to_bl.total_seconds();
  }
  else if ( sim->debug )
    sim->errorf( "Trying to call time_to_bloodlust conditional with overrides.bloodlust=0" );

  // Return a nonsensical time that's much longer than the simulation.  This happens if time_to_bl was negative
  // or if overrides.bloodlust was 0
  return 3 * sim->expected_iteration_time.total_seconds();
}

void player_t::recreate_talent_str( talent_format_e format )
{
  switch ( format )
  {
    case TALENT_FORMAT_UNCHANGED:
      break;
    case TALENT_FORMAT_ARMORY:
      create_talents_armory();
      break;
    case TALENT_FORMAT_WOWHEAD:
      create_talents_wowhead();
      break;
    default:
      create_talents_numbers();
      break;
  }
}

std::string player_t::create_profile( save_e stype )
{
  std::string profile_str;
  std::string term;

  term = "\n";

  if ( !report_information.comment_str.empty() )
  {
    profile_str += "# " + report_information.comment_str + term;
  }

  if ( stype & SAVE_PLAYER )
  {
    profile_str += util::player_type_string( type );
    profile_str += "=\"" + name_str + '"' + term;
    profile_str += "source=" + util::profile_source_string( profile_source_ ) + term;

    if ( !origin_str.empty() )
      profile_str += "origin=\"" + origin_str + '"' + term;
    if ( !report_information.thumbnail_url.empty() )
      profile_str += "thumbnail=\"" + report_information.thumbnail_url + '"' + term;

    profile_str += "spec=";
    profile_str += dbc::specialization_string( specialization() ) + term;
    profile_str += "level=" + util::to_string( true_level ) + term;
    profile_str += "race=" + race_str + term;
    if ( race == RACE_NIGHT_ELF )
    {
      profile_str += "timeofday=" + util::to_string( timeofday == player_t::NIGHT_TIME ? "night" : "day" ) + term;
    }
    if ( race == RACE_ZANDALARI_TROLL )
    {
      profile_str += "zandalari_loa=" + util::to_string(zandalari_loa == player_t::AKUNDA ? "akunda" : zandalari_loa == player_t::BWONSAMDI ? "bwonsamdi"
        : zandalari_loa == player_t::GONK ? "gonk" : zandalari_loa == player_t::KIMBUL ? "kimbul" : zandalari_loa == player_t::KRAGWA ? "kragwa" : "paku") + term;
    }
    profile_str += "role=";
    profile_str += util::role_type_string( primary_role() ) + term;
    profile_str += "position=" + position_str + term;

    if ( professions_str.size() > 0 )
    {
      profile_str += "professions=" + professions_str + term;
    }
  }

  if ( stype & SAVE_TALENTS )
  {
    if ( !talents_str.empty() )
    {
      recreate_talent_str( sim->talent_format );
      profile_str += "talents=" + talents_str + term;
    }

    if ( talent_overrides_str.size() > 0 )
    {
      std::vector<std::string> splits = util::string_split( talent_overrides_str, "/" );
      for ( size_t i = 0; i < splits.size(); i++ )
      {
        profile_str += "talent_override=" + splits[ i ] + term;
      }
    }

    if ( azerite )
    {
      std::string azerite_overrides = azerite -> overrides_str();
      if ( ! azerite_overrides.empty() )
      {
        profile_str += "azerite_override=" + azerite_overrides + term;
      }
    }
  }

  if ( stype & SAVE_PLAYER )
  {
    std::string potion_option = potion_str.empty() ? default_potion() : potion_str;
    std::string flask_option  = flask_str.empty() ? default_flask() : flask_str;
    std::string food_option   = food_str.empty() ? default_food() : food_str;
    std::string rune_option   = rune_str.empty() ? default_rune() : rune_str;

    if ( !potion_option.empty() || !flask_option.empty() || !food_option.empty() || !rune_option.empty() )
    {
      profile_str += term;
      profile_str += "# Default consumables" + term;

      if ( !potion_option.empty() )
        profile_str += "potion=" + potion_option + term;
      if ( !flask_option.empty() )
        profile_str += "flask=" + flask_option + term;
      if ( !food_option.empty() )
        profile_str += "food=" + food_option + term;
      if ( !rune_option.empty() )
        profile_str += "augmentation=" + rune_option + term;
    }

    std::vector<std::string> initial_resources;
    for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
    {
      if ( resources.initial_opt[ r ] >= 0.0 )
      {
        std::string resource_str = util::resource_type_string( r );
        std::string amount_str   = util::to_string( resources.initial_opt[ r ] );
        initial_resources.push_back( resource_str + "=" + amount_str );
      }
    }

    if ( initial_resources.size() > 0 )
    {
      profile_str += term;
      profile_str += "initial_resource=";
      for ( size_t i = 0; i < initial_resources.size(); ++i )
      {
        profile_str += initial_resources[ i ];
        if ( i < initial_resources.size() - 1 )
        {
          profile_str += '/';
        }
      }
      profile_str += term;
    }
  }

  if ( stype & SAVE_ACTIONS )
  {
    if ( !action_list_str.empty() || use_default_action_list )
    {
      // If we created a default action list, add comments
      if ( no_action_list_provided )
        profile_str += action_list_information;

      auto apls = sorted_action_priority_lists( this );
      for ( const auto apl : apls )
      {
        if ( !apl->action_list_comment_str.empty() )
        {
          profile_str += term + "# " + apl->action_list_comment_str;
        }
        profile_str += term;

        bool first = true;
        for ( const auto& action : apl->action_list )
        {
          if ( !action.comment_.empty() )
          {
            profile_str += "# " + action.comment_ + term;
          }

          profile_str += "actions";
          if ( !util::str_compare_ci( apl->name_str, "default" ) )
          {
            profile_str += "." + apl->name_str;
          }

          profile_str += first ? "=" : "+=/";
          profile_str += action.action_ + term;

          first = false;
        }
      }
    }
  }

  if ( ( stype & SAVE_GEAR ) && !items.empty() )
  {
    profile_str += "\n";
    const slot_e SLOT_OUT_ORDER[] = {
        SLOT_HEAD,      SLOT_NECK,      SLOT_SHOULDERS, SLOT_BACK,     SLOT_CHEST,  SLOT_SHIRT,    SLOT_TABARD,
        SLOT_WRISTS,    SLOT_HANDS,     SLOT_WAIST,     SLOT_LEGS,     SLOT_FEET,   SLOT_FINGER_1, SLOT_FINGER_2,
        SLOT_TRINKET_1, SLOT_TRINKET_2, SLOT_MAIN_HAND, SLOT_OFF_HAND, SLOT_RANGED,
    };

    for ( auto& slot : SLOT_OUT_ORDER )
    {
      item_t& item = items[ slot ];
      if ( item.active() )
      {
        profile_str += item.slot_name();
        profile_str += "=" + item.encoded_item() + term;
        if ( sim->save_gear_comments && !item.encoded_comment().empty() )
          profile_str += "# " + item.encoded_comment() + term;
      }
    }
    if ( !items_str.empty() )
    {
      profile_str += "items=" + items_str + term;
    }

    profile_str += "\n# Gear Summary" + term;
    double avg_ilvl = util::round( avg_item_level(), 2 );
    profile_str += "# gear_ilvl=" + util::to_string( avg_ilvl, 2 ) + term;
    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      if ( i == STAT_NONE || i == STAT_ALL )
        continue;

      if ( gear.get_stat( i ) < 0 && total_gear.get_stat( i ) > 0 )
      {
        profile_str += "# gear_";
        profile_str += util::stat_type_string( i );
        profile_str += "=" + util::to_string( total_gear.get_stat( i ), 0 ) + term;
      }
      else if ( gear.get_stat( i ) >= 0 )
      {
        profile_str += "# gear_";
        profile_str += util::stat_type_string( i );
        profile_str += "=" + util::to_string( gear.get_stat( i ), 0 ) + term;
      }
    }
    if ( meta_gem != META_GEM_NONE )
    {
      profile_str += "# meta_gem=";
      profile_str += util::meta_gem_type_string( meta_gem );
      profile_str += term;
    }

    // Set Bonus
    if ( sets != nullptr )
    {
      profile_str += sets->to_profile_string( term );
    }

    if ( enchant.attribute[ ATTR_STRENGTH ] != 0 )
      profile_str += "enchant_strength=" + util::to_string( enchant.attribute[ ATTR_STRENGTH ] ) + term;

    if ( enchant.attribute[ ATTR_AGILITY ] != 0 )
      profile_str += "enchant_agility=" + util::to_string( enchant.attribute[ ATTR_AGILITY ] ) + term;

    if ( enchant.attribute[ ATTR_STAMINA ] != 0 )
      profile_str += "enchant_stamina=" + util::to_string( enchant.attribute[ ATTR_STAMINA ] ) + term;

    if ( enchant.attribute[ ATTR_INTELLECT ] != 0 )
      profile_str += "enchant_intellect=" + util::to_string( enchant.attribute[ ATTR_INTELLECT ] ) + term;

    if ( enchant.attribute[ ATTR_SPIRIT ] != 0 )
      profile_str += "enchant_spirit=" + util::to_string( enchant.attribute[ ATTR_SPIRIT ] ) + term;

    if ( enchant.spell_power != 0 )
      profile_str += "enchant_spell_power=" + util::to_string( enchant.spell_power ) + term;

    if ( enchant.attack_power != 0 )
      profile_str += "enchant_attack_power=" + util::to_string( enchant.attack_power ) + term;

    if ( enchant.expertise_rating != 0 )
      profile_str += "enchant_expertise_rating=" + util::to_string( enchant.expertise_rating ) + term;

    if ( enchant.armor != 0 )
      profile_str += "enchant_armor=" + util::to_string( enchant.armor ) + term;

    if ( enchant.haste_rating != 0 )
      profile_str += "enchant_haste_rating=" + util::to_string( enchant.haste_rating ) + term;

    if ( enchant.hit_rating != 0 )
      profile_str += "enchant_hit_rating=" + util::to_string( enchant.hit_rating ) + term;

    if ( enchant.crit_rating != 0 )
      profile_str += "enchant_crit_rating=" + util::to_string( enchant.crit_rating ) + term;

    if ( enchant.mastery_rating != 0 )
      profile_str += "enchant_mastery_rating=" + util::to_string( enchant.mastery_rating ) + term;

    if ( enchant.versatility_rating != 0 )
      profile_str += "enchant_versatility_rating=" + util::to_string( enchant.versatility_rating ) + term;

    if ( enchant.resource[ RESOURCE_HEALTH ] != 0 )
      profile_str += "enchant_health=" + util::to_string( enchant.resource[ RESOURCE_HEALTH ] ) + term;

    if ( enchant.resource[ RESOURCE_MANA ] != 0 )
      profile_str += "enchant_mana=" + util::to_string( enchant.resource[ RESOURCE_MANA ] ) + term;

    if ( enchant.resource[ RESOURCE_RAGE ] != 0 )
      profile_str += "enchant_rage=" + util::to_string( enchant.resource[ RESOURCE_RAGE ] ) + term;

    if ( enchant.resource[ RESOURCE_ENERGY ] != 0 )
      profile_str += "enchant_energy=" + util::to_string( enchant.resource[ RESOURCE_ENERGY ] ) + term;

    if ( enchant.resource[ RESOURCE_FOCUS ] != 0 )
      profile_str += "enchant_focus=" + util::to_string( enchant.resource[ RESOURCE_FOCUS ] ) + term;

    if ( enchant.resource[ RESOURCE_RUNIC_POWER ] != 0 )
      profile_str += "enchant_runic=" + util::to_string( enchant.resource[ RESOURCE_RUNIC_POWER ] ) + term;
  }
  return profile_str;
}

void player_t::copy_from( player_t* source )
{
  origin_str      = source->origin_str;
  profile_source_ = source->profile_source_;
  true_level      = source->true_level;
  race_str        = source->race_str;
  timeofday       = source->timeofday;
  zandalari_loa   = source->zandalari_loa;
  race            = source->race;
  role            = source->role;
  _spec           = source->_spec;
  position_str    = source->position_str;
  professions_str = source->professions_str;
  source->recreate_talent_str( TALENT_FORMAT_UNCHANGED );
  parse_talent_url( sim, "talents", source->talents_str );

  if ( azerite )
  {
    azerite -> copy_overrides( source -> azerite );
  }

  talent_overrides_str = source->talent_overrides_str;
  action_list_str      = source->action_list_str;
  alist_map            = source->alist_map;
  use_apl              = source->use_apl;

  meta_gem = source->meta_gem;
  for ( size_t i = 0; i < items.size(); i++ )
  {
    items[ i ]        = source->items[ i ];
    items[ i ].player = this;
  }

  if ( sets != nullptr )
  {
    sets        = std::unique_ptr<set_bonus_t>( new set_bonus_t( *source->sets ) );
    sets->actor = this;
  }

  gear    = source->gear;
  enchant = source->enchant;
  bugs    = source->bugs;

  potion_str = source->potion_str;
  flask_str  = source->flask_str;
  food_str   = source->food_str;
  rune_str   = source->rune_str;
}

void player_t::create_options()
{
  options.reserve( 180 );
  add_option( opt_string( "name", name_str ) );
  add_option( opt_func( "origin", parse_origin ) );
  add_option( opt_func( "source", parse_source ) );
  add_option( opt_string( "region", region_str ) );
  add_option( opt_string( "server", server_str ) );
  add_option( opt_string( "thumbnail", report_information.thumbnail_url ) );
  add_option( opt_string( "id", id_str ) );
  add_option( opt_func( "talents", parse_talent_url ) );
  add_option( opt_func( "talent_override", parse_talent_override ) );
  add_option( opt_string( "race", race_str ) );
  add_option( opt_func( "timeofday", parse_timeofday ) );
  add_option( opt_func( "zandalari_loa", parse_loa ) );
  add_option( opt_int( "level", true_level, 1, MAX_LEVEL ) );
  add_option( opt_bool( "ready_trigger", ready_type ) );
  add_option( opt_func( "role", parse_role_string ) );
  add_option( opt_string( "target", target_str ) );
  add_option( opt_float( "skill", base.skill, 0, 1.0 ) );
  add_option( opt_float( "distance", base.distance, 5, std::numeric_limits<double>::max() ) );
  add_option( opt_string( "position", position_str ) );
  add_option( opt_string( "professions", professions_str ) );
  add_option( opt_string( "actions", action_list_str ) );
  add_option( opt_append( "actions+", action_list_str ) );
  add_option( opt_map( "actions.", alist_map ) );
  add_option( opt_string( "action_list", choose_action_list ) );
  add_option( opt_bool( "sleeping", initial.sleeping ) );
  add_option( opt_bool( "quiet", quiet ) );
  add_option( opt_string( "save", report_information.save_str ) );
  add_option( opt_string( "save_gear", report_information.save_gear_str ) );
  add_option( opt_string( "save_talents", report_information.save_talents_str ) );
  add_option( opt_string( "save_actions", report_information.save_actions_str ) );
  add_option( opt_string( "comment", report_information.comment_str ) );
  add_option( opt_bool( "bugs", bugs ) );
  add_option( opt_func( "world_lag", parse_world_lag ) );
  add_option( opt_func( "world_lag_stddev", parse_world_lag_stddev ) );
  add_option( opt_func( "brain_lag", parse_brain_lag ) );
  add_option( opt_func( "brain_lag_stddev", parse_brain_lag_stddev ) );
  add_option( opt_timespan( "cooldown_tolerance", cooldown_tolerance_ ) );
  add_option( opt_bool( "scale_player", scale_player ) );
  add_option( opt_string( "tmi_output", tmi_debug_file_str ) );
  add_option( opt_float( "tmi_window", tmi_window, 0, std::numeric_limits<double>::max() ) );
  add_option( opt_func( "spec", parse_specialization ) );
  add_option( opt_func( "specialization", parse_specialization ) );
  add_option( opt_func( "stat_timelines", parse_stat_timelines ) );
  add_option( opt_bool( "disable_hotfixes", disable_hotfixes ) );
  add_option( opt_func( "min_gcd", parse_min_gcd ) );

  // Cosumables
  add_option( opt_string( "potion", potion_str ) );
  add_option( opt_string( "flask", flask_str ) );
  add_option( opt_string( "food", food_str ) );
  add_option( opt_string( "augmentation", rune_str ) );

  // Positioning
  add_option( opt_float( "x_pos", default_x_position ) );
  add_option( opt_float( "y_pos", default_y_position ) );

  // Items
  add_option( opt_string( "meta_gem", meta_gem_str ) );
  add_option( opt_string( "items", items_str ) );
  add_option( opt_append( "items+", items_str ) );
  add_option( opt_string( "head", items[ SLOT_HEAD ].options_str ) );
  add_option( opt_string( "neck", items[ SLOT_NECK ].options_str ) );
  add_option( opt_string( "shoulders", items[ SLOT_SHOULDERS ].options_str ) );
  add_option( opt_string( "shoulder", items[ SLOT_SHOULDERS ].options_str ) );
  add_option( opt_string( "shirt", items[ SLOT_SHIRT ].options_str ) );
  add_option( opt_string( "chest", items[ SLOT_CHEST ].options_str ) );
  add_option( opt_string( "waist", items[ SLOT_WAIST ].options_str ) );
  add_option( opt_string( "legs", items[ SLOT_LEGS ].options_str ) );
  add_option( opt_string( "leg", items[ SLOT_LEGS ].options_str ) );
  add_option( opt_string( "feet", items[ SLOT_FEET ].options_str ) );
  add_option( opt_string( "foot", items[ SLOT_FEET ].options_str ) );
  add_option( opt_string( "wrists", items[ SLOT_WRISTS ].options_str ) );
  add_option( opt_string( "wrist", items[ SLOT_WRISTS ].options_str ) );
  add_option( opt_string( "hands", items[ SLOT_HANDS ].options_str ) );
  add_option( opt_string( "hand", items[ SLOT_HANDS ].options_str ) );
  add_option( opt_string( "finger1", items[ SLOT_FINGER_1 ].options_str ) );
  add_option( opt_string( "finger2", items[ SLOT_FINGER_2 ].options_str ) );
  add_option( opt_string( "ring1", items[ SLOT_FINGER_1 ].options_str ) );
  add_option( opt_string( "ring2", items[ SLOT_FINGER_2 ].options_str ) );
  add_option( opt_string( "trinket1", items[ SLOT_TRINKET_1 ].options_str ) );
  add_option( opt_string( "trinket2", items[ SLOT_TRINKET_2 ].options_str ) );
  add_option( opt_string( "back", items[ SLOT_BACK ].options_str ) );
  add_option( opt_string( "main_hand", items[ SLOT_MAIN_HAND ].options_str ) );
  add_option( opt_string( "off_hand", items[ SLOT_OFF_HAND ].options_str ) );
  add_option( opt_string( "tabard", items[ SLOT_TABARD ].options_str ) );

  // Set Bonus
  add_option( opt_func( "set_bonus", parse_set_bonus ) );

  // Gear Stats
  add_option( opt_float( "gear_strength", gear.attribute[ ATTR_STRENGTH ] ) );
  add_option( opt_float( "gear_agility", gear.attribute[ ATTR_AGILITY ] ) );
  add_option( opt_float( "gear_stamina", gear.attribute[ ATTR_STAMINA ] ) );
  add_option( opt_float( "gear_intellect", gear.attribute[ ATTR_INTELLECT ] ) );
  add_option( opt_float( "gear_spirit", gear.attribute[ ATTR_SPIRIT ] ) );
  add_option( opt_float( "gear_spell_power", gear.spell_power ) );
  add_option( opt_float( "gear_attack_power", gear.attack_power ) );
  add_option( opt_float( "gear_expertise_rating", gear.expertise_rating ) );
  add_option( opt_float( "gear_haste_rating", gear.haste_rating ) );
  add_option( opt_float( "gear_hit_rating", gear.hit_rating ) );
  add_option( opt_float( "gear_crit_rating", gear.crit_rating ) );
  add_option( opt_float( "gear_parry_rating", gear.parry_rating ) );
  add_option( opt_float( "gear_dodge_rating", gear.dodge_rating ) );
  add_option( opt_float( "gear_health", gear.resource[ RESOURCE_HEALTH ] ) );
  add_option( opt_float( "gear_mana", gear.resource[ RESOURCE_MANA ] ) );
  add_option( opt_float( "gear_rage", gear.resource[ RESOURCE_RAGE ] ) );
  add_option( opt_float( "gear_energy", gear.resource[ RESOURCE_ENERGY ] ) );
  add_option( opt_float( "gear_focus", gear.resource[ RESOURCE_FOCUS ] ) );
  add_option( opt_float( "gear_runic", gear.resource[ RESOURCE_RUNIC_POWER ] ) );
  add_option( opt_float( "gear_armor", gear.armor ) );
  add_option( opt_float( "gear_mastery_rating", gear.mastery_rating ) );
  add_option( opt_float( "gear_versatility_rating", gear.versatility_rating ) );
  add_option( opt_float( "gear_bonus_armor", gear.bonus_armor ) );
  add_option( opt_float( "gear_leech_rating", gear.leech_rating ) );
  add_option( opt_float( "gear_run_speed_rating", gear.speed_rating ) );

  // Stat Enchants
  add_option( opt_float( "enchant_strength", enchant.attribute[ ATTR_STRENGTH ] ) );
  add_option( opt_float( "enchant_agility", enchant.attribute[ ATTR_AGILITY ] ) );
  add_option( opt_float( "enchant_stamina", enchant.attribute[ ATTR_STAMINA ] ) );
  add_option( opt_float( "enchant_intellect", enchant.attribute[ ATTR_INTELLECT ] ) );
  add_option( opt_float( "enchant_spirit", enchant.attribute[ ATTR_SPIRIT ] ) );
  add_option( opt_float( "enchant_spell_power", enchant.spell_power ) );
  add_option( opt_float( "enchant_attack_power", enchant.attack_power ) );
  add_option( opt_float( "enchant_expertise_rating", enchant.expertise_rating ) );
  add_option( opt_float( "enchant_armor", enchant.armor ) );
  add_option( opt_float( "enchant_haste_rating", enchant.haste_rating ) );
  add_option( opt_float( "enchant_hit_rating", enchant.hit_rating ) );
  add_option( opt_float( "enchant_crit_rating", enchant.crit_rating ) );
  add_option( opt_float( "enchant_mastery_rating", enchant.mastery_rating ) );
  add_option( opt_float( "enchant_versatility_rating", enchant.versatility_rating ) );
  add_option( opt_float( "enchant_bonus_armor", enchant.bonus_armor ) );
  add_option( opt_float( "enchant_leech_rating", enchant.leech_rating ) );
  add_option( opt_float( "enchant_run_speed_rating", enchant.speed_rating ) );
  add_option( opt_float( "enchant_health", enchant.resource[ RESOURCE_HEALTH ] ) );
  add_option( opt_float( "enchant_mana", enchant.resource[ RESOURCE_MANA ] ) );
  add_option( opt_float( "enchant_rage", enchant.resource[ RESOURCE_RAGE ] ) );
  add_option( opt_float( "enchant_energy", enchant.resource[ RESOURCE_ENERGY ] ) );
  add_option( opt_float( "enchant_focus", enchant.resource[ RESOURCE_FOCUS ] ) );
  add_option( opt_float( "enchant_runic", enchant.resource[ RESOURCE_RUNIC_POWER ] ) );

  // Regen
  add_option( opt_bool( "infinite_energy", resources.infinite_resource[ RESOURCE_ENERGY ] ) );
  add_option( opt_bool( "infinite_focus", resources.infinite_resource[ RESOURCE_FOCUS ] ) );
  add_option( opt_bool( "infinite_health", resources.infinite_resource[ RESOURCE_HEALTH ] ) );
  add_option( opt_bool( "infinite_mana", resources.infinite_resource[ RESOURCE_MANA ] ) );
  add_option( opt_bool( "infinite_rage", resources.infinite_resource[ RESOURCE_RAGE ] ) );
  add_option( opt_bool( "infinite_runic", resources.infinite_resource[ RESOURCE_RUNIC_POWER ] ) );
  add_option( opt_bool( "infinite_astral_power", resources.infinite_resource[ RESOURCE_ASTRAL_POWER ] ) );

  // Resources
  add_option( opt_func( "initial_resource", parse_initial_resource ) );

  // Misc
  add_option( opt_string( "skip_actions", action_list_skip ) );
  add_option( opt_string( "modify_action", modify_action ) );
  add_option( opt_string( "use_apl", use_apl ) );
  add_option( opt_timespan( "reaction_time_mean", reaction_mean ) );
  add_option( opt_timespan( "reaction_time_stddev", reaction_stddev ) );
  add_option( opt_timespan( "reaction_time_nu", reaction_nu ) );
  add_option( opt_timespan( "reaction_time_offset", reaction_offset ) );
  add_option( opt_timespan( "reaction_time_max", reaction_max ) );
  add_option( opt_bool( "stat_cache", cache.active ) );
  add_option( opt_bool( "karazhan_trinkets_paired", karazhan_trinkets_paired ) );

  // Azerite options
  if ( ! is_enemy() && ! is_pet() )
  {
    add_option( opt_func( "azerite_override", std::bind( &azerite::azerite_state_t::parse_override,
          azerite.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 ) ) );
  }

  // Obsolete options

  // Dummy artifact options
  // TODO: Remove when 8.0 goes live
  add_option( opt_obsoleted( "artifact" ) );
  add_option( opt_obsoleted( "crucible" ) );
  add_option( opt_obsoleted( "artifact_override" ) );
  add_option( opt_obsoleted( "disable_artifact" ) );
}

player_t* player_t::create( sim_t*, const player_description_t& )
{
  return nullptr;
}

/**
 * Analyze statistical data of a player
 */
void player_t::analyze( sim_t& s )
{
  assert( s.iterations > 0 );

  pre_analyze_hook();

  collected_data.analyze( *this );

  range::for_each( buff_list, []( buff_t* b ) { b->analyze(); } );

  range::sort( stats_list, []( const stats_t* l, const stats_t* r ) { return l->name_str < r->name_str; } );

  if ( quiet )
    return;
  if ( collected_data.fight_length.mean() == 0 )
    return;

  range::for_each( sample_data_list, []( luxurious_sample_data_t* sd ) { sd->analyze(); } );

  // Pet Chart Adjustment ===================================================
  size_t max_buckets = static_cast<size_t>( collected_data.fight_length.max() );

  // Make the pet graphs the same length as owner's
  if ( is_pet() )
  {
    player_t* o = cast_pet()->owner;
    max_buckets = static_cast<size_t>( o->collected_data.fight_length.max() );
  }

  // Stats Analysis =========================================================
  std::vector<stats_t*> tmp_stats_list( stats_list.begin(), stats_list.end() );

  for ( size_t i = 0; i < pet_list.size(); ++i )
  {
    pet_t* pet = pet_list[ i ];
    // Append pet -> stats_list to stats_list
    tmp_stats_list.insert( tmp_stats_list.end(), pet->stats_list.begin(), pet->stats_list.end() );
  }

  if ( !is_pet() )
  {
    range::for_each( tmp_stats_list, [this]( stats_t* stats ) {
      stats->analyze();

      if ( stats->type == STATS_DMG )
      {
        stats->portion_amount =
            collected_data.compound_dmg.mean() ? stats->actual_amount.mean() / collected_data.compound_dmg.mean() : 0.0;
      }
      else if ( stats->type == STATS_HEAL || stats->type == STATS_ABSORB )
      {
        stats->portion_amount = collected_data.compound_heal.mean()
                                    ? stats->actual_amount.mean()
                                    : collected_data.compound_absorb.mean() ? stats->actual_amount.mean() : 0.0;

        double total_heal_and_absorb = collected_data.compound_heal.mean() + collected_data.compound_absorb.mean();

        if ( total_heal_and_absorb )
        {
          stats->portion_amount /= total_heal_and_absorb;
        }
      }
    } );
  }

  // Actor Lists ============================================================

  if ( !quiet && !is_enemy() && !is_add() && !( is_pet() && s.report_pets_separately ) )
  {
    s.players_by_dps.push_back( this );
    s.players_by_priority_dps.push_back( this );
    s.players_by_hps.push_back( this );
    s.players_by_hps_plus_aps.push_back( this );
    s.players_by_dtps.push_back( this );
    s.players_by_tmi.push_back( this );
    s.players_by_name.push_back( this );
    s.players_by_apm.push_back( this );
    s.players_by_variance.push_back( this );
  }

  if ( !quiet && ( is_enemy() || is_add() ) && !( is_pet() && s.report_pets_separately ) )
  {
    s.targets_by_name.push_back( this );
  }

  // Resources & Gains ======================================================

  if ( static_cast<size_t>( primary_resource() ) < collected_data.resource_lost.size() )
  {
    double rl = collected_data.resource_lost[ primary_resource() ].mean();

    dpr = ( rl > 0 ) ? ( collected_data.dmg.mean() / rl ) : -1.0;
    hpr = ( rl > 0 ) ? ( collected_data.heal.mean() / rl ) : -1.0;

    rps_loss = rl / collected_data.fight_length.mean();
  }

  if ( static_cast<size_t>( primary_resource() ) < collected_data.resource_gained.size() )
  {
    double rg = collected_data.resource_gained[ primary_resource() ].mean();

    rps_gain = rg / collected_data.fight_length.mean();
  }

  // When single_actor_batch=1 is used in conjunction with target_error, each actor has run varying
  // number of iterations to finish. The total number of iterations ran for each actor (when
  // single_actor_batch=1) is stored in the actor-collected data structure.
  //
  // Note, gain_t objects do not currently skip the first iteration for data collection. Thus, to
  // have information consistent in the reports, we use actor total iterations + the number of
  // threads as the divisor for resource-related data.
  int iterations =
      collected_data.total_iterations > 0 ? collected_data.total_iterations + sim->threads : sim->iterations;

  range::for_each( gain_list, [iterations]( gain_t* g ) { g->analyze( iterations ); } );

  range::for_each( pet_list, [iterations]( pet_t* p ) {
    range::for_each( p->gain_list, [iterations]( gain_t* g ) { g->analyze( iterations ); } );
  } );

  // Damage Timelines =======================================================

  if ( sim->report_details != 0 )
  {
    collected_data.timeline_dmg.init( max_buckets );
    bool is_hps = primary_role() == ROLE_HEAL;
    range::for_each( tmp_stats_list, [this, is_hps, max_buckets]( stats_t* stats ) {
      if ( stats->timeline_amount == nullptr )
      {
        return;
      }

      if ( ( stats->type != STATS_DMG ) == is_hps )
      {
        size_t j_max = std::min( max_buckets, stats->timeline_amount->data().size() );
        for ( size_t j = 0; j < j_max; j++ )
        {
          collected_data.timeline_dmg.add( j, stats->timeline_amount->data()[ j ] );
        }
      }
    } );
  }
  else
  {
    if ( !sim->single_actor_batch )
    {
      collected_data.timeline_dmg.adjust( *sim );
    }
    else
    {
      collected_data.timeline_dmg.adjust( collected_data.fight_length );
    }
  }

  recreate_talent_str( s.talent_format );
}

/**
 * Return the scaling metric over which this player gets scaled.
 *
 * This is used for scale factors, reforge plots, etc. as the default metric to display/calculate.
 * By default this will be personal dps or hps, but can also be dtps, tmi, etc.
 */
scaling_metric_data_t player_t::scaling_for_metric( scale_metric_e metric ) const
{
  const player_t* q = nullptr;
  if ( !sim->scaling->scale_over_player.empty() )
    q = sim->find_player( sim->scaling->scale_over_player );
  if ( !q )
    q = this;

  switch ( metric )
  {
    case SCALE_METRIC_DPS:
      return scaling_metric_data_t( metric, q->collected_data.dps );
    case SCALE_METRIC_DPSE:
      return scaling_metric_data_t( metric, q->collected_data.dpse );
    case SCALE_METRIC_HPS:
      return scaling_metric_data_t( metric, q->collected_data.hps );
    case SCALE_METRIC_HPSE:
      return scaling_metric_data_t( metric, q->collected_data.hpse );
    case SCALE_METRIC_APS:
      return scaling_metric_data_t( metric, q->collected_data.aps );
    case SCALE_METRIC_DPSP:
      return scaling_metric_data_t( metric, q->collected_data.prioritydps );
    case SCALE_METRIC_HAPS:
    {
      double mean   = q->collected_data.hps.mean() + q->collected_data.aps.mean();
      double stddev = sqrt( q->collected_data.hps.mean_variance + q->collected_data.aps.mean_variance );
      return scaling_metric_data_t( metric, "Healing + Absorb per second", mean, stddev );
    }
    case SCALE_METRIC_DTPS:
      return scaling_metric_data_t( metric, q->collected_data.dtps );
    case SCALE_METRIC_DMG_TAKEN:
      return scaling_metric_data_t( metric, q->collected_data.dmg_taken );
    case SCALE_METRIC_HTPS:
      return scaling_metric_data_t( metric, q->collected_data.htps );
    case SCALE_METRIC_TMI:
      return scaling_metric_data_t( metric, q->collected_data.theck_meloree_index );
    case SCALE_METRIC_ETMI:
      return scaling_metric_data_t( metric, q->collected_data.effective_theck_meloree_index );
    case SCALE_METRIC_DEATHS:
      return scaling_metric_data_t( metric, q->collected_data.deaths );
    default:
      if ( q->primary_role() == ROLE_TANK )
        return scaling_metric_data_t( SCALE_METRIC_DTPS, q->collected_data.dtps );
      else if ( q->primary_role() == ROLE_HEAL )
        return scaling_for_metric( SCALE_METRIC_HAPS );
      else
        return scaling_metric_data_t( SCALE_METRIC_DPS, q->collected_data.dps );
  }
}

/**
 * Change the player position ( fron/back, etc. ) and update attack hit table
 */
void player_t::change_position( position_e new_pos )
{
  if ( sim->debug )
    sim->out_debug.printf( "%s changes position from %s to %s.", name(), util::position_type_string( position() ),
                           util::position_type_string( new_pos ) );

  current.position = new_pos;
}

/**
 * Add a new resource callback, firing when a specified threshold for a resource is crossed.
 * Can either be a absolute value, or if use_pct is set, a percent-point value.
 *
 * Warning: Does not work for main enemy target on sim->iterations==0 or sim->fixed_time==1
 */
void player_t::register_resource_callback(resource_e resource, double value, resource_callback_function_t callback,
    bool use_pct, bool fire_once)
{
  resource_callback_entry_t entry{resource, value, use_pct, fire_once, false, callback};
  resource_callbacks.push_back(entry);
  has_active_resource_callbacks = true;
  sim->print_debug("{} resource callback registered. resource={} value={} pct={} fire_once={}",
      name(), util::resource_type_string(resource), value, use_pct, fire_once);
}

/**
 * If we have trigered all active resource callbacks in a iteration, disable the player-wide flag to check for
 * resource callbacks.
 */
void player_t::check_resource_callback_deactivation()
{
  for (auto& callback : resource_callbacks)
  {
    if (!callback.is_consumed)
      return;
  }
  has_active_resource_callbacks = false;
}

/**
 * Reset resource callback system.
 */
void player_t::reset_resource_callbacks()
{
  for (auto& callback : resource_callbacks)
  {
    callback.is_consumed = false;
    has_active_resource_callbacks = true;
  }
}

/**
 * Checks if a resource callback condition has been met and if yes activate it.
 */
void player_t::check_resource_change_for_callback(resource_e resource, double previous_amount, double previous_pct_points)
 {
   for (auto& callback : resource_callbacks)
   {
     if (callback.is_consumed)
       continue;
     if (callback.resource != resource)
       continue;

     // Evaluate if callback condition is met.
     bool callback_condition_valid = false;
     if (callback.is_pct)
     {
       double current_pct_points = resources.current[ resource ] / resources.max[ resource ] * 100.0;
       if ((callback.value < previous_pct_points && callback.value >= current_pct_points) ||
           (callback.value >= previous_pct_points && callback.value < current_pct_points))
       {
         callback_condition_valid = true;
       }
     }
     else
     {
       double current_amount = resources.current[ resource ];
       if ((callback.value < previous_amount && callback.value >= current_amount) ||
           (callback.value >= previous_amount && callback.value < current_amount))
       {
         callback_condition_valid = true;
       }
     }
     if (!callback_condition_valid)
     {
       continue;
     }

     sim->print_debug("{} resource callback triggered.", name());
     // We have a callback event, trigger stuff.
     callback.callback();
     if (callback.fire_once)
     {
       callback.is_consumed = true;
     }
  }

  check_resource_callback_deactivation();
}


timespan_t player_t::time_to_move() const
{
  if ( current.distance_to_move > 0 || current.moving_away > 0 )
  {
    // Add 1ms of time to ensure that we finish this run. This is necessary due to the millisecond accuracy in our
    // timing system.
    return timespan_t::from_seconds( ( current.distance_to_move + current.moving_away ) / composite_movement_speed() +
                                     0.001 );
  }
  else
    return timespan_t::zero();
}

void player_t::trigger_movement( double distance, movement_direction_e direction )
{
  // Distance of 0 disables movement
  if ( distance == 0 )
    do_update_movement( 9999 );
  else
  {
    if ( direction == MOVEMENT_AWAY )
      current.moving_away = distance;
    else
      current.distance_to_move = distance;
    current.movement_direction = direction;
    if ( buffs.movement )
    {
      buffs.movement->trigger();
    }
  }
}

void player_t::update_movement( timespan_t duration )
{
  // Presume stunned players don't move
  if ( buffs.stunned->check() )
    return;

  double yards = duration.total_seconds() * composite_movement_speed();
  do_update_movement( yards );

  if ( sim->debug )
  {
    if ( current.movement_direction == MOVEMENT_TOWARDS )
    {
      sim->out_debug.printf(
          "Player %s movement towards target, direction=%s speed=%f distance_covered=%f to_go=%f duration=%f", name(),
          util::movement_direction_string( movement_direction() ), composite_movement_speed(), yards,
          current.distance_to_move, duration.total_seconds() );
    }
    else
    {
      sim->out_debug.printf(
          "Player %s movement away from target, direction=%s speed=%f distance_covered=%f to_go=%f duration=%f", name(),
          util::movement_direction_string( movement_direction() ), composite_movement_speed(), yards,
          current.moving_away, duration.total_seconds() );
    }
  }
}

/**
 * Instant teleport. No overshooting support for now.
 */
void player_t::teleport( double yards, timespan_t duration )
{
  do_update_movement( yards );

  if ( sim->debug )
    sim->out_debug.printf( "Player %s warp, direction=%s speed=LIGHTSPEED! distance_covered=%f to_go=%f", name(),
                           util::movement_direction_string( movement_direction() ), yards, current.distance_to_move );
  (void)duration;
}

/**
 * Update movement data, and also control the buff
 */
void player_t::do_update_movement( double yards )
{
  if ( ( yards >= current.distance_to_move ) && current.moving_away <= 0 )
  {
    // x_position += current.distance_to_move;
    current.distance_to_move   = 0;
    current.movement_direction = MOVEMENT_NONE;
    if ( buffs.movement )
    {
      buffs.movement->expire();
    }
  }
  else
  {
    if ( current.moving_away > 0 )
    {
      // x_position -= yards;
      current.moving_away -= yards;
      current.distance_to_move += yards;
    }
    else
    {
      // x_position += yards;
      current.moving_away        = 0;
      current.movement_direction = MOVEMENT_TOWARDS;
      current.distance_to_move -= yards;
    }
  }
}

/**
 * Invalidate cache for ALL stats.
 */
void player_stat_cache_t::invalidate_all()
{
  if ( !active )
    return;

  range::fill( valid, false );
  range::fill( spell_power_valid, false );
  range::fill( player_mult_valid, false );
  range::fill( player_heal_mult_valid, false );
}

/**
 * Invalidate cache for a specific cache.
 */
void player_stat_cache_t::invalidate( cache_e c )
{
  switch ( c )
  {
    case CACHE_SPELL_POWER:
      range::fill( spell_power_valid, false );
      break;
    case CACHE_PLAYER_DAMAGE_MULTIPLIER:
      range::fill( player_mult_valid, false );
      break;
    case CACHE_PLAYER_HEAL_MULTIPLIER:
      range::fill( player_heal_mult_valid, false );
      break;
    default:
      valid[ c ] = false;
      break;
  }
}

/**
 * Get access attribute cache functions by attribute-enumeration.
 */
double player_stat_cache_t::get_attribute( attribute_e a ) const
{
  switch ( a )
  {
    case ATTR_STRENGTH:
      return strength();
    case ATTR_AGILITY:
      return agility();
    case ATTR_STAMINA:
      return stamina();
    case ATTR_INTELLECT:
      return intellect();
    case ATTR_SPIRIT:
      return spirit();
    default:
      assert( false );
      break;
  }
  return 0.0;
}

#if defined( SC_USE_STAT_CACHE )

double player_stat_cache_t::strength() const
{
  if ( !active || !valid[ CACHE_STRENGTH ] )
  {
    valid[ CACHE_STRENGTH ] = true;
    _strength               = player->strength();
  }
  else
    assert( _strength == player->strength() );
  return _strength;
}

double player_stat_cache_t::agility() const
{
  if ( !active || !valid[ CACHE_AGILITY ] )
  {
    valid[ CACHE_AGILITY ] = true;
    _agility               = player->agility();
  }
  else
    assert( _agility == player->agility() );
  return _agility;
}

double player_stat_cache_t::stamina() const
{
  if ( !active || !valid[ CACHE_STAMINA ] )
  {
    valid[ CACHE_STAMINA ] = true;
    _stamina               = player->stamina();
  }
  else
    assert( _stamina == player->stamina() );
  return _stamina;
}

double player_stat_cache_t::intellect() const
{
  if ( !active || !valid[ CACHE_INTELLECT ] )
  {
    valid[ CACHE_INTELLECT ] = true;
    _intellect               = player->intellect();
  }
  else
    assert( _intellect == player->intellect() );
  return _intellect;
}

double player_stat_cache_t::spirit() const
{
  if ( !active || !valid[ CACHE_SPIRIT ] )
  {
    valid[ CACHE_SPIRIT ] = true;
    _spirit               = player->spirit();
  }
  else
    assert( _spirit == player->spirit() );
  return _spirit;
}

double player_stat_cache_t::spell_power( school_e s ) const
{
  if ( !active || !spell_power_valid[ s ] )
  {
    spell_power_valid[ s ] = true;
    _spell_power[ s ]      = player->composite_spell_power( s );
  }
  else
    assert( _spell_power[ s ] == player->composite_spell_power( s ) );
  return _spell_power[ s ];
}

double player_stat_cache_t::attack_power() const
{
  if ( !active || !valid[ CACHE_ATTACK_POWER ] )
  {
    valid[ CACHE_ATTACK_POWER ] = true;
    _attack_power               = player->composite_melee_attack_power();
  }
  else
    assert( _attack_power == player->composite_melee_attack_power() );
  return _attack_power;
}

double player_stat_cache_t::attack_expertise() const
{
  if ( !active || !valid[ CACHE_ATTACK_EXP ] )
  {
    valid[ CACHE_ATTACK_EXP ] = true;
    _attack_expertise         = player->composite_melee_expertise();
  }
  else
    assert( _attack_expertise == player->composite_melee_expertise() );
  return _attack_expertise;
}

double player_stat_cache_t::attack_hit() const
{
  if ( !active || !valid[ CACHE_ATTACK_HIT ] )
  {
    valid[ CACHE_ATTACK_HIT ] = true;
    _attack_hit               = player->composite_melee_hit();
  }
  else
  {
    if ( _attack_hit != player->composite_melee_hit() )
    {
      assert( false );
    }
    // assert( _attack_hit == player -> composite_attack_hit() );
  }
  return _attack_hit;
}

double player_stat_cache_t::attack_crit_chance() const
{
  if ( !active || !valid[ CACHE_ATTACK_CRIT_CHANCE ] )
  {
    valid[ CACHE_ATTACK_CRIT_CHANCE ] = true;
    _attack_crit_chance               = player->composite_melee_crit_chance();
  }
  else
    assert( _attack_crit_chance == player->composite_melee_crit_chance() );
  return _attack_crit_chance;
}

double player_stat_cache_t::attack_haste() const
{
  if ( !active || !valid[ CACHE_ATTACK_HASTE ] )
  {
    valid[ CACHE_ATTACK_HASTE ] = true;
    _attack_haste               = player->composite_melee_haste();
  }
  else
    assert( _attack_haste == player->composite_melee_haste() );
  return _attack_haste;
}

double player_stat_cache_t::attack_speed() const
{
  if ( !active || !valid[ CACHE_ATTACK_SPEED ] )
  {
    valid[ CACHE_ATTACK_SPEED ] = true;
    _attack_speed               = player->composite_melee_speed();
  }
  else
    assert( _attack_speed == player->composite_melee_speed() );
  return _attack_speed;
}

double player_stat_cache_t::spell_hit() const
{
  if ( !active || !valid[ CACHE_SPELL_HIT ] )
  {
    valid[ CACHE_SPELL_HIT ] = true;
    _spell_hit               = player->composite_spell_hit();
  }
  else
    assert( _spell_hit == player->composite_spell_hit() );
  return _spell_hit;
}

double player_stat_cache_t::spell_crit_chance() const
{
  if ( !active || !valid[ CACHE_SPELL_CRIT_CHANCE ] )
  {
    valid[ CACHE_SPELL_CRIT_CHANCE ] = true;
    _spell_crit_chance               = player->composite_spell_crit_chance();
  }
  else
    assert( _spell_crit_chance == player->composite_spell_crit_chance() );
  return _spell_crit_chance;
}

double player_stat_cache_t::rppm_haste_coeff() const
{
  if ( !active || !valid[ CACHE_RPPM_HASTE ] )
  {
    valid[ CACHE_RPPM_HASTE ] = true;
    _rppm_haste_coeff          = 1.0 / std::min( player->cache.spell_haste(), player->cache.attack_haste() );
  }
  else
  {
    assert( _rppm_haste_coeff == 1.0 / std::min( player->cache.spell_haste(), player->cache.attack_haste() ) );
  }
  return _rppm_haste_coeff;
}

double player_stat_cache_t::rppm_crit_coeff() const
{
  if ( !active || !valid[ CACHE_RPPM_CRIT ] )
  {
    valid[ CACHE_RPPM_CRIT ] = true;
    _rppm_crit_coeff          = 1.0 + std::max( player->cache.attack_crit_chance(), player->cache.spell_crit_chance() );
  }
  else
  {
    assert( _rppm_crit_coeff == 1.0 + std::max( player->cache.attack_crit_chance(), player->cache.spell_crit_chance() ) );
  }
  return _rppm_crit_coeff;
}

double player_stat_cache_t::spell_haste() const
{
  if ( !active || !valid[ CACHE_SPELL_HASTE ] )
  {
    valid[ CACHE_SPELL_HASTE ] = true;
    _spell_haste               = player->composite_spell_haste();
  }
  else
    assert( _spell_haste == player->composite_spell_haste() );
  return _spell_haste;
}

double player_stat_cache_t::spell_speed() const
{
  if ( !active || !valid[ CACHE_SPELL_SPEED ] )
  {
    valid[ CACHE_SPELL_SPEED ] = true;
    _spell_speed               = player->composite_spell_speed();
  }
  else
    assert( _spell_speed == player->composite_spell_speed() );
  return _spell_speed;
}

double player_stat_cache_t::dodge() const
{
  if ( !active || !valid[ CACHE_DODGE ] )
  {
    valid[ CACHE_DODGE ] = true;
    _dodge               = player->composite_dodge();
  }
  else
    assert( _dodge == player->composite_dodge() );
  return _dodge;
}

double player_stat_cache_t::parry() const
{
  if ( !active || !valid[ CACHE_PARRY ] )
  {
    valid[ CACHE_PARRY ] = true;
    _parry               = player->composite_parry();
  }
  else
    assert( _parry == player->composite_parry() );
  return _parry;
}

double player_stat_cache_t::block() const
{
  if ( !active || !valid[ CACHE_BLOCK ] )
  {
    valid[ CACHE_BLOCK ] = true;
    _block               = player->composite_block();
  }
  else
    assert( _block == player->composite_block() );
  return _block;
}

double player_stat_cache_t::crit_block() const
{
  if ( !active || !valid[ CACHE_CRIT_BLOCK ] )
  {
    valid[ CACHE_CRIT_BLOCK ] = true;
    _crit_block               = player->composite_crit_block();
  }
  else
    assert( _crit_block == player->composite_crit_block() );
  return _crit_block;
}

double player_stat_cache_t::crit_avoidance() const
{
  if ( !active || !valid[ CACHE_CRIT_AVOIDANCE ] )
  {
    valid[ CACHE_CRIT_AVOIDANCE ] = true;
    _crit_avoidance               = player->composite_crit_avoidance();
  }
  else
    assert( _crit_avoidance == player->composite_crit_avoidance() );
  return _crit_avoidance;
}

double player_stat_cache_t::miss() const
{
  if ( !active || !valid[ CACHE_MISS ] )
  {
    valid[ CACHE_MISS ] = true;
    _miss               = player->composite_miss();
  }
  else
    assert( _miss == player->composite_miss() );
  return _miss;
}

double player_stat_cache_t::armor() const
{
  if ( !active || !valid[ CACHE_ARMOR ] || !valid[ CACHE_BONUS_ARMOR ] )
  {
    valid[ CACHE_ARMOR ] = true;
    _armor               = player->composite_armor();
  }
  else
    assert( _armor == player->composite_armor() );
  return _armor;
}

double player_stat_cache_t::mastery() const
{
  if ( !active || !valid[ CACHE_MASTERY ] )
  {
    valid[ CACHE_MASTERY ] = true;
    _mastery               = player->composite_mastery();
    _mastery_value         = player->composite_mastery_value();
  }
  else
    assert( _mastery == player->composite_mastery() );
  return _mastery;
}

/**
 * This is composite_mastery * specialization_mastery_coefficient !
 *
 * If you need the pure mastery value, use player_t::composite_mastery
 */
double player_stat_cache_t::mastery_value() const
{
  if ( !active || !valid[ CACHE_MASTERY ] )
  {
    valid[ CACHE_MASTERY ] = true;
    _mastery               = player->composite_mastery();
    _mastery_value         = player->composite_mastery_value();
  }
  else
    assert( _mastery_value == player->composite_mastery_value() );
  return _mastery_value;
}

double player_stat_cache_t::bonus_armor() const
{
  if ( !active || !valid[ CACHE_BONUS_ARMOR ] )
  {
    valid[ CACHE_BONUS_ARMOR ] = true;
    _bonus_armor               = player->composite_bonus_armor();
  }
  else
    assert( _bonus_armor == player->composite_bonus_armor() );
  return _bonus_armor;
}

double player_stat_cache_t::damage_versatility() const
{
  if ( !active || !valid[ CACHE_DAMAGE_VERSATILITY ] )
  {
    valid[ CACHE_DAMAGE_VERSATILITY ] = true;
    _damage_versatility               = player->composite_damage_versatility();
  }
  else
    assert( _damage_versatility == player->composite_damage_versatility() );
  return _damage_versatility;
}

double player_stat_cache_t::heal_versatility() const
{
  if ( !active || !valid[ CACHE_HEAL_VERSATILITY ] )
  {
    valid[ CACHE_HEAL_VERSATILITY ] = true;
    _heal_versatility               = player->composite_heal_versatility();
  }
  else
    assert( _heal_versatility == player->composite_heal_versatility() );
  return _heal_versatility;
}

double player_stat_cache_t::mitigation_versatility() const
{
  if ( !active || !valid[ CACHE_MITIGATION_VERSATILITY ] )
  {
    valid[ CACHE_MITIGATION_VERSATILITY ] = true;
    _mitigation_versatility               = player->composite_mitigation_versatility();
  }
  else
    assert( _mitigation_versatility == player->composite_mitigation_versatility() );
  return _mitigation_versatility;
}

double player_stat_cache_t::leech() const
{
  if ( !active || !valid[ CACHE_LEECH ] )
  {
    valid[ CACHE_LEECH ] = true;
    _leech               = player->composite_leech();
  }
  else
    assert( _leech == player->composite_leech() );
  return _leech;
}

double player_stat_cache_t::run_speed() const
{
  if ( !active || !valid[ CACHE_RUN_SPEED ] )
  {
    valid[ CACHE_RUN_SPEED ] = true;
    _run_speed               = player->composite_movement_speed();
  }
  else
    assert( _run_speed == player->composite_movement_speed() );
  return _run_speed;
}

double player_stat_cache_t::avoidance() const
{
  if ( !active || !valid[ CACHE_AVOIDANCE ] )
  {
    valid[ CACHE_AVOIDANCE ] = true;
    _avoidance               = player->composite_avoidance();
  }
  else
    assert( _avoidance == player->composite_avoidance() );
  return _avoidance;
}

double player_stat_cache_t::player_multiplier( school_e s ) const
{
  if ( !active || !player_mult_valid[ s ] )
  {
    player_mult_valid[ s ] = true;
    _player_mult[ s ]      = player->composite_player_multiplier( s );
  }
  else
    assert( _player_mult[ s ] == player->composite_player_multiplier( s ) );
  return _player_mult[ s ];
}

double player_stat_cache_t::player_heal_multiplier( const action_state_t* s ) const
{
  school_e sch = s->action->get_school();

  if ( !active || !player_heal_mult_valid[ sch ] )
  {
    player_heal_mult_valid[ sch ] = true;
    _player_heal_mult[ sch ]      = player->composite_player_heal_multiplier( s );
  }
  else
    assert( _player_heal_mult[ sch ] == player->composite_player_heal_multiplier( s ) );
  return _player_heal_mult[ sch ];
}

#endif

player_collected_data_t::action_sequence_data_t::action_sequence_data_t( const action_t* a, const player_t* t,
                                                                         const timespan_t& ts, const player_t* p ) :
  action( a ),
  target( t ),
  time( ts ),
  wait_time( timespan_t::zero() )
{
  for ( size_t i = 0; i < p->buff_list.size(); ++i )
  {
    buff_t* b = p->buff_list[ i ];
    if ( b->check() && !b->quiet && !b->constant )
    {
      std::vector<double> buff_args;
      buff_args.push_back( b->check() );
      if ( p->sim->json_full_states )
      {
        buff_args.push_back( b->remains().total_seconds() );
      }
      buff_list.push_back( std::make_pair( b, buff_args ) );
    }
  }

  // Adding cooldown and debuffs snapshots if asking for json full states
  if ( p->sim->json_full_states )
  {
    for ( size_t i = 0; i < p->cooldown_list.size(); ++i )
    {
      cooldown_t* c = p->cooldown_list[ i ];
      if ( c->down() )
      {
        std::vector<double> cooldown_args;
        cooldown_args.push_back( c->charges );
        cooldown_args.push_back( c->remains().total_seconds() );
        cooldown_list.push_back( std::make_pair( c, cooldown_args ) );
      }
    }
    for ( player_t* current_target : p->sim->target_list )
    {
      std::vector<std::pair<buff_t*, std::vector<double> > > debuff_list;
      for ( size_t i = 0; i < current_target->buff_list.size(); ++i )
      {
        buff_t* d = current_target->buff_list[ i ];
        if ( d->check() && !d->quiet && !d->constant )
        {
          std::vector<double> debuff_args;
          debuff_args.push_back( d->check() );
          debuff_args.push_back( d->remains().total_seconds() );
          debuff_list.push_back( std::make_pair( d, debuff_args ) );
        }
      }
      target_list.push_back( std::make_pair( current_target, debuff_list ) );
    }
  }

  range::fill( resource_snapshot, -1 );
  range::fill( resource_max_snapshot, -1 );

  for ( resource_e i = RESOURCE_HEALTH; i < RESOURCE_MAX; ++i )
  {
    if ( p->resources.max[ i ] > 0.0 )
    {
      resource_snapshot[ i ]     = p->resources.current[ i ];
      resource_max_snapshot[ i ] = p->resources.max[ i ];
    }
  }
}

player_collected_data_t::action_sequence_data_t::action_sequence_data_t( const timespan_t& ts, const timespan_t& wait,
                                                                         const player_t* p ) :
  action( 0 ),
  target( 0 ),
  time( ts ),
  wait_time( wait )
{
  for ( size_t i = 0; i < p->buff_list.size(); ++i )
  {
    buff_t* b = p->buff_list[ i ];
    if ( b->check() && !b->quiet && !b->constant )
    {
      std::vector<double> buff_args;
      buff_args.push_back( b->check() );
      if ( p->sim->json_full_states )
      {
        buff_args.push_back( b->remains().total_seconds() );
      }
      buff_list.push_back( std::make_pair( b, buff_args ) );
    }
  }

  // Adding cooldown and debuffs snapshots if asking for json full states
  if ( p->sim->json_full_states )
  {
    for ( size_t i = 0; i < p->cooldown_list.size(); ++i )
    {
      cooldown_t* c = p->cooldown_list[ i ];
      if ( c->down() )
      {
        std::vector<double> cooldown_args;
        cooldown_args.push_back( c->charges );
        cooldown_args.push_back( c->remains().total_seconds() );
        cooldown_list.push_back( std::make_pair( c, cooldown_args ) );
      }
    }
    for ( player_t* current_target : p->sim->target_list )
    {
      std::vector<std::pair<buff_t*, std::vector<double> > > debuff_list;
      for ( size_t i = 0; i < current_target->buff_list.size(); ++i )
      {
        buff_t* d = current_target->buff_list[ i ];
        if ( d->check() && !d->quiet && !d->constant )
        {
          std::vector<double> debuff_args;
          debuff_args.push_back( d->check() );
          debuff_args.push_back( d->remains().total_seconds() );
          debuff_list.push_back( std::make_pair( d, debuff_args ) );
        }
      }
      target_list.push_back( std::make_pair( current_target, debuff_list ) );
    }
  }

  range::fill( resource_snapshot, -1 );
  range::fill( resource_max_snapshot, -1 );

  for ( resource_e i = RESOURCE_HEALTH; i < RESOURCE_MAX; ++i )
  {
    if ( p->resources.max[ i ] > 0.0 )
    {
      resource_snapshot[ i ]     = p->resources.current[ i ];
      resource_max_snapshot[ i ] = p->resources.max[ i ];
    }
  }
}

namespace
{
bool tank_container_type( const player_t* for_actor, int target_statistics_level )
{
  if ( true ) // FIXME: cannot use virtual function calls here! if ( for_actor->primary_role() == ROLE_TANK )
  {
    return for_actor->sim->statistics_level < target_statistics_level;
  }

  return true;
}

bool generic_container_type( const player_t* for_actor, int target_statistics_level )
{
  if ( !for_actor->is_enemy() && ( !for_actor->is_pet() || for_actor->sim->report_pets_separately ) )
  {
    return for_actor->sim->statistics_level < target_statistics_level;
  }

  return true;
}
}  // namespace

player_collected_data_t::player_collected_data_t( const player_t* player ) :
  fight_length( player->name_str + " Fight Length", generic_container_type( player, 2 ) ),
  waiting_time( player->name_str + " Waiting Time", generic_container_type( player, 2 ) ),
  pooling_time( player->name_str + " Pooling Time", generic_container_type( player, 4 ) ),
  executed_foreground_actions( player->name_str + " Executed Foreground Actions", generic_container_type( player, 4 ) ),
  dmg( player->name_str + " Damage", generic_container_type( player, 2 ) ),
  compound_dmg( player->name_str + " Total Damage", generic_container_type( player, 2 ) ),
  prioritydps( player->name_str + " Priority Target Damage Per Second", generic_container_type( player, 1 ) ),
  dps( player->name_str + " Damage Per Second", generic_container_type( player, 1 ) ),
  dpse( player->name_str + " Damage Per Second (Effective)", generic_container_type( player, 2 ) ),
  dtps( player->name_str + " Damage Taken Per Second", tank_container_type( player, 2 ) ),
  dmg_taken( player->name_str + " Damage Taken", tank_container_type( player, 2 ) ),
  timeline_dmg(),
  heal( player->name_str + " Heal", generic_container_type( player, 2 ) ),
  compound_heal( player->name_str + " Total Heal", generic_container_type( player, 2 ) ),
  hps( player->name_str + " Healing Per Second", generic_container_type( player, 1 ) ),
  hpse( player->name_str + " Healing Per Second (Effective)", generic_container_type( player, 2 ) ),
  htps( player->name_str + " Healing Taken Per Second", tank_container_type( player, 2 ) ),
  heal_taken( player->name_str + " Healing Taken", tank_container_type( player, 2 ) ),
  absorb( player->name_str + " Absorb", generic_container_type( player, 2 ) ),
  compound_absorb( player->name_str + " Total Absorb", generic_container_type( player, 2 ) ),
  aps( player->name_str + " Absorb Per Second", generic_container_type( player, 1 ) ),
  atps( player->name_str + " Absorb Taken Per Second", tank_container_type( player, 2 ) ),
  absorb_taken( player->name_str + " Absorb Taken", tank_container_type( player, 2 ) ),
  deaths( player->name_str + " Deaths", tank_container_type( player, 2 ) ),
  theck_meloree_index( player->name_str + " Theck-Meloree Index", tank_container_type( player, 1 ) ),
  effective_theck_meloree_index( player->name_str + "Theck-Meloree Index (Effective)",
                                 tank_container_type( player, 2 ) ),
  max_spike_amount( player->name_str + " Max Spike Value", tank_container_type( player, 2 ) ),
  target_metric( player->name_str + " Target Metric", generic_container_type( player, 1 ) ),
  resource_timelines(),
  combat_end_resource(
      ( !player->is_enemy() && ( !player->is_pet() || player->sim->report_pets_separately ) ) ? RESOURCE_MAX : 0 ),
  stat_timelines(),
  health_changes(),
  health_changes_tmi(),
  total_iterations( 0 ),
  buffed_stats_snapshot()
{
  if ( !player->is_enemy() && ( !player->is_pet() || player->sim->report_pets_separately ) )
  {
    resource_lost.resize( RESOURCE_MAX );
    resource_gained.resize( RESOURCE_MAX );
  }

  // Enemies only have health
  if ( player->is_enemy() )
  {
    resource_lost.resize( RESOURCE_HEALTH + 1 );
    resource_gained.resize( RESOURCE_HEALTH + 1 );
  }
}

void player_collected_data_t::reserve_memory( const player_t& p )
{
  unsigned size = std::min( as<unsigned>( p.sim->iterations ), 2048u );
  fight_length.reserve( size );
  // DMG
  dmg.reserve( size );
  compound_dmg.reserve( size );
  dps.reserve( size );
  prioritydps.reserve( size );
  dpse.reserve( size );
  dtps.reserve( size );
  // HEAL
  heal.reserve( size );
  compound_heal.reserve( size );
  hps.reserve( size );
  hpse.reserve( size );
  htps.reserve( size );
  heal_taken.reserve( size );
  deaths.reserve( size );

  if ( !p.is_pet() && p.primary_role() == ROLE_TANK )
  {
    theck_meloree_index.reserve( size );
    effective_theck_meloree_index.reserve( size );
    p.sim->num_tanks++;
  }
}

void player_collected_data_t::merge( const player_t& other_player )
{
  const auto& other = other_player.collected_data;
  // No data got collected for this player in this thread, so skip merging player collected data
  // entirely
  if ( other.fight_length.count() == 0 )
  {
    return;
  }

  total_iterations += other.total_iterations;

  fight_length.merge( other.fight_length );
  waiting_time.merge( other.waiting_time );
  executed_foreground_actions.merge( other.executed_foreground_actions );
  // DMG
  dmg.merge( other.dmg );
  compound_dmg.merge( other.compound_dmg );
  dps.merge( other.dps );
  prioritydps.merge( other.prioritydps );
  dtps.merge( other.dtps );
  dpse.merge( other.dpse );
  dmg_taken.merge( other.dmg_taken );
  timeline_dmg.merge( other.timeline_dmg );
  // HEAL
  heal.merge( other.heal );
  compound_heal.merge( other.compound_heal );
  hps.merge( other.hps );
  htps.merge( other.htps );
  hpse.merge( other.hpse );
  heal_taken.merge( other.heal_taken );
  // Tank
  deaths.merge( other.deaths );
  timeline_dmg_taken.merge( other.timeline_dmg_taken );
  timeline_healing_taken.merge( other.timeline_healing_taken );
  theck_meloree_index.merge( other.theck_meloree_index );
  effective_theck_meloree_index.merge( other.effective_theck_meloree_index );

  for ( size_t i = 0, end = resource_lost.size(); i < end; ++i )
  {
    resource_lost[ i ].merge( other.resource_lost[ i ] );
    resource_gained[ i ].merge( other.resource_gained[ i ] );
  }

  if ( resource_timelines.size() == other.resource_timelines.size() )
  {
    for ( size_t i = 0; i < resource_timelines.size(); ++i )
    {
      assert( resource_timelines[ i ].type == other.resource_timelines[ i ].type );
      assert( resource_timelines[ i ].type != RESOURCE_NONE );
      resource_timelines[ i ].timeline.merge( other.resource_timelines[ i ].timeline );
    }
  }

  assert( stat_timelines.size() == other.stat_timelines.size() );
  for ( size_t i = 0; i < stat_timelines.size(); ++i )
  {
    assert( stat_timelines[ i ].type == other.stat_timelines[ i ].type );
    stat_timelines[ i ].timeline.merge( other.stat_timelines[ i ].timeline );
  }

  health_changes.merged_timeline.merge( other.health_changes.merged_timeline );
  health_changes_tmi.merged_timeline.merge( other.health_changes_tmi.merged_timeline );
}

void player_collected_data_t::analyze( const player_t& p )
{
  fight_length.analyze();
  // DMG
  dmg.analyze();
  compound_dmg.analyze();
  dps.analyze();
  prioritydps.analyze();
  dpse.analyze();
  dmg_taken.analyze();
  dtps.analyze();
  // Heal
  heal.analyze();
  compound_heal.analyze();
  hps.analyze();
  hpse.analyze();
  heal_taken.analyze();
  htps.analyze();
  // Absorb
  absorb.analyze();
  compound_absorb.analyze();
  aps.analyze();
  absorb_taken.analyze();
  atps.analyze();
  // Tank
  deaths.analyze();
  theck_meloree_index.analyze();
  effective_theck_meloree_index.analyze();
  max_spike_amount.analyze();

  if ( !p.sim->single_actor_batch )
  {
    timeline_dmg_taken.adjust( *p.sim );
    timeline_healing_taken.adjust( *p.sim );

    range::for_each( resource_timelines, [&p]( resource_timeline_t& tl ) { tl.timeline.adjust( *p.sim ); } );
    range::for_each( stat_timelines, [&p]( stat_timeline_t& tl ) { tl.timeline.adjust( *p.sim ); } );

    // health changes need their own divisor
    health_changes.merged_timeline.adjust( *p.sim );
    health_changes_tmi.merged_timeline.adjust( *p.sim );
  }
  // Single actor batch mode has to analyze the timelines in relation to their own fight lengths,
  // instead of the simulation-wide fight length.
  else
  {
    timeline_dmg_taken.adjust( fight_length );
    timeline_healing_taken.adjust( fight_length );

    range::for_each( resource_timelines, [this]( resource_timeline_t& tl ) { tl.timeline.adjust( fight_length ); } );
    range::for_each( stat_timelines, [this]( stat_timeline_t& tl ) { tl.timeline.adjust( fight_length ); } );

    // health changes need their own divisor
    health_changes.merged_timeline.adjust( fight_length );
    health_changes_tmi.merged_timeline.adjust( fight_length );
  }
}

// This is pretty much only useful for dev debugging at this point, would need to modify to make it useful to users
void player_collected_data_t::print_tmi_debug_csv( const sc_timeline_t* nma, const std::vector<double>& wv,
                                                   const player_t& p )
{
  if ( !p.tmi_debug_file_str.empty() )
  {
    io::ofstream f;
    f.open( p.tmi_debug_file_str );
    // write elements to CSV
    f << p.name_str << " TMI data:\n";

    f << "damage,healing,health chg,norm health chg,norm mov avg, weighted val\n";

    for ( size_t i = 0; i < health_changes.timeline.data().size(); i++ )
    {
      f.printf( "%f,%f,%f,%f,%f,%f\n", timeline_dmg_taken.data()[ i ], timeline_healing_taken.data()[ i ],
                health_changes.timeline.data()[ i ], health_changes.timeline_normalized.data()[ i ], nma->data()[ i ],
                wv[ i ] );
    }
    f << "\n";
  }
}

double player_collected_data_t::calculate_max_spike_damage( const health_changes_timeline_t& tl, int window )
{
  double max_spike = 0;

  // declare sliding average timeline
  sc_timeline_t sliding_average_tl;

  // create sliding average timelines from data
  tl.timeline_normalized.build_sliding_average_timeline( sliding_average_tl, window );

  // pull the data out of the normalized sliding average timeline
  std::vector<double> weighted_value = sliding_average_tl.data();

  // extract the max spike size from the sliding average timeline
  max_spike = *std::max_element( weighted_value.begin(), weighted_value.end() );  // todo: remove weighted_value here
  max_spike *= window;

  return max_spike;
}

double player_collected_data_t::calculate_tmi( const health_changes_timeline_t& tl, int window, double f_length,
                                               const player_t& p )
{
  // The Theck-Meloree Index is a metric that attempts to quantize the smoothness of damage intake.
  // It performs an exponentially-weighted sum of the moving average of damage intake, with larger
  // damage spikes being weighted more heavily. A formal definition of the metric can be found here:
  // http://www.sacredduty.net/theck-meloree-index-standard-reference-document/

  // accumulator
  double tmi = 0;

  // declare sliding average timeline
  sc_timeline_t sliding_average_tl;

  // create sliding average timelines from data
  tl.timeline_normalized.build_sliding_average_timeline( sliding_average_tl, window );

  // pull the data out of the normalized sliding average timeline
  std::vector<double> weighted_value = sliding_average_tl.data();

  // define constants
  double D  = 10;          // filtering strength
  double c2 = 450;         // N_0, default fight length for normalization
  double c1 = 100000 / D;  // health scale factor, determines slope of plot

  for ( auto& elem : weighted_value )
  {
    // weighted_value is the moving average (i.e. 1-second), so multiply by window size to get damage in "window"
    // seconds
    elem *= window;

    // calculate exponentially-weighted contribution of this data point using filter strength D
    elem = std::exp( D * elem );

    // add to the TMI total; strictly speaking this should be moved outside the for loop and turned into a sort()
    // followed by a sum for numerical accuracy
    tmi += elem;
  }

  // multiply by vertical offset factor c2
  tmi *= c2;
  // normalize for fight length - should be equivalent to dividing by tl.timeline_normalized.data().size()
  tmi /= f_length;
  tmi *= tl.get_bin_size();
  // take log of result
  tmi = std::log( tmi );
  // multiply by health decade scale factor
  tmi *= c1;

  // if an output file has been defined, write to it
  if ( !p.tmi_debug_file_str.empty() )
    print_tmi_debug_csv( &sliding_average_tl, weighted_value, p );

  return tmi;
}

void player_collected_data_t::collect_data( const player_t& p )
{
  double f_length   = p.iteration_fight_length.total_seconds();
  // Use a composite uptime for the amount per second calculations to accurately take into account
  // dynamic pets (for example wild imps) spawned through the pet spawner system. Only used for
  // output metrics.
  double uptime     = p.composite_active_time().total_seconds();
  double sim_length = p.sim->current_time().total_seconds();
  double w_time     = p.iteration_waiting_time.total_seconds();
  double p_time     = p.iteration_pooling_time.total_seconds();
  assert( p.iteration_fight_length <= p.sim->current_time() );

  fight_length.add( f_length );
  waiting_time.add( w_time );
  pooling_time.add( p_time );

  executed_foreground_actions.add( p.iteration_executed_foreground_actions );

  // Player only dmg/heal
  dmg.add( p.iteration_dmg );
  heal.add( p.iteration_heal );
  absorb.add( p.iteration_absorb );

  double total_iteration_dmg = p.iteration_dmg;  // player + pet dmg
  for ( size_t i = 0; i < p.pet_list.size(); ++i )
  {
    total_iteration_dmg += p.pet_list[ i ]->iteration_dmg;
  }
  double total_priority_iteration_dmg = p.priority_iteration_dmg;
  for ( size_t i = 0; i < p.pet_list.size(); ++i )
  {
    total_priority_iteration_dmg += p.pet_list[ i ]->priority_iteration_dmg;
  }
  double total_iteration_heal = p.iteration_heal;  // player + pet heal
  for ( size_t i = 0; i < p.pet_list.size(); ++i )
  {
    total_iteration_heal += p.pet_list[ i ]->iteration_heal;
  }
  double total_iteration_absorb = p.iteration_absorb;
  for ( size_t i = 0; i < p.pet_list.size(); ++i )
  {
    total_iteration_absorb += p.pet_list[ i ]->iteration_absorb;
  }

  compound_dmg.add( total_iteration_dmg );
  prioritydps.add( uptime ? total_priority_iteration_dmg / uptime : 0 );
  dps.add( uptime ? total_iteration_dmg / uptime : 0 );
  dpse.add( sim_length ? total_iteration_dmg / sim_length : 0 );
  double dps_metric = uptime ? ( total_iteration_dmg / uptime ) : 0;

  compound_heal.add( total_iteration_heal );
  hps.add( uptime ? total_iteration_heal / uptime : 0 );
  hpse.add( sim_length ? total_iteration_heal / sim_length : 0 );
  compound_absorb.add( total_iteration_absorb );
  aps.add( uptime ? total_iteration_absorb / uptime : 0.0 );
  double heal_metric = uptime ? ( ( total_iteration_heal + total_iteration_absorb ) / uptime ) : 0;

  heal_taken.add( p.iteration_heal_taken );
  htps.add( f_length ? p.iteration_heal_taken / f_length : 0 );
  absorb_taken.add( p.iteration_absorb_taken );
  atps.add( f_length ? p.iteration_absorb_taken / f_length : 0.0 );

  dmg_taken.add( p.iteration_dmg_taken );
  dtps.add( f_length ? p.iteration_dmg_taken / f_length : 0 );

  for ( size_t i = 0, end = resource_lost.size(); i < end; ++i )
  {
    resource_lost[ i ].add( p.iteration_resource_lost[ i ] );
    resource_gained[ i ].add( p.iteration_resource_gained[ i ] );
  }

  for ( size_t i = 0, end = combat_end_resource.size(); i < end; ++i )
  {
    combat_end_resource[ i ].add( p.resources.current[ i ] );
  }

  // Health Change Calculations - only needed for tanks
  double tank_metric = 0;
  if ( !p.is_pet() && p.primary_role() == ROLE_TANK )
  {
    double tmi       = 0;  // TMI result
    double etmi      = 0;  // ETMI result
    double max_spike = 0;  // Maximum spike size
    health_changes.merged_timeline.merge( health_changes.timeline );
    health_changes_tmi.merged_timeline.merge( health_changes_tmi.timeline );

    // Calculate Theck-Meloree Index (TMI), ETMI, and maximum spike damage
    if ( !p.is_enemy() )  // Boss TMI is irrelevant, causes problems in iteration #1
    {
      if ( f_length )
      {
        // define constants and variables
        int window = (int)std::floor( p.tmi_window / health_changes_tmi.get_bin_size() +
                                      0.5 );  // window size, bin time replaces 1 eventually

        // Standard TMI uses health_changes_tmi, ignoring externals - use health_changes_tmi
        tmi = calculate_tmi( health_changes_tmi, window, f_length, p );

        // ETMI includes external healing - use health_changes
        etmi = calculate_tmi( health_changes, window, f_length, p );

        // Max spike uses health_changes_tmi as well, ignores external heals - use health_changes_tmi
        max_spike = calculate_max_spike_damage( health_changes_tmi, window );

        tank_metric = tmi;
      }
    }
    theck_meloree_index.add( tmi );
    effective_theck_meloree_index.add( etmi );
    max_spike_amount.add( max_spike * 100.0 );
  }

  if ( p.sim->target_error > 0 && !p.is_pet() && !p.is_enemy() )
  {
    double metric = 0;

    role_e target_error_role = p.sim -> target_error_role;
    // use player's role if sim didn't provide an override
    if (target_error_role == ROLE_NONE)
    {
      target_error_role = p.primary_role();
      // exception for tanks - use DPS by default.
      if (target_error_role == ROLE_TANK)
      {
        target_error_role = ROLE_DPS;
      }
    }

    // ROLE is used here primarily to stay in-line with the previous version of the code.
    // An ideal implementation is probably to rewrite this to allow specification of a scale_metric_e
    // to make it more flexible. That was beyond my capability/available time and it would also likely be
    // very, very low use (as of legion/bfa, almost all tanks are simming DPS, not survival).
    switch( target_error_role )
    {
      case ROLE_ATTACK:
      case ROLE_SPELL:
      case ROLE_HYBRID:
      case ROLE_DPS:
        metric = dps_metric;
        break;

      case ROLE_TANK:
        metric = tank_metric;
        break;

      case ROLE_HEAL:
        metric = heal_metric;
        break;

      default:;
    }

    player_collected_data_t& cd = p.parent ? p.parent->collected_data : *this;

    AUTO_LOCK( cd.target_metric_mutex );
    cd.target_metric.add( metric );
  }
}

std::ostream& player_collected_data_t::data_str( std::ostream& s ) const
{
  fight_length.data_str( s );
  dmg.data_str( s );
  compound_dmg.data_str( s );
  dps.data_str( s );
  prioritydps.data_str( s );
  dpse.data_str( s );
  dtps.data_str( s );
  dmg_taken.data_str( s );
  timeline_dmg.data_str( s );
  timeline_dmg_taken.data_str( s );
  // Heal
  heal.data_str( s );
  compound_heal.data_str( s );
  hps.data_str( s );
  hpse.data_str( s );
  htps.data_str( s );
  heal_taken.data_str( s );
  timeline_healing_taken.data_str( s );
  // TMI
  theck_meloree_index.data_str( s );
  effective_theck_meloree_index.data_str( s );
  max_spike_amount.data_str( s );

  return s;
}

void player_talent_points_t::clear()
{
  range::fill( choices, -1 );
}

std::string player_talent_points_t::to_string() const
{
  std::ostringstream ss;

  ss << "{ ";
  for ( int i = 0; i < MAX_TALENT_ROWS; ++i )
  {
    if ( i )
      ss << ", ";
    ss << choice( i );
  }
  ss << " }";

  return ss.str();
}

luxurious_sample_data_t::luxurious_sample_data_t( player_t& p, std::string n ) :
  extended_sample_data_t( n, generic_container_type( &p, 3 ) ),
  player( p ),
  buffer_value( 0.0 )
{
}

// Note, root call needs to set player_t::visited_apls_ to 0
action_t* player_t::select_action( const action_priority_list_t& list,
                                   execute_type                  type,
                                   const action_t*               context )
{
  // Mark this action list as visited with the APL internal id
  visited_apls_ |= list.internal_id_mask;

  // Cached copy for recursion, we'll need it if we come back from a
  // call_action_list tree, with nothing to show for it.
  uint64_t _visited       = visited_apls_;
  size_t attempted_random = 0;

  const std::vector<action_t*>* action_list = nullptr;
  switch ( type )
  {
    case execute_type::OFF_GCD:
      action_list = &( list.off_gcd_actions );
      break;
    case execute_type::CAST_WHILE_CASTING:
      action_list = &( list.cast_while_casting_actions );
      break;
    default:
      action_list = &( list.foreground_action_list );
      break;
  }

  for ( size_t i = 0, num_actions = action_list->size(); i < num_actions; ++i )
  {
    visited_apls_ = _visited;
    action_t* a   = 0;

    if ( list.random == 1 )
    {
      size_t random = static_cast<size_t>( rng().range( 0, static_cast<double>( num_actions ) ) );
      a             = (*action_list)[ random ];
    }
    else
    {
      double skill = list.player->current.skill - list.player->current.skill_debuff;
      if ( skill != 1 && rng().roll( ( 1 - skill ) * 0.5 ) )
      {
        size_t max_random_attempts = static_cast<size_t>( num_actions * ( skill * 0.5 ) );
        size_t random              = static_cast<size_t>( rng().range( 0, static_cast<double>( num_actions ) ) );
        a                          = (*action_list)[ random ];
        attempted_random++;
        // Limit the amount of attempts to select a random action based on skill, then bail out and try again in 100 ms.
        if ( attempted_random > max_random_attempts )
          break;
      }
      else
      {
        a = (*action_list)[ i ];
      }
    }

    if ( context && a == context )
    {
      return nullptr;
    }

    if ( a->background )
      continue;

    if ( a->option.wait_on_ready == 1 )
      break;

    if ( a->action_ready() )
    {
      // Execute variable operation, and continue processing
      if ( a->type == ACTION_VARIABLE )
      {
        a->execute();
        continue;
      }
      // Call_action_list action, don't execute anything, but rather recurse
      // into the called action list.
      else if ( a->type == ACTION_CALL )
      {
        call_action_list_t* call = static_cast<call_action_list_t*>( a );
        // If the called APLs bitflag (based on internal id) is up, we're in an
        // infinite loop, and need to cancel the sim
        if ( visited_apls_ & call->alist->internal_id_mask )
        {
          throw std::runtime_error(fmt::format("'{}' action list in infinite loop", name() ));
        }

        // We get an action from the call, return it
        if ( action_t* real_a = select_action( *call->alist, type, context ) )
        {
          if ( context == nullptr && real_a->action_list )
            real_a->action_list->used = true;
          return real_a;
        }
      }
      // .. in branch prediction, we trust
      else
      {
        return a;
      }
    }
  }

  return nullptr;
}

player_t* player_t::actor_by_name_str( const std::string& name ) const
{
  // Check player pets first
  for ( size_t i = 0; i < pet_list.size(); i++ )
  {
    if ( util::str_compare_ci( pet_list[ i ]->name_str, name ) )
      return pet_list[ i ];
  }

  // Check harmful targets list
  for ( size_t i = 0; i < sim->target_list.size(); i++ )
  {
    if ( util::str_compare_ci( sim->target_list[ i ]->name_str, name ) )
      return sim->target_list[ i ];
  }

  // Finally, check player (non pet list), don't support targeting other
  // people's pets for now
  for ( size_t i = 0; i < sim->player_no_pet_list.size(); i++ )
  {
    if ( util::str_compare_ci( sim->player_no_pet_list[ i ]->name_str, name ) )
      return sim->player_no_pet_list[ i ];
  }

  return nullptr;
}

slot_e player_t::parent_item_slot( const item_t& item ) const
{
  unsigned parent = dbc.parent_item( item.parsed.data.id );
  if ( parent == 0 )
  {
    return SLOT_INVALID;
  }

  auto it = range::find_if( items, [parent]( const item_t& item ) { return parent == item.parsed.data.id; } );

  if ( it != items.end() )
  {
    return ( *it ).slot;
  }

  return SLOT_INVALID;
}

slot_e player_t::child_item_slot( const item_t& item ) const
{
  unsigned child = dbc.child_item( item.parsed.data.id );
  if ( child == 0 )
  {
    return SLOT_INVALID;
  }

  auto it = range::find_if( items, [child]( const item_t& item ) { return child == item.parsed.data.id; } );

  if ( it != items.end() )
  {
    return ( *it ).slot;
  }

  return SLOT_INVALID;
}

// TODO: This currently does not take minimum GCD into account, should it?
void player_t::adjust_global_cooldown( haste_type_e haste_type )
{
  // Don't adjust if the current gcd isnt hasted
  if ( gcd_haste_type == HASTE_NONE )
  {
    return;
  }

  // Don't adjust if the changed haste type isnt current GCD haste scaling type
  if ( haste_type != HASTE_ANY && gcd_haste_type != haste_type )
  {
    return;
  }

  // Don't adjust elapsed GCDs
  if ( ( readying && readying->occurs() <= sim->current_time() ) || ( gcd_ready <= sim->current_time() ) )
  {
    return;
  }

  double new_haste = 0;
  switch ( gcd_haste_type )
  {
    case HASTE_SPELL:
      new_haste = cache.spell_haste();
      break;
    case HASTE_ATTACK:
      new_haste = cache.attack_haste();
      break;
    case SPEED_SPELL:
      new_haste = cache.spell_speed();
      break;
    case SPEED_ATTACK:
      new_haste = cache.attack_speed();
      break;
    // SPEED_ANY and HASTE_ANY are nonsensical, actions have to have a correct GCD haste type so
    // they can be adjusted on state changes.
    default:
      assert( 0 && "player_t::adjust_global_cooldown called without proper haste type" );
      break;
  }

  // Haste did not change, don't do anything
  if ( new_haste == gcd_current_haste_value )
  {
    return;
  }

  double delta           = new_haste / gcd_current_haste_value;
  timespan_t remains     = readying ? readying->remains() : ( gcd_ready - sim->current_time() ),
             new_remains = remains * delta;

  // Don't bother processing too small (less than a millisecond) granularity changes
  if ( remains == new_remains )
  {
    return;
  }

  if ( sim->debug )
  {
    sim->out_debug.printf(
        "%s adjusting GCD due to haste change: old_ready=%.3f new_ready=%.3f old_haste=%f new_haste=%f delta=%f",
        name(), ( sim->current_time() + remains ).total_seconds(),
        ( sim->current_time() + new_remains ).total_seconds(), gcd_current_haste_value, new_haste, delta );
  }

  // We need to adjust the event (GCD is already elapsing)
  if ( readying )
  {
    // GCD speeding up, recreate the event
    if ( delta < 1 )
    {
      event_t::cancel( readying );
      readying = make_event<player_ready_event_t>( *sim, *this, new_remains );
    }
    // GCD slowing down, just reschedule into future
    else
    {
      readying->reschedule( new_remains );
    }
  }

  gcd_ready               = sim->current_time() + new_remains;
  gcd_current_haste_value = new_haste;
}

void player_t::adjust_auto_attack( haste_type_e haste_type )
{
  // Don't adjust autoattacks on spell-derived haste
  if ( haste_type == SPEED_SPELL || haste_type == HASTE_SPELL )
  {
    return;
  }

  if ( main_hand_attack )
    main_hand_attack->reschedule_auto_attack( current_attack_speed );
  if ( off_hand_attack )
    off_hand_attack->reschedule_auto_attack( current_attack_speed );

  current_attack_speed = cache.attack_speed();
}

timespan_t find_minimum_cd( const std::vector<const cooldown_t*>& list )
{
  timespan_t min_ready = timespan_t::min();
  range::for_each( list, [ &min_ready ]( const cooldown_t* cd ) {
    if ( min_ready == timespan_t::min() || cd->queueable() < min_ready )
    {
      min_ready = cd->queueable();
    }
  } );

  return min_ready;
}

// Calculate the minimum readiness time for all off gcd actions that are flagged as usable during
// gcd (i.e., action_t::use_off_gcd == true). Note that this does not take into account
// line_cooldown option, since that is APL-line specific.
void player_t::update_off_gcd_ready()
{
  if ( off_gcd_cd.size() == 0 && off_gcd_icd.size() == 0 )
  {
    return;
  }

  off_gcd_ready = std::max( find_minimum_cd( off_gcd_cd ), find_minimum_cd( off_gcd_icd ) );

  if ( sim -> debug )
  {
    sim -> out_debug.print( "{} next off-GCD cooldown ready at {}",
      name(), off_gcd_ready.total_seconds() );
  }

  schedule_off_gcd_ready();
}

// Calculate the minimum readiness time for all off gcd actions that are flagged as usable during
// gcd (i.e., action_t::use_off_gcd == true). Note that this does not take into account
// line_cooldown option, since that is APL-line specific.
void player_t::update_cast_while_casting_ready()
{
  if ( cast_while_casting_cd.size() == 0 && cast_while_casting_icd.size() == 0 )
  {
    return;
  }

  cast_while_casting_ready = std::max( find_minimum_cd( cast_while_casting_cd ),
      find_minimum_cd( cast_while_casting_icd ) );

  if ( sim -> debug )
  {
    sim -> out_debug.print( "{} next cast while casting cooldown ready at {}",
      name(), cast_while_casting_ready.total_seconds() );
  }

  schedule_cwc_ready();
}

/**
 * Verify that the user input (APL) contains an use-item line for all on-use items
 */
bool player_t::verify_use_items() const
{
  if ( !sim->use_item_verification || ( action_list_str.empty() && action_priority_list.size() == 0 ) )
  {
    return true;
  }

  std::vector<const use_item_t*> use_actions;
  std::vector<const item_t*> missing_actions;

  // Collect use_item actions
  range::for_each( action_list, [&use_actions]( const action_t* action ) {
    if ( auto ptr = dynamic_cast<const use_item_t*>( action ) )
    {
      use_actions.push_back( ptr );
    }
  } );

  // For all items that have on-use effects, ensure that there's a use action
  range::for_each( items, [&use_actions, &missing_actions]( const item_t& item ) {
    auto effect_it = range::find_if( item.parsed.special_effects, []( const special_effect_t* effect ) {
      return effect->source == SPECIAL_EFFECT_SOURCE_ITEM && effect->type == SPECIAL_EFFECT_USE;
    } );

    if ( effect_it != item.parsed.special_effects.end() )
    {
      // Each use_item action will have an associated item_t pointer for the item used, so just
      // compare the effect and use_item "item" pointers
      auto action_it = range::find_if(
          use_actions, [&effect_it]( const use_item_t* action ) { return action->item == ( *effect_it )->item; } );

      if ( action_it == use_actions.end() )
      {
        missing_actions.push_back( &item );
      }
    }
  } );

  // Nag about any items that are missing on-use actions
  range::for_each( missing_actions, [this]( const item_t* item ) {
    sim->errorf( "%s missing 'use_item' action for item \"%s\" (slot=%s)", name(), item->name(), item->slot_name() );
  } );

  return missing_actions.empty();
}

/**
 * Poor man's targeting support. Acquire_target is triggered by various events (see retarget_event_e) in the core.
 * Context contains the triggering entity (if relevant). Figures out a target out of all non-sleeping targets.
 * Skip "invulnerable" ones for now, anything else is fair game.
 */
void player_t::acquire_target( retarget_event_e event, player_t* context )
{
  // TODO: This skips certain very custom targets (such as Soul Effigy), is it a problem (since those
  // usually are handled in action target cache regeneration)?
  if ( sim->debug )
  {
    sim->out_debug.printf( "%s retargeting event=%s context=%s", name(), util::retarget_event_string( event ),
                           context ? context->name() : "NONE" );
  }

  player_t* candidate_target = nullptr;
  player_t* first_invuln_target = nullptr;

  // TODO: Fancier system
  for ( auto enemy : sim->target_non_sleeping_list )
  {
    if ( enemy->debuffs.invulnerable != nullptr && enemy->debuffs.invulnerable->up() )
    {
      if ( first_invuln_target == nullptr )
      {
        first_invuln_target = enemy;
      }
      continue;
    }

    candidate_target = enemy;
    break;
  }

  // Only perform target acquisition if the actor's current target would change (to the candidate
  // target).
  if ( candidate_target && candidate_target != target )
  {
    if ( sim->debug )
    {
      sim->out_debug.printf( "%s acquiring (new) target, current=%s candidate=%s", name(),
                             target ? target->name() : "NONE", candidate_target ? candidate_target->name() : "NONE" );
    }

    target = candidate_target;
    range::for_each( action_list, [event, context, candidate_target]( action_t* action ) {
      action->acquire_target( event, context, candidate_target );
    } );

    trigger_ready();
  }
  // If we really cannot find any sensible target, fall back to the first invulnerable target
  else if ( ! candidate_target && first_invuln_target && first_invuln_target != target )
  {
    if ( sim->debug )
    {
      sim->out_debug.printf( "%s acquiring (new) target, current=%s candidate=%s [invulnerable fallback]",
          name(), target ? target->name() : "NONE",
          first_invuln_target ? first_invuln_target->name() : "NONE" );
    }

    target = first_invuln_target;
    range::for_each( action_list, [event, context, first_invuln_target]( action_t* action ) {
      action->acquire_target( event, context, first_invuln_target );
    } );

    trigger_ready();
  }

  if ( candidate_target || first_invuln_target )
  {
    // Finally, re-acquire targeting for all dynamic targeting actions. This needs to be done
    // separately, as their targeting is not strictly bound to player_t::target (i.e., "the players
    // target")
    range::for_each( dynamic_target_action_list, [event, context, candidate_target, first_invuln_target]( action_t* action ) {
      action->acquire_target( event, context, candidate_target ? candidate_target : first_invuln_target );
    } );
  }
}

/**
 * A method to "activate" an actor in the simulator. Single actor batch mode activates one primary actor at a time,
 * while normal simulation mode activates all actors at the beginning of the sim.
 *
 * NOTE: Currently, the only thing that occurs during activation of an actor is the registering of various state change
 * callbacks (via the action_t::activate method) to the global actor lists.
 * Actor pets are also activated by default.
 */
void player_t::activate()
{
  // Activate all actions of the actor
  range::for_each( action_list, []( action_t* a ) { a->activate(); } );

  // .. and activate all actor pets
  range::for_each( pet_list, []( player_t* p ) { p->activate(); } );
}

void player_t::deactivate()
{
  // Record total number of iterations ran for this actor. Relevant in target_error cases for data
  // analysis at the end of simulation
  collected_data.total_iterations = nth_iteration();
}

void player_t::register_combat_begin( buff_t* b )
{
  combat_begin_functions.push_back( [ b ]( player_t* ) { b -> trigger(); } );
}

void player_t::register_combat_begin( action_t* a )
{
  combat_begin_functions.push_back( [ a ]( player_t* ) { a -> execute(); } );
}

void player_t::register_combat_begin( const combat_begin_fn_t& fn )
{
  combat_begin_functions.push_back( fn );
}

void player_t::register_combat_begin( double amount, resource_e resource, gain_t* g )
{
  combat_begin_functions.push_back( [ amount, resource, g ]( player_t* p ) {
    p -> resource_gain( resource, amount, g );
  });
}

std::ostream& operator<<(std::ostream &os, const player_t& p)
{
  fmt::print(os, "Player '{}'", p.name() );
  return os;
}

spawner::base_actor_spawner_t* player_t::find_spawner( const std::string& id ) const
{
  auto it = range::find_if( spawners, [ &id ]( spawner::base_actor_spawner_t* o ) {
    return util::str_compare_ci( id, o -> name() );
  } );

  if ( it != spawners.end() )
  {
    return *it;
  }

  return nullptr;
}

void action_variable_t::optimize()
{
  player_t* player = variable_actions.front()->player;
  if ( !player->sim->optimize_expressions )
  {
    return;
  }

  if ( player->nth_iteration() != 1 )
  {
    return;
  }

  bool is_constant = true;
  for ( auto action : variable_actions )
  {
    variable_t* var_action = debug_cast<variable_t*>( action );

    var_action->optimize_expressions();

    is_constant = var_action->is_constant();
    if ( !is_constant )
    {
      break;
    }
  }

  // This variable only has constant variable actions associated with it. The constant value will be
  // whatever the value is in current_value_
  if ( is_constant )
  {
    player->sim->print_debug( "{} variable {} is constant, value={}",
        player->name(), name_, current_value_ );
    constant_value_ = current_value_;
    // Make default value also the constant value, so that debug output is sensible
    default_ = current_value_;
  }
}
