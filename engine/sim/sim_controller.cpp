#include "sim_controller.hpp"

#include "sc_enums.hpp"
#include "player/set_bonus.hpp"
#include "profileset.hpp"
#include "sim.hpp"

sim_controller_data_wrapper_t::sim_controller_data_wrapper_t() : mutex(), data(), exit_reasons()
{
}

sim_controller_data_wrapper_t::sim_controller_data_wrapper_t( std::unique_ptr<sim_controller_data_t>&& data )
  : mutex(), data( std::move( data ) ), exit_reasons()
{
}

sim_controller_data_t::sim_controller_data_t()
{
}

sim_controller_data_t::sim_controller_data_t( sim_controller_data_t& )
{
}

sim_controller_t::sim_controller_t( sim_t* sim )
  : parent( sim->parent ), sim( sim )
{
  assert( sim );
  assert( sim->parent );
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

  typedef std::unique_ptr<sim_controller_t> sc_ptr_t;
  std::function<bool( sc_ptr_t& )> cb;
  switch ( call_point )
  {
    case POST_INIT:
      cb = []( sc_ptr_t& sc ) { return !sc->evaluate_post_init(); };
      break;
    case POST_ITER:
      cb = []( sc_ptr_t& sc ) { return !sc->evaluate_post_iter(); };
      break;
    default:
      assert( false );
      break;
  }
  auto sc = range::find_if( sim->sim_controllers, cb );
  if ( sc == sim->sim_controllers.end() )
    return;

  auto* controller = sc->get();
  assert( controller->sim == sim );
  assert( controller->parent == sim->parent );

  auto& scd = sim->parent->sim_controller_data.at( controller->name() );
  std::scoped_lock<std::recursive_mutex> L( scd.mutex );

  scd.exit_reasons.emplace_back( sim->parent->profilesets->current_profileset_name(), call_point,
                                 controller->reason() );

  sim->canceled = true;
  sim->error( error_level_e::TRIVIAL, "{}", controller->message( call_point ) );
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

void sim_controller_t::add_option( std::unique_ptr<option_t> option )
{
}

void sim_controller_data_wrapper_t::report_json_profileset( js::JsonOutput& output ) const
{
  for ( const exit_reason_t& exit_reason : exit_reasons )
  {
    output[ "interrupted_by" ] = exit_reason.profileset_name;
    output[ "exit_point" ]     = sim_controller_t::call_point_string( exit_reason.exit_point );
    output[ "exit_reason" ]    = exit_reason.exit_reason;
  }
}

void sim_controller_data_wrapper_t::report_json_options( js::JsonOutput& ) const
{
  // TODO: implement opt parsing and automatic generation of report json from opts
}

void sim_controller_data_wrapper_t::report_html_profileset( std::ostream& output ) const
{
  for ( const exit_reason_t& exit_reason : exit_reasons )
    output << "<li>"
          << util::encode_html( exit_reason.profileset_name ) << " "
          << util::encode_html( sim_controller_t::call_point_string( exit_reason.exit_point ) ) << " "
          << util::encode_html( exit_reason.exit_reason )
          << "</li>";
}

void sim_controller_data_wrapper_t::report_html_options( std::ostream& ) const
{}

min_player_stat_t::min_player_stat_t( sim_t* sim, player_t* target_player, stat_e rating, double amount )
  : sim_controller_t( sim ), target_player( target_player ), rating( rating ), min_rating( amount )
{
}

bool min_player_stat_t::evaluate_post_init()
{
  return true;
}

const std::string min_player_stat_t::reason() const
{
  return fmt::format( "player {} does not exceed {} rating for {}", target_player->name(),
                      min_rating, util::stat_type_string( rating ) );
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
