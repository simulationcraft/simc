// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "player.hpp"

#include "action/action.hpp"
#include "action/action_callback.hpp"
#include "action/action_state.hpp"
#include "action/attack.hpp"
#include "action/dbc_proc_callback.hpp"
#include "action/dot.hpp"
#include "action/heal.hpp"
#include "action/residual_action.hpp"
#include "action/sequence.hpp"
#include "action/snapshot_stats.hpp"
#include "action/spell.hpp"
#include "action/variable.hpp"
#include "buff/buff.hpp"
#include "dbc/active_spells.hpp"
#include "dbc/azerite.hpp"
#include "dbc/character_loadout.hpp"
#include "dbc/covenant_data.hpp"
#include "dbc/dbc.hpp"
#include "dbc/item_database.hpp"
#include "dbc/item_set_bonus.hpp"
#include "dbc/rank_spells.hpp"
#include "dbc/specialization_spell.hpp"
#include "dbc/temporary_enchant.hpp"
#include "dbc/trait_data.hpp"
#include "item/item.hpp"
#include "item/special_effect.hpp"
#include "player/action_priority_list.hpp"
#include "player/action_variable.hpp"
#include "player/actor_target_data.hpp"
#include "player/azerite_data.hpp"
#include "player/consumable.hpp"
#include "player/covenant.hpp"
#include "player/ground_aoe.hpp"
#include "player/instant_absorb.hpp"
#include "player/pet.hpp"
#include "player/player_demise_event.hpp"
#include "player/player_event.hpp"
#include "player/player_scaling.hpp"
#include "player/player_talent_points.hpp"
#include "player/runeforge_data.hpp"
#include "player/sample_data_helper.hpp"
#include "player/scaling_metric_data.hpp"
#include "player/set_bonus.hpp"
#include "player/soulbinds.hpp"
#include "player/spawner_base.hpp"
#include "player/stats.hpp"
#include "player/unique_gear.hpp"
#include "player/unique_gear_thewarwithin.hpp"
#include "sim/benefit.hpp"
#include "sim/cooldown.hpp"
#include "sim/cooldown_waste_data.hpp"
#include "sim/event.hpp"
#include "sim/expressions.hpp"
#include "sim/proc.hpp"
#include "sim/proc_rng.hpp"
#include "sim/scale_factor_control.hpp"
#include "sim/sim.hpp"
#include "util/io.hpp"
#include "util/plot_data.hpp"
#include "util/util.hpp"

#include <cctype>
#include <cerrno>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>

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

  return !apl.empty();
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
    if ( p()->resource_regeneration == regen_type::DYNAMIC)
    {
      p()->do_dynamic_regen();
    }

    p()->visited_apls_ = 0;

    action_t* a = p()->select_action( apl(), type() );

    if ( p()->restore_action_list != nullptr )
    {
      p()->activate_action_list( p()->restore_action_list, p()->restore_action_list_type );
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
  const char* name() const override
  {
    return "Player-Ready";
  }
  void execute() override
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

  resource_threshold_event_t( player_t* p, timespan_t delay ) : event_t( *p, delay ), player( p )
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

  execute_pet_action_t( player_t* player, util::string_view name, util::string_view as,
                        util::string_view options_str ) :
    action_t( ACTION_OTHER, fmt::format( "execute_{}_{}", name, as ), player ),
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

      throw std::invalid_argument(fmt::format("Player {} refers to unknown action {} for pet {}.", player->name(), action_str,
          pet->name()));
    }

    action_t::init_finished();
  }

  void execute() override
  {
    assert(pet_action);
    pet_action->execute();
  }

  bool ready() override
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

struct leech_t : public heal_t
{
  leech_t( player_t* player ) : heal_t( "leech", player, player->find_spell( 143924 ) )
  {
    background = proc = true;
    callbacks = may_crit = may_miss = may_dodge = may_parry = may_block = false;
    target = player;
  }

  void init() override
  {
    heal_t::init();

    snapshot_flags = update_flags = STATE_MUL_DA | STATE_TGT_MUL_DA | STATE_VERSATILITY | STATE_MUL_PERSISTENT;

    player->register_combat_begin( []( player_t* p ) {
      make_repeating_event( *p->sim,
          [ p ] { return p->base_gcd * p->cache.spell_cast_speed(); },
          [ p ] {
            if ( p->leech_pool > 0 )
              p->spells.leech->schedule_execute();
          } );
    } );
  }

  void execute() override
  {
    heal_t::execute();

    player->leech_pool = 0;
  }

  double base_da_min( const action_state_t* ) const override { return player->leech_pool; }

  double base_da_max( const action_state_t* ) const override { return player->leech_pool; }
};

struct invulnerable_debuff_t : public buff_t
{
  invulnerable_debuff_t( player_t* p ) :
    buff_t( p, "invulnerable" )
  {
    set_max_stack( 1 );
  }

  void start( int stacks, double value, timespan_t duration ) override
  {
    buff_t::start( stacks, value, duration );
    if ( sim->ignore_invulnerable_targets && range::contains( sim->target_non_sleeping_list, player ) )
    {
      sim->target_non_sleeping_list.find_and_erase_unordered( player );
      sim->active_enemies--;
    }
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    buff_t::expire_override( expiration_stacks, remaining_duration );
    if ( sim->ignore_invulnerable_targets && !range::contains( sim->target_non_sleeping_list, player ) )
    {
      sim->target_non_sleeping_list.push_back( player );
      sim->active_enemies++;
    }
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

// parse_talent_string ======================================================

bool parse_talent_string( sim_t* sim, std::string_view, std::string_view string )
{
  player_t* p = sim->active_player;

  p->talents_str = std::string( string );

  return true;
}

// parse_timeofday ====================================================

bool parse_timeofday( sim_t* sim, std::string_view, std::string_view override_str )
{
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
    sim->error( "{} timeofday string '{}' not valid.", sim->active_player->name(), override_str );
    return false;
  }

  return true;
}

// parse_loa ====================================================

bool parse_loa( sim_t* sim, std::string_view, std::string_view override_str )
{
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
    sim->error( "{} zandalari_loa string '{}' not valid.", sim->active_player->name(), override_str );
    return false;
  }

  return true;
}

// parse_tricks
bool parse_tricks( sim_t* sim, std::string_view, std::string_view override_str )
{
  player_t* p = sim->active_player;

  if ( util::str_compare_ci( override_str, "corrosive" ) || util::str_compare_ci( override_str, "corrosive_vial" ) )
  {
    p->vulpera_tricks = player_t::CORROSIVE;
  }
  else if ( util::str_compare_ci( override_str, "flames" ) || util::str_compare_ci( override_str, "flames_of_fury" ) )
  {
    p->vulpera_tricks = player_t::FLAMES;
  }
  else if ( util::str_compare_ci( override_str, "shadows" ) ||
            util::str_compare_ci( override_str, "sinister_shadows" ) )
  {
    p->vulpera_tricks = player_t::SHADOWS;
  }
  else if ( util::str_compare_ci( override_str, "healing" ) || util::str_compare_ci( override_str, "healing_vial" ) )
  {
    p->vulpera_tricks = player_t::HEALING;
  }
  else if ( util::str_compare_ci( override_str, "holy" ) || util::str_compare_ci( override_str, "holy_relic" ) )
  {
    p->vulpera_tricks = player_t::HOLY;
  }
  else
  {
    sim->error( "{} vulpera_tricks string '{}' not valid.", sim->active_player->name(), override_str );
    return false;
  }

  return true;
}

// parse_mineral
bool parse_mineral( sim_t* sim, std::string_view, std::string_view override_str )
{
  player_t*p = sim->active_player;

  if ( util::str_compare_ci( override_str, "amber" ) || util::str_prefix_ci( override_str, "sta" ) )
  {
    p->earthen_mineral = player_t::AMBER;
  }
  else if ( util::str_compare_ci( override_str, "emerald" ) || util::str_prefix_ci( override_str, "has" ) )
  {
    p->earthen_mineral = player_t::EMERALD;
  }
  else if ( util::str_compare_ci( override_str, "onyx" ) || util::str_prefix_ci( override_str, "mas" ) )
  {
    p->earthen_mineral = player_t::ONYX;
  }
  else if ( util::str_compare_ci( override_str, "ruby" ) || util::str_prefix_ci( override_str, "crit" ) )
  {
    p->earthen_mineral = player_t::RUBY;
  }
  else if ( util::str_compare_ci( override_str, "sapphire" ) || util::str_prefix_ci( override_str, "vers" ) )
  {
    p->earthen_mineral = player_t::SAPPHIRE;
  }
  else
  {
    sim->error( "{} earthen_mineral string '{}' not valid.", sim->active_player->name(), override_str );
    return false;
  }

  return true;
}

// parse_role_string ========================================================

bool parse_role_string( sim_t* sim, std::string_view, std::string_view value )
{
  sim->active_player->role = util::parse_role_type( value );

  return true;
}

// parse_world_lag ==========================================================

bool parse_world_lag( sim_t* sim, std::string_view, std::string_view value )
{
  sim->active_player->world_lag.mean = timespan_t::from_seconds( util::to_double( value ) );

  if ( sim->active_player->world_lag.mean < 0_ms )
  {
    sim->active_player->world_lag.mean = 0_ms;
  }

  return true;
}

// parse_world_lag ==========================================================

bool parse_world_lag_stddev( sim_t* sim, std::string_view, std::string_view value )
{
  sim->active_player->world_lag.stddev = timespan_t::from_seconds( util::to_double( value ) );

  if ( sim->active_player->world_lag.stddev < 0_ms )
  {
    sim->active_player->world_lag.stddev = 0_ms;
  }

  return true;
}

// parse_brain_lag ==========================================================

bool parse_brain_lag( sim_t* sim, std::string_view, std::string_view value )
{
  sim->active_player->brain_lag.mean = timespan_t::from_seconds( util::to_double( value ) );

  if ( sim->active_player->brain_lag.mean < 0_ms )
  {
    sim->active_player->brain_lag.mean = 0_ms;
  }

  return true;
}

// parse_brain_lag_stddev ===================================================

bool parse_brain_lag_stddev( sim_t* sim, std::string_view, std::string_view value )
{
  sim->active_player->brain_lag.stddev = timespan_t::from_seconds( util::to_double( value ) );

  if ( sim->active_player->brain_lag.stddev < 0_ms )
  {
    sim->active_player->brain_lag.stddev = 0_ms;
  }

  return true;
}

// parse_specialization =====================================================

bool parse_specialization( sim_t* sim, std::string_view, std::string_view value )
{
  sim->active_player->_spec = dbc::translate_spec_str( sim->active_player->type, value );

  if ( sim->active_player->_spec == SPEC_NONE )
  {
    sim->error( "{} specialization string '{}' not valid.", sim->active_player->name(), value );
    return false;
  }

  return true;
}

// parse stat timelines =====================================================

bool parse_stat_timelines( sim_t* sim, std::string_view, std::string_view value )
{
  auto stats = util::string_split<std::string_view>( value, "," );

  for ( auto& stat_type : stats )
  {
    stat_e st = util::parse_stat_type( stat_type );
    if ( st == STAT_NONE )
    {
      sim->error( "'{}' could not parse timeline stat '{}' ({}).\n", sim->active_player->name(), stat_type, value );
      return false;
    }

    sim->active_player->stat_timelines.push_back( st );
  }

  return true;
}

// parse_origin =============================================================

bool parse_origin( sim_t* sim, std::string_view, std::string_view origin )
{
  player_t& p = *sim->active_player;

  p.origin_str = std::string( origin );

  std::string region, server, name;
  if ( util::parse_origin( region, server, name, origin ) )
  {
    p.region_str = region;
    p.server_str = server;
  }

  return true;
}

// parse_source ===============================================================

bool parse_source( sim_t* sim, std::string_view, std::string_view value )
{
  player_t& p = *sim->active_player;

  p.profile_source_ = util::parse_profile_source( value );

  return true;
}

bool parse_set_bonus( sim_t* sim, std::string_view, std::string_view value )
{
  static constexpr const char* error_str = "{} invalid 'set_bonus' option value '{}' given, available options: {}";

  player_t* p = sim->active_player;

  auto set_bonus_split = util::string_split<std::string_view>( value, "=" );

  if ( set_bonus_split.size() != 2 )
  {
    sim->error( error_str, p->name(), value, p->sets->generate_set_bonus_options() );
    return false;
  }

  int opt_val = util::to_int( set_bonus_split[ 1 ] );
  if ( opt_val != 0 && opt_val != 1 )
  {
    sim->error( error_str, p->name(), value, p->sets->generate_set_bonus_options() );
    return false;
  }

  set_bonus_type_e set_bonus = SET_BONUS_NONE;
  set_bonus_e bonus          = B_NONE;

  if ( !p->sets->parse_set_bonus_option( set_bonus_split[ 0 ], set_bonus, bonus ) )
  {
    sim->error( error_str, p->name(), value, p->sets->generate_set_bonus_options() );
    return false;
  }

  p->sets->set_bonus_spec_data[ set_bonus ][ dbc::spec_idx( p->specialization() ) ][ bonus ].overridden = opt_val;

  return true;
}

bool parse_initial_resource( sim_t* sim, std::string_view, std::string_view value )
{
  player_t* player = sim->active_player;
  auto opts        = util::string_split<std::string_view>( value, ":/" );
  for ( const auto& opt_str : opts )
  {
    auto resource_split = util::string_split<std::string_view>( opt_str, "=" );
    if ( resource_split.size() != 2 )
    {
      sim->error( "{} unknown initial_resources option '{}'", player->name(), opt_str );
      return false;
    }

    resource_e resource = util::parse_resource_type( resource_split[ 0 ] );
    if ( resource == RESOURCE_NONE )
    {
      sim->error( "{} could not parse valid resource from '{}'", player->name(), resource_split[ 0 ] );
      return false;
    }

    double amount = util::to_double( resource_split[ 1 ] );
    if ( amount < 0 )
    {
      sim->error( "{} too low initial_resources option '{}'", player->name(), opt_str );
      return false;
    }

    player->resources.initial_opt[ resource ] = amount;
  }

  return true;
}

bool tank_container_type(const player_t* for_actor, int target_statistics_level)
{
  if (true) // FIXME: cannot use virtual function calls here! if ( for_actor->primary_role() == ROLE_TANK )
  {
    return for_actor->sim->statistics_level < target_statistics_level;
  }

  return true;
}

bool generic_container_type(const player_t* for_actor, int target_statistics_level)
{
  if (!for_actor->is_enemy() && (!for_actor->is_pet() || for_actor->sim->report_pets_separately))
  {
    return for_actor->sim->statistics_level < target_statistics_level;
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
      return p->sim->rng().gauss( p->sim->default_aura_delay );
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

    const char* name() const override
    {
      return "residual_action_delay_event";
    }

    void execute() override
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
      action->snapshot_state( s, action->type == ACTION_HEAL ? result_amount_type::HEAL_OVER_TIME : result_amount_type::DMG_OVER_TIME );
      s->result_amount = additional_residual_amount;
      action->schedule_travel( s );
      if ( !action->dual )
        action->stats->add_execute( timespan_t::zero(), s->target );
    }
  };

  assert( residual_action );

  make_event<delay_event_t>( *residual_action->sim, t, residual_action, amount );
}

player_t::player_t( sim_t* s, player_e t, util::string_view n, race_e r )
  : actor_t( s, n ),
    type( t ),
    parent( nullptr ),
    index( -1 ),
    creation_iteration( sim->current_iteration ),
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
    target( nullptr ),
    initialized( false ),
    precombat_initialized( false ),
    potion_used( false ),
    leech_pool( 0 ),
    region_str( s->default_region_str ),
    server_str( s->default_server_str ),
    origin_str(),
    timeofday( NIGHT_TIME ),  // Depends on server time, default to night that's more common for raid hours
    zandalari_loa( PAKU ),  // Default to Paku as it has some non-zero dps benefit for all specs
    vulpera_tricks( CORROSIVE ),  // default trick for damage
    earthen_mineral( RUBY ),  // default mineral is ruby (crit)
    gcd_ready( 0_ms ),
    base_gcd( 1.5_s ),
    min_gcd( 750_ms ),
    gcd_type( gcd_haste_type::NONE ),
    gcd_current_haste_value( 1.0 ),
    started_waiting( timespan_t::min() ),
    pet_list(),
    active_pets(),
    invert_scaling( 0 ),
    // Reaction
    reaction( 250_ms, 0_ms ),
    reaction_offset( 100_ms ),
    reaction_max( 1.4_s ),
    reaction_nu( 150_ms ),
    // Latency
    world_lag( s->world_lag ),
    brain_lag( 0_ms, timespan_t::min() ),
    cooldown_tolerance_( timespan_t::min() ),
    dbc( new dbc_t(*(s->dbc)) ),
    dbc_override( sim->dbc_override.get() ),
    // talent_points( new player_talent_points_t()),
    talent_points( nullptr ),
    profession(),
    azerite( nullptr ),
    base(),
    initial(),
    current(),
    passive_rating_multiplier( 1.0 ),
    last_cast( 0_ms),
    // Defense Mechanics
    def_dr( diminishing_returns_constants_t() ),
    // Attacks
    main_hand_attack( nullptr ),
    off_hand_attack( nullptr ),
    current_auto_attack_speed( 1.0 ),
    // Resources
    resources(),
    // Consumables
    // Events
    executing( nullptr ),
    queueing( nullptr ),
    channeling( nullptr ),
    strict_sequence( nullptr ),
    demise_event(),
    readying( nullptr ),
    off_gcd( nullptr ),
    cast_while_casting_poll_event(),
    off_gcd_ready( timespan_t::min() ),
    cast_while_casting_ready( timespan_t::min() ),
    in_combat( false ),
    in_boss_encounter( 1 ),
    action_queued( false ),
    first_cast( true ),
    last_foreground_action( nullptr ),
    prev_gcd_actions( 0 ),
    off_gcdactions(),
    cast_delay_reaction( 0_ms ),
    cast_delay_occurred( 0_ms ),
    callbacks( s ),
    use_apl( "" ),
    // Actions
    use_default_action_list( false ),
    precombat_action_list( 0 ),
    active_action_list(),
    default_action_list(),
    active_off_gcd_list(),
    active_cast_while_casting_list(),
    restore_action_list(),
    restore_action_list_type( execute_type::FOREGROUND ),
    no_action_list_provided(),
    // Reporting
    quiet( false ),
    report_extension(),
    mixin_reports(),
    arise_time( timespan_t::min() ),
    iteration_fight_length(),
    iteration_waiting_time(),
    iteration_pooling_time(),
    iteration_executed_foreground_actions( 0 ),
    iteration_resource_lost(),
    iteration_resource_gained(),
    iteration_resource_overflowed(),
    rps_gain( 0 ),
    rps_loss( 0 ),
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
    item_cooldown( new cooldown_t("item_cd", *this) ),
    default_item_group_cooldown( 20_s ),
    load_default_gear( false ),
    load_default_talents( false ),
    auto_attack_modifier( 0.0 ),
    auto_attack_base_modifier( 0.0 ),
    auto_attack_multiplier( 1.0 ),
    scaling( ( !is_pet() || sim->report_pets_separately ) ? new player_scaling_t() : nullptr ),
    // Movement & Position
    base_movement_speed( 7.0 ),
    passive_modifier( 0 ),
    x_position( 0.0 ),
    y_position( 0.0 ),
    default_x_position( std::numeric_limits<double>::lowest() ),
    default_y_position( std::numeric_limits<double>::lowest() ),
    consumables(),
    buffs(),
    debuffs(),
    external_buffs(),
    gains(),
    spells(),
    procs(),
    uptimes(),
    racials(),
    passive_values(),
    active_during_iteration( false ),
    spec_spell( spell_data_t::nil() ),
    _mastery( &spelleffect_data_t::nil() ),
    cache( this ),
    resource_regeneration( regen_type::STATIC ),
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

  if ( ! is_enemy() && ! is_pet() )
  {
    azerite = azerite::create_state( this );
    azerite_essence = azerite::create_essence_state( this );
    covenant = covenant::create_player_state( this );
    dbc_override_ = std::make_unique<dbc_override_t>( dbc_override );
    dbc_override = dbc_override_.get();
  }

  // Set the gear object to a special default value, so we can support gear_x=0 properly.
  // player_t::init_items will replace the defaulted gear_stats_t object with a computed one once
  // the item stats have been computed.
  gear.initialize( std::numeric_limits<double>::lowest() );

  base.skill              = sim->default_skill;
  base.mastery            = 8.0;
  base.movement_direction = movement_direction_type::NONE;

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

  if ( reaction.stddev == 0_ms )
    reaction.stddev = reaction.mean * 0.2;

  action_list_information =
      "\n"
      "# This default action priority list is automatically created based on your character.\n"
      "# It is a attempt to provide you with a action list that is both simple and practicable,\n"
      "# while resulting in a meaningful and good simulation. It may not result in the absolutely highest possible "
      "dps.\n"
      "# Feel free to edit, adapt and improve it to your own needs.\n"
      "# SimulationCraft is always looking for updates and improvements to the default action lists.\n";

  if ( !is_pet() && !is_enemy() )
  {
    sim->register_heartbeat_event_callback( [ this ]( sim_t* ) {
      if ( in_combat )
        trigger_callbacks( PROC1_HEARTBEAT, PROC2_LANDED, nullptr, nullptr );

      for ( auto& pet : active_pets )
      {
        pet->update_stats();
      }
    } );
  }
}

// Added in source file to get full definitions of all objects to destruct here;
player_t::~player_t() = default;

player_t::base_initial_current_t::base_initial_current_t() :
  stats(),
  spell_power_per_intellect( 0 ),
  spell_power_per_attack_power( 0 ),
  spell_crit_per_intellect( 0 ),
  attack_power_per_strength( 0 ),
  attack_power_per_agility( 0 ),
  attack_crit_per_agility( 0 ),
  attack_power_per_spell_power( 0 ),
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
  leech( 0 ),
  avoidance( 0 ),
  spell_crit_chance(),
  attack_crit_chance(),
  block_reduction(),
  mastery(),
  versatility( 0 ),
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
  crit_damage_multiplier( 1.0 ),
  crit_healing_multiplier( 1.0 ),
  position( POSITION_BACK )
{
  range::fill( attribute_multiplier, 1.0 );
}

void sc_format_to( const player_t::base_initial_current_t& s, fmt::format_context::iterator out )
{
  fmt::format_to( out, "{}", s.stats );
  fmt::format_to( out, " spell_power_per_intellect={}", s.spell_power_per_intellect );
  fmt::format_to( out, " spell_power_per_attack_power={}", s.spell_power_per_attack_power );
  fmt::format_to( out, " spell_crit_per_intellect={}", s.spell_crit_per_intellect );
  fmt::format_to( out, " attack_power_per_strength={}", s.attack_power_per_strength );
  fmt::format_to( out, " attack_power_per_agility={}", s.attack_power_per_agility );
  fmt::format_to( out, " attack_power_per_spell_power={}", s.attack_power_per_spell_power );
  fmt::format_to( out, " attack_crit_per_agility={}", s.attack_crit_per_agility );
  fmt::format_to( out, " dodge_per_agility={}", s.dodge_per_agility );
  fmt::format_to( out, " parry_per_strength={}", s.parry_per_strength );
  fmt::format_to( out, " health_per_stamina={}", s.health_per_stamina );
  // resource_reduction
  fmt::format_to( out, " miss={}", s.miss );
  fmt::format_to( out, " dodge={}", s.dodge );
  fmt::format_to( out, " parry={}", s.parry );
  fmt::format_to( out, " block={}", s.block );
  fmt::format_to( out, " spell_crit_chance={}", s.spell_crit_chance );
  fmt::format_to( out, " attack_crit_chance={}", s.attack_crit_chance );
  fmt::format_to( out, " block_reduction={}", s.block_reduction );
  fmt::format_to( out, " mastery={}", s.mastery );
  fmt::format_to( out, " versatility={}", s.versatility );
  fmt::format_to( out, " leech={}", s.leech );
  fmt::format_to( out, " skill={}", s.skill );
  fmt::format_to( out, " distance={}", s.distance );
  fmt::format_to( out, " armor_coeff={}", s.armor_coeff );
  fmt::format_to( out, " sleeping={}", s.sleeping );
  // attribute_multiplier
  fmt::format_to( out, " spell_power_multiplier={}", s.spell_power_multiplier );
  fmt::format_to( out, " attack_power_multiplier={}", s.attack_power_multiplier );
  fmt::format_to( out, " base_armor_multiplier={}", s.base_armor_multiplier );
  fmt::format_to( out, " armor_multiplier={}", s.armor_multiplier );
  fmt::format_to( out, " crit_damage_multiplier={}", s.crit_damage_multiplier );
  fmt::format_to( out, " crit_healing_multiplier={}", s.crit_healing_multiplier );
  fmt::format_to( out, " position={}", s.position );
}

void player_t::init()
{
  sim->print_debug( "Initializing {}.", *this );

  // Validate current fight style is supported by the actor's module.
  if ( !validate_fight_style( sim->fight_style ) )
  {
    sim->error( "{} does not support fight style {}, results may be unreliable.", *this,
                util::fight_style_string( sim->fight_style ) );
  }

  // Fight style dependent option defaults. Note that these options must be of type player_option_t<T>
  if ( sim->fight_style == FIGHT_STYLE_DUNGEON_ROUTE || sim->fight_style == FIGHT_STYLE_DUNGEON_SLICE )
  {
    if ( dragonflight_opts.balefire_branch_loss_rng_type.is_default() )
      dragonflight_opts.balefire_branch_loss_rng_type = "rppm";
  }

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
  if (resource_regeneration == regen_type::DYNAMIC)
  {
    for ( auto pet : pet_list )
    {
      if ( pet->resource_regeneration != regen_type::DYNAMIC)
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

  if ( world_lag.stddev < 0_ms )
    world_lag.stddev = world_lag.mean * 0.1;
  if ( brain_lag.stddev < 0_ms )
    brain_lag.stddev = brain_lag.mean * 0.1;
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
      sim->error( "{} stat {} is {} != 0.0", name(), stat, stat_value );
      sim->error( " Please do not modify player_t::base before init_base_stats is called\n" );
    }
  }

  if ( !is_enemy() )
    base.rating.init( *dbc, level() );

  sim->print_debug( "{} base ratings initialized: {}", *this, base.rating );

  if ( !is_enemy() )
  {
    base.stats.attribute[ STAT_STRENGTH ]  = dbc->race_base( race ).strength + dbc->attribute_base( type, level() ).strength;
    base.stats.attribute[ STAT_AGILITY ]   = dbc->race_base( race ).agility + dbc->attribute_base( type, level() ).agility;
    base.stats.attribute[ STAT_STAMINA ]   = dbc->race_base( race ).stamina + dbc->attribute_base( type, level() ).stamina;
    base.stats.attribute[ STAT_INTELLECT ] = dbc->race_base( race ).intellect + dbc->attribute_base( type, level() ).intellect;
    base.stats.attribute[ STAT_SPIRIT ]    = dbc->race_base( race ).spirit + dbc->attribute_base( type, level() ).spirit;

    // heroic presence is treated like base stats, floored before adding in; tested 2014-07-20
    base.stats.attribute[ STAT_STRENGTH ]  += util::floor( racials.heroic_presence->effectN( 1 ).average( this ) );
    base.stats.attribute[ STAT_AGILITY ]   += util::floor( racials.heroic_presence->effectN( 2 ).average( this ) );
    base.stats.attribute[ STAT_INTELLECT ] += util::floor( racials.heroic_presence->effectN( 3 ).average( this ) );
    // Endurance seems to be using ceiling
    base.stats.attribute[ STAT_STAMINA ]   += util::ceil( racials.endurance->effectN( 1 ).average( this ) );

    base.spell_crit_chance        = dbc->spell_crit_base( type, level() ) +
                                    racials.viciousness->effectN( 1 ).percent() +
                                    racials.arcane_acuity->effectN( 1 ).percent();
    base.attack_crit_chance       = dbc->melee_crit_base( type, level() ) +
                                    racials.viciousness->effectN( 1 ).percent() +
                                    racials.arcane_acuity->effectN( 1 ).percent();
    if ( timeofday == DAY_TIME )
    {
      base.spell_crit_chance      += racials.touch_of_elune->effectN( 1 ).percent();
      base.attack_crit_chance     += racials.touch_of_elune->effectN( 1 ).percent();
    }
    base.spell_crit_per_intellect = dbc->spell_crit_scaling( type, level() );
    base.attack_crit_per_agility  = dbc->melee_crit_scaling( type, level() );
    base.mastery                  = 8.0 + racials.awakened->effectN( 1 ).base_value();
    base.versatility              = racials.mountaineer->effectN( 1 ).percent() +
                                    racials.brush_it_off->effectN( 1 ).percent();
    base.leech                    = 0.0;
    base.avoidance                = 0.0;

    base.base_armor_multiplier    *= ( 1.0 + racials.titanwrought_frame->effectN( 1 ).percent() );
    base.crit_damage_multiplier   *= ( 1.0 + racials.brawn->effectN( 1 ).percent() ) *
                                     ( 1.0 + racials.might_of_the_mountain->effectN( 1 ).percent() );
    base.crit_healing_multiplier  *= ( 1.0 + racials.brawn->effectN( 3 ).percent() ) *
                                     ( 1.0 + racials.might_of_the_mountain->effectN( 3 ).percent() );

    resources.base[ RESOURCE_HEALTH ] = dbc->health_base( type, level() );
    resources.base[ RESOURCE_MANA ]   = dbc->resource_base( type, level() );

    // 1% of base mana as mana regen per second for all classes.
    resources.base_regen_per_second[ RESOURCE_MANA ] = dbc->resource_base( type, level() ) * 0.01;

    // Automatically parse mana regen and max mana modifiers from class passives.
    for ( auto spell : dbc::class_passives( this ) )
    {
      for ( const spelleffect_data_t& effect : spell->effects() )
      {
        if ( effect.subtype() == A_MOD_MANA_REGEN_PCT )
        {
          resources.base_regen_per_second[ RESOURCE_MANA ] *= 1.0 + effect.percent();
        }
        if ( effect.subtype() == A_MOD_MAX_MANA_PCT || effect.subtype() == A_MOD_MANA_POOL_PCT )
        {
          resources.base_multiplier[ RESOURCE_MANA ] *= 1.0 + effect.percent();
        }
      }
    }

    base.health_per_stamina = dbc->health_per_stamina( level() );

    // players have a base 7.5% hit/exp
    base.hit       = 0.075;
    base.expertise = 0.075;

    if ( base.distance < 1 )
      base.distance = 5;

    // Armor Coefficient, based on level (1054 @ 50; 2500 @ 60-63)
    base.armor_coeff = dbc->armor_mitigation_constant( level() );
    sim->print_debug( "{} base armor coefficient set to {}.", *this, base.armor_coeff );
  }

  // initialize sp->ap and ap->sp overrides for hybrid specs
  if ( is_player() && spec_spell->ok() )
  {
    base.attack_power_per_spell_power =
        spell_data_t::find_spelleffect( *spec_spell, E_APPLY_AURA, A_OVERRIDE_AP_PER_SP ).percent();
    base.spell_power_per_attack_power =
        spell_data_t::find_spelleffect( *spec_spell, E_APPLY_AURA, A_OVERRIDE_SP_PER_AP ).percent();
  }

  // only certain classes get Agi->Dodge conversions, dodge_per_agility defaults to 0.00
  if ( type == MONK || type == DRUID || type == ROGUE || type == HUNTER || type == SHAMAN || type == DEMON_HUNTER )
    base.dodge_per_agility =
        dbc->avoid_per_str_agi_by_level( level() ) / 100.0;  // exact values given by Blizzard, only have L90-L100 data

  // only certain classes get Str->Parry conversions, parry_per_strength defaults to 0.00
  if ( type == PALADIN || type == WARRIOR || type == DEATH_KNIGHT )
    base.parry_per_strength =
        dbc->avoid_per_str_agi_by_level( level() ) / 100.0;  // exact values given by Blizzard, only have L90-L100 data

  // All classes get 3% dodge and miss
  base.dodge = 0.03;
  base.miss = 0.03;

  // Dodge from base agility isn't affected by diminishing returns and is added here
  if (base.dodge_per_agility > 0)
  {
    base.dodge += (dbc->race_base(race).agility + dbc->attribute_base(type, level()).agility) * base.dodge_per_agility;
  }

  // Night Elf dodge is additive
  base.dodge += racials.quickness->effectN(1).percent();

  // Only Warriors and Paladins (and enemies) can block, defaults to 0
  if ( type == WARRIOR || type == PALADIN || type == ENEMY || type == TANK_DUMMY )
  {
    // Base block chance is 3%, increased in warriors' and paladins' class aura and protection warrior's spec aura
    // Further increased by mastery for both Protection specs
    base.block = 0.03;

    // Set block reduction to 0 for warrior/paladin because it's computed in composite_block_reduction()
    switch ( type )
    {
    case WARRIOR:
    case PALADIN:
      base.block_reduction = 0;
      break;
    default:
      base.block_reduction = 0.30;
      break;
    }
  }

  // Only certain classes can parry, and get 3% base parry, default is 0
  // Parry from base strength isn't affected by diminishing returns and is added here
  if ( type == WARRIOR || type == PALADIN || type == ROGUE || type == DEATH_KNIGHT || type == MONK ||
       type == DEMON_HUNTER || specialization() == SHAMAN_ENHANCEMENT  )
  {
    base.parry = 0.03 + ( dbc->race_base( race ).strength + dbc->attribute_base( type, level() ).strength ) * base.parry_per_strength;
  }
  else if ( type == ENEMY || type == TANK_DUMMY )
  {
    base.parry = 0.03;
  }

  // Extract avoidance DR values from table in sc_extra_data.inc
  def_dr.horizontal_shift       = dbc->horizontal_shift( type );
  def_dr.block_vertical_stretch = dbc->block_vertical_stretch( type );
  def_dr.vertical_stretch       = dbc->vertical_stretch( type );
  def_dr.dodge_factor           = dbc->dodge_factor( type );
  def_dr.parry_factor           = dbc->parry_factor( type );
  def_dr.miss_factor            = dbc->miss_factor( type );
  def_dr.block_factor           = dbc->block_factor( type );

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

  if ( primary_role() == ROLE_TANK )
  {
    // Collect DTPS data for tanks even for statistics_level == 1
    if ( sim->statistics_level >= 1 )
      collected_data.dtps.change_mode( false );
  }

  sim->print_debug( "{} generic base stats: {}", *this, base );
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
      sim->error( "{} initial stat {} is {} != 0", *this, stat, stat_value );
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
    gear_stats_t item_stats = range::accumulate( items, gear_stats_t{}, &item_t::total_stats );

    for ( stat_e stat = STAT_NONE; stat < STAT_MAX; ++stat )
    {
      if ( gear.get_stat( stat ) < 0 )
        total_gear.add_stat( stat, item_stats.get_stat( stat ) );
      else
        total_gear.add_stat( stat, gear.get_stat( stat ) );
    }

    sim->print_debug( "{} total gear stats: {}", *this, total_gear );

    initial.stats += enchant;
    initial.stats += sim->enchant;

    // crit damage multiplier meta gems
    initial.crit_damage_multiplier *= util::crit_multiplier( meta_gem );
  }

  initial.stats += total_gear;

  sim->print_debug( "{} generic initial stats: {}", *this, initial );
}

