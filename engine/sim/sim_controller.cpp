#include "sim_controller.hpp"

#include "player/set_bonus.hpp"
#include "profileset.hpp"
#include "sim.hpp"

sim_controller_data_wrapper_t::sim_controller_data_wrapper_t() : mutex(), data( nullptr )
{
}

sim_controller_data_wrapper_t::sim_controller_data_wrapper_t( std::shared_ptr<sim_controller_data_t> data )
  : mutex(), data( data )
{
}

sim_controller_data_t::sim_controller_data_t()
{
}

sim_controller_data_t::sim_controller_data_t( sim_controller_data_t& )
{
}

sim_controller_t::sim_controller_t( sim_t* sim )
  : parent( sim->parent ), exit_point( sim_controller_t::NONE ), exit_reason( {} ), sim( sim )
{
}

const std::string sim_controller_t::call_point_string( call_point_e call_point )
{
  switch ( call_point )
  {
    case POST_INIT:
      return "simulation initialization";
    case POST_ITER:
      return "iteration";
    default:
      assert( false );
      return "no matching call point";
  }
}

void sim_controller_t::evaluate( sim_t* sim, call_point_e call_point )
{
  if ( !sim->profileset_enabled || !sim->parent )
    return;

  typedef std::shared_ptr<sim_controller_t> iter_t;
  std::function<bool( iter_t& )> cb;
  switch ( call_point )
  {
    case POST_INIT:
      cb = []( iter_t& sc ) { return !sc->evaluate_post_init(); };
      break;
    case POST_ITER:
      cb = []( iter_t& sc ) { return !sc->evaluate_post_iter(); };
      break;
    default:
      assert( false );
      break;
  }
  auto sc = range::find_if( sim->sim_controllers, cb );
  if ( sc == sim->sim_controllers.end() )
    return;

  std::shared_ptr<sim_controller_t>& controller = *sc;
  assert( controller->sim == sim );
  assert( controller->parent == sim->parent );

  controller->exit_point  = call_point;
  controller->exit_reason = controller->reason();

  sim->canceled = true;
  sim->error( controller->message( call_point ) );
  sim->interrupt();
}

const std::string sim_controller_t::message( call_point_e call_point )
{
  std::string msg =
      fmt::format( "Profileset {} was canceled by {} after {}", parent->profilesets->current_profileset_name(), name(),
                   call_point_string( call_point ) );
  if ( call_point == POST_ITER )
    msg += std::to_string( sim->current_iteration );

  if ( const std::string r = reason(); r != "" )
    msg += fmt::format( " because {}.", r );
  else
    msg += ".";

  return msg;
}

min_player_stat_t::min_player_stat_t( sim_t* sim, player_t* target_player, stat_e rating, double amount )
  : sim_controller_t( sim ), target_player( target_player ), rating( rating ), min_rating( amount )
{
}

bool min_player_stat_t::evaluate_post_init()
{
  return target_player->get_stat_value( rating ) >= min_rating;
}

tier_set_count_t::tier_set_count_t( sim_t* sim, player_t* target_player, set_bonus_type_e tier, set_bonus_e count )
  : sim_controller_t( sim ), target_player( target_player ), tier( tier ), count( count )
{
}

bool tier_set_count_t::evaluate_post_init()
{
  return target_player->sets->has_set_bonus( target_player->specialization(), tier, count );
}

const std::string tier_set_count_t::reason() const
{
  // no to string for set bonus tier or count...
  // that should definitely exist :)
  return fmt::format( "player {} does not have tier {} {} active", target_player->name(), static_cast<int>( tier ),
                      static_cast<int>( count ) );
}