void player_t::parse_temporary_enchants()
{
  std::string tench_str = temporary_enchant_str.empty() ? default_temporary_enchant() : temporary_enchant_str;

  if ( util::str_compare_ci( tench_str, "disabled" ) )
  {
    return;
  }

  auto split = util::string_split<util::string_view>( tench_str, "/" );
  for ( const auto& token : split )
  {
    std::string expr_str, options_str, value_str;
    std::unique_ptr<expr_t> if_expr;
    std::vector<std::unique_ptr<option_t>> options;

    options.emplace_back( opt_string( "if", expr_str ) );

    auto cut_pt = token.find_first_of( ',' );
    if ( cut_pt != std::string::npos )
    {
      options_str = token.substr( cut_pt + 1 );
      value_str = token.substr( 0, cut_pt );
    }
    else
    {
      value_str = token;
    }

    try
    {
      opts::parse( sim, value_str, options, options_str );

      if ( !expr_str.empty() )
      {
        if_expr = expr_t::parse( this, expr_str, false );
      }
    }
    catch ( const std::exception& e )
    {
      sim->error( "Player {} Unable to parse temporary enchant string str '{}': {}",
        name(), token, e.what() );
      sim->cancel();
      return;
    }

    auto token_split = util::string_split<util::string_view>( value_str, ":" );
    if ( token_split.size() != 2 )
    {
      sim->error( "Player {} invalid temporary enchant token {}, format is 'slot:name'",
        name(), token );
      continue;
    }
    auto slot = util::parse_slot_type( token_split[ 0 ] );
    util::string_view enchant_str = token_split[ 1 ];
    unsigned rank = 0;

    auto it = token_split[ 1 ].rfind( '_' );
    if ( it != std::string_view::npos )
    {
      auto rank_str = token_split[ 1 ].substr( it + 1 );
      auto parsed_rank = util::to_unsigned_ignore_error( rank_str,
                                                        std::numeric_limits<unsigned>::max() );

      if ( parsed_rank != std::numeric_limits<unsigned>::max() )
      {
        enchant_str = token_split[ 1 ].substr( 0, it );
        rank = parsed_rank;
      }
    }

    const auto& enchant = temporary_enchant_entry_t::find( enchant_str, rank, dbc->ptr );
    if ( slot == SLOT_INVALID || enchant.enchant_id == 0 )
    {
      sim->error( "Player {} unknown temporary enchant token '{}' (slot={}, enchant_id={})",
        name(), token, util::slot_type_string( slot ), enchant.enchant_id );
      continue;
    }

    items[ slot ].parsed.temporary_enchants.emplace_back( enchant.enchant_id, if_expr );
  }
}

void player_t::init_items()
{
  sim->print_debug( "Initializing items for {}.", *this );

  std::array<bool, SLOT_MAX> matching_gear_slots;  // true if the given item is equal to the highest armor type the player can wear
  for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
    matching_gear_slots[ i ] = !util::is_match_slot( i );

  // Override with item slot overrides. Note this will completely replace any player-scoped item options
  if ( is_player() )
  {
    for ( const auto& [ override_slot, override_str ] : sim->item_slot_overrides )
    {
      if ( auto slot = util::parse_slot_type( override_slot ); slot != SLOT_INVALID )
      {
        items[ slot ].options_str = override_str;
      }
      else
      {
        sim->error( "{} overriding unknown item slot '{}={}'; ignoring.", *this, override_slot, override_str );
      }
    }

    if ( load_default_gear )
    {
      uint32_t class_, spec_;
      if ( !dbc->spec_idx( specialization(), class_, spec_ ) )
      {
        sim->error( "{} cannot load default gear, invalid class and/or specialization.", *this );
        return;
      }

      for ( const auto& gear : character_loadout_data_t::data( class_, spec_, is_ptr() ) )
      {
        const auto& item = dbc->item( gear.id_item );
        auto inv_type = static_cast<inventory_type>( item.inventory_type );
        auto slot = util::translate_invtype( inv_type );

        if ( !items[ slot ].options_str.empty() )
        {
          if ( inv_type == INVTYPE_WEAPON && slot == SLOT_MAIN_HAND && items[ SLOT_OFF_HAND ].options_str.empty() )
            slot = SLOT_OFF_HAND;
          else if ( inv_type == INVTYPE_FINGER && slot == SLOT_FINGER_1 && items[ SLOT_FINGER_2 ].options_str.empty() )
            slot = SLOT_FINGER_2;
          else if ( inv_type == INVTYPE_TRINKET && slot == SLOT_TRINKET_1 && items[ SLOT_TRINKET_2 ].options_str.empty() )
            slot = SLOT_TRINKET_2;
          else
            continue;
        }

        items[ slot ].options_str =
            fmt::format( ",id={},ilevel={}", gear.id_item, character_loadout_data_t::default_item_level( is_ptr() ) );
      }
    }

    // Legendary Shadoweave Shirt used as base item for enable_all_item_effects
    if ( sim->enable_all_item_effects )
    {
      items[ SLOT_SHIRT ].options_str =
          fmt::format( ",id=45037,ilevel={}", character_loadout_data_t::default_item_level( is_ptr() ) );
    }
  }
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
    catch (const std::exception&)
    {
      std::throw_with_nested(std::runtime_error(fmt::format("Item '{}' Slot '{}'", item.name(), item.slot_name() )));
    }
  }

  parse_temporary_enchants();

  // Once item data is initialized, initialize the parent - child relationships of each item
  range::for_each( items, [this]( item_t& i ) {
    i.parent_slot = parent_item_slot( i );
  } );

  // Slot initialization order vector. Needed to ensure parents of children get initialized first
  std::vector<slot_e> init_slots;
  range::for_each( items, [&init_slots]( const item_t& i ) { init_slots.push_back( i.slot ); } );

  // Sort items with children before items without children
  range::sort( init_slots, [this]( slot_e first, slot_e second ) {
    const item_t &fi = items[ first ];
    const item_t &si = items[ second ];
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
    catch (const std::exception& )
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
    collected_data.health_changes.collect = true;

}

void player_t::init_weapon( weapon_t& w )
{
  sim->print_debug( "Initializing weapon ( type {} ) for {}.", w.type, *this );

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

    if ( effect.custom_init_object.empty() )
    {
      continue;
    }

    special_effects.push_back( new special_effect_t( effect ) );
  }

  if ( sim->enable_all_item_effects )
  {
    for ( auto id : unique_gear::thewarwithin::__tww_special_effect_ids )
    {
      if ( unique_gear::find_special_effect( this, id ) )
        continue;

      special_effect_t effect( &items[ SLOT_SHIRT ] );
      unique_gear::initialize_special_effect( effect, id );

      special_effects.push_back( new special_effect_t( effect ) );
    }
  }


  unique_gear::initialize_racial_effects( this );

  if ( sim->overrides.skyfury && may_benefit_from_skyfury() )
  {
    special_effect_t effect( this );

    unique_gear::initialize_special_effect( effect, 462854 );
    if ( !effect.custom_init_object.empty() )
    {
      special_effects.push_back( new special_effect_t( effect ) );
    }
  }

  // Initialize generic azerite powers. Note that this occurs later in the process than the class
  // module spell initialization (init_spells()), which is where the core presumes that each class
  // module gets the state their azerite powers (through the invocation of find_azerite_spells).
  // This means that any enabled azerite power that is not referenced in a class module will be
  // initialized here.
  azerite::initialize_azerite_powers( this );

  covenant::soulbinds::initialize_soulbinds( this );

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

void player_t::init_special_effect( special_effect_t& /* effect */ )
{
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

      sim->print_debug( "{} resource {} overridden to {}", *this, resource, actual_resource );

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
    if ( collected_data.resource_timelines.empty() )
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

  auto splits = util::string_split<util::string_view>( professions_str, ",/" );

  for ( auto& split : splits )
  {
    util::string_view prof_name;
    int prof_value = 0;

    auto subsplit = util::string_split<util::string_view>( split, "=" );
    if ( subsplit.size() == 2 )
    {
      prof_name = subsplit[ 0 ];
      try
      {
        prof_value = util::to_int( subsplit[ 1 ] );
      }
      catch ( const std::exception& )
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

std::vector<std::string> player_t::get_item_actions()
{
  std::vector<std::string> actions;

  for ( const auto& item : items )
  {
    // This will skip Addon and Enchant-based on-use effects. Addons especially are important to
    // skip from the default APLs since they will interfere with the potion timer, which is almost
    // always preferred over an Addon.
    if ( item.has_special_effect( SPECIAL_EFFECT_SOURCE_ITEM, SPECIAL_EFFECT_USE ) )
    {
      actions.push_back( fmt::format( "use_item,slot={}", item.slot_name() ) );
    }
  }

  return actions;
}

std::vector<std::string> player_t::get_profession_actions()
{
  std::vector<std::string> actions;

  return actions;
}

std::vector<std::string> player_t::get_racial_actions()
{
  std::vector<std::string> actions;

  actions.emplace_back( "blood_fury" );
  actions.emplace_back( "berserking" );
  actions.emplace_back( "arcane_torrent" );
  actions.emplace_back( "lights_judgment" );
  actions.emplace_back( "fireblood" );
  actions.emplace_back( "ancestral_call" );
  actions.emplace_back( "bag_of_tricks" );

  return actions;
}

/**
 * Add action to action list.
 *
 * Helper function to add actions with spell data of the same name to the action list,
 * and check if that spell data is ok()
 * returns true if spell data is ok(), false otherwise
 */
bool player_t::add_action( util::string_view action, util::string_view options, util::string_view alist )
{
  return add_action( find_class_spell( action ), options, alist );
}

/**
 * Add action to action list
 *
 * Helper function to add actions to the action list if given spell data is ok()
 * returns true if spell data is ok(), false otherwise
 */
bool player_t::add_action( const spell_data_t* s, util::string_view options, util::string_view alist )
{
  if ( s->ok() )
  {
    std::string& str =
        ( alist == "default" ) ? action_list_str : ( get_action_priority_list( alist )->action_list_str );
    auto name = util::tokenize_fn( s->name_cstr() );
    str += "/" + name;
    if ( !options.empty() )
    {
      str += ",";
      str.append( options.data(), options.size() );
    }
    return true;
  }
  return false;
}

void player_t::add_option( std::unique_ptr<option_t> o )
{
  // Push_front so derived classes (eg. enemy_t) can override existing options
  options.insert( options.begin(), std::move( o ) );
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

static void parse_traits( talent_tree tree, const std::string& opt_str, player_t* player )
{
  auto talents = util::string_split<std::string_view>( opt_str, "/" );
  for ( const auto talent : talents )
  {
    auto talent_split = util::string_split<std::string_view>( talent, ":" );
    if ( talent_split.size() != 2 )
    {
      player->sim->error( "Invalid talent string {}", talent );
      player->sim->cancel();
      return;
    }

    bool is_spell_id = false;
    auto entry_id = 0U;
    if ( !talent_split[ 0 ].empty() && ( talent_split[ 0 ][ 0 ] == 's' || talent_split[ 0 ][ 0 ] == 'S' ) )
    {
      entry_id = util::to_unsigned_ignore_error( talent_split[ 0 ].substr( 1 ), 0 );
      is_spell_id = true;
    }
    else
    {
      entry_id = util::to_unsigned_ignore_error( talent_split[ 0 ], 0 );
    }

    auto ranks = util::to_unsigned( talent_split[ 1 ] );
    const trait_data_t* trait_obj = nullptr;
    if ( entry_id != 0 )
    {
      if ( is_spell_id )
      {
        auto objs = trait_data_t::find_by_spell( tree, entry_id, util::class_id( player->type ),
                                                 player->specialization(), player->dbc->ptr );
        if ( objs.empty() )
        {
          trait_obj = &( trait_data_t::nil() );
        }
        else if ( objs.size() > 1U )
        {
          player->sim->error( "Multiple talents for spell id {} found", talent_split[ 0 ].substr( 1 ) );
          player->sim->cancel();
        }
        else
        {
          trait_obj = objs[ 0 ];
        }
      }
      else
      {
        trait_obj = trait_data_t::find( entry_id, player->dbc->ptr );
      }
    }
    else
    {
      trait_obj = trait_data_t::find_tokenized( tree, talent_split[ 0 ], util::class_id( player->type ),
                                                player->specialization(), player->dbc->ptr );
    }

    if ( trait_obj->id_spell == 0 )
    {
      player->sim->error( "Unable to find talent {}", talent_split[ 0 ] );
      player->sim->cancel();
    }
    else
    {
      auto it = range::find_if( player->player_traits, [ trait_obj ]( const auto& entry ) {
        return std::get<1>( entry ) == trait_obj->id_trait_node_entry;
      } );

      auto id_entry = trait_obj->id_trait_node_entry;
      auto entry = std::make_tuple( tree, id_entry, std::min( ranks, trait_obj->max_ranks ) );

      if ( it != player->player_traits.end() )
      {
        if ( std::get<2>( *it ) != std::get<2>( entry ) )
        {
          player->sim->print_log( "Overwriting talent {} ({}), rank {} -> {}", trait_obj->name, id_entry,
                                  std::get<2>( *it ), std::get<2>( entry ) );
        }

        *it = entry;
      }
      else
      {
        player->player_traits.push_back( entry );
        player->sim->print_debug( "{} adding {} talent {}", *player, util::talent_tree_string( tree ),
                                  trait_obj->name );
      }

      if ( tree == talent_tree::HERO )
      {
        player->player_sub_traits.push_back( id_entry );

        if( !player->player_sub_trees.count( trait_obj->id_sub_tree ) )
        {
          player->player_sub_trees.insert( trait_obj->id_sub_tree );
          player->sim->print_debug( "{} activating sub tree {} ({})", *player,
                                    trait_data_t::get_hero_tree_name( trait_obj->id_sub_tree, player->is_ptr() ),
                                    trait_obj->id_sub_tree );
        }
      }
    }
  }

  // add any freely granted traits
  for ( const auto& trait : trait_data_t::data( util::class_id( player->type ), tree, player->is_ptr() ) )
  {
    if ( trait_data_t::is_granted( &trait, player->type, player->specialization(), player->is_ptr() ) )
    {
      auto id = trait.id_trait_node_entry;
      auto it = range::find_if( player->player_traits, [ id ]( const auto& e ) { return std::get<1>( e ) == id; } );
      if ( it == player->player_traits.end() )
        player->player_traits.emplace_back( tree, id, 1 );
    }
  }
}

// Blizzard in-game talent tree export hash
static bool generate_tree_nodes( player_t* player,
                                 std::map<unsigned, std::vector<std::pair<const trait_data_t*, unsigned>>>& tree_nodes )
{
  specialization_e spec = player->specialization();

  uint32_t class_idx, spec_idx;
  if ( !player->dbc->spec_idx( spec, class_idx, spec_idx ) )
    return false;

  for ( auto i = talent_tree::INVALID; i != talent_tree::MAX; i++ )
  {
    for ( const auto& trait : trait_data_t::data( class_idx, i, maybe_ptr( player->dbc->ptr ) ) )
      tree_nodes[ trait.id_node ].emplace_back( &trait, 0 );
  }

  return true;
}

// Different entries within the same node are allowed to have non-unique selection indices.
// Manually resolve such conflicts here.
// ***THIS WILL NEED TO BE CONFIRMED AND UPDATED EVERY NEW BUILD***
static bool sort_node_entries( const trait_data_t* a, const trait_data_t* b, bool /* is_ptr */ )
{
  auto get_index = [ = ]( const trait_data_t* t ) -> short {
    if ( t->selection_index == -1 )
    {
      // Voidweaver Devour Matter / Darkening Horizon clash resolution
      // Darkening Horizon data was not fully removed after being moved out of the node
      // The lower ID trait is the correct one; return lower index to get it sorted first
      if ( t->id_trait_node_entry == 117271 )
        return 1;
      else if ( t->id_trait_node_entry == 117298 )
        return 2;
    }

    return t->selection_index;
  };

  auto a_idx = get_index( a );
  auto b_idx = get_index( b );

  if ( a_idx != -1 && b_idx != -1 )
    return a_idx < b_idx;
  else
    return a->id_trait_node_entry > b->id_trait_node_entry;
}

namespace
{
// MakeBase64ConversionTable() from Interface/AddOns/Blizzard_SharedXMLBase/ExportUtil.lua
const std::string base64_char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
// hardcoded values from Interface/AddOns/Blizzard_PlayerSpells/ClassTalents/Blizzard_ClassTalentImportExport.lua
constexpr unsigned LOADOUT_SERIALIZATION_VERSION = 2;
constexpr size_t version_bits = 8;    // serialization version
constexpr size_t spec_bits    = 16;   // specialization id
constexpr size_t tree_bits    = 128;  // C_Traits.GetTreeHash(), optionally can be 0-filled
constexpr size_t rank_bits    = 6;    // ranks purchased if node is partially filled
constexpr size_t choice_bits  = 2;    // choice index, 0-based
// hardcoded value from Interface/AddOns/Blizzard_SharedXMLBase/ExportUtil.lua
constexpr size_t byte_size    = 6;
}

static std::string generate_traits_hash( player_t* player )
{
  std::string export_str;
  auto ptr = player->is_ptr();

  if ( player->player_traits.empty() )
    return export_str;

  size_t head = 0;
  size_t byte = 0;
  auto put_bit = [ &export_str, &head, &byte ]( size_t bits, unsigned value ) {
    for ( size_t i = 0; i < bits; i++ )
    {
      size_t bit = head % byte_size;
      head++;
      byte += ( ( value >> std::min( i, sizeof( value ) * 8 - 1 ) ) & 0b1 ) << bit;
      if ( bit == byte_size - 1 )
      {
        export_str += base64_char[ byte ];
        byte = 0;
      }
    }
  };

  put_bit( version_bits, LOADOUT_SERIALIZATION_VERSION );
  put_bit( spec_bits, static_cast<unsigned>( player->specialization() ) );
  put_bit( tree_bits, 0 );  // 0-filled to bypass validation, as GetTreeHash() is unavailable externally

  std::map<unsigned, std::vector<std::pair<const trait_data_t*, unsigned>>> tree_nodes;

  generate_tree_nodes( player, tree_nodes );

  // populate tree_nodes with rank info from player_traits
  for ( const auto& trait : player->player_traits )
  {
    auto id_entry = std::get<1>( trait );
    auto ranks = std::get<2>( trait );

    if ( !ranks )
      continue;

    auto trait_data = trait_data_t::find( id_entry, ptr );
    auto tree_entry = &tree_nodes[ trait_data->id_node ];

    for ( auto& entry : *tree_entry )
    {
      if ( entry.first->id_trait_node_entry == id_entry )
      {
        entry.second = ranks;
        break;
      }
    }
  }

  for ( auto& [ id, node ] : tree_nodes )
  {
    if ( node.size() > 1 )
    {
      range::sort( node, [ ptr ]( const auto& a, const auto& b ) {
        return sort_node_entries( a.first, b.first, ptr );
      } );
    }

    const trait_data_t* trait = nullptr;
    unsigned rank = 0;
    unsigned index = 0;
    bool is_choice = node.size() > 0 ? node[ 0 ].first->node_type == 2 || node[ 0 ].first->node_type == 3 : false;

    for ( size_t i = 0; i < node.size(); i++ )
    {
      rank = node[ i ].second;
      if ( rank )
      {
        trait = node[ i ].first;
        index = as<unsigned>( i );
        break;
      }
    }

    if ( rank )  // is node selected?
    {
      put_bit( 1, 1 );
    }
    else
    {
      put_bit( 1, 0 );
      continue;
    }

    // is node purchased? granted nodes are baseline 1 rank.
    if ( rank > ( trait_data_t::is_granted( trait, player->type, player->specialization(), ptr ) ? 1U : 0U ) )
    {
      put_bit( 1, 1 );
    }
    else
    {
      put_bit( 1, 0 );
      continue;
    }

    if ( rank == trait->max_ranks )  // is node partially ranked?
    {
      put_bit( 1, 0 );
    }
    else
    {
      put_bit( 1, 1 );
      put_bit( rank_bits, rank );
    }

    if ( is_choice )  // is choice node?
    {
      put_bit( 1, 1 );
      put_bit( choice_bits, index );
    }
    else
    {
      put_bit( 1, 0 );
    }
  }

  if ( head % byte_size )
    export_str += base64_char[ byte ];

  return export_str;
}

static void parse_traits_hash( const std::string& talents_str, player_t* player )
{
  auto do_error = [ player, &talents_str ]( std::string_view msg = {} ) {
    player->sim->error( "Player {} has invalid talent tree hash {}{}{}", player->name(), talents_str, msg.empty() ? "" : ": ", msg );
  };

  if ( talents_str.find_first_not_of( base64_char ) != std::string::npos )
  {
    do_error();
    return;
  }

  if ( version_bits + spec_bits + tree_bits > talents_str.size() * byte_size )
  {
    do_error( "Not enough characters" );
    return;
  }

  size_t head = 0;
  size_t byte = base64_char.find( talents_str[ 0 ] );
  auto get_bit = [ &talents_str, &head, &byte ]( size_t bits ) {
    size_t val = 0;
    for ( size_t i = 0; i < bits; i++ )
    {
      size_t bit = head % byte_size;
      head++;
      val += ( byte >> bit & 0b1 ) << std::min( i, sizeof( byte ) * 8 - 1 );
      if ( bit == byte_size - 1 )
      {
        if ( ( head / byte_size ) >= talents_str.size() )
          byte = 0;
        else
          byte = base64_char.find( talents_str[ head / byte_size ] );
      }
    }
    return val;
  };

  auto version_id = get_bit( version_bits );
  auto spec_id    = get_bit( spec_bits );

  if ( version_id != LOADOUT_SERIALIZATION_VERSION )
  {
    do_error( "Invalid serialization version" );
    return;
  }

  if ( spec_id != player->specialization() )
  {
    do_error( "Wrong specialization" );
    return;
  }

  // Clear all existing traits
  player->player_traits.clear();
  player->player_sub_trees.clear();
  player->player_sub_traits.clear();

  // As per Interface/AddOns/Blizzard_ClassTalentUI/Blizzard_ClassTalentImportExport.lua: treeHash is a 128bit hash,
  // passed as an array of 16, 8-bit values. For SimC purposes we can ignore it, as invalid/outdated strings can error
  // in later checks
  get_bit( tree_bits );

  std::map<unsigned, std::vector<std::pair<const trait_data_t*, unsigned>>> tree_nodes;

  generate_tree_nodes( player, tree_nodes );

  for ( auto& [ id, node ] : tree_nodes )
  {
    if ( get_bit( 1 ) )  // selected
    {
      // it is possible to have multiple entries per node that are not choice node.
      // default assumption is that the higher trait entry id is used.
      // if this is not the case, clashes must be resolved manually in sort_node_entries().
      if ( node.size() > 1 )
      {
        range::sort( node, [ player ]( std::pair<const trait_data_t*, unsigned> a, std::pair<const trait_data_t*, unsigned> b ) {
          return sort_node_entries( a.first, b.first, player->is_ptr() );
        } );
      }

      auto trait = node.front().first;
      size_t rank = trait->max_ranks;
      auto _tree = static_cast<talent_tree>( trait->tree_index );

      // hero talents don't seem to require a matching id_spec_set
      // TODO: utilize logic in trait_data_t::is_granted() to check against id_spec_set of the subtree selection trait
      if ( _tree != talent_tree::HERO &&
           !std::all_of( trait->id_spec.begin(), trait->id_spec.end(), []( unsigned i ) { return i == 0; } ) &&
           !range::contains( trait->id_spec, player->specialization() ) )
      {
        do_error( fmt::format( "selected node {} is not available to player's spec.", id ) );
        return;
      }

      if ( !get_bit( 1 ) )  // purchased
      {
        rank = 1;  // non-purchased nodes are granted at rank 1
      }
      else
      {
        if ( get_bit( 1 ) )  // partially ranked normal trait
        {
          if ( node.size() > 1 )
          {
            do_error( fmt::format( "non-choice node {} has multiple entries.", id ) );
            return;
          }

          rank = get_bit( rank_bits );

          if ( rank > trait->max_ranks )
          {
            do_error( fmt::format( "{} ranks selected for node {}, {} ranks max.", rank, id, trait->max_ranks ) );
            return;
          }

          if ( rank == trait->max_ranks )
          {
            do_error( fmt::format( "partial rank for node {} but all {} ranks are allocated.", id, rank ) );
            return;
          }
        }

        if ( get_bit( 1 ) )  // choice trait
        {
          if ( node[ 0 ].first->node_type != 2 && node[ 0 ].first->node_type != 3 )
          {
            do_error( fmt::format( "node {} is not a choice node but has index selection.", id ) );
            return;
          }

          size_t index = get_bit( choice_bits );
          if ( index >= node.size() )
          {
            do_error( fmt::format( "index {} for choice node {} out of bounds.", index, id ) );
            return;
          }

          trait = node[ index ].first;
        }
      }

      player->player_traits.emplace_back( _tree, trait->id_trait_node_entry, as<unsigned>( rank ) );

      if ( _tree == talent_tree::SELECTION )
      {
        player->player_sub_trees.insert( trait->id_sub_tree );

        player->sim->print_debug( "{} activating sub tree {} ({})", *player,
                                  trait_data_t::get_hero_tree_name( trait->id_sub_tree, player->is_ptr() ),
                                  trait->id_sub_tree );
      }
      else
      {
        player->sim->print_debug( "{} adding {} talent {}", *player, util::talent_tree_string( _tree ), trait->name );
      }
    }
  }
}

static void enable_all_talents( player_t* player )
{
  std::map<unsigned, std::vector<std::pair<const trait_data_t*, unsigned>>> tree_nodes;

  generate_tree_nodes( player, tree_nodes );

  for ( auto& [ id, node ] : tree_nodes )
  {
    for ( auto& entry : node )
    {
      auto trait = entry.first;

      // assume 'off-screen' nodes are invalid
      if ( trait->row <= 0 || trait->col <= 0 || trait->max_ranks <= 0 )
        continue;

      if ( std::all_of( trait->id_spec.begin(), trait->id_spec.end(), []( unsigned i ) { return i == 0; } ) ||
           range::contains( trait->id_spec, player->specialization() ) ||
           trait_data_t::is_hero_trait_available( trait, player->type, player->specialization(), player->is_ptr() ) )
      {
        auto _tree = static_cast<talent_tree>( trait->tree_index );

        player->player_traits.emplace_back( _tree, trait->id_trait_node_entry, trait->max_ranks );

        if ( _tree == talent_tree::SELECTION )
        {
          player->player_sub_trees.insert( trait->id_sub_tree );
          player->sim->print_debug( "{} activating sub tree {} ({})", *player,
                                    trait_data_t::get_hero_tree_name( trait->id_sub_tree, player->is_ptr() ),
                                    trait->id_sub_tree );
        }
        else
        {
          player->sim->print_debug( "{} adding {} talent {}", *player, util::talent_tree_string( _tree ), trait->name );
        }
      }
    }
  }
}

static void enable_default_talents( player_t* player )
{
  player->sim->print_debug( "Loading default talents for {}.", *player );

  auto traits = trait_loadout_data_t::data( player->specialization(), player->is_ptr() );

  for ( size_t i = 0; i < traits.size(); i++ )
  {
    if ( ( i + 1 ) < traits.size() && traits[ i ].id_trait_node_entry == traits[ i + 1 ].id_trait_node_entry )
      i++;

    auto trait = trait_data_t::find( traits[ i ].id_trait_node_entry, player->is_ptr() );

    if ( !trait->id_node )
    {
      player->sim->error( "{} default talent not found: id_trait_node_entry={} order={}", *player,
                          traits[ i ].id_trait_node_entry, traits[ i ].order );
    }
    else
    {
      auto tree = static_cast<talent_tree>( trait->tree_index );

      player->player_traits.emplace_back( tree, traits[ i ].id_trait_node_entry, traits[ i ].rank );
      player->sim->print_debug( "{} adding {} talent {} ({})", *player, util::talent_tree_string( tree ), trait->name,
                                traits[ i ].rank );
    }
  }
}

void player_t::init_talents()
{
  sim->print_debug( "Initializing talents for {}.", *this );

  if ( !is_player() )
  {
    sim->print_debug( "Not a player, halting further talent initialization." );
    return;
  }

  if ( sim->enable_all_talents )
  {
    enable_all_talents( this );
  }
  else if ( load_default_talents )
  {
    enable_default_talents( this );
  }
  else if ( !talents_str.empty() )
  {
    parse_traits_hash( talents_str, this );
  }

  auto parsed_sub_trees = player_sub_trees;

  parse_traits( talent_tree::CLASS, class_talents_str, this );
  parse_traits( talent_tree::SPECIALIZATION, spec_talents_str, this );
  parse_traits( talent_tree::HERO, hero_talents_str, this );

  // Add selection traits for any manually added hero traits from new trees
  if ( player_sub_trees.size() > parsed_sub_trees.size() )
  {
    std::vector<unsigned> diff;
    std::set_difference( player_sub_trees.begin(), player_sub_trees.end(),
                        parsed_sub_trees.begin(), parsed_sub_trees.end(), std::back_inserter( diff ) );

    for ( const auto& trait : trait_data_t::data( util::class_id( type ), talent_tree::SELECTION, is_ptr() ) )
      if ( range::contains( trait.id_spec, specialization() ) && range::contains( diff, trait.id_sub_tree ) )
        player_traits.emplace_back( talent_tree::SELECTION, trait.id_trait_node_entry, 1 );
  }

  if ( talents_str.empty() || !class_talents_str.empty() || !spec_talents_str.empty() || !hero_talents_str.empty() )
  {
    talents_str = generate_traits_hash( this );
  }

  // Generate talent effect overrides based on parsed trait information
  for ( const auto& player_trait : player_traits )
  {
    auto trait_node_entry_id = std::get<1>( player_trait );
    auto rank = std::get<2>( player_trait );
    if ( rank == 0U )
    {
      continue;
    }

    const auto trait = trait_data_t::find( trait_node_entry_id, is_ptr() );
    assert( trait );
    const auto effect_points = trait_definition_effect_entry_t::find( trait->id_trait_definition, is_ptr() );
    auto spell = dbc::find_spell( this, trait->id_spell );

    if ( spell->id() != trait->id_spell )
    {
      sim->print_debug( "{} talent {} (spell_id={}, trait_node_entry_id={}) rank {} "
                        "trait spell not found",
        name(), trait->name, spell->id(), trait->id_trait_node_entry, rank );
      continue;
    }

    for ( const auto& effect_point : effect_points )
    {
      auto eff_idx = effect_point.effect_index + 1U;
      // Skip if more effect point entries exist than there are effects
      if ( spell->effect_count() < eff_idx )
        continue;

      auto effect_id = spell->effectN( eff_idx ).id();
      // Don't adjust already overridden effects, as those are defined by player options from the
      // command line (i.e., using override.spell_data or override.player.spell_data options).
      if ( dbc_override_->is_overridden_effect( *dbc, effect_id, "base_value" ) )
      {
          sim->print_debug( "{} talent {} (spell_id={}, trait_node_entry_id={}) rank {} "
                            "effect id {} (index={}) manually overridden, ignoring",
            name(), trait->name, spell->id(), trait->id_trait_node_entry, rank, effect_id,
            effect_point.effect_index + 1U );
        continue;
      }

      if ( effect_point.effect_index + 1U > spell->effect_count() )
      {
        sim->print_debug( "{} talent {} (spell_id={}, trait_node_entry_id={}) rank {} "
                          "effect id {} (index={}) effect index out of bounds, max effects {}",
          name(), trait->name, spell->id(), trait->id_trait_node_entry, rank, effect_id,
          effect_point.effect_index + 1U , spell->effect_count() );
        continue;
      }

      auto curve_data = curve_point_t::find( effect_point.id_curve, is_ptr() );
      auto value = 0.0f;
      auto it = range::find_if( curve_data, [rank]( const auto& point ) {
        return point.primary1 == as<float>( rank );
      } );

      if ( it == curve_data.end() )
      {
        sim->print_debug( "{} talent {} (spell_id={}, trait_node_entry_id={}) rank {} "
                          "effect id {} (index={}) curve data for rank not found for curve {}",
          name(), trait->name, spell->id(), trait->id_trait_node_entry, rank, effect_id,
          effect_point.effect_index + 1U, effect_point.id_curve );
        continue;
      }

      value = it->primary2;

      switch ( effect_point.operation )
      {
        case TRAIT_OP_NONE:
          sim->print_debug( "{} not affecting talent {} (spell_id={}, trait_node_entry_id={}) rank {} "
                            "effect id {} (index={}), effect={}, curve={}",
            name(), trait->name, spell->id(), trait->id_trait_node_entry, rank, effect_id,
            effect_point.effect_index + 1U,
            spell->effectN( effect_point.effect_index + 1U ).base_value(), value );
            break;
        case TRAIT_OP_SET:
          sim->print_debug( "{} setting talent {} (spell_id={}, trait_node_entry_id={}) rank {} "
                            "effect id {} (index={}) value, old={}, new={}",
            name(), trait->name, spell->id(), trait->id_trait_node_entry, rank, effect_id,
            effect_point.effect_index + 1U,
            spell->effectN( effect_point.effect_index + 1U ).base_value(), value );
          dbc_override_->register_effect( *dbc, effect_id, "base_value", value );
          break;
        case TRAIT_OP_MUL:
          sim->print_debug( "{} multiplying talent {} (spell_id={}, trait_node_entry_id={}) rank {} "
                            "effect id {} (index={}) value, multiplier={}, old={}, new={}",
            name(), trait->name, spell->id(), trait->id_trait_node_entry, rank, effect_id,
            effect_point.effect_index + 1U, value,
            spell->effectN( effect_point.effect_index + 1U ).base_value(),
            spell->effectN( effect_point.effect_index + 1U ).base_value() * value );
          dbc_override_->register_effect( *dbc, effect_id, "base_value",
              spell->effectN( effect_point.effect_index + 1U ).base_value() * value );
          break;
        default:
          assert( 0 );
      }
    }
  }
}

void player_t::init_spells()
{
  sim->print_debug( "Initializing spells for {}.", *this );

  // Racials
  racials.quickness             = find_racial_spell( "Quickness" );
  racials.elusiveness           = find_racial_spell( "Elusiveness" );
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
  racials.awakened              = find_racial_spell( "Awakened" );
  racials.azerite_surge         = find_racial_spell( "Azerite Surge" );
  racials.titanwrought_frame    = find_racial_spell( "Titan-Wrought Frame" );

  if ( is_player() )
  {
    spec_spell = find_spell( util::specialization_string( specialization() ) );

    const spell_data_t* s = find_mastery_spell( specialization() );
    if ( s->ok() )
      _mastery = &( s->effectN( 1 ) );
  }

}

void player_t::init_gains()
{
  sim->print_debug( "Initializing gains for {}.", *this );

  for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
  {
    std::string name = util::resource_type_string( r );
    name += "_regen";
    gains.resource_regen[ r ] = get_gain( util::inverse_tokenize( name ) );
  }

  if ( !is_pet() )
    gains.health = get_gain( "external_healing" );

  gains.vampiric_embrace = get_gain( "vampiric_embrace" );
}

void player_t::init_procs()
{
  sim->print_debug( "Initializing procs for {}.", *this );

  procs.parry_haste = get_proc( "parry_haste" );
  procs.delayed_aa_cast = get_proc( "delayed_aa_cast" );
  procs.delayed_aa_channel = get_proc( "delayed_aa_channel" );
  procs.reset_aa_cast = get_proc( "reset_aa_cast" );
  procs.reset_aa_channel = get_proc( "reset_aa_channel" );
}

void player_t::init_uptimes()
{
  sim->print_debug( "Initializing uptimes for {}.", *this );

  if ( primary_resource() )
  {
    uptimes.primary_resource_cap =
      get_uptime( util::inverse_tokenize( util::resource_type_string( primary_resource() ) ) + " Cap" );
  }
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
    if ( collected_data.stat_timelines.empty() )
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

void player_t::init_background_actions()
{
  if ( !is_enemy() )
  {
    if ( record_healing() )
    {
      spells.leech = new leech_t( this );
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

  std::vector<util::string_view> skip_actions;
  if ( !action_list_skip.empty() )
  {
    sim->print_debug( "{} action_list_skip={}", *this, action_list_skip );

    skip_actions = util::string_split<util::string_view>( action_list_skip, "/" );
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
      for ( auto& split : util::string_split<util::string_view>( apl->action_list_str, "/" ) )
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
        auto pet_name   = action_name.substr( 0, cut_pt );
        auto pet_action = util::tokenize_fn( action_name.substr( cut_pt + 1 ) );

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
    catch (const std::exception&)
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

        auto it = range::find_if( off_gcd_cd, [action]( std::pair<const cooldown_t*, const cooldown_t*> cds ) {
          return cds.first == action->cooldown && cds.second == action->internal_cooldown; }
        );

        if ( it == off_gcd_cd.end() )
        {
          off_gcd_cd.emplace_back( action->cooldown, action->internal_cooldown );
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

        auto it = range::find_if( cast_while_casting_cd, [action]( std::pair<const cooldown_t*, const cooldown_t*> cds ) {
          return cds.first == action->cooldown && cds.second == action->internal_cooldown; }
        );

        if ( it == cast_while_casting_cd.end() )
        {
          cast_while_casting_cd.emplace_back( action->cooldown, action->internal_cooldown );
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
  assessor_out_damage.add( assessor::TARGET_MITIGATION, []( result_amount_type dmg_type, action_state_t* state ) {
    state->target->assess_damage( state->action->get_school(), dmg_type, state );
    return assessor::CONTINUE;
  } );

  // Target damage
  assessor_out_damage.add( assessor::TARGET_DAMAGE, []( result_amount_type, action_state_t* state ) {
    state->target->do_damage( state );
    return assessor::CONTINUE;
  } );

  // Logging and debug .. Technically, this should probably be in action_t::assess_damage, but we
  // don't need this piece of code for the vast majority of sims, so it makes sense to yank it out
  // completely from there, and only conditionally include it if logging/debugging is enabled.
  if ( sim->log || sim->debug || !sim->debug_seed.empty() )
  {
    assessor_out_damage.add( assessor::LOG, [this]( result_amount_type type, action_state_t* state ) {
      if ( sim->debug )
      {
        state->debug();
      }

      if ( sim->log )
      {
        if ( type == result_amount_type::DMG_DIRECT )
        {
          sim->print_log( "{} {} hits {} for {} {} damage ({})", *this, state->action->name(),
                               *state->target, state->result_amount,
                               state->action->get_school(), state->result );
        }
        else  // result_amount_type::DMG_OVER_TIME
        {
          dot_t* dot = state->action->get_dot( state->target );
          sim->print_log( "{} {} ticks ({} of {}) on {} for {} {} damage ({})", *this, state->action->name(),
                               dot->current_tick, dot->num_ticks(), *state->target, state->result_amount,
                               state->action->get_school(), state->result );
        }
      }
      return assessor::CONTINUE;
    } );
  }

  // Leech, if the player has leeching enabled (disabled by default)
  if ( spells.leech )
  {
    assessor_out_damage.add( assessor::LEECH, [ this ]( result_amount_type, action_state_t* state ) {
      // Leeching .. sanity check that the result type is a damaging one, so things hopefully don't
      // break in the future if we ever decide to not separate heal and damage assessing.
      double leech_pct = 0;

      if ( ( state->result_type == result_amount_type::DMG_DIRECT ||
             state->result_type == result_amount_type::DMG_OVER_TIME ) &&
           state->result_amount > 0 && ( leech_pct = state->action->composite_leech( state ) ) > 0 )
      {
        leech_pool += leech_pct * state->result_amount;
      }

      return assessor::CONTINUE;
    } );
  }

  // Generic actor callbacks
  assessor_out_damage.add( assessor::CALLBACKS, [this]( result_amount_type, action_state_t* state ) {
    if ( !state->action->callbacks )
    {
      return assessor::CONTINUE;
    }

    proc_types pt   = state->proc_type();
    proc_types2 pt2 = state->impact_proc_type2();
    if ( pt != PROC1_INVALID && pt2 != PROC2_INVALID )
      trigger_callbacks( pt, pt2, state->action, state );

    return assessor::CONTINUE;
  } );
}

void player_t::init_finished()
{
  // Add dynamic cooldowns first before action_t::init_finished so actions can adjust their behavior accordingly if
  // necessary.
  range::for_each( cooldown_list, [ this ]( cooldown_t* c ) {
    if ( c->hasted )
    {
      dynamic_cooldown_list.push_back( c );
    }
  } );

  for ( auto action : action_list )
  {
    try
    {
      action_init_finished( *action );
    }
    catch ( const std::exception& )
    {
      std::throw_with_nested( std::runtime_error( fmt::format( "Action '{}'", action->name() ) ) );
    }
  }

  for ( auto action : action_list )
  {
    try
    {
      action->init_finished();
    }
    catch ( const std::exception& )
    {
      std::throw_with_nested( std::runtime_error( fmt::format( "Action '{}'", action->name() ) ) );
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

  // Sort outbound assessors
  assessor_out_damage.sort();

  // Print items to debug log
  if ( sim->debug )
  {
    range::for_each( items, [ this ]( const item_t& item ) {
      if ( item.active() )
      {
        sim->print_debug( "{}", item );
      }
    } );
  }

  if ( !precombat_state_map.empty() )
  {
    sim->error(
      "Warning: The 'override.precombat_state' option may not be fully supported for all buffs and cooldowns and may "
      "produce incorrect or misleading results." );

    struct buff_state_t
    {
      int stacks = -1;
      double value = buff_t::DEFAULT_VALUE();
      timespan_t duration = timespan_t::min();
    };

    std::unordered_map<buff_t*, buff_state_t> precombat_buff_state;
    auto update_buff_state = [ this, &precombat_buff_state ]( std::string_view buff_name, std::string_view type,
                                                              std::string_view value ) {
      buff_t* buff = buff_t::find( this, buff_name );
      if ( !buff )
      {
        sim->error( "No buff found for 'override.precombat_state' buff expression: '{}'", buff_name );
        return;
      }

      if ( type == "stack" )
        precombat_buff_state[ buff ].stacks = util::to_int( value );
      else if ( type == "value" )
        precombat_buff_state[ buff ].value = util::to_double( value );
      else if ( type == "remains" )
        precombat_buff_state[ buff ].duration = timespan_t::from_seconds( util::to_double( value ) );
      else
        throw std::invalid_argument(
            fmt::format( "Invalid 'override.precombat_state' buff expression type: '{}'", type ) );
    };

    for ( const auto& v : precombat_state_map )
    {
      auto splits = util::string_split<std::string_view>( v.first, "." );

      if ( splits.size() < 2 )
      {
        sim->error( "Invalid 'override.precombat_state' option: '{}'", v.first );
        continue;
      }

      auto type = splits[ 0 ];
      auto name = splits[ 1 ];

      if ( type == "buff" )
      {
        if ( splits.size() != 3 )
        {
          throw std::invalid_argument(
              fmt::format( "Invalid 'override.precombat_state' buff expression: '{}'", v.first ) );
        }

        update_buff_state( name, splits[ 2 ], v.second );
      }
      else if ( type == "cooldown" )
      {
        if ( splits.size() != 2 )
        {
          throw std::invalid_argument(
              fmt::format( "Invalid 'override.precombat_state' cooldown expression: '{}'", v.first ) );
        }

        auto cd = find_cooldown( name );
        if ( !cd )
        {
          sim->error( "No cooldown found for 'override.precombat_state' cooldown expression: '{}'", name );
          continue;
        }

        timespan_t duration = timespan_t::from_seconds( util::to_double( v.second ) );
        add_precombat_cooldown_state( cd, duration );
      }
      else
      {
        throw std::invalid_argument(
            fmt::format( "Invalid type '{}' for 'override.precombat_state' option.", type ) );
      }
    }

    for ( const auto& [ buff, buff_state ] : precombat_buff_state )
    {
      add_precombat_buff_state( buff, buff_state.stacks, buff_state.value, buff_state.duration );
    }
  }

  buff_t* custom_buff;
  for ( const auto& [ buff_name, c ] : custom_stat_buffs )
  {
    if ( c.is_percentage )
    {
      stat_pct_buff_type stat_pct;
      switch ( convert_hybrid_stat( c.stat ) )
      {
        case STAT_CRIT_RATING:        stat_pct = STAT_PCT_BUFF_CRIT; break;
        case STAT_HASTE_RATING:       stat_pct = STAT_PCT_BUFF_HASTE; break;
        case STAT_VERSATILITY_RATING: stat_pct = STAT_PCT_BUFF_VERSATILITY; break;
        case STAT_MASTERY_RATING:     stat_pct = STAT_PCT_BUFF_MASTERY; break;
        case STAT_STRENGTH:           stat_pct = STAT_PCT_BUFF_STRENGTH; break;
        case STAT_AGILITY:            stat_pct = STAT_PCT_BUFF_AGILITY; break;
        case STAT_STAMINA:            stat_pct = STAT_PCT_BUFF_STAMINA; break;
        case STAT_INTELLECT:          stat_pct = STAT_PCT_BUFF_INTELLECT; break;
        case STAT_SPIRIT:             stat_pct = STAT_PCT_BUFF_SPIRIT; break;
        default:
          throw std::invalid_argument(
            fmt::format( "Unsupported 'custom_stat' percentage stat type: '{}'", util::stat_type_string( c.stat ) ) );
      }

      custom_buff = make_buff( this, buff_name )
                      ->set_default_value( c.amount * 0.01 )
                      ->set_pct_buff_type( stat_pct );
      register_precombat_begin( [ custom_buff ] ( player_t* ) { custom_buff->execute(); } );
    }
    else
    {
      custom_buff = make_buff<stat_buff_t>( this, buff_name )
                      ->add_stat( convert_hybrid_stat( c.stat ), c.amount );
      register_precombat_begin( [ custom_buff ] ( player_t* ) { custom_buff->execute(); } );
    }
  }
}

void player_t::add_precombat_buff_state( buff_t* buff, int stacks, double value, timespan_t duration )
{
  if ( !buff->allow_precombat )
    throw std::invalid_argument( fmt::format( "Invalid buff for 'override.precombat_state' option. Precombat states for '{}' are disabled.", buff->name_str ) );

  register_precombat_begin( [ buff, stacks, value, duration ] ( player_t* )
  {
    buff->execute( stacks, value, duration );
    buff->predict();
  } );
}

void player_t::add_precombat_cooldown_state( cooldown_t* cd, timespan_t duration )
{
  if ( !cd->allow_precombat )
    throw std::invalid_argument( fmt::format( "Invalid cooldown for 'override.precombat_state' option. Precombat states for '{}' are disabled.", cd->name_str ) );

  // A cooldown may need an action to have its recharge rate properly adjusted.
  // Attempt to find an action with the same name that uses this cooldown.
  auto action = find_action( cd->name_str );
  if ( action->cooldown != cd )
    action = nullptr;

  register_precombat_begin( [ cd, action, duration ] ( player_t* ) { cd->start( action, duration ); } );
}

/// Called in every action constructor for all actions constructred for a player
void player_t::apply_affecting_auras(action_t&)
{
}

void player_t::action_init_finished(action_t&)
{
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

  // Infinite-Stacking Buffs and De-Buffs for everyone
  buffs.stunned = make_buff( this, "stunned" )
    ->set_max_stack( 1 )
    ->set_stack_change_callback( [ this ]( buff_t*, int, int new_ ) {
      if ( new_ == 0 )
        schedule_ready();
    } );

  buffs.rooted = make_buff( this, "rooted" )->set_max_stack( 1 )->set_quiet( true );

  debuffs.casting = make_buff( this, "casting" )->set_max_stack( 1 )->set_quiet( true );

  // .. for players
  if ( !is_enemy() && type != HEALING_ENEMY )
  {
    // Racials
    buffs.berserking = make_buff_fallback( race == RACE_TROLL, this, "berserking", find_spell( 26297 ) )
                           ->add_invalidate( CACHE_HASTE );

    buffs.stoneform = make_buff_fallback( race == RACE_DWARF, this, "stoneform", find_spell( 65116 ) );

    buffs.blood_fury = make_buff_fallback<stat_buff_t>( race == RACE_ORC, this, "blood_fury", find_racial_spell( "Blood Fury" ) )
                           ->add_invalidate( CACHE_SPELL_POWER )
                           ->add_invalidate( CACHE_ATTACK_POWER );

    buffs.shadowmeld = make_buff_fallback( race == RACE_NIGHT_ELF, this, "shadowmeld", find_spell( 58984 ) )
                           ->set_cooldown( 0_ms );

    buffs.ancestral_call[ 0 ] = make_buff_fallback<stat_buff_t>( race == RACE_MAGHAR_ORC, this,
                                                                 "rictus_of_the_laughing_skull", find_spell( 274739 ) );
    buffs.ancestral_call[ 1 ] = make_buff_fallback<stat_buff_t>( race == RACE_MAGHAR_ORC, this,
                                                                 "zeal_of_the_burning_blade", find_spell( 274740 ) );
    buffs.ancestral_call[ 2 ] = make_buff_fallback<stat_buff_t>( race == RACE_MAGHAR_ORC, this,
                                                                 "ferocity_of_the_frostwolf", find_spell( 274741 ) );
    buffs.ancestral_call[ 3 ] = make_buff_fallback<stat_buff_t>( race == RACE_MAGHAR_ORC, this,
                                                                 "might_of_the_blackrock", find_spell( 274742 ) );

    if ( race == RACE_DARK_IRON_DWARF )
    {
      buffs.fireblood =
          make_buff<stat_buff_t>( this, "fireblood", find_spell( 265226 ) )
              ->add_stat( convert_hybrid_stat( STAT_STR_AGI_INT ),
                          util::round( find_spell( 265226 )->effectN( 1 ).average( this, level() ) ) * 3 );
    }
    else
    {
      buffs.fireblood = buff_t::make_fallback( this, "fireblood", this );
    }

    buffs.darkflight = make_buff_fallback( race == RACE_WORGEN, this, "darkflight", find_racial_spell( "darkflight" ) );

    // fallback for ingest mineral isn't necessary as it's a passive effect that should not have any apl interaction
    if ( race == RACE_EARTHEN_HORDE || race == RACE_EARTHEN_ALLIANCE )
    {
      unsigned _id = 451918;  // crit is default

      switch ( earthen_mineral )
      {
        case player_t::AMBER:    _id = 451921; break;  // stamina
        case player_t::EMERALD:  _id = 451916; break;  // haste
        case player_t::ONYX:     _id = 451920; break;  // mastery
        case player_t::SAPPHIRE: _id = 451917; break;  // versatility
        default:                 break;
      }

      buffs.ingest_mineral = make_buff<stat_buff_t>( this, "ingest_mineral", find_spell( _id ) )
        ->set_name_reporting( "Ingest Mineral" );
    }

    buffs.movement = new movement_buff_t( this );

    if ( !is_pet() )
    {
      // 9.0 class buffs
      buffs.focus_magic = make_buff( this, "focus_magic", find_spell( 321358 ) )
        ->set_default_value_from_effect( 1 )
        ->add_invalidate( CACHE_SPELL_CRIT_CHANCE );

      buffs.power_infusion = make_buff( this, "power_infusion", find_spell( 10060 ) )
        ->set_default_value_from_effect( 1 )
        ->set_cooldown( 0_ms )
        ->add_invalidate( CACHE_HASTE );

      // External trinkets
      if ( external_buffs.soleahs_secret_technique )
      {
        // TODO: confirm what happens if ratings are the same. For now assuming it follows same priority as IQD.
        static constexpr std::array<stat_e, 4> ratings = { STAT_VERSATILITY_RATING, STAT_MASTERY_RATING,
                                                           STAT_HASTE_RATING, STAT_CRIT_RATING };

        auto ilevel = external_buffs.soleahs_secret_technique;
        auto coeff  = find_spell( 368513 )->effectN( 2 ).m_coefficient();
        auto points = dbc->random_property( ilevel ).p_epic[ 0 ];
        auto mult   = dbc->combat_rating_multiplier( ilevel, CR_MULTIPLIER_TRINKET );

        buffs.soleahs_secret_technique_external =
            make_buff<stat_buff_t>( this, "soleahs_secret_technique_external", find_spell( 368510 ) )
                ->add_stat( util::highest_stat( this, ratings ), coeff * points * mult );
      }

      if ( !external_buffs.elegy_of_the_eternals.empty() )
      {
        buffs.elegy_of_the_eternals_external =
            make_buff<stat_buff_t>( this, "elegy_of_the_eternals_external", find_spell( 369439 ) )
                ->set_duration( 0_ms );

        auto s_data  = find_spell( 367246 );
        auto coeff   = s_data->effectN( 1 ).m_coefficient() * s_data->effectN( 2 ).percent();
        auto entries = util::string_split<std::string_view>( external_buffs.elegy_of_the_eternals, "/" );

        for ( auto entry : entries )
        {
          auto values = util::string_split<std::string_view>( entry, ":" );

          try
          {
            if ( values.size() != 2 )
              throw std::invalid_argument( "Missing value." );

            auto stat = util::parse_stat_type( values[ 1 ] );
            if ( stat < STAT_CRIT_RATING || stat > STAT_VERSATILITY_RATING )
              throw std::invalid_argument( "Invalid stat." );

            auto ilevel = std::clamp( util::to_int( values[ 0 ] ), 0, MAX_ILEVEL );
            auto points = dbc->random_property( ilevel ).p_epic[ 0 ];
            auto mult   = dbc->combat_rating_multiplier( ilevel, CR_MULTIPLIER_TRINKET );

            debug_cast<stat_buff_t*>( buffs.elegy_of_the_eternals_external )->add_stat( stat, coeff * points * mult );
          }
          catch ( const std::invalid_argument& msg )
          {
            throw std::invalid_argument(
                fmt::format( "\n\tInvalid entry '{}' for external_buffs.elegy_of_the_eternals. {}"
                             "\n\tFormat is <ilevel>:<stat>/...",
                             entry, msg.what() ) );
          }
        }
      }

      if (!external_buffs.tome_of_unstable_power.empty() && external_buffs.tome_of_unstable_power_ilevel)
      {
          auto buff_spell = find_spell(388583);
          auto data_spell = find_spell(391290);

          auto buff = make_buff<stat_buff_t>(this, "tome_of_unstable_power_external", buff_spell);

          auto ilevel = external_buffs.tome_of_unstable_power_ilevel;
          auto coeff_main_stat = data_spell->effectN(1).m_coefficient();
          auto coeff_crit = data_spell->effectN(2).m_coefficient();
          auto points = dbc->random_property(ilevel).p_epic[0];
          auto mult = dbc->combat_rating_multiplier(ilevel, CR_MULTIPLIER_TRINKET);

          buff->set_duration(find_spell(388559)->duration());
          buff->manual_stats_added = false;
          // This is currently scaling class -1, change if this ever changes
          buff->add_stat(convert_hybrid_stat(STAT_STR_AGI_INT), coeff_main_stat * points);
          buff->add_stat(STAT_CRIT_RATING, coeff_crit * points * mult);

          buffs.tome_of_unstable_power = buff;
      }

      // 9.2 Jailer raid buff
      // Values are hard-coded because difficulty-specific spell data is not fully extracted.
      buffs.boon_of_azeroth = make_buff<stat_buff_t>( this, "boon_of_azeroth", find_spell( 363338 ) )
        ->add_stat( STAT_MASTERY_RATING, 350 )
        ->set_default_value( 0.1 )
        ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
        ->set_pct_buff_type( STAT_PCT_BUFF_VERSATILITY )
        ->set_pct_buff_type( STAT_PCT_BUFF_CRIT );

      buffs.boon_of_azeroth_mythic = make_buff<stat_buff_t>( this, "boon_of_azeroth_mythic", find_spell( 363338 ) )
        ->add_stat( STAT_MASTERY_RATING, 418 )
        ->set_default_value( 0.12 )
        ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
        ->set_pct_buff_type( STAT_PCT_BUFF_VERSATILITY )
        ->set_pct_buff_type( STAT_PCT_BUFF_CRIT );
    }
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
        ->set_default_value_from_effect( 1 )
        ->set_cooldown( timespan_t::from_seconds( 5.0 ) );
    debuffs.mystic_touch = make_buff( this, "mystic_touch", find_spell( 113746 ) )
        ->set_default_value_from_effect( 1 )
        ->set_cooldown( timespan_t::from_seconds( 5.0 ) );

    // Dragonflight Raid Damage Modifier Debuffs
    auto buff_spell      = find_spell( 428402 );
    debuffs.hunters_mark = make_buff( this, "hunters_mark", find_spell( 257284 ) )
        ->set_period( 0_s )
        ->set_default_value( buff_spell->effectN( 1 ).percent() )
        ->set_schools( buff_spell->effectN( 1 ).affected_schools() );
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

item_t* player_t::find_item_by_name( util::string_view item_name )
{
  auto it = range::find(items, item_name, &item_t::name );

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

item_t* player_t::find_item_by_use_effect_name( util::string_view effect_name )
{
  auto it = range::find_if(items, [effect_name](const item_t& item) {
    return item.has_use_special_effect() && effect_name == item.special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE )->name();
  } );

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

  if ( r == RESOURCE_FOCUS || r == RESOURCE_ENERGY || r == RESOURCE_ESSENCE )
  {
    if ( reg )
    {
      reg *= ( 1.0 / cache.attack_haste() );
    }
  }

  return reg;
}

double player_t::apply_combat_rating_dr( rating_e rating, double value ) const
{
  switch ( rating )
  {
    case RATING_LEECH:
    case RATING_SPEED:
    case RATING_AVOIDANCE:
      return item_database::curve_point_value( *dbc, DIMINISHING_RETURN_TERTIARY_CR_CURVE, value * 100.0 ) / 100.0;
    case RATING_MASTERY:
      return item_database::curve_point_value( *dbc, DIMINISHING_RETURN_SECONDARY_CR_CURVE, value );
    case RATING_MITIGATION_VERSATILITY:
      return item_database::curve_point_value( *dbc, DIMINISHING_RETURN_VERS_MITIG_CR_CURVE, value * 100.0 ) / 100.0;
    default:
      // Note, curve uses %-based values, not values divided by 100
      return item_database::curve_point_value( *dbc, DIMINISHING_RETURN_SECONDARY_CR_CURVE, value * 100.0 ) / 100.0;
  }
}

double player_t::composite_melee_haste() const
{
  double h;

  h = std::max( 0.0, composite_melee_haste_rating() ) / current.rating.attack_haste;
  h = apply_combat_rating_dr( RATING_MELEE_HASTE, h );

  h = 1.0 / ( 1.0 + h );

  if ( !is_pet() && !is_enemy() && type != HEALING_ENEMY )
  {
    for ( auto b : buffs.stat_pct_buffs[ STAT_PCT_BUFF_HASTE ] )
      h /= 1.0 + b->check_stack_value();

    if ( buffs.bloodlust->check() )
      h *= 1.0 / ( 1.0 + buffs.bloodlust->check_stack_value() );

    if ( buffs.berserking->check() )
      h *= 1.0 / ( 1.0 + buffs.berserking->data().effectN( 1 ).percent() );

    h *= 1.0 / ( 1.0 + racials.nimble_fingers->effectN( 1 ).percent() );
    h *= 1.0 / ( 1.0 + racials.time_is_money->effectN( 1 ).percent() );

    if ( timeofday == NIGHT_TIME )
      h *= 1.0 / ( 1.0 + racials.touch_of_elune->effectN( 1 ).percent() );

    if ( buffs.power_infusion )
      h *= 1.0 / ( 1.0 + buffs.power_infusion->check_value() );
  }

  return h;
}

double player_t::composite_melee_auto_attack_speed() const
{
  double h = composite_melee_haste();

  if ( buffs.galeforce_striking && buffs.galeforce_striking->check() )
    h *= 1.0 / ( 1.0 + buffs.galeforce_striking->check_value() );

  if ( buffs.delirious_frenzy && buffs.delirious_frenzy->check() )
    h *= 1.0 / ( 1.0 + buffs.delirious_frenzy->check_stack_value() );

  if ( buffs.way_of_controlled_currents && buffs.way_of_controlled_currents->check() )
    h *= 1.0 / ( 1.0 + buffs.way_of_controlled_currents->check_stack_value() );

  return h;
}

double player_t::composite_melee_attack_power() const
{
  double ap = current.stats.attack_power;

  ap += current.attack_power_per_strength * cache.strength();
  ap += current.attack_power_per_agility * cache.agility();

  return ap;
}

double player_t::composite_weapon_attack_power_by_type( attack_power_type type ) const
{
  double wdps = 0;
  bool has_mh = main_hand_weapon.type != WEAPON_NONE;
  bool has_oh = off_hand_weapon.type != WEAPON_NONE;

  switch ( type )
  {
    case attack_power_type::WEAPON_MAINHAND:
      if ( has_mh )
      {
        wdps = main_hand_weapon.dps;
      }
      else  // Unarmed is apparently a 0.5 dps weapon, Bruce Lee would be ashamed.
      {
        wdps = .5;
      }
      break;

    case attack_power_type::WEAPON_OFFHAND:
      if ( has_oh )
      {
        wdps = off_hand_weapon.dps;
      }
      else
      {
        wdps = .5;
      }
      break;

    case attack_power_type::WEAPON_BOTH:
      // Don't use with weapon = player -> off_hand_weapon or the OH penalty will be applied to the whole spell
      wdps = ( has_mh ? main_hand_weapon.dps : .5 ) + ( has_oh ? off_hand_weapon.dps : .5 ) / 2;
      wdps *= 2.0 / 3.0;
      break;

    default:  // Nohand, just base AP then
      break;
  }

  // 2022-08-25 -- Aura type 141 works as a general base weapon damage modifier which affects AP calculations
  //               This is normalized to AP based on weapon speed in a similar way as base weapon DPS above
  //               Aura type 530 does not apply to this, as it is only added to the result of white hits
  if ( auto_attack_base_modifier > 0 )
  {
    if ( type == attack_power_type::WEAPON_MAINHAND )
    {
      wdps += auto_attack_base_modifier / main_hand_weapon.swing_time.total_seconds();
    }
    else if ( type == attack_power_type::WEAPON_OFFHAND )
    {
      wdps += auto_attack_base_modifier / off_hand_weapon.swing_time.total_seconds();
    }
    else if ( type == attack_power_type::WEAPON_BOTH )
    {
      wdps += ( auto_attack_base_modifier / main_hand_weapon.swing_time.total_seconds()
              + auto_attack_base_modifier / off_hand_weapon.swing_time.total_seconds() * 0.5 ) * ( 2.0 / 3.0 );
    }
  }

  // weapon attack power is truncated to integer
  return static_cast<int>( wdps * WEAPON_POWER_COEFFICIENT );
}

double player_t::composite_total_attack_power_by_type( attack_power_type type ) const
{
  // duplicate code to prevent recursion
  if ( current.attack_power_per_spell_power > 0 )
  {
    // total spell power is rounded to integer
    auto sp = std::round( cache.spell_power( SCHOOL_MAX ) * composite_spell_power_multiplier() );

    return std::round( current.attack_power_per_spell_power * sp );
  }

  auto mul = composite_attack_power_multiplier();

  // total attack power is rounded to integer
  return std::round( cache.attack_power() * mul + cache.weapon_attack_power( type ) * mul );
}

double player_t::composite_attack_power_multiplier() const
{
  if ( is_pet() || is_enemy() || type == HEALING_ENEMY || current.attack_power_per_spell_power > 0 )
  {
    return 1.0;
  }

  double m = current.attack_power_multiplier;

  m *= 1.0 + sim->auras.battle_shout->check_value();

  // multiplier is rounded to 3 digits
  return std::round( m * 1000 ) * 0.001;
}

double player_t::composite_melee_crit_chance() const
{
  double ac = current.attack_crit_chance;

  ac += apply_combat_rating_dr( RATING_MELEE_CRIT, composite_melee_crit_rating() / current.rating.attack_crit );

  if ( current.attack_crit_per_agility )
    ac += ( cache.agility() / current.attack_crit_per_agility / 100.0 );

  for ( auto b : buffs.stat_pct_buffs[ STAT_PCT_BUFF_CRIT ] )
    ac += b->check_stack_value();

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

bool player_t::is_moving() const
{
  return buffs.movement && buffs.movement -> check();
}

bool player_t::in_gcd() const
{
  return gcd_ready > sim -> current_time();
}

double player_t::composite_dodge() const
{
  // Start with sources not subject to DR - base dodge + dodge from base agility (stored in current.dodge)
  double total_dodge = current.dodge;

  // bonus_dodge is from crit (through dodge rating) and bonus Agility
  double bonus_dodge = composite_dodge_rating() / current.rating.dodge;
  if ( !is_enemy() )
  {
    bonus_dodge += ( cache.agility() - dbc->race_base( race ).agility -
        dbc->attribute_base( type, level() ).agility ) * current.dodge_per_agility;
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
    bonus_parry += ( cache.strength() - dbc->race_base( race ).strength -
        dbc->attribute_base( type, level() ).strength ) * current.parry_per_strength;
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
  h = apply_combat_rating_dr( RATING_SPELL_HASTE, h );

  h = 1.0 / ( 1.0 + h );

  if ( !is_pet() && !is_enemy() && type != HEALING_ENEMY )
  {
    for ( auto b : buffs.stat_pct_buffs[ STAT_PCT_BUFF_HASTE ] )
      h /= 1.0 + b->check_stack_value();

    if ( buffs.bloodlust->check() )
      h *= 1.0 / ( 1.0 + buffs.bloodlust->check_stack_value() );

    if ( buffs.berserking->check() )
      h *= 1.0 / ( 1.0 + buffs.berserking->data().effectN( 1 ).percent() );

    h *= 1.0 / ( 1.0 + racials.nimble_fingers->effectN( 1 ).percent() );
    h *= 1.0 / ( 1.0 + racials.time_is_money->effectN( 1 ).percent() );

    if ( timeofday == NIGHT_TIME )
      h *= 1.0 / ( 1.0 + racials.touch_of_elune->effectN( 1 ).percent() );

    if ( buffs.power_infusion )
      h *= 1.0 / ( 1.0 + buffs.power_infusion->check_value() );
  }

  return h;
}

/**
 * This is the old spell_haste and incorporates everything that buffs cast speed
 */
double player_t::composite_spell_cast_speed() const
{
  auto speed = cache.spell_haste();

  if ( !is_pet() && !is_enemy() && type != HEALING_ENEMY )
  {
    if ( buffs.tempus_repit &&  buffs.tempus_repit->check() )
    {
      speed *= 1.0 / ( 1.0 + buffs.tempus_repit->check_stack_value() );
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

  return sp;
}

double player_t::composite_total_spell_power( school_e school ) const
{
  // dupplicate code to prevent recursion
  if ( current.spell_power_per_attack_power > 0 )
  {
    auto mul = composite_attack_power_multiplier();

    // total attack power is rounded to integer
    auto ap = std::round( cache.attack_power() * mul +
                          cache.weapon_attack_power( attack_power_type::WEAPON_MAINHAND ) * mul );

    return std::round( current.spell_power_per_attack_power * ap );
  }

  // total spell power is rounded to integer
  return std::round( cache.spell_power( school ) * composite_spell_power_multiplier() );
}

double player_t::composite_spell_power_multiplier() const
{
  if ( is_pet() || is_enemy() || type == HEALING_ENEMY || current.spell_power_per_attack_power > 0 )
  {
    return 1.0;
  }

  // multiplier is rounded to 3 digits
  return std::round( current.spell_power_multiplier * 1000 ) * 0.001;
}

double player_t::composite_spell_crit_chance() const
{
  double sc = current.spell_crit_chance;

  sc += apply_combat_rating_dr( RATING_SPELL_CRIT, composite_spell_crit_rating() / current.rating.spell_crit );

  if ( current.spell_crit_per_intellect > 0 )
  {
    sc += ( cache.intellect() / current.spell_crit_per_intellect / 100.0 );
  }

  for ( auto b : buffs.stat_pct_buffs[ STAT_PCT_BUFF_CRIT ] )
    sc += b->check_stack_value();

  if ( buffs.focus_magic )
    sc += buffs.focus_magic->check_value();

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
  double cm = current.mastery;

  cm += apply_combat_rating_dr( RATING_MASTERY, composite_mastery_rating() / current.rating.mastery );

  for ( auto b : buffs.stat_pct_buffs[ STAT_PCT_BUFF_MASTERY ] )
    cm += b->check_stack_value();

  if ( !is_pet() && !is_enemy() && type != HEALING_ENEMY )
  {
    cm += sim->auras.skyfury->check_value();
  }

  return cm;
}

double player_t::composite_bonus_armor() const
{
  return current.stats.bonus_armor;
}

double player_t::composite_damage_versatility() const
{
  double cdv = current.versatility;

  cdv += apply_combat_rating_dr( RATING_DAMAGE_VERSATILITY,
           composite_damage_versatility_rating() / current.rating.damage_versatility );

  for ( auto b : buffs.stat_pct_buffs[ STAT_PCT_BUFF_VERSATILITY ] )
    cdv += b->check_stack_value();

  if ( !is_pet() && !is_enemy() && type != HEALING_ENEMY )
  {
    cdv += sim->auras.mark_of_the_wild->check_value();
  }

  if ( buffs.dmf_well_fed )
    cdv += buffs.dmf_well_fed->check_value();

  return cdv;
}

double player_t::composite_heal_versatility() const
{
  double chv = current.versatility;

  chv += apply_combat_rating_dr( RATING_HEAL_VERSATILITY,
           composite_heal_versatility_rating() / current.rating.heal_versatility );

  for ( auto b : buffs.stat_pct_buffs[ STAT_PCT_BUFF_VERSATILITY ] )
    chv += b->check_stack_value();

  if ( !is_pet() && !is_enemy() && type != HEALING_ENEMY )
  {
    chv += sim->auras.mark_of_the_wild->check_value();
  }

  if ( buffs.dmf_well_fed )
    chv += buffs.dmf_well_fed->check_value();

  return chv;
}

double player_t::composite_mitigation_versatility() const
{
  double cmv = current.versatility / 2;

  cmv += apply_combat_rating_dr( RATING_MITIGATION_VERSATILITY,
           composite_mitigation_versatility_rating() / current.rating.mitigation_versatility );

  for ( auto b : buffs.stat_pct_buffs[ STAT_PCT_BUFF_VERSATILITY ] )
    cmv += b->check_stack_value() / 2;

  if ( !is_pet() && !is_enemy() && type != HEALING_ENEMY )
  {
    cmv += sim->auras.mark_of_the_wild->check_value() / 2;
  }

  if ( buffs.dmf_well_fed )
    cmv += buffs.dmf_well_fed->check_value() / 2;

  return cmv;
}

double player_t::composite_leech() const
{
  return current.leech + apply_combat_rating_dr( RATING_LEECH, composite_leech_rating() / current.rating.leech );
}

double player_t::composite_run_speed() const
{
  // speed DRs using the following formula:
  double pct = apply_combat_rating_dr( RATING_SPEED, composite_speed_rating() / current.rating.speed );

  double coefficient = std::exp( -.0003 * composite_speed_rating() );

  return pct * coefficient * .1;
}

double player_t::composite_avoidance() const
{
  return apply_combat_rating_dr( RATING_AVOIDANCE, composite_avoidance_rating() / current.rating.avoidance );
}

double player_t::composite_corruption() const
{
  return composite_corruption_rating() / current.rating.corruption;
}

double player_t::composite_corruption_resistance() const
{
  return composite_corruption_resistance_rating() / current.rating.corruption_resistance;
}

double player_t::composite_total_corruption() const
{
  return cache.corruption() - cache.corruption_resistance();
}

double player_t::composite_player_pet_damage_multiplier( const action_state_t*, bool guardian ) const
{
  double m = 1.0;

  m *= 1.0 + racials.command->effectN(1).percent();

  if (!guardian)
  {
    if (buffs.coldhearted && buffs.coldhearted->check())
      m *= 1.0 + buffs.coldhearted->check_value();

    // By default effect 1 is used for the player modifier, effect 2 is for the pet modifier
    if (buffs.battlefield_presence && buffs.battlefield_presence->check())
      m *= 1.0 + (buffs.battlefield_presence->data().effectN(2).percent() * buffs.battlefield_presence->current_stack);
  }

  return m;
}

double player_t::composite_player_target_pet_damage_multiplier( player_t*, bool ) const
{
  return 1.0;
}

double player_t::composite_player_multiplier( school_e school ) const
{
  double m = 1.0;

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

  if ( buffs.echo_of_eonar && buffs.echo_of_eonar->check() )
    m *= 1 + buffs.echo_of_eonar->check_value();

  if ( buffs.volatile_solvent_damage && buffs.volatile_solvent_damage->has_common_school( school ) )
    m *= 1.0 + buffs.volatile_solvent_damage->check_value();

  if ( buffs.battlefield_presence && buffs.battlefield_presence->check() )
    m *= 1.0 + buffs.battlefield_presence->check_stack_value();

  if ( buffs.coldhearted && buffs.coldhearted->check() )
    m *= 1.0 + buffs.coldhearted->check_value();

  return m;
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

  if ( target->race == RACE_ABERRATION && buffs.damage_to_aberrations && buffs.damage_to_aberrations->check() )
    m *= 1.0 + buffs.damage_to_aberrations->stack_value();

  if ( buffs.wild_hunt_tactics )
  {
    double health_threshold = 100.0 - ( 100.0 - buffs.wild_hunt_tactics->data().effectN( 5 ).base_value() ) *
                                        sim->shadowlands_opts.wild_hunt_tactics_duration_multiplier;
    // This buff is never triggered so use default_value.
    if ( target->health_percentage() > health_threshold )
    {
      // WHS is triggered when you cast something that gets the damage benefit from WHT
      if ( buffs.wild_hunt_strategem_tracking )
      {
        buffs.wild_hunt_strategem_tracking->trigger();
      }

      m *= 1.0 + buffs.wild_hunt_tactics->default_value;
    }
  }

  auto td = find_target_data( target );
  if ( td )
  {
    // Always created debuffs, TODO: move to target_specific_debuffs
    m *= 1.0 + td->debuff.condensed_lifeforce->check_value();
    m *= 1.0 + td->debuff.adversary->check_value();
    m *= 1.0 + td->debuff.plagueys_preemptive_strike->check_value();
    m *= 1.0 + td->debuff.sinful_revelation->check_value();
    m *= 1.0 + td->debuff.dream_delver->check_stack_value();
    m *= 1.0 + td->debuff.soulglow_spectrometer->check_stack_value();
    m *= 1.0 + td->debuff.scouring_touch->check_stack_value();
    m *= 1.0 + td->debuff.exsanguinated->check_value();
    m *= 1.0 + td->debuff.kevins_wrath->check_value();
    m *= 1.0 + td->debuff.wild_hunt_strategem->check_value();

    // target specific debuffs, MUST check for null
    if ( td->debuff.unwavering_focus )
      m *= 1.0 + td->debuff.unwavering_focus->check_value();
  }

  return m;
}

double player_t::composite_player_heal_multiplier( const action_state_t* ) const
{
  double m = 1.0;

  if ( buffs.blessing_of_spring->check() )
    m *= 1.0 + buffs.blessing_of_spring->data().effectN( 1 ).percent();

  return m;
}

double player_t::composite_player_th_multiplier( school_e /* school */ ) const
{
  return 1.0;
}

double player_t::composite_player_absorb_multiplier( const action_state_t* ) const
{
  return 1.0;
}

double player_t::composite_player_target_crit_chance( player_t* target ) const
{
  double c = 0.0;

  if ( const actor_target_data_t* td = get_owner_or_self()->find_target_data( target ) )
  {
    // Essence: Blood of the Enemy Major debuff
    c += td->debuff.blood_of_the_enemy->check_stack_value();

    // Consumable: Potion of Focused Resolve
    c += td->debuff.focused_resolve->check_stack_value();

    // Darkmoon Deck: Putrescence
    c += td->debuff.putrid_burst->check_stack_value();
  }

  return c;
}

double player_t::composite_player_critical_damage_multiplier( const action_state_t* /* s */ ) const
{
  double m = current.crit_damage_multiplier;

  if ( buffs.elemental_chaos_fire )
    m *= 1.0 + buffs.elemental_chaos_fire->check_value();

  if ( buffs.incensed )
    m *= 1.0 + buffs.incensed->check_value();

  // Critical hit damage buff from R3 Blood of the Enemy major on-use
  if ( buffs.seething_rage_essence )
    m *= 1.0 + buffs.seething_rage_essence->check_value();

  // Critical hit damage buff from follower themed Benthic boots
  if ( buffs.fathom_hunter )
    m *= 1.0 + buffs.fathom_hunter->check_value();

  return m;
}

double player_t::composite_player_critical_healing_multiplier() const
{
  double m = current.crit_healing_multiplier;

  if ( buffs.elemental_chaos_frost )
    m *= 1.0 + buffs.elemental_chaos_frost->check_value();

  return m;
}

/**
 * Return the non-stacking movement modifier Increase Speed% (31).
 * These speed buffs do not stack with each other and only the highest one will be applied.
 */
double player_t::non_stacking_movement_modifier() const
{
  double speed = 0.0;

  if ( !is_enemy() && type != HEALING_ENEMY )
  {
    if ( buffs.stampeding_roar && buffs.stampeding_roar->check() )
      speed = std::max( buffs.stampeding_roar->check_value(), speed );
  }

  if ( !is_enemy() && !is_pet() && type != HEALING_ENEMY )
  {
    if ( buffs.darkflight->check() )
      speed = std::max( buffs.darkflight->data().effectN( 1 ).percent(), speed );

    if ( buffs.nitro_boosts && buffs.nitro_boosts->check() )
      speed = std::max( buffs.nitro_boosts->data().effectN( 1 ).percent(), speed );

    if ( buffs.body_and_soul && buffs.body_and_soul->check() )
      speed = std::max( buffs.body_and_soul->data().effectN( 1 ).percent(), speed );

    if ( buffs.angelic_feather && buffs.angelic_feather->check() )
      speed = std::max( buffs.angelic_feather->data().effectN( 1 ).percent(), speed );

    if ( buffs.normalization_increase && buffs.normalization_increase->check() )
      speed = std::max( buffs.normalization_increase->data().effectN( 3 ).percent(), speed );

    if ( buffs.surekian_grace && buffs.surekian_grace->check() )
      speed = std::max( buffs.surekian_grace->check_value(), speed );
  }

  return speed;
}

/**
 * Return the sum of all stacking movement modifier Increase Movement Speed% (Stacking) (129)
 */
double player_t::stacking_movement_modifier() const
{
  double speed = passive_modifier;

  // speed tertiary rating
  speed += composite_run_speed();

  speed += racials.quickness->effectN( 2 ).percent();

  if ( buffs.windwalking_movement_aura )
    speed += buffs.windwalking_movement_aura->check_value();

  if ( buffs.elemental_chaos_air )
    speed += buffs.elemental_chaos_air->check_value();

  return speed;
}

double player_t::composite_movement_speed() const
{
  double speed = base_movement_speed;

  double non_stacking = non_stacking_movement_modifier();

  double stacking = stacking_movement_modifier();

  speed *= ( 1 + non_stacking + stacking );

  return speed;
}

double player_t::composite_attribute( attribute_e attr ) const
{
  return current.stats.attribute[ attr ];
}

double player_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = current.attribute_multiplier[ attr ];

  if ( is_pet() || is_enemy() || type == HEALING_ENEMY )
    return m;

  if ( ( true_level >= 27 ) && matching_gear )
    m *= 1.0 + matching_gear_multiplier( attr );

  stat_pct_buff_type pct_type = STAT_PCT_BUFF_MAX;
  switch ( attr )
  {
    case ATTR_STRENGTH:
      pct_type = STAT_PCT_BUFF_STRENGTH;
      break;
    case ATTR_AGILITY:
      pct_type = STAT_PCT_BUFF_AGILITY;
      break;
    case ATTR_INTELLECT:
      pct_type = STAT_PCT_BUFF_INTELLECT;
      if ( sim->auras.arcane_intellect->check() )
        m *= 1.0 + sim->auras.arcane_intellect->current_value;
      break;
    case ATTR_SPIRIT:
      pct_type = STAT_PCT_BUFF_SPIRIT;
      break;
    case ATTR_STAMINA:
      pct_type = STAT_PCT_BUFF_STAMINA;
      if ( sim->auras.power_word_fortitude->check() )
      {
        m *= 1.0 + sim->auras.power_word_fortitude->current_value;
      }
      break;
    default:
      break;
  }

  if ( pct_type != STAT_PCT_BUFF_MAX )
  {
    for ( auto b : buffs.stat_pct_buffs[ pct_type ] )
      m *= 1.0 + b->check_stack_value();
  }

  return m;
}

double player_t::composite_rating_multiplier( rating_e rating ) const
{
  double v = passive_rating_multiplier.get( rating );

  switch ( rating )
  {
    case RATING_SPELL_HASTE:
    case RATING_MELEE_HASTE:
    case RATING_RANGED_HASTE:
      v *= 1.0 + passive_values.amplification_1;
      v *= 1.0 + passive_values.amplification_2;
      v *= 1.0 + racials.the_human_spirit->effectN( 1 ).percent();
      break;
    case RATING_MASTERY:
      v *= 1.0 + passive_values.amplification_1;
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
      v *= 1.0 + passive_values.amplification_1;
      v *= 1.0 + passive_values.amplification_2;
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
    case RATING_CORRUPTION:
      v = current.stats.corruption_rating;
      break;
    case RATING_CORRUPTION_RESISTANCE:
      v = current.stats.corruption_resistance_rating;
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
    m *= 1.0 + debuffs.vulnerable->check_value();

  // 1% damage taken per stack, arbitrary because this buff is completely fabricated!
  if ( debuffs.damage_taken && debuffs.damage_taken->check() )
    m *= 1.0 + debuffs.damage_taken->current_stack * 0.01;

  if ( debuffs.mystic_touch && debuffs.mystic_touch->has_common_school( school ) )
    m *= 1.0 + debuffs.mystic_touch->check_value();

  if ( debuffs.chaos_brand && debuffs.chaos_brand->has_common_school( school ) )
    m *= 1.0 + debuffs.chaos_brand->check_value();

  if ( debuffs.hunters_mark && debuffs.hunters_mark->has_common_school( school ) &&
       health_percentage() > debuffs.hunters_mark->data().effectN( 3 ).base_value() )
    m *= 1.0 + debuffs.hunters_mark->check_value();

  return m;
}

double player_t::composite_player_target_armor( player_t* target ) const
{
  return target->cache.armor();
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

  sim->print_debug( "{} invalidates stat cache for {}.", *this, c );

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
      if ( current.attack_crit_per_agility > 0 )
        invalidate_cache( CACHE_ATTACK_CRIT_CHANCE );
      break;

    case CACHE_INTELLECT:
      if ( current.spell_power_per_intellect > 0 )
        invalidate_cache( CACHE_SPELL_POWER );
      if ( current.spell_crit_per_intellect > 0 )
        invalidate_cache( CACHE_SPELL_CRIT_CHANCE );
      break;

    case CACHE_SPELL_POWER:
      if ( current.attack_power_per_spell_power > 0 )
        invalidate_cache( CACHE_ATTACK_POWER );
      break;

    case CACHE_ATTACK_POWER:
      if ( current.spell_power_per_attack_power > 0 )
        invalidate_cache( CACHE_SPELL_POWER );
      break;

    case CACHE_ATTACK_HASTE:
      invalidate_cache( CACHE_AUTO_ATTACK_SPEED );
      invalidate_cache( CACHE_RPPM_HASTE );
      break;

    case CACHE_SPELL_HASTE:
      invalidate_cache( CACHE_SPELL_CAST_SPEED );
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
void invalidate_cache( cache_e ) {}
#endif

void player_t::sequence_add_wait( timespan_t amount, timespan_t ts )
{
  // Collect iteration#1 data, for log/debug/iterations==1 simulation iteration#0 data
  if ( ( sim->iterations <= 1 && sim->current_iteration == 0 ) ||
       ( sim->iterations > 1 && nth_iteration() == 1 ) )
  {
    if ( collected_data.action_sequence.size() <= sim->expected_max_time() * 2.0 + 3.0 )
    {
      if ( in_combat )
      {
        if ( !collected_data.action_sequence.empty() &&
             collected_data.action_sequence.back().wait_time > timespan_t::zero() )
          collected_data.action_sequence.back().wait_time += amount;
        else
          collected_data.action_sequence.emplace_back( ts, amount, this );
      }
    }
    else
    {
      assert( false &&
              "Collected too much action sequence data."
              "This means there is a serious overflow of executed actions in the first iteration, which should be "
              "fixed." );
    }
  }
}

void player_t::sequence_add( const action_t* a, const player_t* target, timespan_t ts )
{
  // Collect iteration#1 data, for log/debug/iterations==1 simulation iteration#0 data
  if ( ( a->sim->iterations <= 1 && a->sim->current_iteration == 0 ) ||
       ( a->sim->iterations > 1 && nth_iteration() == 1 ) )
  {
    if ( collected_data.action_sequence.size() <= sim->expected_max_time() * 2.0 + 3.0 )
    {
      if ( a->is_precombat )
        collected_data.action_sequence_precombat.emplace_back( a, target, ts, this );
      else
        collected_data.action_sequence.emplace_back( a, target, ts, this );
    }
    else
    {
      assert( false &&
              "Collected too much action sequence data."
              "This means there is a serious overflow of executed actions in the first iteration, which should be "
              "fixed." );
    }
  }
}

void player_t::precombat_init()
{
  precombat_initialized = true;

  sim->print_debug( "Precombat begins for {}.", *this );
  if ( !is_pet() && !is_add() )
  {
    arise();
  }

  init_resources( true );
}

void player_t::combat_begin()
{
  if ( !precombat_initialized )
    precombat_init();

  // Trigger registered pre-pull functions
  for ( const auto& f : precombat_begin_functions )
  {
    f( this );
  }

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
      if ( in_combat && ( action->channeled || action->travel_time() == timespan_t::zero() ) )
        break;
    }
  }
  first_cast = false;

  sim->print_debug( "Combat begins for {}.", *this );

  if ( !precombat_action_list.empty() )
    enter_combat();

  // re-initialize collected_data.health_changes.previous_*_level
  // necessary because food/flask are counted as resource gains, and thus provide phantom
  // gains on the timeline if not corrected
  collected_data.health_changes.previous_gain_level     = 0.0;
  // forcing a resource timeline data collection in combat_end() seems to have rendered this next line unnecessary
  collected_data.health_changes.previous_loss_level     = 0.0;

  for ( size_t i = 0, end = collected_data.combat_start_resource.size(); i < end; ++i )
  {
    collected_data.combat_start_resource[ i ].add( resources.current[ i ] );
  }

  auto add_timed_buff_triggers = [ this ]( const std::vector<timespan_t>& times, buff_t* buff,
                                           timespan_t duration = timespan_t::min() ) {
    if ( buff )
      for ( auto t : times )
        make_event( *sim, t, [ buff, duration ] { buff->trigger( duration ); } );
  };

  add_timed_buff_triggers( external_buffs.power_infusion, buffs.power_infusion );
  add_timed_buff_triggers( external_buffs.symbol_of_hope, buffs.symbol_of_hope );
  add_timed_buff_triggers( external_buffs.conquerors_banner, buffs.conquerors_banner );
  add_timed_buff_triggers( external_buffs.rallying_cry, buffs.rallying_cry );
  add_timed_buff_triggers( external_buffs.pact_of_the_soulstalkers, buffs.pact_of_the_soulstalkers );
  add_timed_buff_triggers( external_buffs.boon_of_azeroth, buffs.boon_of_azeroth );
  add_timed_buff_triggers( external_buffs.boon_of_azeroth_mythic, buffs.boon_of_azeroth_mythic );
  add_timed_buff_triggers( external_buffs.tome_of_unstable_power, buffs.tome_of_unstable_power );

  auto add_timed_blessing_triggers = [ add_timed_buff_triggers ]( const std::vector<timespan_t>& times, buff_t* buff,
                                                                  timespan_t duration = timespan_t::min() ) {
    add_timed_buff_triggers( times, buff, duration );
  };

  timespan_t summer_duration =
    buffs.blessing_of_summer->buff_duration() * ( 1.0 + external_buffs.blessing_of_summer_duration_multiplier );
  add_timed_blessing_triggers( external_buffs.blessing_of_summer, buffs.blessing_of_summer, summer_duration );
  add_timed_blessing_triggers( external_buffs.blessing_of_autumn, buffs.blessing_of_autumn );
  add_timed_blessing_triggers( external_buffs.blessing_of_winter, buffs.blessing_of_winter );
  add_timed_blessing_triggers( external_buffs.blessing_of_spring, buffs.blessing_of_spring );

  // Trigger registered combat-begin functions
  for ( const auto& f : combat_begin_functions)
  {
    f( this );
  }
}

void player_t::combat_end()
{
  for ( auto* pet : pet_list )
  {
    pet->combat_end();
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
      sim->parent->initialized && sim->thread_index > 0 )
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
  if ( !requires_data_collection() )
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
  range::fill( iteration_resource_overflowed, 0.0 );

  if ( collected_data.health_changes.collect )
  {
    collected_data.health_changes.timeline.clear();  // Drop Data
    collected_data.health_changes.timeline_normalized.clear();
  }

  range::for_each( buff_list, std::mem_fn( &buff_t::datacollection_begin ) );
  range::for_each( stats_list, std::mem_fn( &stats_t::datacollection_begin ) );
  range::for_each( uptime_list, std::mem_fn( &uptime_t::datacollection_begin ) );
  range::for_each( benefit_list, std::mem_fn( &benefit_t::datacollection_begin ) );
  range::for_each( proc_list, std::mem_fn( &proc_t::datacollection_begin ) );
  range::for_each( pet_list, std::mem_fn( &pet_t::datacollection_begin ) );
  range::for_each( sample_data_list, std::mem_fn( &sample_data_helper_t::datacollection_begin ) );
  range::for_each( cooldown_waste_data_list, std::mem_fn( &cooldown_waste_data_t::datacollection_begin ) );
}

/**
 * End data collection for statistics.
 */
void player_t::datacollection_end()
{
  // This checks if the actor was arisen at least once during this iteration.
  if ( !requires_data_collection() )
    return;

  for ( auto* pet : pet_list )
    pet->datacollection_end();

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

  if ( !is_enemy() && !is_add() && type != HEALING_ENEMY )
  {
    sim->iteration_dmg += iteration_dmg;
    sim->priority_iteration_dmg += priority_iteration_dmg;
    sim->iteration_heal += iteration_heal;
    sim->iteration_absorb += iteration_absorb;
  }

  // make sure TMI-relevant timeline lengths all match for tanks
  if ( !is_enemy() && !is_pet() && type != HEALING_ENEMY && primary_role() == ROLE_TANK )
  {
    collected_data.timeline_healing_taken.add( sim->current_time(), 0.0 );
    collected_data.timeline_dmg_taken.add( sim->current_time(), 0.0 );
    collected_data.health_changes.timeline.add( sim->current_time(), 0.0 );
    collected_data.health_changes.timeline_normalized.add( sim->current_time(), 0.0 );
  }
  collected_data.collect_data( *this );

  range::for_each( buff_list, std::mem_fn( &buff_t::datacollection_end ) );

  for ( auto& uptime : uptime_list )
  {
    uptime->datacollection_end( iteration_fight_length );
  }

  range::for_each( benefit_list, std::mem_fn( &benefit_t::datacollection_end ) );
  range::for_each( proc_list, std::mem_fn( &proc_t::datacollection_end ) );
  range::for_each( sample_data_list, std::mem_fn( &sample_data_helper_t::datacollection_end ) );
  range::for_each( cooldown_waste_data_list, std::mem_fn( &cooldown_waste_data_t::datacollection_end ) );
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

void report_unmatched( [[maybe_unused]] const buff_t& b )
{
#ifndef NDEBUG
  /* Don't complain about targetdata buffs, since it is perfectly viable that the buff
   * is not created in another thread, because of our on-demand targetdata creation
   */
  if ( !b.source || b.source == b.player )
  {
    b.sim->error( "{} can't merge buff '{}' with source '{}'.", *b.player, b.name(), b.source_name() );
  }
#endif
}

void check_tail( [[maybe_unused]] player_t& p, [[maybe_unused]] size_t first )
{
#ifndef NDEBUG
  for ( size_t last = p.buff_list.size(); first < last; ++first )
    report_unmatched( *p.buff_list[ first ] );
#endif
}

// Merge stats of matching buffs from right into left.
void merge( player_t& left, player_t& right )
{
  prepare( left );
  prepare( right );

  // Both buff_lists are sorted, join them mergesort-style.
  size_t i = 0;
  size_t j = 0;
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
 * After simulating the same setup in multiple sim/threads, this function will merge the same player (and its
 * collected data) from two simulations together, aggregating the collected data.
 */
void player_t::merge( player_t& other )
{
  collected_data.merge( other );

  for ( resource_e i = RESOURCE_NONE; i < RESOURCE_MAX; ++i )
  {
    iteration_resource_lost[ i ] += other.iteration_resource_lost[ i ];
    iteration_resource_gained[ i ] += other.iteration_resource_gained[ i ];
    iteration_resource_overflowed[ i ] += other.iteration_resource_overflowed[ i ];
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
    sample_data_helper_t& sd = *sample_data_list[ i ];
    if ( sample_data_helper_t* other_sd = other.find_sample_data( sd.name_str ) )
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

  // Cooldown waste data
  for ( size_t i = 0; i < cooldown_waste_data_list.size(); i++ )
  {
    const auto& ours   = cooldown_waste_data_list[ i ];
    const auto& theirs = other.cooldown_waste_data_list[ i ];
    assert( ours->cd->name_str == theirs->cd->name_str );
    ours->merge( *theirs );
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

  sim->print_debug( "{} resets current stats ( reset to initial ): {}", *this, current );

  for ( auto& buff : buff_list )
    buff->reset();

  last_foreground_action = nullptr;
  prev_gcd_actions.clear();
  off_gcdactions.clear();

  first_cast = true;

  executing       = nullptr;
  queueing        = nullptr;
  channeling      = nullptr;
  demise_event    = nullptr;
  readying        = nullptr;
  strict_sequence = nullptr;
  off_gcd         = nullptr;
  cast_while_casting_poll_event = nullptr;
  in_combat       = false;

  if ( sim->fight_style == FIGHT_STYLE_DUNGEON_ROUTE || sim->fight_style == FIGHT_STYLE_DUNGEON_SLICE )
    in_boss_encounter = 0;
  else
    in_boss_encounter = 1;

  current_execute_type = execute_type::FOREGROUND;

  current_auto_attack_speed = 1.0;
  gcd_current_haste_value   = 1.0;
  gcd_type = gcd_haste_type::NONE;

  cast_delay_reaction = timespan_t::zero();
  cast_delay_occurred = timespan_t::zero();

  main_hand_weapon.buff_type  = 0;
  main_hand_weapon.buff_value = 0;
  main_hand_weapon.bonus_dmg  = 0;

  off_hand_weapon.buff_type  = 0;
  off_hand_weapon.buff_value = 0;
  off_hand_weapon.bonus_dmg  = 0;

  assert( default_x_position != std::numeric_limits<decltype(default_x_position)>::lowest() );
  assert( default_y_position != std::numeric_limits<decltype(default_y_position)>::lowest() );
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

  range::for_each( target_specific_cooldown_list, []( target_specific_cooldown_t* tcd ) { tcd->reset(); } );

  range::for_each( dot_list, []( dot_t* dot ) { dot->reset(); } );

  range::for_each( stats_list, []( stats_t* stat ) { stat->reset(); } );

  range::for_each( uptime_list, []( uptime_t* uptime ) { uptime->reset(); } );

  range::for_each( proc_list, []( proc_t* proc ) { proc->reset(); } );

  range::for_each( proc_rng_list, []( proc_rng_t* prng ) { prng->reset(); } );

  range::for_each( spawners, []( spawner::base_actor_spawner_t* obj ) { obj->reset(); } );

  precombat_initialized = false;
  potion_used = false;
  leech_pool = 0;

  item_cooldown -> reset( false );

  incoming_damage.clear();

  resource_threshold_trigger = nullptr;

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
  if ( delta_time < 0_ms )
    delta_time = timespan_t::from_millis( rng().gauss( 100, 10 ) );

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
  if ( delta_time < 0_ms )
    delta_time = timespan_t::from_millis( rng().gauss( 100, 10 ) );

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
    threshold = sim->current_time() + channeling->get_dot()->remains();
    threshold += sim->channel_lag.mean + 4 * sim->channel_lag.stddev;
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
    throw std::runtime_error(fmt::format("{} scheduled ready while already ready.", *this ));
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
  if ( gcd_adjust > 0_ms )
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
    timespan_t lag = 0_ms;

    if ( last_foreground_action )
    {
      if ( last_foreground_action->ability_lag.mean > 0_ms )
      {
        timespan_t ability_lag = rng().gauss( last_foreground_action->ability_lag );
        timespan_t gcd_lag = rng().gauss( sim->gcd_lag );
        timespan_t diff    = ( gcd_ready + gcd_lag ) - ( sim->current_time() + ability_lag );
        if ( diff > 0_ms && sim->strict_gcd_queue )
        {
          lag = gcd_lag;
        }
        else
        {
          lag           = ability_lag;
          action_queued = true;
        }
      }
      else if ( last_foreground_action->channeled && !last_foreground_action->interrupt_immediate_occurred )
      {
        lag = rng().gauss( sim->channel_lag );
      }
      else if ( last_foreground_action->gcd() == 0_ms )
      {
        lag = 0_ms;
      }
      else
      {
        timespan_t gcd_lag   = rng().gauss( sim->gcd_lag );
        timespan_t queue_lag = rng().gauss( sim->queue_lag );

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

    if ( lag < 0_ms )
      lag = 0_ms;

    if ( type == PLAYER_GUARDIAN )
      lag = 0_ms;  // Guardians do not seem to feel the effects of queue/gcd lag in WoD.

    delta_time += lag;
  }

  if ( last_foreground_action && last_foreground_action->gcd() > 0_ms )
  {
    // This is why "total_execute_time" is not tracked per-target!
    last_foreground_action->stats->iteration_total_execute_time += delta_time;
  }

  readying = make_event<player_ready_event_t>( *sim, *this, delta_time );

  if ( was_executing && was_executing->gcd() > 0_ms && !was_executing->background &&
       !was_executing->proc && !was_executing->repeating )
  {
    // Record the last ability use time for cast_react
    cast_delay_occurred = readying->occurs();
    if ( !is_pet() )
    {
      cast_delay_reaction = rng().gauss( brain_lag );
    }
    else
    {
      cast_delay_reaction = 0_ms;
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

  demise_event = nullptr;
  readying = nullptr;
  off_gcd  = nullptr;
  cast_while_casting_poll_event = nullptr;

  arise_time = sim->current_time();
  last_regen = sim->current_time();

  if ( buffs.focus_magic && external_buffs.focus_magic )
    buffs.focus_magic->override_buff();

  if ( buffs.soleahs_secret_technique_external )
    buffs.soleahs_secret_technique_external->trigger();

  if ( buffs.elegy_of_the_eternals_external )
    buffs.elegy_of_the_eternals_external->trigger();

  if ( buffs.ingest_mineral )
    buffs.ingest_mineral->trigger();

  if ( is_enemy() )
  {
    sim->active_enemies++;
    sim->target_non_sleeping_list.push_back( this );

    // When an enemy arises, trigger players to potentially acquire a new target
    range::for_each( sim->player_non_sleeping_list, [this]( player_t* p ) { p->acquire_target( retarget_source::ACTOR_ARISE, this ); } );

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
    acquire_target( retarget_source::SELF_ARISE );
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

  current_auto_attack_speed = cache.auto_attack_speed();

  // Requires index-based lookup since on-arise callbacks may
  // insert new on-arise callbacks to the vector.
  for ( size_t i = 0; i < callbacks_on_arise.size(); ++i ) // NOLINT(modernize-loop-convert)
  {
    auto& cb = callbacks_on_arise[ i ];
    // If the callback comes from a different actor, execute it only
    // if that actor is active.
    if ( this == cb.first || cb.first->is_active() )
      cb.second();
  }
}

timespan_t player_t::available() const
{
  return timespan_t::from_millis( rng().gauss( 100, 10 ) );
}

/**
 * Player dies.
 */
void player_t::demise()
{
  // No point in demising anything if we're not even active
  if ( current.sleeping )
    return;

  leave_combat();

  current.sleeping = true;

  if ( sim->log )
    sim->out_log.printf( "%s demises.. Spawn Index=%u", name(), actor_spawn_index );

  /* Do not reset spawn index, because the player can still have damaging events ( dots ) which
   * need to be associated with eg. resolve Diminishing Return list.
   */

  if ( arise_time >= 0_ms )
  {
    iteration_fight_length += sim->current_time() - arise_time;
  }
  // Arise time has to be set to default value before actions are canceled.
  arise_time               = timespan_t::min();
  current.distance_to_move = 0;

  event_t::cancel( readying );
  event_t::cancel( off_gcd );
  event_t::cancel( cast_while_casting_poll_event );

  if ( is_enemy() )
  {
    sim->active_enemies--;
    sim->target_non_sleeping_list.find_and_erase_unordered( this );

    // When an enemy dies, trigger players to acquire a new target
    range::for_each( sim->player_non_sleeping_list,
                     [ this ]( player_t* p ) { p->acquire_target( retarget_source::ACTOR_DEMISE, this ); } );
  }
  else
  {
    sim->active_allies--;
    sim->player_non_sleeping_list.find_and_erase_unordered( this );
  }

  // If an enemy mob dies, trigger on-kill callback on all active players
  if ( is_enemy() )
  {
    for ( auto p : sim->player_non_sleeping_list )
    {
      // Use index-based lookup since on-kill callbacks may insert new on-kill callbacks to the vector
      for ( size_t i = 0; i < p->callbacks_on_kill.size(); ++i )  // NOLINT(modernize-loop-convert)
      {
        p->callbacks_on_kill[ i ]( this );
      }
    }
  }

  // Requires index-based lookup since on-demise callbacks may
  // insert new on-demise callbacks to the vector.
  for ( size_t i = 0; i < callbacks_on_demise.size(); ++i ) // NOLINT(modernize-loop-convert)
  {
    auto& cb = callbacks_on_demise[ i ];
    // If the callback comes from a different actor, execute it only
    // if that actor is active.
    if ( this == cb.first || cb.first->is_active() )
      cb.second( this );
  }

  for ( size_t i = 0; i < buff_list.size(); ++i )
    buff_list[ i ]->cancel();

  for ( size_t i = 0; i < action_list.size(); ++i )
    action_list[ i ]->cancel();

  // sim -> cancel_events( this );

  for ( auto* pet : pet_list )
  {
    pet->demise();
  }

  for ( size_t i = 0; i < dot_list.size(); ++i )
    dot_list[ i ]->cancel();

}

// Player enters/leaves "combat". Primarily relevant for DungeonSlice/DungeonRoute sims where "combat" is defined as
// sim_t::target_non_sleeping_list.size() > 0
// Use index-based lookup since enter/leaving combat may insert new combat state callbacks to the vector
void player_t::enter_combat()
{
  if ( in_combat )
    return;

  in_combat = true;

  sim->print_log( "{} enters combat.", *this );

  for ( size_t i = 0; i < callbacks_on_combat_state.size(); ++i )
    callbacks_on_combat_state[ i ]( this, in_combat );
}

void player_t::leave_combat()
{
  if ( !in_combat )
    return;

  in_combat = false;

  sim->print_log( "{} leaves combat.", *this );

  for ( size_t i = 0; i < callbacks_on_combat_state.size(); ++i )
    callbacks_on_combat_state[ i ]( this, in_combat );
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
    strict_sequence = nullptr;
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
  halt();
}

void player_t::finish_moving()
{
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

  action_t* action = nullptr;

  if (resource_regeneration == regen_type::DYNAMIC)
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

  if ( restore_action_list != nullptr )
  {
    activate_action_list( restore_action_list, restore_action_list_type );
    restore_action_list = nullptr;
  }

  if ( action )
  {
    action->line_cooldown->start();
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
  if (resource_regeneration == regen_type::DYNAMIC&& sim->debug )
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


double player_t::get_stat_value(stat_e stat)
{
  switch (stat)
  {
  case STAT_STRENGTH:
    return cache.strength();
  case STAT_AGILITY:
    return cache.agility();
  case STAT_INTELLECT:
    return cache.intellect();
  case STAT_SPELL_POWER:
    return cache.spell_power( SCHOOL_NONE );
  case STAT_ATTACK_POWER:
    return cache.attack_power();
  case STAT_CRIT_RATING:
    return composite_melee_crit_rating();
  case STAT_HASTE_RATING:
    return composite_melee_haste_rating();
  case STAT_MASTERY_RATING:
    return composite_mastery_rating();
  case STAT_VERSATILITY_RATING:
    return composite_damage_versatility_rating();
  case STAT_ARMOR:
    return cache.armor();
  default:
    break;
  }

  return 0.0;
}

void player_t::collect_resource_timeline_information()
{
  for ( auto& elem : collected_data.resource_timelines )
  {
    elem.timeline.add( sim->current_time(), resources.current[ elem.type ] );
  }

  collected_data.health_pct.add( sim->current_time(), health_percentage() );

  for ( auto& elem : collected_data.stat_timelines )
  {
    auto value = get_stat_value(elem.type);
    elem.timeline.add(sim->current_time(), value);
  }
}

double player_t::resource_loss( resource_e resource_type, double amount, gain_t* source, action_t* )
{
  if ( amount == 0 )
    return 0.0;

  if ( current.sleeping )
    return 0.0;

  if ( resource_type && resource_type == primary_resource() )
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
    sim->print_debug( "Player {} loses {:.2f} ({:.2f}) {}. pct={:.2f}% ({:.2f}/{:.2f})",
                      name(), actual_amount, amount, resource_type,
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
  double overflow_amount = amount - actual_amount;
  if (overflow_amount > 0)
  {
    iteration_resource_overflowed[ resource_type ] += overflow_amount;
  }

  if ( resource_type && resource_type == primary_resource() &&
       resources.max[ resource_type ] <= resources.current[ resource_type ] )
    uptimes.primary_resource_cap->update( true, sim->current_time() );

  if ( source )
  {
    source->add( resource_type, actual_amount, amount - actual_amount );
  }

  if ( sim->log )
  {
    sim->print_log( "{} gains {:.2f} ({:.2f}) {} from {} ({:.2f}/{:.2f})",
                    name(), actual_amount, amount, resource_type,
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

void player_t::recalculate_resource_max( resource_e resource_type, gain_t* source )
{
  double old_amount = resources.current[ resource_type ];
  double old_max    = resources.max[ resource_type ];

  resources.max[ resource_type ] = resources.base[ resource_type ];
  resources.max[ resource_type ] *= resources.base_multiplier[ resource_type ];
  resources.max[ resource_type ] += total_gear.resource[ resource_type ];
  resources.max[ resource_type ] += resources.temporary[ resource_type ];

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

  resources.max[ resource_type ] *= resources.initial_multiplier[ resource_type ];

  // Sanity check on current values
  if ( source && resources.current[ resource_type ] > resources.max[ resource_type ] )
  {
    // Track overflow from loss if applicable
    source->add( resource_type, 0, resources.current[ resource_type ] - resources.max[ resource_type ] );
  }
  resources.current[ resource_type ] = std::min( resources.current[ resource_type ], resources.max[ resource_type ] );

  sim->print_debug( "Recalculated maximum {} for {}: old={:.2f}/{:.2f}, new={:.2f}/{:.2f}",
                    util::resource_type_string( resource_type ), name(), old_amount, old_max,
                    resources.current[ resource_type ], resources.max[ resource_type ] );
}

role_e player_t::primary_role() const
{
  return role;
}

const char* player_t::primary_tree_name() const
{
  return dbc::specialization_string( specialization() );
}

bool player_t::has_shield_equipped() const
{
  return  items[ SLOT_OFF_HAND ].parsed.data.item_subclass == ITEM_SUBCLASS_ARMOR_SHIELD;
}

bool player_t::record_healing() const
{
  return role == ROLE_TANK || role == ROLE_HEAL || sim -> enable_dps_healing;
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
  else if ( role == ROLE_TANK && sm == SCALE_METRIC_DEATHS &&
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
  // Adjust time_to_0/time_to_die expressions to point to the death_pct, if applicable
  if ( percent == 0.0 )
    percent = death_pct;

  // Early exit if we're already at the relevant health percentage
  if ( health_percentage() <= percent )
    return timespan_t::zero();

  timespan_t time_to_percent;

  if ( iteration_dmg_taken > 0.0 && resources.base[ RESOURCE_HEALTH ] > 0 &&
       sim->current_time() >= timespan_t::from_seconds( 1.0 ) && !sim->fixed_time )
  {
    // If not using fixed_time and we have tracked damage intake, use the incoming DPS to predict
    double ttp = ( resources.current[ RESOURCE_HEALTH ] - ( percent * 0.01 * resources.base[ RESOURCE_HEALTH ] ) ) /
      ( iteration_dmg_taken / sim->current_time().total_seconds() );
    time_to_percent = timespan_t::from_seconds( ttp );
  }
  else
  {
    // Otherwise, just use the simulation length as a basis for the predictions
    time_to_percent = sim->expected_iteration_time * ( 1.0 - percent * 0.01 ) - sim->current_time();
  }

  if ( time_to_percent < timespan_t::zero() )
    return timespan_t::zero();

  return time_to_percent;
}

timespan_t player_t::total_reaction_time()
{
  return std::min( reaction_max, reaction_offset + rng().exgauss( reaction, reaction_nu ) );
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
  if (resource_regeneration == regen_type::DYNAMIC&& regen_caches[ cache_type ] )
    do_dynamic_regen( true );

  sim->print_log( "{} gains {:.2f} {}{}", name(), amount, stat,
                  temporary_stat ? " (temporary)" : "" );

  switch ( stat )
  {
    case STAT_STAMINA:
    case STAT_STRENGTH:
    case STAT_AGILITY:
    case STAT_INTELLECT:
    case STAT_AGI_INT:
    case STAT_STR_AGI:
    case STAT_STR_INT:
    case STAT_STR_AGI_INT:
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
    case STAT_CORRUPTION_RESISTANCE:
      current.stats.add_stat( stat, amount );
      invalidate_cache( cache_type );
      break;

    case STAT_HASTE_RATING:
    {
      current.stats.add_stat( stat, amount );
      invalidate_cache( cache_type );

      adjust_dynamic_cooldowns();
      // adjust_global_cooldown( gcd_type::HASTE_ANY );
      adjust_auto_attack(gcd_haste_type::HASTE );
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

  if ( sim->debug )
  {
    sim->out_debug.print( "{} stats: {}", name(), current.stats );
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
  if (resource_regeneration == regen_type::DYNAMIC&& regen_caches[ cache_type ] )
    do_dynamic_regen( true );

  sim->print_log( "{} loses {:.2f} {}{}", name(), amount, stat,
                  temporary_buff ? " (temporary)" : "" );

  int temp_value = temporary_buff ? 1 : 0;
  switch ( stat )
  {
    case STAT_STAMINA:
    case STAT_STRENGTH:
    case STAT_AGILITY:
    case STAT_INTELLECT:
    case STAT_AGI_INT:
    case STAT_STR_AGI:
    case STAT_STR_INT:
    case STAT_STR_AGI_INT:
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
    case STAT_CORRUPTION_RESISTANCE:
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
      // adjust_global_cooldown( gcd_type::HASTE_ANY );
      adjust_auto_attack(gcd_haste_type::HASTE );
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

  if ( sim->debug )
  {
    sim->out_debug.print( "{} stats: {}", name(), current.stats );
  }
}

void player_t::cost_reduction_gain( school_e school, double amount, gain_t* /* gain */, action_t* /* action */ )
{
  if ( amount <= 0 )
    return;

  sim->print_log( "{} gains a cost reduction of {:.1f} on abilities of school {}", name(), amount, school );

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

  sim->print_log( "{} loses a cost reduction of {:.1f} on abilities of school {}", name(), amount, school );

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
        // 1) if we parry when the current swing timer has 1.8 seconds remaining, then it gets reduced by 40% of 2.0,
        // or 0.8 seconds,
        //    and the current swing timer becomes 1.0 seconds.
        // 2) if we parry when the current swing timer has 1.0 second remaining the game tries to subtract 0.8
        // seconds, but hits the
        //    minimum value (20% of 2.0, or 0.4 seconds.  The current swing timer becomes 0.4 seconds.
        // Thus, the result is that the current swing timer becomes
        // max(current_swing_timer-0.4*base_swing_timer,0.2*base_swing_timer)

        // the reschedule_execute(x) function we call to perform this tries to reschedule the effect such that it
        // occurs at (sim->current_time() + x).  Thus we need to give it the difference between sim->current_time()
        // and the new target of execute_event->occurs(). That value is simply the remaining time on the current swing
        // timer.

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
    double chance = p.buffs.blessing_of_sacrifice->default_chance;
    if ( chance < 0 )
      chance = s->action->ppm_proc_chance( -chance );

    p.buffs.blessing_of_sacrifice->trigger( 0, redirected_damage, chance, 0_ms );

    // mitigate that amount from the target.
    // Slight inaccuracy: We do not get a feedback of paladin health buffer expiration here.
    s->result_amount -= redirected_damage;

    if ( p.sim->debug && s->action && !s->target->is_enemy() && !s->target->is_add() )
      p.sim->out_debug.printf( "Damage to %s after Blessing of Sacrifice is %f", s->target->name(), s->result_amount );
  }
}

bool absorb_sort( absorb_buff_t* a, absorb_buff_t* b )
{
  double lv = a->current_value;
  double rv = a->current_value;
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
              double absorbed = ab->consume( s->result_amount, s );

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

/**
 * Statistical data collection for damage taken.
 */
void collect_dmg_taken_data( player_t& p, const action_state_t* s, double /* result_ignoring_external_absorbs */ )
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
    // stats_t* stat = buffs.guardian_spirit -> source ? buffs.guardian_spirit -> source -> get_stats(
    // "guardian_spirit" ) : 0; double gs_amount = resources.max[ RESOURCE_HEALTH ] * buffs.guardian_spirit ->
    // data().effectN( 2
    // ).percent(); resource_gain( RESOURCE_HEALTH, s -> result_amount ); if ( stat ) stat -> add_result( gs_amount,
    // gs_amount, result_amount_type::HEAL_DIRECT, RESULT_HIT );
    p.buffs.guardian_spirit->expire();
    return true;
  }

  return false;
}

}  // namespace assess_dmg_helper_functions

}

void player_t::assess_damage( school_e school, result_amount_type type, action_state_t* s )
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
  if ( incoming_state->action && incoming_state->action->callbacks && !incoming_state->action->suppress_target_procs )
  {
    proc_types pt = incoming_state->proc_type();
    if ( pt != PROC1_INVALID )
    {
      // On damage/heal in. Proc flags are arranged as such that the "incoming"
      // version of the primary proc flag is always follows the outgoing version...
      proc_types pt_taken = static_cast<proc_types>( pt + 1 );
      // ...except for PROC1_HELPFUL_PERIODIC_TAKEN
      if ( pt == PROC1_HELPFUL_PERIODIC )
        pt_taken = PROC1_HELPFUL_PERIODIC_TAKEN;

      // Because most procs in simc default to using PROC2_LANDED for most proc types,
      // trigger the execute_proc_type2() here to ensure that those procs will work.
      proc_types2 execute_pt2 = incoming_state->execute_proc_type2();
      if ( execute_pt2 != PROC2_INVALID )
        trigger_callbacks( pt_taken, execute_pt2, incoming_state->action, incoming_state );

      // Additionally, trigger the impact_proc_type2() so that periodic effects and
      // procs not using execute proc types will also work.
      proc_types2 impact_pt2 = incoming_state->impact_proc_type2();
      if ( impact_pt2 != PROC2_INVALID )
        trigger_callbacks( pt_taken, impact_pt2, incoming_state->action, incoming_state );
    }
  }

  // Check if target is dying
  if ( !demise_event && health_percentage() <= death_pct && !resources.is_infinite( RESOURCE_HEALTH ) )
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

void player_t::assess_damage_imminent_pre_absorb( school_e, result_amount_type, action_state_t* )
{
}

void player_t::assess_damage_imminent( school_e, result_amount_type, action_state_t* )
{
}

void player_t::target_mitigation( school_e school, result_amount_type dmg_type, action_state_t* s )
{
  if ( s->result_amount == 0 )
    return;

  if ( buffs.pain_suppression && buffs.pain_suppression->up() )
    s->result_amount *= 1.0 + buffs.pain_suppression->data().effectN( 1 ).percent();

  if ( buffs.stoneform && buffs.stoneform->up() )
    s->result_amount *= 1.0 + buffs.stoneform->data().effectN( 1 ).percent();

  if ( buffs.fortitude && buffs.fortitude->up() )
    s->result_amount *= 1.0 + buffs.fortitude->check_value();

  if ( buffs.elemental_chaos_earth && buffs.elemental_chaos_earth->check() )
    s->result_amount *= 1.0 + buffs.elemental_chaos_earth->check_value();

  if ( s->action->is_aoe() )
    s->result_amount *= 1.0 - cache.avoidance();

  // TODO-WOD: Where should this be? Or does it matter?
  s->result_amount *= 1.0 - cache.mitigation_versatility();

  if ( debuffs.invulnerable && debuffs.invulnerable->check() )
  {
    s->result_amount = 0;
  }

  if ( school == SCHOOL_PHYSICAL && dmg_type == result_amount_type::DMG_DIRECT )
  {
    if ( s->action && !s->target->is_enemy() && !s->target->is_add() )
      sim->print_debug( "Damage to {} before armor mitigation is {}", s->target->name(), s->result_amount );

    // Maximum amount of damage reduced by armor
    double armor_cap = 0.85;

    // Armor
    if ( s->action && !s->action->ignores_armor )
    {
      double armor  = s -> target_armor;
      double resist = armor / ( armor + s -> action -> player -> base.armor_coeff );
      resist        = clamp( resist, 0.0, armor_cap );
      s -> result_amount *= 1.0 - resist;
    }

    if ( s->action && !s->target->is_enemy() && !s->target->is_add() )
    {
      if ( s->action->ignores_armor )
        sim->print_debug( "Damage to {} after armor mitigation is {} (ignores armor)", s->target->name(), s->result_amount );
      else
        sim->print_debug( "Damage to {} after armor mitigation is {} ({} armor, {} armor coeff)",
                          s->target->name(), s->result_amount, s->target_armor, s->action->player->current.armor_coeff );
    }

    double pre_block_amount = s->result_amount;

    // In BfA, Block and Crit Block work in the same manner as armor and are affected by the same cap
    if ( s ->action && (s -> block_result == BLOCK_RESULT_BLOCKED || s -> block_result == BLOCK_RESULT_CRIT_BLOCKED ))
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

void player_t::assess_heal( school_e, result_amount_type, action_state_t* s )
{
  // Increases to healing taken should modify result_total in order to correctly calculate overhealing
  // and other effects based on raw healing.
  if ( buffs.guardian_spirit->up() )
    s->result_total *= 1.0 + buffs.guardian_spirit->data().effectN( 1 ).percent();

  if ( buffs.blessing_of_spring->up() )
    s->result_total *= 1.0 + buffs.blessing_of_spring->data().effectN( 2 ).percent();

  // process heal
  s->result_amount = resource_gain( RESOURCE_HEALTH, s->result_total, nullptr, s->action );

  // if the target is a tank record this event on damage timeline
  if ( !is_pet() && primary_role() == ROLE_TANK )
  {
    // health_changes and timeline_healing_taken record everything, accounting for overheal and so on
    collected_data.timeline_healing_taken.add( sim->current_time(), -( s->result_amount ) );
    collected_data.health_changes.timeline.add( sim->current_time(), -( s->result_amount ) );
    double normalized =
        resources.max[ RESOURCE_HEALTH ] ? -( s->result_amount ) / resources.max[ RESOURCE_HEALTH ] : 0.0;
    collected_data.health_changes.timeline_normalized.add( sim->current_time(), normalized );
  }

  // store iteration heal taken
  iteration_heal_taken += s->result_amount;
}

void player_t::trigger_callbacks( proc_types type, proc_types2 type2, action_t* action, action_state_t* state )
{
  action_callback_t::trigger( callbacks.procs[ type ][ type2 ], action, state );
}

void player_t::summon_pet( util::string_view pet_name, const timespan_t duration )
{
  if ( pet_t* p = find_pet( pet_name ) )
    p->summon( duration );
  else
    sim->error( "Player {} is unable to summon pet '{}'\n", name(), pet_name );
}

void player_t::dismiss_pet( util::string_view pet_name )
{
  pet_t* p = find_pet( pet_name );
  if ( !p )
  {
    sim->error( "Player {}: Could not find pet with name '{}' to dismiss.", name(), pet_name );
    return;
  }
  p->dismiss();
}

bool player_t::recent_cast() const
{
  return ( last_cast > timespan_t::zero() ) &&
         ( ( last_cast + timespan_t::from_seconds( 5.0 ) ) > sim->current_time() );
}

dot_t* player_t::find_dot( util::string_view name, player_t* source ) const
{
  for ( dot_t* d : dot_list )
  {
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
void player_t::copy_action_priority_list( util::string_view old_list, util::string_view new_list )
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
T* find_vector_member( const std::vector<T*>& list, util::string_view name )
{
  for ( auto t : list )
  {
    if ( t->name_str == name )
      return t;
  }
  return nullptr;
}

action_priority_list_t* player_t::find_action_priority_list( util::string_view name ) const
{
  return find_vector_member( action_priority_list, name );
}

pet_t* player_t::find_pet( util::string_view name ) const
{
  return find_vector_member( pet_list, name );
}

stats_t* player_t::find_stats( util::string_view name ) const
{
  return find_vector_member( stats_list, name );
}

gain_t* player_t::find_gain( util::string_view name ) const
{
  return find_vector_member( gain_list, name );
}

proc_t* player_t::find_proc( util::string_view name ) const
{
  return find_vector_member( proc_list, name );
}

sample_data_helper_t* player_t::find_sample_data( util::string_view name ) const
{
  return find_vector_member( sample_data_list, name );
}

benefit_t* player_t::find_benefit( util::string_view name ) const
{
  return find_vector_member( benefit_list, name );
}

uptime_t* player_t::find_uptime( util::string_view name ) const
{
  return find_vector_member( uptime_list, name );
}

cooldown_t* player_t::find_cooldown( util::string_view name ) const
{
  return find_vector_member( cooldown_list, name );
}

target_specific_cooldown_t* player_t::find_target_specific_cooldown( util::string_view name ) const
{
  return find_vector_member( target_specific_cooldown_list, name );
}

action_t* player_t::find_action( util::string_view name ) const
{
  return find_vector_member( action_list, name );
}

cooldown_t* player_t::get_cooldown( util::string_view name, action_t* a )
{
  cooldown_t* c = find_cooldown( name );

  if ( !c )
  {
    c = new cooldown_t( name, *this );

    cooldown_list.push_back( c );
  }

  if ( a )
    c->action = a;

  return c;
}

target_specific_cooldown_t* player_t::get_target_specific_cooldown( util::string_view name, timespan_t duration )
{
  target_specific_cooldown_t* tcd = find_target_specific_cooldown( name );

  if ( !tcd )
  {
    tcd = new target_specific_cooldown_t( name, *this, duration );
    target_specific_cooldown_list.push_back( tcd );
  }

  return tcd;
}

target_specific_cooldown_t* player_t::get_target_specific_cooldown( cooldown_t& base_cooldown )
{
  target_specific_cooldown_t* tcd = find_target_specific_cooldown( base_cooldown.name() );

  if ( !tcd )
  {
    tcd = new target_specific_cooldown_t( *this, base_cooldown );
    target_specific_cooldown_list.push_back( tcd );
  }

  return tcd;
}

simple_proc_t* player_t::get_simple_proc_rng( std::string_view name, double chance )
{
  return get_rng<simple_proc_t>( name, chance );
}

real_ppm_t* player_t::get_rppm( std::string_view name, double frequency, double modifier, unsigned scales_with,
                                real_ppm_t::blp blp_state )
{
  return get_rng<real_ppm_t>( name, frequency, modifier, scales_with, blp_state );
}

real_ppm_t* player_t::get_rppm( std::string_view name, const spell_data_t* spell_data, const item_t* item )
{
  return get_rng<real_ppm_t>( name, spell_data, item );
}

shuffled_rng_t* player_t::get_shuffled_rng( std::string_view name, shuffled_rng_t::initializer data )
{
  return get_rng<shuffled_rng_t>( name, data );
}

shuffled_rng_t* player_t::get_shuffled_rng( std::string_view name, int success_entries, int total_entries )
{
  return get_rng<shuffled_rng_t>( name, success_entries, total_entries );
}

accumulated_rng_t* player_t::get_accumulated_rng( std::string_view name, double chance,
                                                  std::function<double( double, unsigned )> accumulator_fn,
                                                  unsigned initial_count )
{
  return get_rng<accumulated_rng_t>( name, chance, accumulator_fn, initial_count );
}

threshold_rng_t* player_t::get_threshold_rng( std::string_view name, double increment_max,
                                              std::function<double( double )> accumulator_fn, bool random_initial_state,
                                              bool roll_over )
{
  return get_rng<threshold_rng_t>( name, increment_max, accumulator_fn, random_initial_state, roll_over );
}

dot_t* player_t::get_dot( util::string_view name, player_t* source )
{
  dot_t* d = find_dot( name, source );

  if ( !d )
  {
    d = new dot_t( name, this, source );
    dot_list.push_back( d );
  }

  return d;
}

gain_t* player_t::get_gain( util::string_view name )
{
  gain_t* g = find_gain( name );

  if ( !g )
  {
    g = new gain_t( name );

    gain_list.push_back( g );
  }

  return g;
}

proc_t* player_t::get_proc( util::string_view name )
{
  proc_t* p = find_proc( name );

  if ( !p )
  {
    p = new proc_t( *sim, name );

    proc_list.push_back( p );
  }

  return p;
}

sample_data_helper_t* player_t::get_sample_data( util::string_view name )
{
  sample_data_helper_t* sd = find_sample_data( name );

  if ( !sd )
  {
    sd = new sample_data_helper_t( name, generic_container_type(this, 3));

    sample_data_list.push_back( sd );
  }

  return sd;
}

stats_t* player_t::get_stats( util::string_view n, action_t* a )
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

benefit_t* player_t::get_benefit( util::string_view name )
{
  benefit_t* u = find_benefit( name );

  if ( !u )
  {
    u = new benefit_t( name );

    benefit_list.push_back( u );
  }

  return u;
}

uptime_t* player_t::get_uptime( util::string_view name )
{
  uptime_t* u = find_uptime( name );

  if ( !u )
  {
    u = new uptime_t( name );

    uptime_list.push_back( u );
  }

  return u;
}

action_priority_list_t* player_t::get_action_priority_list( util::string_view name, util::string_view comment )
{
  action_priority_list_t* a = find_action_priority_list( name );
  if ( !a )
  {
    if ( action_list_id_ == 64 )
    {
      throw std::invalid_argument("Maximum number of action lists is 64");
    }

    a                   = new action_priority_list_t( name, this, comment );
    a->internal_id      = action_list_id_++;
    a->internal_id_mask = 1ULL << ( a->internal_id );

    action_priority_list.push_back( a );
  }
  return a;
}

int player_t::find_action_id( util::string_view name ) const
{
  for ( size_t i = 0; i < action_map.size(); i++ )
  {
    if ( util::str_compare_ci( name, action_map[ i ] ) )
      return static_cast<int>( i );
  }

  return -1;
}

int player_t::get_action_id( util::string_view name )
{
  auto id = find_action_id( name );
  if ( id != -1 )
  {
    return id;
  }

  action_map.emplace_back( name );
  return static_cast<int>(action_map.size() - 1);
}

int player_t::find_dot_id( util::string_view name ) const
{
  for ( size_t i = 0; i < dot_map.size(); i++ )
  {
    if ( util::str_compare_ci( name, dot_map[ i ] ) )
      return static_cast<int>( i );
  }

  return -1;
}

int player_t::get_dot_id( util::string_view name )
{
  auto id = find_dot_id( name );
  if ( id != -1 )
  {
    return id;
  }

  dot_map.emplace_back( name );
  return static_cast<int>( dot_map.size() - 1 );
}

cooldown_waste_data_t* player_t::get_cooldown_waste_data( const cooldown_t* cd )
{
  for ( const auto& cdw : cooldown_waste_data_list )
  {
    if ( cdw->cd->name_str == cd->name_str )
      return cdw.get();
  }

  cooldown_waste_data_list.push_back( std::make_unique<cooldown_waste_data_t>( cd ) );
  return cooldown_waste_data_list.back().get();
}

namespace
{  // ANONYMOUS

// Chosen Movement Actions ==================================================

struct start_moving_t : public action_t
{
  start_moving_t( player_t* player, util::string_view options_str ) : action_t( ACTION_OTHER, "start_moving", player )
  {
    parse_options( options_str );
    trigger_gcd           = timespan_t::zero();
    cooldown->duration    = timespan_t::from_seconds( 0.5 );
    harmful               = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    player->buffs.movement->trigger();

    if ( sim->log )
      sim->out_log.printf( "%s starts moving.", player->name() );

    update_ready();
  }

  bool ready() override
  {
    if ( player->buffs.movement->check() )
      return false;

    return action_t::ready();
  }
};

struct stop_moving_t : public action_t
{
  stop_moving_t( player_t* player, util::string_view options_str ) : action_t( ACTION_OTHER, "stop_moving", player )
  {
    parse_options( options_str );
    trigger_gcd           = timespan_t::zero();
    cooldown->duration    = timespan_t::from_seconds( 0.5 );
    harmful               = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    player->buffs.movement->expire();

    if ( sim->log )
      sim->out_log.printf( "%s stops moving.", player->name() );
    update_ready();
  }

  bool ready() override
  {
    if ( !player->buffs.movement->check() )
      return false;

    return action_t::ready();
  }
};


// ===== Racial Abilities ===================================================

struct racial_spell_t : public spell_t
{
  racial_spell_t( player_t* p, util::string_view token, const spell_data_t* spell ) :
    spell_t( token, p, spell )
  {
  }

  void init() override
  {
    spell_t::init();

    if ( &data() == spell_data_t::not_found() )
      background = true;
  }
};

struct racial_heal_t : public heal_t
{
  racial_heal_t( player_t* p, util::string_view token, const spell_data_t* spell ) :
    heal_t( token, p, spell )
  { }

  void init() override
  {
    heal_t::init();

    if ( &data() == spell_data_t::not_found() )
      background = true;
  }
};

// Shadowmeld ===============================================================

struct shadowmeld_t : public racial_spell_t
{
  shadowmeld_t( player_t* p, util::string_view options_str ) :
    racial_spell_t( p, "shadowmeld", p->find_racial_spell( "Shadowmeld" ) )
  {
    parse_options( options_str );
  }

  void execute() override
  {
    racial_spell_t::execute();

    player->buffs.shadowmeld->trigger();

    // Shadowmeld stops autoattacks
    player->cancel_auto_attacks();

    if ( !player->in_boss_encounter )
      player->leave_combat();
  }
};

// Arcane Torrent ===========================================================

struct arcane_torrent_t : public racial_spell_t
{
  double gain_pct;
  double gain_energy;

  arcane_torrent_t( player_t* p, util::string_view options_str ) :
    racial_spell_t( p, "arcane_torrent", p->find_racial_spell( "Arcane Torrent" ) ),
    gain_pct( 0 ),
    gain_energy( 0 )
  {
    parse_options( options_str );
    harmful = false;
    energize_type = action_energize::ON_CAST;

    target = p;

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
      case MONK_BREWMASTER:
        gain_energy = data().effectN( 4 ).base_value();
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
      double gain_mana = player->resources.max[ RESOURCE_MANA ] * gain_pct;
      player->resource_gain( RESOURCE_MANA, gain_mana, gain );
    }

    if ( gain_energy > 0 )
      player->resource_gain( RESOURCE_ENERGY, gain_energy, gain );
  }
};

// Berserking ===============================================================

struct berserking_t : public racial_spell_t
{
  berserking_t( player_t* p, util::string_view options_str ) :
    racial_spell_t( p, "berserking", p->find_racial_spell( "Berserking" ) )
  {
    parse_options( options_str );
    harmful = false;
    target = p;
  }

  void execute() override
  {
    racial_spell_t::execute();

    player->buffs.berserking->trigger();
  }
};

// Blood Fury ===============================================================

struct blood_fury_t : public racial_spell_t
{
  blood_fury_t( player_t* p, util::string_view options_str ) :
    racial_spell_t( p, "blood_fury", p->find_racial_spell( "Blood Fury" ) )
  {
    parse_options( options_str );
    harmful = false;
    target = p;
  }

  void execute() override
  {
    racial_spell_t::execute();

    player->buffs.blood_fury->trigger();
  }
};

// Darkflight ==============================================================

struct darkflight_t : public racial_spell_t
{
  darkflight_t( player_t* p, util::string_view options_str ) :
    racial_spell_t( p, "darkflight", p->find_racial_spell( "Darkflight" ) )
  {
    parse_options( options_str );
    target = p;
  }

  void execute() override
  {
    racial_spell_t::execute();

    player->buffs.darkflight->trigger();
  }
};

// Rocket Barrage ===========================================================

struct rocket_barrage_t : public racial_spell_t
{
  rocket_barrage_t( player_t* p, util::string_view options_str ) :
    racial_spell_t( p, "rocket_barrage", p->find_racial_spell( "Rocket Barrage" ) )
  {
    parse_options( options_str );
    // This extra damage is hardcoded in the tooltip.
    base_dd_min = base_dd_max = 2 * p->level();
  }
};

// Stoneform ================================================================

struct stoneform_t : public racial_spell_t
{
  stoneform_t( player_t* p, util::string_view options_str ) :
    racial_spell_t( p, "stoneform", p->find_racial_spell( "Stoneform" ) )
  {
    parse_options( options_str );
    harmful = false;
    target = p;
  }

  void execute() override
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
      auto ap = composite_total_attack_power();
      auto sp = composite_total_spell_power();

      if ( ap <= sp )
        return 0;
      return spell_t::attack_direct_power_coefficient( s );
    }

    double spell_direct_power_coefficient( const action_state_t* s ) const override
    {
      auto ap = composite_total_attack_power();
      auto sp = composite_total_spell_power();

      if ( ap > sp )
        return 0;
      return spell_t::spell_direct_power_coefficient( s );
    }
  };

  action_t* damage;

  lights_judgment_t( player_t* p, util::string_view options_str ) :
    racial_spell_t( p, "lights_judgment", p->find_racial_spell( "Light's Judgment" ) )
  {
    parse_options( options_str );
    // The cast doesn't trigger combat
    may_miss = callbacks = harmful = false;

    damage = p->find_action( "lights_judgment_damage" );
    if ( damage == nullptr )
      damage = new lights_judgment_damage_t( p );

    add_child( damage );
  }

  void execute() override
  {
    racial_spell_t::execute();
    // Reduce the cooldown by the player's gcd when used during precombat
    if ( ! player -> in_combat && is_precombat )
    {
      cooldown -> adjust( - player -> base_gcd );

      sim -> print_debug( "{} used Light's Judgment in precombat with precombat time = {}, adjusting travel time and remaining cooldown.",
                          player -> name(), player -> base_gcd.total_seconds() );
    }
  }

  // Missile travels for 3 seconds, stored in missile speed
  timespan_t travel_time() const override
  {
    // Reduce the delay before the hit by the player's gcd when used during precombat
    if ( ! player -> in_combat && is_precombat )
      return racial_spell_t::travel_time() - player -> base_gcd;

    return racial_spell_t::travel_time();
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
  arcane_pulse_t( player_t* p, util::string_view options_str ) :
    racial_spell_t( p, "arcane_pulse", p->find_racial_spell( "Arcane Pulse" ) )
  {
    parse_options( options_str );
    may_crit = true;
    aoe      = -1;
    // these are sadly hardcoded in the tooltip
    attack_power_mod.direct = 0.5;
    spell_power_mod.direct = 0.25;
  }

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    auto ap = composite_total_attack_power();
    auto sp = composite_total_spell_power();

    if ( ap <= sp )
      return 0;
    return racial_spell_t::attack_direct_power_coefficient( s );
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    auto ap = composite_total_attack_power();
    auto sp = composite_total_spell_power();

    if ( ap > sp )
      return 0;
    return racial_spell_t::spell_direct_power_coefficient( s );
  }
};

// Gift of the Naaru ========================================================

struct gift_of_the_naaru : public racial_heal_t
{
  gift_of_the_naaru( player_t* p, util::string_view options_str ) :
    racial_heal_t( p, "gift_of_the_naaru", p->find_racial_spell( "Gift of the Naaru" ) )
  {
    parse_options( options_str );
    target = p;
  }

  double base_ta( const action_state_t* /* state */ ) const override
  {
    return player->max_health() * data().effectN( 2 ).percent();
  }
};

// Ancestral Call ===========================================================

struct ancestral_call_t : public racial_spell_t
{
  ancestral_call_t( player_t* p, util::string_view options_str ) :
    racial_spell_t( p, "ancestral_call", p->find_racial_spell( "Ancestral Call" ) )
  {
    parse_options( options_str );
    harmful = false;
    target = p;
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
  fireblood_t( player_t* p, util::string_view options_str ) :
    racial_spell_t( p, "fireblood", p->find_racial_spell( "Fireblood" ) )
  {
    parse_options( options_str );
    harmful = false;
    target = p;
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
  haymaker_t( player_t* p, util::string_view options_str ) :
    racial_spell_t( p, "haymaker", p->find_racial_spell( "Haymaker" ) )
  {
    parse_options( options_str );
    may_crit = true;
    // Hardcoded in the tooltip.
    attack_power_mod.direct = 0.75;
    spell_power_mod.direct = 0.75;
  }

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    auto ap = composite_total_attack_power();
    auto sp = composite_total_spell_power();

    if ( ap <= sp )
      return 0;

    return racial_spell_t::attack_direct_power_coefficient( s );
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    auto ap = composite_total_attack_power();
    auto sp = composite_total_spell_power();

    if ( ap > sp )
      return 0;

    return racial_spell_t::spell_direct_power_coefficient( s );
  }
};

// Bag of Tricks ===========================================================
struct bag_of_tricks_t : public racial_spell_t
{
  bag_of_tricks_t( player_t* p, util::string_view options_str )
    : racial_spell_t( p, "bag_of_tricks", p->find_racial_spell( "Bag of Tricks" ) )
  {
    parse_options( options_str );
    // This either a healing or damage spell depending on the chosen trick
    //Values for damage are hardcoded in tooltip
    if ( p->vulpera_tricks == player_t::HEALING || p->vulpera_tricks == player_t::HOLY )
    {
      attack_power_mod.direct = 0;
      spell_power_mod.direct  = 0;
    }
    else
    {
      attack_power_mod.direct = 1.8;
      spell_power_mod.direct  = 1.8;
    }

    may_crit = true;

    if ( p->vulpera_tricks == player_t::CORROSIVE )
    {
      school = SCHOOL_NATURE;
    }
    else if ( p->vulpera_tricks == player_t::FLAMES )
    {
      school = SCHOOL_FIRE;
    }
    else if ( p->vulpera_tricks == player_t::SHADOWS )
    {
      school = SCHOOL_SHADOW;
    }
  }

  double attack_direct_power_coefficient( const action_state_t* s ) const override
  {
    auto ap = composite_total_attack_power();
    auto sp = composite_total_spell_power();

    if ( ap <= sp )
      return 0;

    return racial_spell_t::attack_direct_power_coefficient( s );
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    auto ap = composite_total_attack_power();
    auto sp = composite_total_spell_power();

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

  restart_sequence_t( player_t* player, util::string_view options_str ) :
    action_t( ACTION_OTHER, "restart_sequence", player ),
    seq( nullptr ),
    seq_name_str( "default" )  // matches default name for sequences
  {
    add_option( opt_string( "name", seq_name_str ) );
    parse_options( options_str );
    ignore_false_positive = true;
    trigger_gcd           = timespan_t::zero();
  }

  void init() override
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

  void execute() override
  {
    if ( sim->debug )
      sim->out_debug.printf( "%s restarting sequence %s", player->name(), seq_name_str.c_str() );
    seq->restart();
  }

  bool ready() override
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

  restore_mana_t( player_t* player, util::string_view options_str ) :
    action_t( ACTION_OTHER, "restore_mana", player ),
    mana( 0 )
  {
    add_option( opt_float( "mana", mana ) );
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();

    target = player;
  }

  void execute() override
  {
    double mana_missing = player->resources.max[ RESOURCE_MANA ] - player->resources.current[ RESOURCE_MANA ];
    double mana_gain    = mana;

    if ( mana_gain == 0 || mana_gain > mana_missing )
      mana_gain = mana_missing;

    if ( mana_gain > 0 )
    {
      player->resource_gain( RESOURCE_MANA, mana_gain, gain );
    }
  }
};

// Base Wait Action =========================================================

struct wait_action_base_t : public action_t
{
  wait_action_base_t(player_t* player, util::string_view name) :
    action_t(ACTION_OTHER, name, player, spell_data_t::nil())
  {
    trigger_gcd = timespan_t::zero();
    interrupt_auto_attack = false;
    quiet = true;
    target = player;
  }

  void execute() override
  {
    player->iteration_waiting_time += time_to_execute;
  }
};

// Wait For Cooldown Action =================================================

struct wait_for_cooldown_t : public wait_action_base_t
{
  cooldown_t* wait_cd;
  action_t* a;

  wait_for_cooldown_t( player_t* player, util::string_view options_str ) :
    wait_action_base_t( player, "wait_for_cooldown" )
  {
    std::string cd_name;
    add_option( opt_string( "name", cd_name ) );
    parse_options( options_str );

    wait_cd = player->get_cooldown( cd_name );
    a = player->find_action( cd_name );
  }

  bool usable_moving() const override
  {
    return !a || a->usable_moving();
  }

  timespan_t execute_time() const override
  {
    // Ensures that the cooldown exists and has been given a default duration
    assert( wait_cd->duration > timespan_t::zero() );
    return wait_cd->remains();
  }

  bool ready() override
  {
    // Don't attempt to wait for a cooldown that is already up
    if ( wait_cd -> is_ready() )
      return false;
    return wait_action_base_t::ready();
  }
};


// Wait Fixed Action ========================================================

struct wait_fixed_t : public wait_action_base_t
{
  std::unique_ptr<expr_t> time_expr;

  wait_fixed_t( player_t* player, util::string_view options_str ) :
    wait_action_base_t( player, "wait" ), time_expr()
  {
    std::string sec_str = "1.0";

    add_option( opt_string( "sec", sec_str ) );
    parse_options( options_str );

    time_expr = expr_t::parse( this, sec_str );
    if ( !time_expr )
    {
      sim->error( "{}: Unable to generate wait expression from '{}'", player->name(), options_str );
      background = true;
    }
  }

  timespan_t execute_time() const override
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
  wait_until_ready_t( player_t* player, util::string_view options_str ) :
    wait_fixed_t( player, options_str )
  {
  }

  timespan_t execute_time() const override
  {
    timespan_t wait    = wait_fixed_t::execute_time();
    timespan_t remains = timespan_t::zero();

    for ( size_t i = 0; i < player->action_list.size(); ++i )
    {
      action_t* a = player->action_list[ i ];
      assert( a );
      if ( a == nullptr )  // For release builds.
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
  std::string item_name, item_slot, effect_name;

  use_item_t( player_t* player, util::string_view options_str ) :
    action_t( ACTION_OTHER, "use_item", player ),
    item( nullptr ),
    action( nullptr ),
    buff( nullptr ),
    cooldown_group( nullptr ),
    cooldown_group_duration( timespan_t::zero() )
  {
    add_option( opt_string( "name", item_name ) );
    add_option( opt_string( "slot", item_slot ) );
    add_option( opt_string( "effect_name", effect_name ) );
    parse_options( options_str );

    special = true;

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
        item       = nullptr;
        background = true;
      }
      else
      {
        name_str = name_str + "_" + item->name();
      }
    }
    else if ( !effect_name.empty() )
    {
      item = player->find_item_by_use_effect_name( effect_name );
      if ( !item )
      {
        if ( sim->debug )
        {
          sim->errorf( "Player %s attempting 'use_item' action with effect '%s' which cannot be found.\n",
                       player->name(), effect_name.c_str() );
        }
        background = true;
        return;
      }

      name_str = name_str + "_" + item->name();
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
    action_priority_list_t* apl = nullptr;
    if ( action_list )
    {
      apl = player->find_action_priority_list( action_list->name_str );
    }

    if ( !item )
    {
      action_t::init();
      erase_action( apl );
      background = true;
      return;
    }

    // Parse Special Effect
    const special_effect_t* e = item->special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE );
    if ( e && e->type == SPECIAL_EFFECT_USE )
    {
      // Create a buff
      buff = e->create_buff();

      // On-use buff cooldowns are unconditionally handled by the action, so as a precaution, reset any cooldown
      // associated with the buff itself
      if ( buff )
        buff->set_cooldown( 0_ms );

      // Create an action
      action = e->create_action();

      // if the action is the same as the driver, has a direct/periodic damage effect, and the driver has a cast time,
      // then the action is not considered a proc
      if ( action && action->id == e->driver()->id() && e->driver()->cast_time() > 0_ms &&
           ( action_t::has_direct_damage_effect( *e->driver() ) ||
             action_t::has_periodic_damage_effect( *e->driver() ) ) )
      {
        action->not_a_proc = true;
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
          cooldown_group = player->item_cooldown.get();
        }
      }
      else
      {
        cooldown_group = player->item_cooldown.get();
        cooldown_group_duration = player->default_item_group_cooldown;
      }
    }

    if ( action )
    {
      interrupt_auto_attack = action->interrupt_auto_attack;
      reset_auto_attack = action->reset_auto_attack;
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

    action_t::init();

    if ( action )
      action->is_precombat = is_precombat;
  }

  timespan_t execute_time() const override
  {
    return action ? action->execute_time() : action_t::execute_time();
  }

  void execute() override
  {
    bool triggered = buff == nullptr;
    if ( buff )
      triggered = buff->trigger();

    if ( triggered && action && ( !buff || buff->check() == buff->max_stack() ) )
    {
      action->target = target;
      action->execute();

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
        sim->out_debug.print( "{} starts shared cooldown for {} ({}). Will be ready at {}", *player, name(),
                               cooldown_group->name(), cooldown_group->ready );
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

    if ( action && ( !action->ready() || !action->cooldown->up() ) )
    {
      return false;
    }

    return action_t::ready();
  }

  bool target_ready( player_t* t ) override
  {
    if ( !item )
      return false;

    if ( action )
      return action->target_ready( t );

    return action_t::target_ready( t );
  }

  std::unique_ptr<expr_t> create_special_effect_expr( util::span<const util::string_view> data_str_split )
  {
    struct use_item_buff_type_expr_t : public expr_t
    {
      double v;

      use_item_buff_type_expr_t( bool state ) : expr_t( "use_item_buff_type" ), v( state ? 1.0 : 0 )
      {
      }

      bool is_constant() override
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
      return nullptr;
    }

    stat_e stat = util::parse_stat_type( data_str_split[ 1 ] );
    if ( stat == STAT_NONE )
    {
      return nullptr;
    }

    const special_effect_t* e = item->special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE );
    if ( !e )
    {
      return nullptr;
    }

    return std::make_unique<use_item_buff_type_expr_t>( e->stat_type() == stat );
  }

  std::unique_ptr<expr_t> create_expression( util::string_view name_str ) override
  {
    auto split = util::string_split<util::string_view>( name_str, "." );
    if ( auto e = create_special_effect_expr( split ) )
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

  use_items_t( player_t* player, util::string_view options_str ) :
    action_t( ACTION_USE, "use_items", player ),
    // Ensure trinkets are checked before all other items by default
    priority_slots( {SLOT_TRINKET_1, SLOT_TRINKET_2} ),
    custom_slots( false )
  {
    callbacks = may_miss = may_crit = may_block = may_parry = false;
    special = true;

    trigger_gcd = timespan_t::zero();

    add_option( opt_func( "slots", std::bind( &use_items_t::parse_slots, this, std::placeholders::_1,
                                              std::placeholders::_2, std::placeholders::_3 ) ) );

    parse_options( options_str );
  }

  result_e calculate_result( action_state_t* ) const override
  {
    return RESULT_HIT;
  }

  timespan_t execute_time() const override
  {
    for ( auto action : use_actions )
    {
      if ( action->ready() && action->action )
      {
        return action->action->execute_time();
      }
    }

    return action_t::execute_time();
  }

  void execute() override
  {
    // Execute first ready sub-action. Note that technically an on-use action can go "unavailable"
    // between the ready check and the execution, meaning this action actually executes no
    // sub-action
    for ( auto action : use_actions )
    {
      if ( action->ready() && action->target_ready( target ) )
      {
        action->set_target( target );
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
    if ( use_actions.empty() )
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
      if ( action->ready() && action->cooldown->up() )
      {
        return true;
      }
    }

    return false;
  }

  bool target_ready( player_t* t ) override
  {
    if ( !action_t::target_ready( t ) )
      return false;

    for ( const auto action : use_actions )
    {
      if ( action->ready() && action->target_ready( t ) )
        return true;
    }

    return false;
  }

  bool parse_slots( sim_t* /* sim */, util::string_view /* opt_name */, util::string_view opt_value )
  {
    // Empty out default priority slots. slots= option will change the use_items action logic so
    // that only the designated slots will be checked/executed for a special effect
    priority_slots.clear();

    auto split = util::string_split<util::string_view>( opt_value, ":|" );
    range::for_each( split, [this]( util::string_view slot_str ) {
      auto slot = util::parse_slot_type( slot_str );
      if ( slot != SLOT_INVALID )
      {
        priority_slots.push_back( slot );
      }
    } );

    custom_slots = true;

    // If slots= option is given, presume that at aleast one of those slots is a valid slot name
    return !priority_slots.empty();
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

      // As precombat /use_item,name=X are only used once, don't remove them.
      if ( action->action_list && action->action_list->name_str == "precombat" && action->action )
        return;

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

    // Remove any slots from the list, where the actor has an item equipped, and corresponding a
    // use_item,effect_name=X action for the item.
    range::for_each( use_item_actions, [this, &slot_order]( const use_item_t* action ) {
      if ( action->effect_name.empty() )
      {
        return;
      }

      // As precombat /use_item,effect_name=X are only used once, don't remove them.
      if ( action->action_list && action->action_list->name_str == "precombat" && action->action )
        return;

      // Find out if the item is worn
      auto it = range::find_if( player->items, [action]( const item_t& item ) {
        return item.has_use_special_effect() && util::str_compare_ci( item.special_effect( SPECIAL_EFFECT_SOURCE_NONE, SPECIAL_EFFECT_USE)->name(), action->effect_name );
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
      const auto has_effect = range::any_of( item.parsed.special_effects, []( const special_effect_t* e ) {
        return (e->source == SPECIAL_EFFECT_SOURCE_ITEM || e->source == SPECIAL_EFFECT_SOURCE_GEM ||
                e->source == SPECIAL_EFFECT_SOURCE_ENCHANT) && e->type == SPECIAL_EFFECT_USE;
      } );

      // No item-based on-use effect in the slot, skip
      if ( !has_effect )
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
  std::string buff_name;
  buff_t* buff;

  cancel_buff_t( player_t* player, util::string_view options_str ) :
    action_t( ACTION_OTHER, "cancel_buff", player ),
    buff_name(),
    buff( nullptr )
  {
    add_option( opt_string( "name", buff_name ) );
    parse_options( options_str );
    ignore_false_positive = true;
    harmful = false;
    trigger_gcd = timespan_t::zero();
  }

  void init_finished() override
  {
    action_t::init_finished();

    if ( buff_name.empty() )
    {
      throw std::invalid_argument( fmt::format(
        "Player {} uses cancel_buff without specifying the name of the buff", player->name() ) );
    }

    buff = buff_t::find( player, buff_name );

    // if the buff isn't in the player_t -> buff_list, try again in the player_td_t -> target -> buff_list
    if ( !buff )
    {
      buff = buff_t::find( player->get_target_data( player )->target, buff_name );
    }

    if ( !buff )
    {
      if ( sim->debug ) {
        player->sim->error(
          "Player {} uses cancel_buff with unknown buff {}", player->name(), buff_name );
      }
    }
    else if ( !buff->can_cancel )
    {
      throw std::invalid_argument( fmt::format(
        "Player {} uses cancel_buff on {}, which cannot be cancelled in game", player->name(), buff_name ) );
    }
  }

  void execute() override
  {
    if ( sim->log )
      sim->out_log.printf( "%s cancels buff %s", player->name(), buff->name() );
    buff->expire();
  }

  bool ready() override
  {
    if ( !buff || !buff->check() )
      return false;

    return action_t::ready();
  }
};

struct cancel_action_t : public action_t
{
  cancel_action_t( player_t* player, util::string_view options_str ) :
    action_t( ACTION_OTHER, "cancel_action", player )
  {
    parse_options( options_str );
    harmful = false;
    usable_while_casting = use_while_casting = ignore_false_positive = true;
    trigger_gcd = timespan_t::zero();
    action_skill = 1;
  }

  void execute() override
  {
    sim->print_log( "{} performs {}", player->name(), name() );
    player->interrupt();
  }

  bool ready() override
  {
    if ( !player->executing && !player->channeling )
      return false;

    return action_t::ready();
  }
};

struct dismiss_pet_t final : public action_t
{
  std::string pet_name;
  pet_t* pet;

  dismiss_pet_t( player_t* player, util::string_view options_str ) :
    action_t( ACTION_OTHER, "dismiss_pet", player ),
    pet_name(),
    pet()
  {
    add_option( opt_string( "name", pet_name ) );
    parse_options( options_str );
    harmful = false;
    usable_while_casting = use_while_casting = ignore_false_positive = true;
    trigger_gcd = timespan_t::zero();

    if ( pet_name.empty() )
      throw std::invalid_argument( fmt::format( "{} must specify the name of the pet", name() ) );
  }

  void init() override
  {
    pet = player->find_pet( pet_name );
    if ( !pet && sim->debug )
      sim->error( "Player {}: Could not find pet with name '{}' for Action '{}'", player->name(), pet_name, name() );

    if ( pet && !pet->can_dismiss )
      throw std::invalid_argument( fmt::format( "{} cannot be dismissed", pet->name() ) );

    action_t::init();
  }

  void execute() override
  {
    assert( pet );
    assert( !pet->is_sleeping() );
    sim->print_log( "{} performs {} on {}", player->name(), name(), pet->name() );
    pet->dismiss( false );
  }

  bool ready() override
  {
    if ( !pet || pet->is_sleeping() )
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
  std::unique_ptr<expr_t> amount_expr;

  pool_resource_t( player_t* p, util::string_view options_str, resource_e r = RESOURCE_NONE ) :
    action_t( ACTION_OTHER, "pool_resource", p ),
    resource( r != RESOURCE_NONE ? r : p->primary_resource() ),
    wait( timespan_t::from_seconds( 0.251 ) ),
    for_next( 0 ),
    next_action( nullptr ),
    amount_expr()
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

  void init_finished() override
  {
    action_t::init_finished();

    if ( !amount_str.empty() )
    {
      amount_expr = expr_t::parse( this, amount_str, false );
      if (amount_expr == nullptr)
      {
        throw std::invalid_argument(fmt::format("Could not parse amount if expression from '{}'", amount_str));
      }

    }
  }

  void reset() override
  {
    action_t::reset();

    if ( !next_action && !background && for_next )
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

  void execute() override
  {
    if ( sim->log )
      sim->out_log.printf( "%s performs %s", player->name(), name() );

    player->iteration_pooling_time += wait;
  }

  timespan_t gcd() const override
  {
    return wait;
  }

  bool ready() override
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

// Auto-Attack Retargeting ==================================================

struct retarget_auto_attack_t : public action_t
{
  retarget_auto_attack_t( player_t* player, util::string_view options_str ) :
    action_t( ACTION_OTHER, "retarget_auto_attack", player )
  {
    parse_options( options_str );
    use_off_gcd = quiet = true;
    trigger_gcd = timespan_t::zero();
  }

  void execute() override
  {
    if ( player->main_hand_attack && player->main_hand_attack->target != target )
    {
      sim->print_debug( "{} MH auto attack changed from target {} to {}.", name(), player->main_hand_attack->target->name(), target->name() );
      player->main_hand_attack->set_target( target );
    }

    if ( player->off_hand_attack && player->off_hand_attack->target != target )
    {
      sim->print_debug( "{} OH auto attack changed from target {} to {}.", name(), player->off_hand_attack->target->name(), target->name() );
      player->off_hand_attack->set_target( target );
    }
  }

  bool action_ready() override
  {
    if ( !action_t::action_ready() )
      return false;

    if ( player->main_hand_attack && player->main_hand_attack->target != target )
      return true;

    if ( player->off_hand_attack && player->off_hand_attack->target != target )
      return true;

    return false;
  }
};

// Invoke External Buff ==================================================

struct invoke_external_buff_t : public action_t
{
  std::string buff_str;

  timespan_t buff_duration;
  int buff_stacks;
  bool use_pool;

  buff_t* buff;
  std::vector<cooldown_t*>* invoke_cds;

  invoke_external_buff_t( player_t* player, util::string_view options_str )
    : action_t( ACTION_OTHER, "invoke_external_buff", player ),
      buff_str(),
      buff_duration( timespan_t::min() ),
      buff_stacks( -1 ),
      use_pool( true ),
      buff( nullptr ),
      invoke_cds( nullptr )
  {
    add_option( opt_string( "name", buff_str ) );
    add_option( opt_timespan( "duration", buff_duration ) );
    add_option( opt_int( "stacks", buff_stacks ) );
    add_option( opt_bool( "use_pool", use_pool ) );
    parse_options( options_str );

    trigger_gcd           = timespan_t::zero();
    ignore_false_positive = true;

  }

  void init_finished() override
  {
    action_t::init_finished();

    if ( buff_str.empty() )
    {
      throw std::invalid_argument( fmt::format(
          "Player {} uses invoke_external_buff without specifying the name of the buff", player->name() ) );
    }

    buff = buff_t::find( player, buff_str );

    if ( !buff )
    {
      if ( sim->debug )
      {
        player->sim->error( "Player {} uses invoke_external_buff with unknown buff {}", player->name(), buff_str );
      }
    }

    // Initialise an action cooldown per buff type.
    cooldown = player->get_cooldown( "invoke_external_buff_" + buff_str );

    if ( use_pool )
    {
      if ( player->external_buffs.invoke_cds.empty() )
      {
        // Run initialisation code
        auto splits_buffs = util::string_split<util::string_view>( player->external_buffs.pool, "/" );

        for ( auto split_buffs : splits_buffs )
        {
          auto splits = util::string_split<util::string_view>( split_buffs, ":" );
          // Min arguments Name and CD - without both, skip
          if ( splits.size() < 2 )
            continue;

          // Buff name is empty - skip
          if ( splits[ 0 ].empty() )
            continue;

          auto external_buff = buff_t::find( player, splits[ 0 ] );

          // If the buff cannot be found, skip
          if ( !external_buff )
            continue;

          // Number of buffs to create (third arugment)
          int num = splits.size() >= 3 ? util::to_int( splits[ 2 ] ) : 1;

          auto cds = &player->external_buffs.invoke_cds[ external_buff ];

          timespan_t cd_time = timespan_t::from_seconds( util::to_double( splits[ 1 ] ) );

          for ( auto i = 0; i < num; i++ )
          {
            auto cd =
                cds->emplace_back( player->get_cooldown( fmt::format( "invoke_{}_{}", splits[ 0 ], cds->size() ) ) );
            cd->duration = cd->base_duration = cd_time;

            sim->print_debug( "{} creates cooldown {} with cd {} ", *player, cd->name(),
                              cd->duration );
          }
        }
      }

      auto cds = &player->external_buffs.invoke_cds[ buff ];

      if ( !cds->empty() )
      {
        // External buffs initialised and buff found - set cooldown object to this
        invoke_cds = cds;
      }
      else
      {
        // External buffs initialied and buffs not found - set buff to nullptr, this action will never be used.
        buff = nullptr;
      }
    }
  }

  bool ready() override
  {
    if ( !buff )
      return false;

    return action_t::ready();
  }

  cooldown_t* get_shortest_cd()
  {
    if ( !use_pool )
      return nullptr;

    auto comp = []( cooldown_t* a, cooldown_t* b ) {
      if ( a->remains() < b->remains() )
        return true;
      if ( a->remains() > b->remains() )
        return false;

      return a->duration < b->duration;
    };

    auto min = min_element( invoke_cds->begin(), invoke_cds->end(), comp );
    if ( min != invoke_cds->end() )
      return *min;

    return nullptr;
  }

  void execute() override
  {
    if ( sim->log )
      sim->out_log.printf( "%s invokes buff %s", player->name(), buff->name() );

    buff->trigger( buff_stacks, buff_duration );

    if ( use_pool )
    {
      auto shortest = get_shortest_cd();
      if ( shortest )
        shortest->start();

      shortest = get_shortest_cd();
      if ( shortest && shortest->remains() > timespan_t::zero() )
      {
        cooldown->duration = shortest->remains();
        cooldown->start();
      }
    }
  }
};


}  // UNNAMED NAMESPACE

action_t* player_t::create_action( util::string_view name, util::string_view options_str )
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
  if ( name == "gift_of_the_naaru" )
    return new gift_of_the_naaru( this, options_str );
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
  if ( name == "bag_of_tricks" )
    return new bag_of_tricks_t( this, options_str );

  if ( name == "cancel_action" )
    return new cancel_action_t( this, options_str );
  if ( name == "cancel_buff" )
    return new cancel_buff_t( this, options_str );
  if ( name == "dismiss_pet" )
    return new dismiss_pet_t( this, options_str );
  if ( name == "invoke_external_buff" )
    return new invoke_external_buff_t( this, options_str );
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
  if ( name == "cycling_variable" )
    return new cycling_variable_t( this, options_str );
  if ( name == "wait_for_cooldown" )
    return new wait_for_cooldown_t( this, options_str );
  if ( name == "retarget_auto_attack" )
    return new retarget_auto_attack_t( this, options_str );

  if ( auto action = unique_gear::create_action( this, name, options_str ) )
    return action;

  if ( auto action = azerite::create_action( this, name, options_str ) )
    return action;

  if ( auto action = covenant::create_action( this, name, options_str ) )
    return action;

  if ( auto action = covenant::soulbinds::create_action( this, name, options_str ) )
    return action;

  return consumable::create_action( this, name, options_str );
}

pet_t* player_t::create_pet( util::string_view, util::string_view )
{
  return nullptr;
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

double player_t::get_attribute( attribute_e a ) const
{
  return util::floor( composite_attribute( a ) * composite_attribute_multiplier( a ) );
}

static bool parse_min_gcd( sim_t* sim, util::string_view name, util::string_view value )
{
  if ( name != "min_gcd" )
    return false;

  double v = util::to_double( value );
  if ( v <= 0 )
  {
    sim->error( "{}: Invalid value '{}' for global minimum cooldown.", sim->active_player->name(), value );
    return false;
  }

  sim->active_player->min_gcd = timespan_t::from_seconds( v );
  return true;
}

// TODO: HOTFIX handling
void player_t::replace_spells()
{
  uint32_t class_idx;
  uint32_t spec_index;

  if ( !dbc->spec_idx( _spec, class_idx, spec_index ) )
    return;

  // Search spec spells for spells to replace.
  if ( _spec != SPEC_NONE )
  {
    range::for_each( specialization_spell_entry_t::data( dbc->ptr ),
      [this]( const specialization_spell_entry_t& entry ) {
        if ( static_cast<unsigned>( _spec ) != entry.specialization_id )
        {
          return;
        }

        if ( entry.override_spell_id == 0 )
        {
          return;
        }

        const spell_data_t* s = dbc->spell( entry.spell_id );
        if ( as<int>( s->level() ) <= true_level )
        {
          dbc->replace_id( entry.override_spell_id, entry.spell_id );
        }
    } );
  }

  // Search general spells for spells to replace (a spell you learn earlier might be
  // replaced by one you learn later)
  if ( _spec != SPEC_NONE )
  {
    range::for_each( active_class_spell_t::data( dbc->ptr ),
      [this, class_idx]( const active_class_spell_t& entry ) {
        if ( class_idx != entry.class_id )
        {
          return;
        }

        if ( entry.spec_id != 0 && static_cast<unsigned>( _spec ) != entry.spec_id )
        {
          return;
        }

        if ( entry.override_spell_id == 0 )
        {
          return;
        }

        const spell_data_t* s = dbc->spell( entry.spell_id );
        if ( as<int>( s->level() ) <= true_level )
        {
          dbc->replace_id( entry.override_spell_id, entry.spell_id );
        }
    } );

    range::for_each( rank_class_spell_t::data( dbc->ptr ),
      [this, class_idx]( const rank_class_spell_t& entry ) {
        if ( class_idx != entry.class_id )
        {
          return;
        }

        if ( entry.spec_id != 0 && static_cast<unsigned>( _spec ) != entry.spec_id )
        {
          return;
        }

        if ( entry.override_spell_id == 0 )
        {
          return;
        }

        const spell_data_t* s = dbc->spell( entry.spell_id );
        if ( as<int>( s->level() ) <= true_level )
        {
          dbc->replace_id( entry.override_spell_id, entry.spell_id );
        }
    } );
  }

  for ( const auto& talent : player_traits )
  {
    if ( std::get<2>( talent ) == 0U )
    {
      continue;
    }

    auto talent_obj = find_talent_spell( std::get<1>( talent ) );
    if ( !talent_obj.ok() )
    {
      continue;
    }

    if ( talent_obj.trait()->id_replace_spell )
    {
      dbc->replace_id( talent_obj.trait()->id_replace_spell,
          talent_obj.trait()->id_spell );
    }
  }
}

static player_talent_t create_talent_obj( const player_t* player, const trait_data_t* trait )
{
  auto it = range::find_if( player->player_traits, [ trait ]( const auto& entry ) {
    return std::get<1>( entry ) == trait->id_trait_node_entry;
  } );

  auto _tree = static_cast<talent_tree>( trait->tree_index );
  auto rank = it == player->player_traits.end() ? 0U : std::get<2>( *it );

  // all allocated hero talents are present but disabled if the control talent is not active unless it has been manually
  // added to the profile or sim_t::enable_all_talents is set
  if ( _tree == talent_tree::HERO && !range::contains( player->player_sub_trees, trait->id_sub_tree ) &&
       !range::contains( player->player_sub_traits, trait->id_trait_node_entry ) && !player->sim->enable_all_talents )
  {
    rank = 0U;
  }

  if ( !rank )
  {
    return { player };  // Trait not found on player
  }

  return { player, trait, rank };
}

player_talent_t player_t::find_talent_spell(
    talent_tree        tree,
    util::string_view name,
    specialization_e  s,
    bool              name_tokenized ) const
{
  const trait_data_t* trait;
  uint32_t class_idx, spec_idx;

  dbc->spec_idx( s == SPEC_NONE ? _spec : s, class_idx, spec_idx );

  if ( name_tokenized )
  {
    trait = trait_data_t::find_tokenized( tree, name, class_idx, s == SPEC_NONE ? _spec : s, dbc->ptr );
  }
  else
  {
    trait = trait_data_t::find( tree, name, class_idx, s == SPEC_NONE ? _spec : s, dbc->ptr );
  }

  if ( trait == &trait_data_t::nil() )
  {
    sim->print_debug( "Player {}: Can't find {} talent with name '{}'.", this->name(),
        util::talent_tree_string( tree ), name );
    return {};  // Invalid trait
  }

  return create_talent_obj( this, trait );
}

player_talent_t player_t::find_talent_spell(
  talent_tree      tree,
  unsigned         spell_id,
  specialization_e s ) const
{
  uint32_t class_idx, spec_idx;

  dbc->spec_idx( s == SPEC_NONE ? _spec : s, class_idx, spec_idx );

  auto traits = trait_data_t::find_by_spell( tree, spell_id, class_idx, s == SPEC_NONE ? _spec : s, dbc->ptr );
  if ( traits.size() == 0 )
  {
    sim->print_debug( "Player {}: Can't find {} talent with spell_id '{}'.", this->name(),
        util::talent_tree_string( tree ), spell_id );
    return {};  // Invalid trait
  }

  return create_talent_obj( this, traits[ 0 ] );
}

player_talent_t player_t::find_talent_spell( unsigned trait_node_entry_id ) const
{
  const trait_data_t* trait = trait_data_t::find( trait_node_entry_id, dbc->ptr );
  if ( trait == &trait_data_t::nil() )
  {
    sim->print_debug( "Player {}: Can't find talent with node_entry_id '{}'.", this->name(),
        trait_node_entry_id );
    return {};  // Invalid trait
  }

  return create_talent_obj( this, trait );
}

const spell_data_t* player_t::find_specialization_spell( util::string_view name,
                                                         specialization_e s ) const
{
  return find_specialization_spell( name, "", s );
}

const spell_data_t* player_t::find_specialization_spell( util::string_view name,
                                                         util::string_view desc,
                                                         specialization_e s ) const
{
  if ( s == SPEC_NONE || s == _spec )
  {
    if ( unsigned spell_id = dbc->specialization_ability_id( _spec, name, desc ) )
    {
      auto spell = dbc::find_spell( this, spell_id );

      if ( as<int>( spell->level() ) <= true_level )
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
    if ( dbc->is_specialization_ability( _spec, spell_id ) && ( as<int>( spell->level() ) <= true_level ) )
    {
      return spell;
    }
  }

  return spell_data_t::not_found();
}

const spell_data_t* player_t::find_mastery_spell( specialization_e s ) const
{
  if ( s == SPEC_NONE || s != _spec )
  {
    return spell_data_t::not_found();
  }

  if ( auto spell_id = dbc->mastery_ability_id( s != SPEC_NONE ? s : _spec ) )
  {
    const auto spell = dbc::find_spell( this, spell_id );
    if ( spell->ok() && as<int>( spell->level() ) <= true_level )
    {
      return spell;
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

azerite_power_t player_t::find_azerite_spell( util::string_view name, bool tokenized ) const
{
  if ( ! azerite )
  {
    return {};
  }

  // Note, no const propagation here, so this works
  return azerite -> get_power( name, tokenized );
}

azerite_essence_t player_t::find_azerite_essence( unsigned id ) const
{
  if ( !azerite_essence )
  {
    return { this };
  }

  return azerite_essence->get_essence( id );
}

azerite_essence_t player_t::find_azerite_essence( util::string_view name, bool tokenized ) const
{
  if ( !azerite_essence )
  {
    return { this };
  }

  return azerite_essence->get_essence( name, tokenized );
}

conduit_data_t player_t::find_conduit_spell( util::string_view name ) const
{
  if ( !covenant )
  {
    return {};
  }

  return covenant->get_conduit_ability( name );
}

const spell_data_t* player_t::find_soulbind_spell( util::string_view name ) const
{
  if ( !covenant )
  {
    return spell_data_t::not_found();
  }

  return covenant->get_soulbind_ability( name );
}

const spell_data_t* player_t::find_covenant_spell( util::string_view name ) const
{
  if ( !covenant )
  {
    return spell_data_t::not_found();
  }

  return covenant->get_covenant_ability( name );
}

item_runeforge_t player_t::find_runeforge_legendary( util::string_view name, bool tokenized, bool force_unity ) const
{
  if ( !sim->shadowlands_opts.enabled )
  {
    return item_runeforge_t::nil();
  }

  auto entries = runeforge_legendary_entry_t::find( name, dbc->ptr, tokenized );
  if ( entries.empty() )
  {
    return item_runeforge_t::nil();
  }

  covenant_e cov_type = covenant->type();
  unsigned unity_bonus_id = 0;
  unsigned unity_spell_id = 0;

  if ( force_unity || ( cov_type != covenant_e::DISABLED && cov_type != covenant_e::INVALID &&
                        entries.front().covenant_id == static_cast<unsigned>( cov_type ) ) )
  {
    auto unity_entries = runeforge_legendary_entry_t::find( "Unity", dbc->ptr );
    if ( !unity_entries.empty() )
    {
      auto it = range::find( unity_entries, specialization(), &runeforge_legendary_entry_t::specialization_id );
      if ( it != unity_entries.end() )
      {
        unity_bonus_id = it->bonus_id;
        unity_spell_id = it->spell_id;
      }
    }
  }

  // 8/22/2020 - Removed spec filtering for now since these currently have no spec limitations in-game
  //             May need to restore some logic at some point if Blizzard points to different spells per-spec

  // Iterate over all items to find the bonus id on an item. Note that Simulationcraft
  // currently does not enforce the item restrictions on the legendary bonuses. This is
  // intentional to allow people who know what they are doing(tm) to override in-game
  // rules.
  const item_t* item = nullptr;
  for ( const auto& i : items )
  {
    if ( range::contains( i.parsed.bonus_id, as<int>( entries.front().bonus_id ) ) ||
         ( unity_bonus_id && range::contains( i.parsed.bonus_id, as<int>( unity_bonus_id ) ) ) ||
         ( unity_spell_id && range::contains( i.parsed.data.effects, unity_spell_id, &item_effect_t::spell_id ) ) )
    {
      item = &i;
      break;
    }
  }

  if ( item == nullptr )
  {
    return item_runeforge_t::not_found();
  }

  return { entries.front(), item };
}

/**
 * 8.2 Vision of Perfection proc handler
 *
 * Carries out class & spec-specific code when Vision of Perfection Procs.
 * Overridden in class modules.
 */
void player_t::vision_of_perfection_proc()
{
  return;
}

/**
 * Tries to find spell data by name.
 *
 * It does this by going through various spell lists in following order:
 * class spell, specialization spell, mastery spell, class/spec/hero talent spell, racial spell, pet_spell
 */
const spell_data_t* player_t::find_spell( util::string_view name, specialization_e s ) const
{
  const spell_data_t* sp = find_class_spell( name, s );
  if ( sp->ok() )
    return sp;

  sp = find_specialization_spell( name );
  if ( sp->ok() )
    return sp;

  if ( s != SPEC_NONE )
  {
    sp = find_mastery_spell( s );
    if ( sp->ok() )
      return sp;
  }

  sp = find_talent_spell( talent_tree::CLASS, name );
  if ( sp->ok() )
    return sp;

  sp = find_talent_spell( talent_tree::SPECIALIZATION, name );
  if ( sp->ok() )
    return sp;

  sp = find_talent_spell( talent_tree::HERO, name );
  if ( sp->ok() )
    return sp;

  sp = find_racial_spell( name );
  if ( sp->ok() )
    return sp;

  sp = find_pet_spell( name );
  if ( sp->ok() )
    return sp;

  return spell_data_t::not_found();
}

const spell_data_t* player_t::find_racial_spell( util::string_view name, race_e r ) const
{
  if ( unsigned spell_id = dbc->race_ability_id( type, ( r != RACE_NONE ) ? r : race, name ) )
  {
    const spell_data_t* s = dbc->spell( spell_id );
    if ( s->id() == spell_id )
    {
      return dbc::find_spell( this, s );
    }
  }

  return spell_data_t::not_found();
}

const spell_data_t* player_t::find_class_spell( util::string_view name, specialization_e s ) const
{
  if ( s == SPEC_NONE || s == _spec )
  {
    if ( unsigned spell_id = dbc->class_ability_id( type, _spec, name ) )
    {
      const spell_data_t* spell = dbc->spell( spell_id );
      if ( spell->id() == spell_id && as<int>( spell->level() ) <= true_level )
      {
        return dbc::find_spell( this, spell );
      }
    }
  }

  return spell_data_t::not_found();
}

const spell_data_t* player_t::find_rank_spell( util::string_view name,
                                               util::string_view rank,
                                               specialization_e spec ) const
{
  if ( spec == SPEC_NONE || spec == _spec )
  {
    const auto& entry = rank_class_spell_t::find( name, rank, util::class_id( type ),
        static_cast<unsigned>( _spec ), dbc->ptr );
    if ( entry.spell_id == 0 )
    {
      return spell_data_t::not_found();
    }

    if ( entry.class_id != as<unsigned>( util::class_id( type ) ) )
    {
      return spell_data_t::not_found();
    }

    if ( entry.spec_id > 0 && entry.spec_id != static_cast<unsigned>( _spec ) )
    {
      return spell_data_t::not_found();
    }

    if ( entry.override_spell_id )
    {
      const auto replace_spell = dbc::find_spell( this, entry.override_spell_id );
      if ( replace_spell->ok() && as<int>( replace_spell->level() ) <= true_level )
      {
        return spell_data_t::not_found();
      }
    }

    const spell_data_t* spell = dbc::find_spell( this, entry.spell_id );
    if ( !spell->ok() || as<int>( spell->level() ) > true_level )
    {
      return spell_data_t::not_found();
    }

    return spell;
  }

  return spell_data_t::not_found();
}

const spell_data_t* player_t::find_pet_spell( util::string_view name ) const
{
  if ( unsigned spell_id = dbc->pet_ability_id( type, name ) )
  {
    const spell_data_t* s = dbc->spell( spell_id );
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
std::unique_ptr<expr_t> deprecate_expression( player_t& p, util::string_view old_name, util::string_view new_name, action_t* a = nullptr  )
{
  p.sim->error( "Use of \"{}\" ( action {} ) in action expressions is deprecated: use \"{}\" instead.\n",
                  old_name, a?a->name() : "unknown", new_name );

  return p.create_expression( new_name );
}

struct player_expr_t : public expr_t
{
  player_t& player;

  player_expr_t( util::string_view n, player_t& p ) : expr_t( n ), player( p )
  {
  }
};

struct position_expr_t : public player_expr_t
{
  int mask;
  position_expr_t( util::string_view n, player_t& p, int m ) : player_expr_t( n, p ), mask( m )
  {
  }
  double evaluate() override
  {
    return ( 1 << player.position() ) & mask;
  }
};

std::unique_ptr<expr_t> deprecated_player_expressions( player_t& player, util::string_view expression_str )
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

  return {};
}

}  // namespace

/**
 * Player specific action expressions
 *
 * Use this function for expressions which are bound to some action property (eg. target, cast_time, etc.) and not
 * just to the player itself.
 */
std::unique_ptr<expr_t> player_t::create_action_expression( action_t&, util::string_view expression_str )
{
  return create_expression( expression_str );
}

std::unique_ptr<expr_t> player_t::create_expression( util::string_view expression_str )
{
  if (auto e = deprecated_player_expressions(*this, expression_str))
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

  if ( expression_str == "in_boss_encounter" )
    return make_ref_expr( "in_boss_encounter", in_boss_encounter );

  if ( expression_str == "ptr" )
    return expr_t::create_constant( "ptr", dbc->ptr );

  if ( expression_str == "bugs" )
    return expr_t::create_constant( "bugs", bugs );

  if ( expression_str == "is_add" )
    return expr_t::create_constant( "is_add", is_add() );

  if ( expression_str == "is_boss" )
    return expr_t::create_constant( "is_boss", is_boss() );

  if ( expression_str == "is_enemy" )
    return expr_t::create_constant( "is_enemy", is_enemy() );

  if ( expression_str == "attack_haste" )
    return make_fn_expr( expression_str, [this] { return cache.attack_haste(); } );

  if ( expression_str == "auto_attack_speed" )
    return make_fn_expr( expression_str, [this] { return cache.auto_attack_speed(); } );

  if ( expression_str == "spell_haste" )
    return make_fn_expr( expression_str, [this] { return cache.spell_haste(); } );

  if ( expression_str == "spell_cast_speed" )
    return make_fn_expr( expression_str, [this] { return cache.spell_cast_speed(); } );

  if ( expression_str == "mastery_value" )
    return make_mem_fn_expr( expression_str, this->cache, &player_stat_cache_t::mastery_value );

  if ( expression_str == "attack_crit" )
    return make_fn_expr( expression_str, [this] { return cache.attack_crit_chance(); } );

  if ( expression_str == "spell_crit" )
    return make_fn_expr( expression_str, [this] { return cache.spell_crit_chance(); } );

  if ( expression_str == "parry_chance" )
    return make_fn_expr( expression_str, [ this ] { return cache.parry(); } );

  if ( expression_str == "dodge_chance" )
    return make_fn_expr( expression_str, [ this ] { return cache.dodge(); } );

  if ( expression_str == "block_chance" )
    return make_fn_expr( expression_str, [ this ] { return cache.block(); } );

  if ( expression_str == "position_front" )
    return std::make_unique<position_expr_t>( "position_front", *this, ( 1 << POSITION_FRONT ) | ( 1 << POSITION_RANGED_FRONT ) );
  if ( expression_str == "position_back" )
    return std::make_unique<position_expr_t>( "position_back", *this, ( 1 << POSITION_BACK ) | ( 1 << POSITION_RANGED_BACK ) );

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
  if ( auto q = create_resource_expression( expression_str ) )
    return q;

  // time_to_pct expressions
  if ( util::str_prefix_ci( expression_str, "time_to_" ) )
  {
    auto parts = util::string_split<util::string_view>( expression_str, "_" );
    double percent = -1.0;

    if ( util::str_in_str_ci( parts[ 2 ], "die" ) )
      percent = 0.0;
    else if ( util::str_in_str_ci( parts[ 2 ], "pct" ) )
    {
      if (parts.size() == 4 )
      {
        // eg. time_to_pct_90.1
        percent = util::to_double( parts[ 3 ] );
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
    auto parts = util::string_split<util::string_view>( expression_str, "_" );
    timespan_t window_duration;

    if ( util::str_in_str_ci( parts.back(), "ms" ) )
      window_duration = timespan_t::from_millis( util::to_int( parts.back() ) );
    else
      window_duration = timespan_t::from_seconds( util::to_double( parts.back() ) );

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
  auto splits = util::string_split<util::string_view>( expression_str, "." );

  if ( splits.size() == 2 )
  {
    // player variables
    if ( splits[ 0 ] == "variable"  )
    {
      struct variable_expr_t : public expr_t
      {
        const action_variable_t* var_;

        variable_expr_t( player_t* p, util::string_view name ) : expr_t( "variable" ),
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

        bool is_constant() override
        { return var_->is_constant(); }

        double evaluate() override
        { return var_->current_value_; }
      };

      return std::make_unique<variable_expr_t>( this, splits[ 1 ] );
    }

    // item equipped by item_id or name or effect_name
    if ( splits[ 0 ] == "equipped" )
    {
      unsigned item_id = util::to_unsigned_ignore_error( splits[ 1 ], 0 );
      for ( const auto& item : items )
      {
        if ( item_id > 0 && item.parsed.data.id == item_id )
        {
          return expr_t::create_constant( "item_equipped", 1 );
        }
        else if ( util::str_compare_ci( item.name_str, splits[ 1 ] ) )
        {
          return expr_t::create_constant( "item_equipped", 1 );
        }
        else if ( item.special_effect_with_name( splits[ 1 ] ) )
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
          return make_fn_expr( expression_str, [ this, stat ] {
            return cache.get_attribute( static_cast<attribute_e>( stat ) );
          } );
        }

        case STAT_SPELL_POWER:
        {
          return make_fn_expr( expression_str, [ this ] {
            return static_cast<int>( cache.spell_power( SCHOOL_MAX ) * composite_spell_power_multiplier() );
          } );
        }

        case STAT_ATTACK_POWER:
        {
          return make_fn_expr( expression_str, [ this ] {
            return static_cast<int>( cache.attack_power() * composite_attack_power_multiplier() );
          } );
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
      action_t* action = find_action( splits[ 1 ] );
      if ( action )
      {
        return make_fn_expr( expression_str, [ this, action ] {
          return !action->data().ok() ? 0 : get_active_dots( action->get_dot() );
        } );
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

    // specific bfa. options
    if ( splits[ 0 ] == "bfa" )
    {
      if ( splits[ 1 ] == "font_of_power_precombat_channel" )
      {
        return make_fn_expr( expression_str, [this] {
          return sim->bfa_opts.font_of_power_precombat_channel.total_seconds();
        } );
      }

      throw std::invalid_argument( fmt::format( "Unsupported bfa. option '{}'.", splits[ 1 ] ) );
    }

    if ( splits[ 0 ] == "shadowlands" )
    {
      if ( splits[ 1 ] == "shadowed_orb_of_torment_precombat_channel" )
      {
        return make_fn_expr( expression_str, [this] {
          return sim->shadowlands_opts.shadowed_orb_of_torment_precombat_channel.total_seconds();
        } );
      }

      throw std::invalid_argument( fmt::format( "Unsupported shadowlands. option '{}'.", splits[ 1 ] ) );
    }

    if ( splits[ 0 ] == "dragonflight" )
    {
      if ( splits[ 1 ] == "screaming_black_dragonscale_damage" )
      {
        return make_fn_expr( expression_str, [this] {
          return sim->dragonflight_opts.screaming_black_dragonscale_damage;
        } );
      }

      if ( splits[ 1 ] == "rallied_to_victory_ally_estimate" )
      {
        return make_fn_expr( expression_str, [ this ] {
          return dragonflight_opts.rallied_to_victory_ally_estimate;
        } );
      }

      if (splits[ 1 ] == "string_of_delicacies_ally_estimate")
      {
        return make_fn_expr( expression_str, [ this ] {
           return dragonflight_opts.string_of_delicacies_ally_estimate;
        } );
      }

      throw std::invalid_argument( fmt::format( "Unsupported dragonflight. option '{}'.", splits[ 1 ] ) );
    }

    if ( splits[ 0 ] == "hero_tree" )
    {
      if ( auto id = trait_data_t::get_hero_tree_id( splits[ 1 ] ) )
      {
        // check hash-activated hero trees
        if ( range::contains( player_sub_trees, id ) )
          return expr_t::create_constant( expression_str, 1 );

        // check manually added hero talents
        for ( auto trait_id : player_sub_traits )
        {
          auto trait = trait_data_t::find( trait_id, is_ptr() );
          if ( trait->id_sub_tree == id )
            return expr_t::create_constant( expression_str, 1 );
        }

        return expr_t::create_constant( expression_str, 0 );
      }

      throw std::invalid_argument( fmt::format( "Cannot find hero tree '{}'.", splits[ 1 ] ) );
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
        buff = buff_t::find( this, splits[ 1 ], this );  // Raid debuffs & fallback buffs
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
      const auto s = splits[ 1 ];
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
          double evaluate() override
          {
            attack_t* attack = ( slot == SLOT_MAIN_HAND ) ? player.main_hand_attack : player.off_hand_attack;
            if ( attack && attack->execute_event )
              return attack->execute_event->remains().total_seconds();
            return 9999;
          }
        };
        return std::make_unique<swing_remains_expr_t>( *this, hand );
      }
    }

    if ( splits[ 0 ] == "spell" && splits[ 2 ] == "exists" )
    {
      return expr_t::create_constant(expression_str, find_spell( splits[ 1 ] )->ok());
    }
  } // splits.size() == 3

  // *** Variable-Length expressions from here on ***

  // talents
  if ( splits.size() >= 2 && splits[ 0 ] == "talent" )
  {
    const auto ctalent = find_talent_spell( talent_tree::CLASS, splits[ 1 ], specialization(), true );
    const auto stalent = find_talent_spell( talent_tree::SPECIALIZATION, splits[ 1 ], specialization(), true );
    const auto htalent = find_talent_spell( talent_tree::HERO, splits[ 1 ], specialization(), true );

    if ( ctalent.invalid() && stalent.invalid() && htalent.invalid() )
    {
      throw std::invalid_argument(fmt::format("Cannot find talent '{}'.", splits[ 1 ]));
    }

    if ( splits.size() == 2 || ( splits.size() == 3 && splits[ 2 ] == "enabled" ) )
    {
      return expr_t::create_constant( expression_str, ctalent.enabled() || stalent.enabled() || htalent.enabled() );
    }
    else if ( splits.size() == 3 && splits[ 2 ] == "rank" )
    {
      return expr_t::create_constant( expression_str, std::max( { ctalent.rank(), stalent.rank(), htalent.rank() } ) );
    }

    throw std::invalid_argument(
        fmt::format( "Unsupported talent expression '{}'.", splits[ 2 ] ) );
  }

  // trinkets
  if ( !splits.empty() && splits[ 0 ] == "trinket" )
  {
    if ( auto expr = unique_gear::create_expression( *this, expression_str ) )
      return expr;
  }

  if ( splits[ 0 ] == "legendary_ring" )
  {
    if ( auto expr = unique_gear::create_expression( *this, expression_str ) )
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
        auto tail = expression_str.substr( splits[ 1 ].length() + 5 );
        if ( auto e = pet->create_expression( tail ) )
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
      auto expr = spawner -> create_expression( util::make_span( splits ).subspan( 2 ), expression_str );
      if ( expr )
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
        auto tail = expression_str.substr( 6 );
        if ( auto e = pet->owner->create_expression( tail ) )
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

  // dbc
  if ( splits.size() == 4 && util::str_compare_ci( splits[ 0 ], "dbc" ) )
  {
    double value = -std::numeric_limits<double>::max();

    if ( util::str_compare_ci( splits[ 1 ], "spell" ) )
    {
      unsigned spell_id = util::to_int( splits[ 2 ] );
      auto field = splits[ 3 ];
      const spell_data_t* s = find_spell( spell_id );
      if ( s->ok() )
        value = s->get_field( field );
      return expr_t::create_constant( name_str, value );
    }

    if ( util::str_compare_ci( splits[ 1 ], "effect" ) )
    {
      unsigned effect_id = util::to_int( splits[ 2 ] );
      auto field = splits[ 3 ];
      const spelleffect_data_t* e = dbc::find_effect( this, effect_id );
      if ( e->ok() )
        value = e->get_field( field );
      return expr_t::create_constant( name_str, value );
    }

    if ( util::str_compare_ci( splits[ 1 ], "power" ) )
    {
      unsigned power_id = util::to_int( splits[ 2 ] );
      auto field = splits[ 3 ];
      const spellpower_data_t* p = dbc::find_power( this, power_id );
      if ( p->id() != 0 )
        value = p->get_field( field );
      return expr_t::create_constant( name_str, value );
    }
  }

  if ( splits[ 0 ] == "azerite" )
  {
    return azerite -> create_expression( splits );
  }

  if ( splits[ 0 ] == "essence" )
  {
    return azerite_essence->create_expression( splits );
  }

  if ( splits[ 0 ] == "soulbind" || splits[ 0 ] == "conduit" || splits[ 0 ] == "covenant" )
  {
    return covenant->create_expression( splits );
  }

  if ( splits[ 0 ] == "rune_word" || splits[ 0 ] == "hyperthread_wristwraps" )
  {
    return unique_gear::create_expression( *this, expression_str );
  }

  if ( auto expr = runeforge::create_expression( this, splits, expression_str ) )
  {
    return expr;
  }

  return sim->create_expression( expression_str );
}

std::unique_ptr<expr_t> player_t::create_resource_expression( util::string_view expression_str )
{
  auto splits = util::string_split<util::string_view>( expression_str, "." );
  if ( splits.empty() )
    return nullptr;

  resource_e r = util::parse_resource_type( splits[ 0 ] );
  if ( r == RESOURCE_NONE )
    return nullptr;

  if ( splits.size() == 1 )
    return make_ref_expr( expression_str, resources.current[ r ] );

  if ( splits.size() == 2 )
  {
    if ( splits[ 1 ] == "deficit" )
    {
      return make_fn_expr( expression_str, [ this, r ] { return resources.max[ r ] - resources.current[ r ]; } );
    }

    else if ( splits[ 1 ] == "pct" || splits[ 1 ] == "percent" )
    {
      if ( r == RESOURCE_HEALTH )
      {
        return make_mem_fn_expr( expression_str, *this, &player_t::health_percentage );
      }
      else
      {
        return make_fn_expr( expression_str, [ this, r ] { return resources.pct( r ) * 100.0; } );
      }
    }

    else if ( splits[ 1 ] == "max" )
      return make_ref_expr( expression_str, resources.max[ r ] );

    else if ( splits[ 1 ] == "max_nonproc" )
      return make_ref_expr( expression_str, collected_data.buffed_stats_snapshot.resource[ r ] );

    else if ( splits[ 1 ] == "pct_nonproc" )
    {
      return make_fn_expr( expression_str, [ this, r ] {
        return resources.current[ r ] / collected_data.buffed_stats_snapshot.resource[ r ] * 100.0;
      } );
    }

    else if ( splits[ 1 ] == "net_regen" )
    {
      return make_fn_expr( expression_str, [ this, r ] {
        timespan_t now = sim->current_time();
        if ( now != timespan_t::zero() )
          return ( iteration_resource_gained[ r ] - iteration_resource_lost[ r ] ) / now.total_seconds();
        else
          return 0.0;
      } );
    }

    else if ( splits[ 1 ] == "regen" )
    {
      return make_fn_expr( expression_str, [ this, r ] { return resource_regen_per_second( r ); } );
    }

    else if ( util::str_prefix_ci( splits[ 1 ], "time_to_" ) )
    {
      auto parts = util::string_split<util::string_view>( splits[ 1 ], "_" );

      // foo.time_to_max
      if ( util::str_in_str_ci( parts[ 2 ], "max" ) )
      {
        return make_fn_expr( expression_str, [ this, r ] {
          return ( resources.max[ r ] - resources.current[ r ] ) / resource_regen_per_second( r );
        } );
      }

      // foo.time_to_x
      const double amount = util::to_double( parts[ 2 ] );
      return make_fn_expr( expression_str, [ this, r, amount ] {
        if ( resources.current[ r ] >= amount )
        {
          return timespan_t::zero().total_seconds();
        }
        else if ( amount > resources.max[ r ] )
        {
          // If the value is impossible to reach, return functional infinity
          return timespan_t::max().total_seconds();
        }

        return ( amount - resources.current[ r ] ) / resource_regen_per_second( r );
      } );
    }
  }

  auto tail = expression_str.substr( splits[ 0 ].length() + 1 );
  throw std::invalid_argument(fmt::format("Unsupported resource expression '{}'.", tail));
}

double player_t::compute_incoming_damage( timespan_t interval ) const
{
  double amount = 0;

  if ( !incoming_damage.empty() )
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

  if ( !incoming_damage.empty() )
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
      // the sim's bloodlust_check event fires every second, so negative times under 1 second should be treated as
      // zero for safety. without this, time_to_bloodlust can spike to the next target value up to a second before
      // bloodlust is actually cast. probably a non-issue since the worst case is likely to be casting something 1
      // second too early, but we may as well account for it
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
    else if ( race == RACE_ZANDALARI_TROLL )
    {
      profile_str += "zandalari_loa=" +
                     util::to_string( zandalari_loa == player_t::AKUNDA      ? "akunda"
                                      : zandalari_loa == player_t::BWONSAMDI ? "bwonsamdi"
                                      : zandalari_loa == player_t::GONK      ? "gonk"
                                      : zandalari_loa == player_t::KIMBUL    ? "kimbul"
                                      : zandalari_loa == player_t::KRAGWA    ? "kragwa"
                                                                             : "paku" ) + term;
    }
    else if ( race == RACE_VULPERA )
    {
      profile_str += "vulpera_tricks=" +
                     util::to_string( vulpera_tricks == player_t::HOLY      ? "holy"
                                      : vulpera_tricks == player_t::FLAMES  ? "flames"
                                      : vulpera_tricks == player_t::SHADOWS ? "shadows"
                                      : vulpera_tricks == player_t::HEALING ? "healing"
                                                                            : "corrosive" ) + term;
    }
    else if ( race == RACE_EARTHEN_HORDE || race == RACE_EARTHEN_ALLIANCE )
    {
      profile_str += "earthen_mineral=" +
                     util::to_string( earthen_mineral == player_t::AMBER      ? "amber"
                                      : earthen_mineral == player_t::EMERALD  ? "emerald"
                                      : earthen_mineral == player_t::ONYX     ? "onyx"
                                      : earthen_mineral == player_t::SAPPHIRE ? "sapphire"
                                                                              : "ruby" ) + term;
    }

    profile_str += "role=";
    profile_str += util::role_type_string( primary_role() ) + term;
    profile_str += "position=" + position_str + term;

    if ( !professions_str.empty() )
    {
      profile_str += "professions=" + professions_str + term;
    }
  }

  if ( stype & SAVE_TALENTS )
  {
    if ( !talents_str.empty() )
    {
      profile_str += "talents=" + talents_str + term;
    }

    if ( !class_talents_str.empty() )
    {
      profile_str += "class_talents=" + class_talents_str + term;
    }

    if ( !spec_talents_str.empty() )
    {
      profile_str += "spec_talents=" + spec_talents_str + term;
    }

    if ( !hero_talents_str.empty() )
    {
      profile_str += "hero_talents=" + hero_talents_str + term;
    }

    if ( azerite )
    {
      std::string azerite_overrides = azerite -> overrides_str();
      if ( ! azerite_overrides.empty() )
      {
        profile_str += "azerite_override=" + azerite_overrides + term;
      }
    }

    if ( azerite_essence )
    {
      std::string azerite_essence_str = azerite_essence->option_str();
      if ( !azerite_essence_str.empty() )
      {
        profile_str += "azerite_essences=" + azerite_essence_str + term;
      }
    }

    if ( covenant )
    {
      if ( !covenant->covenant_option_str().empty() )
      {
        profile_str += covenant->covenant_option_str() + term;
      }

      if ( !covenant->soulbind_option_str().empty() )
      {
        profile_str += covenant->soulbind_option_str() + term;
      }

      if ( covenant->renown() > 0 )
      {
        profile_str += "renown=" + util::to_string( covenant->renown() ) + term;
      }
    }

    auto print_option = [ &profile_str, term ]( std::string_view n, auto option ) {
      if ( !option.is_default() )
      {
        if constexpr ( std::is_same_v<decltype( option ), player_option_t<bool>> )
          profile_str += fmt::format( "{}={}{}", n, static_cast<int>( option ), term );
        else
          profile_str += fmt::format( "{}={}{}", n, option, term );
      }
    };

    print_option( "shadowlands.soleahs_secret_technique_type_override", shadowlands_opts.soleahs_secret_technique_type );

    print_option( "dragonflight.gyroscopic_kaleidoscope_stat", dragonflight_opts.gyroscopic_kaleidoscope_stat );
    print_option( "dragonflight.player.ruby_whelp_shell_training", dragonflight_opts.ruby_whelp_shell_training );
    print_option( "dragonflight.player.ruby_whelp_shell_context", dragonflight_opts.ruby_whelp_shell_context );
    print_option( "dragonflight.ominous_chromatic_essence_dragonflight", dragonflight_opts.ominous_chromatic_essence_dragonflight );
    print_option( "dragonflight.ominous_chromatic_essence_allies", dragonflight_opts.ominous_chromatic_essence_allies );
    print_option( "dragonflight.ashkandur_humanoid", dragonflight_opts.ashkandur_humanoid );
    print_option( "dragonflight.flowstone_starting_state", dragonflight_opts.flowstone_starting_state );
    print_option( "dragonflight.spoils_of_neltharus_initial_type", dragonflight_opts.spoils_of_neltharus_initial_type );
  }

  if ( stype & SAVE_PLAYER )
  {
    std::string potion_option = potion_str.empty() ? default_potion() : potion_str;
    std::string flask_option  = flask_str.empty() ? default_flask() : flask_str;
    std::string food_option   = food_str.empty() ? default_food() : food_str;
    std::string rune_option   = rune_str.empty() ? default_rune() : rune_str;
    std::string tench_option  = temporary_enchant_str.empty() ? default_temporary_enchant() : temporary_enchant_str;

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
      if ( !tench_option.empty() )
        profile_str += "temporary_enchant=" + tench_option + term;
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

    if ( !initial_resources.empty() )
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

    if ( !apl_variable_map.empty() )
    {
      profile_str += term;
      profile_str += "# Custom default values for APL variables." + term;
      for ( const auto& v : apl_variable_map )
      {
        profile_str += "apl_variable." + v.first + "=" + v.second + term;
      }
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
  origin_str            = source->origin_str;
  profile_source_       = source->profile_source_;
  true_level            = source->true_level;
  race_str              = source->race_str;
  timeofday             = source->timeofday;
  zandalari_loa         = source->zandalari_loa;
  vulpera_tricks        = source->vulpera_tricks;
  earthen_mineral       = source->earthen_mineral;
  race                  = source->race;
  role                  = source->role;
  _spec                 = source->_spec;
  base.distance         = source->base.distance;
  position_str          = source->position_str;
  professions_str       = source->professions_str;
  talents_str           = source->talents_str;
  class_talents_str     = source->class_talents_str;
  spec_talents_str      = source->spec_talents_str;
  hero_talents_str      = source->hero_talents_str;
  player_traits         = source->player_traits;
  player_sub_trees      = source->player_sub_trees;
  player_sub_traits     = source->player_sub_traits;
  shadowlands_opts      = source->shadowlands_opts;
  dragonflight_opts     = source->dragonflight_opts;
  thewarwithin_opts     = source->thewarwithin_opts;
  resources.initial_opt = source->resources.initial_opt;

  if ( azerite )
  {
    azerite -> copy_overrides( source -> azerite );
  }
  if ( azerite_essence )
  {
    azerite_essence -> copy_state( source -> azerite_essence );
  }

  if ( covenant )
  {
    covenant->copy_state( source->covenant );
  }

  if ( source->dbc_override_ )
  {
    dbc_override_ = source->dbc_override_->clone();
    dbc_override = dbc_override_.get();
  }

  action_list_str      = source->action_list_str;
  alist_map            = source->alist_map;
  use_apl              = source->use_apl;
  apl_variable_map     = source->apl_variable_map;
  precombat_state_map  = source->precombat_state_map;
  custom_stat_buffs    = source->custom_stat_buffs;

  meta_gem = source->meta_gem;
  for ( size_t i = 0; i < items.size(); i++ )
  {
    items[ i ]        = source->items[ i ];
    items[ i ].player = this;
  }

  if ( sets != nullptr )
  {
    sets        = std::make_unique<set_bonus_t>( *source->sets );
    sets->actor = this;
  }

  gear    = source->gear;
  enchant = source->enchant;
  bugs    = source->bugs;

  potion_str = source->potion_str;
  flask_str  = source->flask_str;
  food_str   = source->food_str;
  rune_str   = source->rune_str;
  temporary_enchant_str = source->temporary_enchant_str;

  external_buffs = source->external_buffs;

  antumbra = source->antumbra;
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
  add_option( opt_func( "talents", parse_talent_string ) );
  add_option( opt_string( "race", race_str ) );
  add_option( opt_func( "timeofday", parse_timeofday ) );
  add_option( opt_func( "zandalari_loa", parse_loa ) );
  add_option( opt_func( "vulpera_tricks", parse_tricks ) );
  add_option( opt_func( "earthen_mineral", parse_mineral ) );
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
  add_option( opt_map( "apl_variable.", apl_variable_map ) );
  add_option( opt_string( "action_list", choose_action_list ) );
  add_option( opt_bool( "sleeping", base.sleeping ) );
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
  add_option( opt_func( "spec", parse_specialization ) );
  add_option( opt_func( "specialization", parse_specialization ) );
  add_option( opt_func( "stat_timelines", parse_stat_timelines ) );
  add_option( opt_bool( "disable_hotfixes", disable_hotfixes ) );
  add_option( opt_func( "min_gcd", parse_min_gcd ) );
  add_option( opt_bool( "load_default_gear", load_default_gear ) );

  // Talents
  add_option( opt_string( "class_talents", class_talents_str ) );
  add_option( opt_append( "class_talents+", class_talents_str ) );
  add_option( opt_string( "spec_talents", spec_talents_str ) );
  add_option( opt_append( "spec_talents+", spec_talents_str ) );
  add_option( opt_string( "hero_talents", hero_talents_str ) );
  add_option( opt_append( "hero_talents+", hero_talents_str ) );
  add_option( opt_bool( "load_default_talents", load_default_talents ) );

  // Consumables
  add_option( opt_string( "potion", potion_str ) );
  add_option( opt_string( "flask", flask_str ) );
  add_option( opt_string( "phial", flask_str ) );
  add_option( opt_string( "food", food_str ) );
  add_option( opt_string( "augmentation", rune_str ) );
  add_option( opt_string( "temporary_enchant", temporary_enchant_str ) );

  // Positioning
  add_option( opt_float( "x_pos", default_x_position ) );
  add_option( opt_float( "y_pos", default_y_position ) );

  // Items
  add_option( opt_string( "meta_gem", meta_gem_str ) );
  add_option( opt_deprecated( "items", "use individual slot options" ) );
  add_option( opt_deprecated( "items+", "use individual slot options" ) );
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
  add_option( opt_float( "gear_corruption", gear.corruption_rating ) );
  add_option( opt_float( "gear_corruption_resistance", gear.corruption_resistance_rating ) );

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
  add_option( opt_float( "enchant_corruption", enchant.corruption_rating ) );
  add_option( opt_float( "enchant_corruption_resistance", enchant.corruption_resistance_rating ) );

  // Regen
  add_option( opt_bool( "infinite_energy", resources.infinite_resource[ RESOURCE_ENERGY ] ) );
  add_option( opt_bool( "infinite_focus", resources.infinite_resource[ RESOURCE_FOCUS ] ) );
  add_option( opt_bool( "infinite_health", resources.infinite_resource[ RESOURCE_HEALTH ] ) );
  add_option( opt_bool( "infinite_mana", resources.infinite_resource[ RESOURCE_MANA ] ) );
  add_option( opt_bool( "infinite_rage", resources.infinite_resource[ RESOURCE_RAGE ] ) );
  add_option( opt_bool( "infinite_runic", resources.infinite_resource[ RESOURCE_RUNIC_POWER ] ) );
  add_option( opt_bool( "infinite_astral_power", resources.infinite_resource[ RESOURCE_ASTRAL_POWER ] ) );

  // Rygelon Dagger / Antumbra
  add_option( opt_bool( "shadowlands.antumbra.swap", antumbra.swap ) );
  add_option( opt_float( "shadowlands.antumbra.int_diff", antumbra.int_diff ) );
  add_option( opt_float( "shadowlands.antumbra.crit_diff", antumbra.crit_diff ) );
  add_option( opt_float( "shadowlands.antumbra.haste_diff", antumbra.haste_diff ) );
  add_option( opt_float( "shadowlands.antumbra.mastery_diff", antumbra.mastery_diff ) );
  add_option( opt_float( "shadowlands.antumbra.vers_diff", antumbra.vers_diff ) );
  add_option( opt_float( "shadowlands.antumbra.stam_diff", antumbra.stam_diff ) );

  // Resources
  add_option( opt_func( "initial_resource", parse_initial_resource ) );

  // Misc
  add_option( opt_string( "skip_actions", action_list_skip ) );
  add_option( opt_string( "modify_action", modify_action ) );
  add_option( opt_string( "use_apl", use_apl ) );
  add_option( opt_timespan( "reaction_time_mean", reaction.mean ) );
  add_option( opt_timespan( "reaction_time_stddev", reaction.stddev ) );
  add_option( opt_timespan( "reaction_time_nu", reaction_nu ) );
  add_option( opt_timespan( "reaction_time_offset", reaction_offset ) );
  add_option( opt_timespan( "reaction_time_max", reaction_max ) );
  add_option( opt_bool( "stat_cache", cache.active ) );
  add_option( opt_timespan( "default_item_group_cooldown", default_item_group_cooldown, 0_ms, timespan_t::max() ) );
  add_option( opt_func( "override.precombat_state",
    [ this ] ( sim_t*, util::string_view, util::string_view value ) {
      auto splits = util::string_split<std::string>( value, "=" );
      if ( splits.size() != 2 )
        throw std::invalid_argument( fmt::format( "Invalid 'override.precombat_state' option: '{}'", value ) );
      precombat_state_map[ splits[ 0 ] ] = splits[ 1 ];
      return true;
    } ) );
  add_option( opt_func( "set_custom_buff",
    [ this ] ( sim_t*, util::string_view, util::string_view value ) {
      if ( value.empty() )
        return true;

      auto splits = util::string_split<std::string>( value, "," );
      std::string name{};
      bool has_data = false;
      std::string stat_value{};
      for ( auto s : splits )
      {
        auto sub_splits = util::string_split<std::string>( s, "=" );

        if ( sub_splits.size() == 1 )
        {
          name = fmt::format( "custom_buff_{}", sub_splits[ 0 ] );
          continue;
        }

        if ( sub_splits.size() != 2 )
          throw std::invalid_argument( fmt::format( "Invalid 'custom_stat_buff' sub option: '{}'", s ) );

        if ( sub_splits[ 0 ] == "name" )
        {

        }
        else if ( sub_splits[ 0 ] == "stat_value" )
        {
          stat_value = sub_splits[ 1 ];
          has_data = true;
        }
        else
        {
          throw std::invalid_argument( fmt::format( "Unsupported 'custom_stat_buff' option: '{}'", sub_splits[ 0 ] ) );
        }
      }

      if ( name.empty() )
        throw std::invalid_argument( fmt::format( "Invalid 'custom_stat_buff' usage, the buff must have a name: '{}'", value ) );

      // If the custom buff has no data, remove any existing buffs with the name.
      if ( !has_data )
      {
        custom_stat_buffs.erase( name );
        return true;
      }

      // Parse the data for the stat buff
      if ( !stat_value.empty() )
      {
        splits = util::string_split<std::string>( stat_value, "_" );
        if ( splits.size() != 2 )
          throw std::invalid_argument( fmt::format( "Invalid 'custom_stat_buff' value option: '{}'", stat_value ) );
        stat_e stat = util::parse_stat_type( splits[ 1 ] );
        if ( stat == STAT_NONE )
          throw std::invalid_argument( fmt::format( "Invalid 'custom_stat_buff' stat type: '{}'", splits[ 1 ] ) );
        bool is_percentage = false;
        if ( splits[ 0 ].back() == '%' )
        {
          // The stat type will be checked later when creating the buff to ensure that it supports percentage buffs.
          is_percentage = true;
          splits[ 0 ].pop_back();
        }
        double amount = util::to_double( splits[ 0 ] );
        custom_stat_buffs[ name ] = { stat, amount, is_percentage };
      }

      return true;
    } ) );

  // Invoke External Buffs
  add_option( opt_string( "external_buffs.pool", external_buffs.pool ) );

  // Permanent External Buffs
  add_option( opt_bool( "external_buffs.focus_magic", external_buffs.focus_magic ) );
  add_option( opt_int( "external_buffs.soleahs_secret_technique_ilevel", external_buffs.soleahs_secret_technique, 1, MAX_ILEVEL ) );
  add_option( opt_string( "external_buffs.elegy_of_the_eternals", external_buffs.elegy_of_the_eternals ) );

  // Timed External Buffs
  auto opt_external_buff_times = [] ( util::string_view name, std::vector<timespan_t>& times )
  {
    return opt_func( name, [ & times ] ( sim_t*, util::string_view, util::string_view val )
    {
      times.clear();
      auto splits = util::string_split<util::string_view>( val, "/" );
      for ( auto split : splits )
      {
        double t = util::to_double( split );
        if ( t < 0.0 )
          throw std::invalid_argument( "external buffs cannot be applied at negative times" );
        times.push_back( timespan_t::from_seconds( t ) );
      }
      return true;
    } );
  };

  add_option( opt_external_buff_times( "external_buffs.power_infusion", external_buffs.power_infusion ) );
  add_option( opt_external_buff_times( "external_buffs.symbol_of_hope", external_buffs.symbol_of_hope ) );
  add_option( opt_external_buff_times( "external_buffs.blessing_of_summer", external_buffs.blessing_of_summer ) );
  add_option( opt_external_buff_times( "external_buffs.blessing_of_autumn", external_buffs.blessing_of_autumn ) );
  add_option( opt_external_buff_times( "external_buffs.blessing_of_winter", external_buffs.blessing_of_winter ) );
  add_option( opt_external_buff_times( "external_buffs.blessing_of_spring", external_buffs.blessing_of_spring ) );
  add_option( opt_external_buff_times( "external_buffs.conquerors_banner", external_buffs.conquerors_banner ) );
  add_option( opt_external_buff_times( "external_buffs.rallying_cry", external_buffs.rallying_cry ) );
  add_option( opt_external_buff_times( "external_buffs.pact_of_the_soulstalkers", external_buffs.pact_of_the_soulstalkers ) ); // 9.1 Kyrian Hunter Legendary
  add_option( opt_external_buff_times( "external_buffs.boon_of_azeroth", external_buffs.boon_of_azeroth ) );
  add_option( opt_external_buff_times( "external_buffs.boon_of_azeroth_mythic", external_buffs.boon_of_azeroth_mythic ) );
  add_option( opt_external_buff_times( "external_buffs.tome_of_unstable_power", external_buffs.tome_of_unstable_power) );

  // Additional Options for Timed External Buffs
  add_option( opt_func( "external_buffs.the_long_summer_rank", [ this ] ( sim_t*, util::string_view, util::string_view val )
  {
    unsigned rank = util::to_unsigned( val );
    if ( rank <= 0 )
      return true;

    const auto &conduit = conduit_entry_t::find( "The Long Summer", dbc->ptr );
    if ( conduit.id == 0 )
      throw std::invalid_argument( "unable to find conduit entry data for The Long Summer" );

    const auto &rank_entry = conduit_rank_entry_t::find( conduit.id, rank - 1, dbc->ptr );
    if ( rank_entry.conduit_id == 0 )
      throw std::invalid_argument( "invalid conduit rank" );

    external_buffs.blessing_of_summer_duration_multiplier = 0.01 * rank_entry.value;
    return true;
  } ) );
  add_option(opt_int("external_buffs.tome_of_unstable_power_ilevel", external_buffs.tome_of_unstable_power_ilevel, 1, MAX_ILEVEL));

  // Azerite options
  if ( ! is_enemy() && ! is_pet() )
  {
    add_option( opt_func( "azerite_override", std::bind( &azerite::azerite_state_t::parse_override,
          azerite.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 ) ) );
    add_option( opt_func( "azerite_essences", std::bind( &azerite::azerite_essence_state_t::parse_azerite_essence,
          azerite_essence.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 ) ) );

    if ( covenant )
    {
      covenant->register_options( this );
    }

    add_option( opt_func( "override.player.spell_data",
        [ this ]( sim_t*, util::string_view, util::string_view value ) {
          dbc_override_->parse( *dbc, value );
          return true;
        } ) );
  }

  // Shadowlands options
  add_option( opt_string( "shadowlands.soleahs_secret_technique_type_override", shadowlands_opts.soleahs_secret_technique_type ) );

  // Dragonflight options
  add_option( opt_string( "dragonflight.gyroscopic_kaleidoscope_stat", dragonflight_opts.gyroscopic_kaleidoscope_stat ) );
  add_option( opt_string( "dragonflight.player.ruby_whelp_shell_training", dragonflight_opts.ruby_whelp_shell_training ) );
  add_option( opt_string( "dragonflight.player.ruby_whelp_shell_context", dragonflight_opts.ruby_whelp_shell_context ) );
  add_option( opt_string( "dragonflight.ominous_chromatic_essence_dragonflight", dragonflight_opts.ominous_chromatic_essence_dragonflight ) );
  add_option( opt_string( "dragonflight.ominous_chromatic_essence_allies", dragonflight_opts.ominous_chromatic_essence_allies ) );
  add_option( opt_bool( "dragonflight.ashkandur_humanoid", dragonflight_opts.ashkandur_humanoid ) );
  add_option( opt_string( "dragonflight.flowstone_starting_state", dragonflight_opts.flowstone_starting_state ) );
  add_option( opt_string( "dragonflight.spoils_of_neltharus_initial_type", dragonflight_opts.spoils_of_neltharus_initial_type ) );
  add_option( opt_float( "dragonflight.igneous_flowstone_double_lava_wave_chance", dragonflight_opts.igneous_flowstone_double_lava_wave_chance ) );
  add_option( opt_bool( "dragonflight.voice_of_the_silent_star_enable", dragonflight_opts.voice_of_the_silent_star_enable ) );
  add_option( opt_bool( "dragonflight.nymue_forced_immobilized", dragonflight_opts.nymue_forced_immobilized ) );
  add_option( opt_func( "dragonflight.witherbarks_branch_timing", [ this ]( sim_t*, util::string_view,
                                                                            util::string_view value ) {
    auto splits = util::string_split<std::string>( value, "/" );
    if ( splits.size() != 3 )
      throw std::invalid_argument( fmt::format( "Invalid 'dragonflight.witherbarks_branch_timing' option: '{}'", value ) );

    for ( size_t i = 0; i < 3; i++ )
    {
      dragonflight_opts.witherbarks_branch_timing[ i ] = timespan_t::from_seconds( util::to_double( splits[ i ] ) );
    }
    return true;
  } ) );
  add_option( opt_bool( "dragonflight.rallied_to_victory_ally_estimate", dragonflight_opts.rallied_to_victory_ally_estimate ) );
  add_option( opt_float( "dragonflight.rallied_to_victory_min_allies", dragonflight_opts.rallied_to_victory_min_allies, 0.0, 4 ) );
  add_option( opt_bool( "dragonflight.player.embersoul_debuff_immune", dragonflight_opts.embersoul_debuff_immune ) );
  add_option( opt_float( "dragonflight.rallied_to_victory_multi_actor_skip_chance",
                         dragonflight_opts.rallied_to_victory_multi_actor_skip_chance, 0.0, 1 ) );
  add_option( opt_bool( "dragonflight.string_of_delicacies_ally_estimate", dragonflight_opts.string_of_delicacies_ally_estimate ) );
  add_option( opt_float( "dragonflight.string_of_delicacies_min_allies", dragonflight_opts.string_of_delicacies_min_allies, 0.0, 4 ) );
  add_option( opt_float( "dragonflight.string_of_delicacies_multi_actor_skip_chance",
                         dragonflight_opts.string_of_delicacies_multi_actor_skip_chance, 0.0, 1 ) );
  add_option( opt_string( "dragonflight.balefire_branch_loss_rng_type", dragonflight_opts.balefire_branch_loss_rng_type ) );
  add_option( opt_float( "dragonflight.balefire_branch_loss_rppm", dragonflight_opts.balefire_branch_loss_rppm, 0.0, std::numeric_limits<double>::max() ) );
  add_option( opt_float( "dragonflight.balefire_branch_loss_percent", dragonflight_opts.balefire_branch_loss_percent, 0.0, 1.0 ) );
  add_option( opt_timespan( "dragonflight.balefire_branch_loss_tick", dragonflight_opts.balefire_branch_loss_tick, 1_ms, 20_s ) );
  add_option( opt_int( "dragonflight.balefire_branch_loss_stacks", dragonflight_opts.balefire_branch_loss_stacks, 0, 20 ) );
  add_option( opt_uint( "dragonflight.verdant_conduit_allies", dragonflight_opts.verdant_conduit_allies, 0, 2 ) );
  add_option( opt_bool( "dragonflight.rashoks_use_true_overheal", dragonflight_opts.rashoks_use_true_overheal ) );
  add_option( opt_float( "dragonflight.rashoks_fake_overheal", dragonflight_opts.rashoks_fake_overheal, 0.0, 1.0 ) );
  add_option( opt_string( "dragonflight.timerunners_advantage", dragonflight_opts.timerunners_advantage ) );
  add_option( opt_int( "dragonflight.brilliance_party", dragonflight_opts.brilliance_party, 0, 4 ) );
  add_option( opt_int( "dragonflight.windweaver_party", dragonflight_opts.windweaver_party, 0, 4 ) );
  add_option( opt_string( "dragonflight.windweaver_party_ilvls", dragonflight_opts.windweaver_party_ilvls ) );

  // The War Within options
  add_option( opt_string( "thewarwithin.sikrans_endless_arsenal_stance", thewarwithin_opts.sikrans_endless_arsenal_stance ) );
  add_option( opt_int( "thewarwithin.ovinaxs_mercurial_egg_initial_primary_stacks", thewarwithin_opts.ovinaxs_mercurial_egg_initial_primary_stacks, 0, 30 ) );
  add_option( opt_int( "thewarwithin.ovinaxs_mercurial_egg_initial_secondary_stacks", thewarwithin_opts.ovinaxs_mercurial_egg_initial_secondary_stacks, 0, 30 ) );
  add_option( opt_timespan( "thewarwithin.entropic_skardyn_core_pickup_delay", thewarwithin_opts.entropic_skardyn_core_pickup_delay, 0_ms, 30_s ) );
  add_option( opt_timespan( "thewarwithin.entropic_skardyn_core_pickup_stddev", thewarwithin_opts.entropic_skardyn_core_pickup_stddev, 0_ms, 30_s ) );
  add_option( opt_timespan( "thewarwithin.carved_blazikon_wax_enter_light_delay", thewarwithin_opts.carved_blazikon_wax_enter_light_delay, 0_ms, 15_s ) );
  add_option( opt_timespan( "thewarwithin.carved_blazikon_wax_enter_light_stddev", thewarwithin_opts.carved_blazikon_wax_enter_light_stddev, 0_ms, 15_s ) );
  add_option( opt_timespan( "thewarwithin.carved_blazikon_wax_stay_in_light_duration", thewarwithin_opts.carved_blazikon_wax_stay_in_light_duration, 0_ms, 15_s ) );
  add_option( opt_timespan( "thewarwithin.carved_blazikon_wax_stay_in_light_stddev", thewarwithin_opts.carved_blazikon_wax_stay_in_light_stddev, 0_ms, 15_s ) );
  add_option( opt_string( "thewarwithin.signet_of_the_priory_party_stats", thewarwithin_opts.signet_of_the_priory_party_stats ) );
  add_option( opt_timespan( "thewarwithin.signet_of_the_priory_party_use_cooldown", thewarwithin_opts.signet_of_the_priory_party_use_cooldown, 120_s, 240_s ) );
  add_option( opt_timespan( "thewarwithin.signet_of_the_priory_party_use_stddev", thewarwithin_opts.signet_of_the_priory_party_use_stddev, 0_ms, 120_s ) );
  add_option( opt_float( "thewarwithin.harvesters_edict_intercept_chance", thewarwithin_opts.harvesters_edict_intercept_chance, 0.0, 1.0 ) );
  add_option( opt_float( "thewarwithin.dawn_dusk_thread_lining_uptime", thewarwithin_opts.dawn_dusk_thread_lining_uptime, 0.0, 1.0 ) );
  add_option( opt_timespan( "thewarwithin.dawn_dusk_thread_lining_update_interval", thewarwithin_opts.dawn_dusk_thread_lining_update_interval, 1_s, timespan_t::max() ) );
  add_option( opt_timespan( "thewarwithin.dawn_dusk_thread_lining_update_interval_stddev", thewarwithin_opts.dawn_dusk_thread_lining_update_interval_stddev, 1_s, timespan_t::max() ) );
  add_option( opt_timespan( "thewarwithin.embrace_of_the_cinderbee_timing", thewarwithin_opts.embrace_of_the_cinderbee_timing, 100_ms, 10_s ) );
  add_option( opt_float( "thewarwithin.embrace_of_the_cinderbee_miss_chance", thewarwithin_opts.embrace_of_the_cinderbee_miss_chance, 0, 1 ) );
  add_option( opt_int( "thewarwithin.nerubian_pheromone_secreter_pheromones", thewarwithin_opts.nerubian_pheromone_secreter_pheromones, 0, 3 ) );
  add_option( opt_int( "thewarwithin.binding_of_binding_on_you", thewarwithin_opts.binding_of_binding_on_you, 0, 29 ) );
  add_option( opt_float( "thewarwithin.binding_of_binding_ally_skip_chance", thewarwithin_opts.binding_of_binding_ally_skip_chance, 0, 1 ) );
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

  range::for_each( buff_list, []( buff_t* b ) { b->analyze(); } );

  range::for_each( proc_list, []( proc_t* pr ) { pr->analyze(); } );
  range::for_each( uptime_list, []( uptime_t* up ) { up->analyze(); } );
  range::for_each( benefit_list, []( benefit_t* ben ) { ben->analyze(); } );
  range::for_each( cooldown_waste_data_list, std::mem_fn( &cooldown_waste_data_t::analyze ) );

  range::sort( stats_list, []( const stats_t* l, const stats_t* r ) { return l->name_str < r->name_str; } );
  range::sort( gain_list, []( const gain_t* l, const gain_t* r ) { return l->name_str < r-> name_str; } );

  if ( quiet )
    return;

  // If we have no data collected for an actor, also verify thier pets have data collected
  // Relevant for fight styles with an initial sleeping target
  if ( collected_data.fight_length.mean() == 0 )
  {
    auto it = range::find_if( pet_list, []( pet_t* pet ) {
      return pet->collected_data.fight_length.mean() > 0 ||
             pet->iteration_fight_length.total_seconds() > 0;
    });

    if ( it == pet_list.end() )
      return;
  }

  range::for_each( sample_data_list, []( sample_data_helper_t* sd ) { sd->analyze(); } );

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

  for ( const auto* pet : pet_list )
  {
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
      return { metric, q->collected_data.dps };
    case SCALE_METRIC_DPSE:
      return { metric, q->collected_data.dpse };
    case SCALE_METRIC_HPS:
      return { metric, q->collected_data.hps };
    case SCALE_METRIC_HPSE:
      return { metric, q->collected_data.hpse };
    case SCALE_METRIC_APS:
      return { metric, q->collected_data.aps };
    case SCALE_METRIC_DPSP:
      return { metric, q->collected_data.prioritydps };
    case SCALE_METRIC_HAPS:
    {
      double mean   = q->collected_data.hps.mean() + q->collected_data.aps.mean();
      double stddev = sqrt( q->collected_data.hps.mean_variance + q->collected_data.aps.mean_variance );
      return { metric, "Healing + Absorb per second", mean, stddev };
    }
    case SCALE_METRIC_DTPS:
      return { metric, q->collected_data.dtps };
    case SCALE_METRIC_DMG_TAKEN:
      return { metric, q->collected_data.dmg_taken };
    case SCALE_METRIC_HTPS:
      return { metric, q->collected_data.htps };
    case SCALE_METRIC_DEATHS:
      return { metric, q->collected_data.deaths };
    case SCALE_METRIC_TIME:
      return { metric, q->collected_data.fight_length };
    case SCALE_METRIC_RAID_DPS:
      return { metric, q->sim->raid_dps };
    default:
      if ( q->primary_role() == ROLE_TANK )
        return { SCALE_METRIC_DTPS, q->collected_data.dtps };
      else if ( q->primary_role() == ROLE_HEAL )
        return scaling_for_metric( SCALE_METRIC_HAPS );
      else
        return { SCALE_METRIC_DPS, q->collected_data.dps };
  }
}

bool player_t::requires_data_collection() const
{
  if ( active_during_iteration )
    return true;

  for ( const auto* pet : pet_list )
  {
    if ( pet->requires_data_collection() )
      return true;
  }

  return false;
}

/**
 * Change the player position ( fron/back, etc. ) and update attack hit table
 */
void player_t::change_position( position_e new_pos )
{
  sim->print_debug( "{} changes position from {} to {}.", name(), position(), new_pos );

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
  resource_callback_entry_t entry{resource, value, use_pct, fire_once, false, std::move(callback)};
  resource_callbacks.push_back(entry);
  has_active_resource_callbacks = true;
  sim->print_debug("{} resource callback registered. resource={} value={} pct={} fire_once={}",
      name(), resource, value, use_pct, fire_once);
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

rng::rng_t& player_t::rng()
{
  return sim -> rng();
}

rng::rng_t& player_t::rng() const
{
  return sim -> rng();
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

void player_t::trigger_movement( double distance, movement_direction_type direction )
{
  // Distance of 0 disables movement
  if ( distance == 0 )
    do_update_movement( 9999 );
  else
  {
    if ( direction == movement_direction_type::AWAY )
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
    if ( current.movement_direction == movement_direction_type::TOWARDS )
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
void player_t::teleport( double yards, timespan_t )
{
  do_update_movement( yards );

  if ( sim->debug )
    sim->out_debug.printf( "Player %s warp, direction=%s speed=LIGHTSPEED! distance_covered=%f to_go=%f", name(),
                           util::movement_direction_string( movement_direction() ), yards, current.distance_to_move );
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
    current.movement_direction = movement_direction_type::NONE;
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
      current.movement_direction = movement_direction_type::TOWARDS;
      current.distance_to_move -= yards;
    }
  }
}

player_collected_data_t::action_sequence_data_t::action_sequence_data_t( const action_t* a, const player_t* t,
                                                                         timespan_t ts, timespan_t wait,
                                                                         const player_t* p )
  : action( a ), target( t ), target_name( t ? t->name_str : "" ), time( ts ), wait_time( wait ), queue_failed( false )
{
  for ( buff_t* b : p->buff_list )
  {
    if ( b->check() && !b->quiet && !b->constant )
    {
      buff_list.emplace_back( b, b->check(), b->remains() );
    }
  }

  // Adding cooldown and debuffs snapshots if asking for json full states
  if ( p->sim->json_full_states )
  {
    for ( cooldown_t* c : p->cooldown_list )
    {
      if ( c->down() )
      {
        cooldown_list.emplace_back( c, c->charges, c->remains() );
      }
    }
    for ( player_t* current_target : p->sim->target_list )
    {
      decltype(target_list)::value_type::second_type debuff_list;
      for ( buff_t* d : current_target->buff_list )
      {
        if ( d->check() && !d->quiet && !d->constant )
        {
          debuff_list.emplace_back( d, d->check(), d->remains() );
        }
      }
      target_list.emplace_back( current_target, std::move( debuff_list ) );
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
  target_metric( player->name_str + " Target Metric", generic_container_type( player, 1 ) ),
  resource_timelines(),
  health_pct(),
  combat_start_resource(
    ( !player->is_enemy() && ( !player->is_pet() || player->sim->report_pets_separately ) ) ? RESOURCE_MAX : 0 ),
  combat_end_resource(
    ( !player->is_enemy() && ( !player->is_pet() || player->sim->report_pets_separately ) ) ? RESOURCE_MAX : 0 ),
  stat_timelines(),
  health_changes(),
  total_iterations( 0 ),
  buffed_stats_snapshot()
{
  if ( !player->is_enemy() && ( !player->is_pet() || player->sim->report_pets_separately ) )
  {
    resource_lost.resize( RESOURCE_MAX );
    resource_gained.resize( RESOURCE_MAX );
    resource_overflowed.resize( RESOURCE_MAX );
  }

  // Enemies only have health
  if ( player->is_enemy() )
  {
    resource_lost.resize( RESOURCE_HEALTH + 1 );
    resource_gained.resize( RESOURCE_HEALTH + 1 );
    resource_overflowed.resize( RESOURCE_HEALTH + 1 );
  }
}

void player_collected_data_t::reserve_memory( const player_t& p )
{
  unsigned size = std::min( as<unsigned>( p.sim->iterations ), 2048U );
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

  if ( !p.is_pet() && p.primary_role() == ROLE_TANK && p.type != PLAYER_SIMPLIFIED )
    p.sim->num_tanks++;
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

  for ( size_t i = 0, end = resource_lost.size(); i < end; ++i )
  {
    resource_lost[ i ].merge( other.resource_lost[ i ] );
    resource_gained[ i ].merge( other.resource_gained[ i ] );
    resource_overflowed[ i ].merge( other.resource_overflowed[ i ] );
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

  health_pct.merge( other.health_pct );

  for ( size_t i = 0, end = combat_start_resource.size(); i < end; ++i )
  {
    combat_start_resource[ i ].merge( other.combat_start_resource[ i ] );
    combat_end_resource[ i ].merge( other.combat_end_resource[ i ] );
  }

  assert( stat_timelines.size() == other.stat_timelines.size() );
  for ( size_t i = 0; i < stat_timelines.size(); ++i )
  {
    assert( stat_timelines[ i ].type == other.stat_timelines[ i ].type );
    stat_timelines[ i ].timeline.merge( other.stat_timelines[ i ].timeline );
  }

  health_changes.merged_timeline.merge( other.health_changes.merged_timeline );
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

  if ( !p.sim->single_actor_batch )
  {
    timeline_dmg_taken.adjust( *p.sim );
    timeline_healing_taken.adjust( *p.sim );

    health_pct.adjust( *p.sim );
    range::for_each( resource_timelines, [&p]( resource_timeline_t& tl ) { tl.timeline.adjust( *p.sim ); } );
    range::for_each( stat_timelines, [&p]( stat_timeline_t& tl ) { tl.timeline.adjust( *p.sim ); } );

    // health changes need their own divisor
    health_changes.merged_timeline.adjust( *p.sim );
  }
  // Single actor batch mode has to analyze the timelines in relation to their own fight lengths,
  // instead of the simulation-wide fight length.
  else
  {
    timeline_dmg_taken.adjust( fight_length );
    timeline_healing_taken.adjust( fight_length );

    health_pct.adjust( fight_length );
    range::for_each( resource_timelines, [this]( resource_timeline_t& tl ) { tl.timeline.adjust( fight_length ); } );
    range::for_each( stat_timelines, [this]( stat_timeline_t& tl ) { tl.timeline.adjust( fight_length ); } );

    // health changes need their own divisor
    health_changes.merged_timeline.adjust( fight_length );
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

  // player + pet dmg
  double total_iteration_dmg = range::accumulate(p.pet_list, p.iteration_dmg, &player_t::iteration_dmg);

  double total_priority_iteration_dmg = range::accumulate(p.pet_list, p.priority_iteration_dmg, &player_t::priority_iteration_dmg);

  // player + pet heal
  double total_iteration_heal = range::accumulate(p.pet_list, p.iteration_heal, &player_t::iteration_heal);

  double total_iteration_absorb = range::accumulate(p.pet_list, p.iteration_absorb, &player_t::iteration_absorb);

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
  }
  for ( size_t i = 0, end = resource_gained.size(); i < end; ++i )
  {
    resource_gained[ i ].add( p.iteration_resource_gained[ i ] );
  }
  for ( size_t i = 0, end = resource_overflowed.size(); i < end; ++i )
  {
    resource_overflowed[ i ].add( p.iteration_resource_overflowed[ i ] );
  }

  for ( size_t i = 0, end = combat_end_resource.size(); i < end; ++i )
  {
    combat_end_resource[ i ].add( p.resources.current[ i ] );
  }

  if ( !p.is_pet() && p.primary_role() == ROLE_TANK && p.type != PLAYER_SIMPLIFIED )
    health_changes.merged_timeline.merge( health_changes.timeline );

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
        metric = dps_metric;
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

  util::span<action_t* const> action_list;
  switch ( type )
  {
    case execute_type::OFF_GCD:
      action_list = list.off_gcd_actions;
      break;
    case execute_type::CAST_WHILE_CASTING:
      action_list = list.cast_while_casting_actions;
      break;
    default:
      action_list = list.foreground_action_list;
      break;
  }

  for ( action_t* a : action_list )
  {
    visited_apls_ = _visited;

    if ( list.random == 1 )
    {
      size_t random = rng().range( action_list.size() );
      a             = action_list[ random ];
    }
    else
    {
      double skill = list.player->current.skill - list.player->current.skill_debuff;
      if ( skill != 1 && rng().roll( ( 1 - skill ) * 0.5 ) )
      {
        size_t max_random_attempts = static_cast<size_t>( action_list.size() * ( skill * 0.5 ) );
        size_t random              = rng().range( action_list.size() );
        a                          = action_list[ random ];
        attempted_random++;
        // Limit the amount of attempts to select a random action based on skill, then bail out and try again in 100
        // ms.
        if ( attempted_random > max_random_attempts )
          break;
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

player_t* player_t::actor_by_name_str( util::string_view name ) const
{
  // Check player pets first
  for ( auto* pet : pet_list )
  {
    if ( util::str_compare_ci( pet->name_str, name ) )
      return pet;
  }

  // Check harmful targets list
  for ( auto* t : sim->target_list )
  {
    if ( util::str_compare_ci( t->name_str, name ) )
      return t;
  }

  // Finally, check player (non pet list), don't support targeting other
  // people's pets for now
  for ( auto* p : sim->player_no_pet_list )
  {
    if ( util::str_compare_ci( p->name_str, name ) )
      return p;
  }

  return nullptr;
}

slot_e player_t::parent_item_slot( const item_t& item ) const
{
  unsigned parent = dbc->parent_item( item.parsed.data.id );
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
  unsigned child = dbc->child_item( item.parsed.data.id );
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

timespan_t player_t::cooldown_tolerance() const
{
  return cooldown_tolerance_ < timespan_t::zero() ? sim -> default_cooldown_tolerance : cooldown_tolerance_;
}

pet_t* player_t::cast_pet()
{
  return debug_cast<pet_t*>( this );
}

const pet_t* player_t::cast_pet() const
{
  return debug_cast<const pet_t*>( this );
}

void player_t::add_active_dot( const dot_t* dot )
{
  assert( dot );
  if ( !dot )
    return;

  if ( active_dots.size() < as<size_t>( dot->internal_id + 1 ) )
    active_dots.resize( dot->internal_id + 1 );

  active_dots[ dot->internal_id ]++;
  if ( sim->debug )
    sim->out_debug.printf( "%s Increasing %s dot count to %u", name(), dot_map[ dot->internal_id ].c_str(), active_dots[ dot->internal_id ] );
}

void player_t::remove_active_dot( const dot_t* dot )
{
  assert( dot );
  assert( active_dots.size() > as<size_t>( dot->internal_id ) );
  assert( active_dots[ dot->internal_id ] > 0 );

  if ( !dot )
    return;

  active_dots[ dot->internal_id ]--;
  if ( sim->debug )
    sim->out_debug.printf( "%s Decreasing %s dot count to %u", name(), dot_map[ dot->internal_id ].c_str(), active_dots[ dot->internal_id ] );
}

unsigned player_t::get_active_dots( const dot_t* dot ) const
{
  assert( dot );

  if ( !dot || active_dots.size() <= as<size_t>( dot->internal_id ) )
    return 0;

  return active_dots[ dot->internal_id ];
}

void player_t::adjust_dynamic_cooldowns()
{
  range::for_each( dynamic_cooldown_list, []( cooldown_t* cd ) { cd -> adjust_recharge_multiplier(); } );
}

// TODO: This currently does not take minimum GCD into account, should it?
void player_t::adjust_global_cooldown( gcd_haste_type gcd_type )
{
  // Don't adjust if the current gcd isnt hasted
  if ( gcd_type == gcd_haste_type::NONE )
  {
    return;
  }

  // Don't adjust if the changed haste type isnt current GCD haste scaling type
  if ( gcd_type != gcd_haste_type::HASTE && gcd_type != this ->gcd_type )
  {
    return;
  }

  // Don't adjust elapsed GCDs
  if ( ( readying && readying->occurs() <= sim->current_time() ) || ( gcd_ready <= sim->current_time() ) )
  {
    return;
  }

  double new_haste = 0;
  switch ( gcd_type )
  {
    case gcd_haste_type::SPELL_HASTE:
      new_haste = cache.spell_haste();
      break;
    case gcd_haste_type::ATTACK_HASTE:
      new_haste = cache.attack_haste();
      break;
    case gcd_haste_type::SPELL_CAST_SPEED:
      new_haste = cache.spell_cast_speed();
      break;
    case gcd_haste_type::AUTO_ATTACK_SPEED:
      new_haste = cache.auto_attack_speed();
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
  timespan_t remains     = readying ? readying->remains() : ( gcd_ready - sim->current_time() );
  timespan_t new_remains = remains * delta;

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

void player_t::adjust_auto_attack( gcd_haste_type type )
{
  // Don't adjust autoattacks on spell-derived haste
  if ( type == gcd_haste_type::SPELL_CAST_SPEED || type == gcd_haste_type::SPELL_HASTE )
  {
    return;
  }

  if ( main_hand_attack )
    main_hand_attack->reschedule_auto_attack( current_auto_attack_speed );
  if ( off_hand_attack )
    off_hand_attack->reschedule_auto_attack( current_auto_attack_speed );

  current_auto_attack_speed = cache.auto_attack_speed();
}

timespan_t find_minimum_cd( const std::vector<std::pair<const cooldown_t*, const cooldown_t*>>& list )
{
  timespan_t min_ready = timespan_t::min();

  range::for_each( list, [ &min_ready ]( std::pair<const cooldown_t*, const cooldown_t*> cds ) {
    // Take the larger of the primary cooldown and the internal cooldown for a given ability
    timespan_t action_cd = std::max( cds.first ? cds.first->queueable() : timespan_t::min(),
                                     cds.second ? cds.second->queueable() : timespan_t::min() );
    if ( min_ready == timespan_t::min() || action_cd < min_ready )
    {
      min_ready = action_cd;
    }
  } );

  return min_ready;
}

// Calculate the minimum readiness time for all off gcd actions that are flagged as usable during
// gcd (i.e., action_t::use_off_gcd == true). Note that this does not take into account
// line_cooldown option, since that is APL-line specific.
void player_t::update_off_gcd_ready()
{
  if ( off_gcd_cd.empty() )
  {
    return;
  }

  off_gcd_ready = find_minimum_cd( off_gcd_cd );

  sim->print_debug( "{} next off-GCD cooldown ready at {}", *this, off_gcd_ready.total_seconds() );

  schedule_off_gcd_ready();
}

// Calculate the minimum readiness time for all off gcd actions that are flagged as usable during
// gcd (i.e., action_t::use_off_gcd == true). Note that this does not take into account
// line_cooldown option, since that is APL-line specific.
void player_t::update_cast_while_casting_ready()
{
  if ( cast_while_casting_cd.empty() )
  {
    return;
  }

  cast_while_casting_ready = find_minimum_cd( cast_while_casting_cd );

  sim->print_debug( "{} next cast while casting cooldown ready at {}",
                    *this, cast_while_casting_ready.total_seconds() );

  schedule_cwc_ready();
}

/**
 * Verify that the user input (APL) contains an use-item line for all on-use items
 */
bool player_t::verify_use_items() const
{
  if ( !sim->use_item_verification || ( action_list_str.empty() && action_priority_list.empty() ) )
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
 * Cancel the main hand (and off hand, if applicable) swing timer
 */
void player_t::cancel_auto_attacks()
{
  if ( main_hand_attack || off_hand_attack )
  {
    sim->print_debug( "Cancelling auto attack swings" );
  }

  if ( main_hand_attack && main_hand_attack->execute_event )
  {
    event_t::cancel( main_hand_attack->execute_event );
  }

  if ( off_hand_attack && off_hand_attack->execute_event )
  {
    event_t::cancel( off_hand_attack->execute_event );
  }
}

/**
 * Reset the main hand (and off hand, if applicable) swing timer
 * Optionally delay by a set amount
 */
void player_t::reset_auto_attacks( timespan_t delay, proc_t* proc )
{
  if( main_hand_attack || off_hand_attack )
  {
    if ( delay == timespan_t::zero() )
      sim->print_debug( "Resetting auto attack swing timers" );
    else
      sim->print_debug( "Resetting auto attack swing timers with an additional delay of {}", delay );

    if ( proc )
      proc->occur();
  }

  if ( main_hand_attack && main_hand_attack->execute_event )
  {
    event_t::cancel( main_hand_attack->execute_event );
    main_hand_attack->schedule_execute();
    if ( delay > timespan_t::zero() && main_hand_attack->execute_event )
    {
      main_hand_attack->execute_event->reschedule( main_hand_attack->execute_event->remains() + delay );
    }
  }

  if ( off_hand_attack && off_hand_attack->execute_event )
  {
    event_t::cancel( off_hand_attack->execute_event );
    off_hand_attack->schedule_execute();
    if ( delay > timespan_t::zero() && off_hand_attack->execute_event )
    {
      off_hand_attack->execute_event->reschedule( off_hand_attack->execute_event->remains() + delay );
    }
  }
}

/**
 * Delay the main hand (and off hand, if applicable) swing timer
 */
void player_t::delay_auto_attacks( timespan_t delay, proc_t* proc )
{
  if ( delay == timespan_t::zero() )
    return;

  bool delayed = false;

  if ( main_hand_attack && main_hand_attack->execute_event )
  {
    sim->print_debug( "Delaying MH auto attack swing timer by {} to {}", delay, main_hand_attack->execute_event->remains() + delay );
    main_hand_attack->execute_event->reschedule( main_hand_attack->execute_event->remains() + delay );
    delayed = true;
  }

  if ( off_hand_attack && off_hand_attack->execute_event )
  {
    sim->print_debug( "Delaying OH auto attack swing timer by {} to {}", delay, off_hand_attack->execute_event->remains() + delay );
    off_hand_attack->execute_event->reschedule( off_hand_attack->execute_event->remains() + delay );
    delayed = true;
  }

  if ( proc && delayed )
    proc->occur();
}

/**
* Delay ranged swing timer
*/
void player_t::delay_ranged_auto_attacks( timespan_t delay, proc_t* proc )
{
  if ( delay == timespan_t::zero() )
    return;

  bool delayed = false;

  if ( main_hand_attack && main_hand_attack->execute_event && dynamic_cast<ranged_attack_t*>(main_hand_attack) )
  {
    sim->print_debug( "Delaying ranged auto attack swing timer by {} to {}", delay, main_hand_attack->execute_event->remains() + delay );
    main_hand_attack->execute_event->reschedule( main_hand_attack->execute_event->remains() + delay );
    delayed = true;
  }

  if ( proc && delayed )
    proc->occur();
}

/**
 * Poor man's targeting support. Acquire_target is triggered by various events (see retarget_event_e) in the core.
 * Context contains the triggering entity (if relevant). Figures out a target out of all non-sleeping targets.
 * Skip "invulnerable" ones for now, anything else is fair game.
 */
void player_t::acquire_target( retarget_source event, player_t* context )
{
  // TODO: This skips certain very custom targets (such as Soul Effigy), is it a problem (since those
  // usually are handled in action target cache regeneration)?
  if ( sim->debug )
  {
    sim->out_debug.printf( "%s retargeting event=%s context=%s current_target=%s", name(),
        util::retarget_event_string( event ),
        context ? context->name() : "NONE", target ? target->name() : "NONE" );
  }

  player_t* candidate_target = nullptr;
  player_t* first_invuln_target = nullptr;

  // TODO: Fancier system
  for ( auto enemy : sim->target_non_sleeping_list )
  {
    if ( enemy->debuffs.invulnerable != nullptr && enemy->debuffs.invulnerable->check() )
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

  // Invulnerable targets are currently not in the target_non_sleeping_list, so fall back to
  // checking if the first target has the invulnerability buff up, and use that as the fallback
  auto first_target = sim->target_list.data().front();
  if ( !first_invuln_target && first_target->debuffs.invulnerable->check() )
  {
    first_invuln_target = first_target;
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
 * NOTE: Currently, the only thing that occurs during activation of an actor is the registering of various state
 * change callbacks (via the action_t::activate method) to the global actor lists. Actor pets are also activated by
 * default.
 */
void player_t::activate()
{
  // Activate all actions of the actor
  range::for_each( action_list, []( action_t* a ) { a->activate(); } );

  // .. and activate all actor pets
  range::for_each( pet_list, []( player_t* p ) { p->activate(); } );

  // .. aaand activate any special effect with an activation callback
  range::for_each( special_effects, []( special_effect_t* e ) {
    if ( e->activation_cb )
    {
      e->activation_cb();
    }
  } );
}

void player_t::deactivate()
{
  // Record total number of iterations ran for this actor. Relevant in target_error cases for data
  // analysis at the end of simulation
  collected_data.total_iterations = nth_iteration();
}

void player_t::register_combat_begin( buff_t* b )
{
  combat_begin_functions.emplace_back([ b ]( player_t* ) { b -> trigger(); } );
}

void player_t::register_precombat_begin( buff_t* b )
{
  precombat_begin_functions.emplace_back( [ b ]( player_t* ) { b->trigger(); } );
}

void player_t::register_combat_begin( action_t* a )
{
  combat_begin_functions.emplace_back([ a ]( player_t* ) { a -> execute(); } );
}

void player_t::register_combat_begin( const combat_begin_fn_t& fn )
{
  combat_begin_functions.push_back( fn );
}

void player_t::register_precombat_begin( const combat_begin_fn_t& fn )
{
  precombat_begin_functions.push_back( fn );
}

void player_t::register_combat_begin( double amount, resource_e resource, gain_t* g )
{
  combat_begin_functions.emplace_back([ amount, resource, g ]( player_t* p ) {
    p -> resource_gain( resource, amount, g );
  });
}

void player_t::register_on_demise_callback( player_t* source, std::function<void( player_t* )> fn )
{
  callbacks_on_demise.emplace_back( source, std::move( fn ) );
}

void player_t::register_on_arise_callback( player_t* source, std::function<void( void )> fn )
{
  callbacks_on_arise.emplace_back( source, std::move( fn ) );
}

void player_t::register_on_kill_callback( std::function<void( player_t* )> fn )
{
  callbacks_on_kill.emplace_back( std::move( fn ) );
}

void player_t::register_on_combat_state_callback( std::function<void( player_t*, bool )> fn )
{
  callbacks_on_combat_state.emplace_back( std::move( fn ) );
}

void player_t::register_movement_callback( std::function<void( bool )> fn )
{
  callbacks_on_movement.emplace_back( std::move( fn ) );
}

spawner::base_actor_spawner_t* player_t::find_spawner( util::string_view id ) const
{
  auto it = range::find_if( spawners, [ id ]( spawner::base_actor_spawner_t* o ) {
    return util::str_compare_ci( id, o -> name() );
  } );

  if ( it != spawners.end() )
  {
    return *it;
  }

  return nullptr;
}

int player_t::nth_iteration() const
{
  return creation_iteration == -1
          ? sim -> current_iteration
          : sim -> current_iteration - creation_iteration;
}

bool player_t::is_my_pet(const player_t* t) const
{
  return t->is_pet() && t->cast_pet()->owner == this;
}

bool player_t::is_active() const
{
  if ( sim->single_actor_batch )
  {
    if ( is_enemy() || is_pet() )
    {
      return !is_sleeping();
    }
    else
    {
      return sim->player_no_pet_list[ sim->current_index ] == this;
    }
  }
  else
  {
    return !is_sleeping();
  }
}

/**
 * Perform dynamic resource regeneration
 *
 * The forced parameter indicates whether the resource regen would
 * also occur in game at the same time.
 */
void player_t::do_dynamic_regen( bool forced )
{
  if (sim->current_time() == last_regen)
    return;

  regen(sim->current_time() - last_regen);
  last_regen = sim->current_time();

  if (dynamic_regen_pets)
  {
    for (auto& elem : active_pets)
    {
      if (elem->resource_regeneration == regen_type::DYNAMIC)
        elem->do_dynamic_regen( forced );
    }
  }
}

double player_t::get_position_distance(double m, double v) const
{
  double delta_x = this->x_position - m;
  double delta_y = this->y_position - v;
  double sqrtnum = delta_x * delta_x + delta_y * delta_y;
  return util::approx_sqrt(sqrtnum);
}

double player_t::mastery_coefficient() const
{
  return _mastery->mastery_value();
}

double player_t::get_player_distance(const player_t& target) const
{
  if (sim->distance_targeting_enabled)
  {
    return get_position_distance(target.x_position, target.y_position);
  }
  else
    return current.distance;
}

double player_t::get_ground_aoe_distance(const action_state_t& a) const
{
  return get_position_distance(a.original_x, a.original_y);
}

void player_t::init_distance_targeting()
{
  if (default_x_position == std::numeric_limits<decltype(default_x_position)>::lowest())
  {
    default_x_position = -1 * base.distance;
  }
  if (default_y_position == std::numeric_limits<decltype(default_y_position)>::lowest())
  {
    default_y_position = 0;
  }
}

void sc_format_to( const player_t& player, fmt::format_context::iterator out )
{
  fmt::format_to( out, "Player '{}'", player.name() );
}

bool player_t::is_ptr() const
{
  return maybe_ptr(dbc->ptr);
}
