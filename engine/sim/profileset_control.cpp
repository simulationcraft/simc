#include "profileset_control.hpp"

#include "dbc/dbc.hpp"
#include "dbc/item_set_bonus.hpp"
#include "interfaces/sc_js.hpp"
#include "player/set_bonus.hpp"
#include "profileset.hpp"
#include "sc_enums.hpp"
#include "sim.hpp"

std::unordered_map<std::string, profileset_controller_t::factory_fn_pair_t> profileset_controller_t::factory = {
    { "set_bonus_enabled", profileset_controller::create_fn_pair<set_bonus_enabled_t>() } };

std::atomic_uint profileset_controller_data_wrapper_t::id_generator;

profileset_controller_data_t::profileset_controller_data_t( std::string_view key, std::string_view options )
  : key( key ), options( options )
{
}

void profileset_controller_data_t::report_html_options( std::ostream& output ) const
{
  output << "<tr>"
         << "<td>" << util::encode_html( key ) << "</td>"
         << "<td class=\"center\">" << exit_reasons.size() << "</td>"
         << "<td>" << util::encode_html( options ) << "</td>"
         << "</tr>\n";
}

void profileset_controller_data_t::report_html_profileset( std::ostream& output ) const
{
  bool first = true;
  output << fmt::format( "<tr><td rowspan=\"{}\" class=\"dark\">{}</td>", exit_reasons.size(),
                         util::encode_html( key ) );
  for ( const auto& [ name, call_point, reason ] : exit_reasons )
  {
    if ( !first )
      output << "<tr>";
    output << "<td class=\"center\">" << util::encode_html( name ) << "</td>"
           << "<td>" << util::encode_html( profileset_controller::call_point_string( call_point ) ) << "</td>"
           << "<td>" << util::encode_html( reason ) << "</td>"
           << "</tr>\n";
    first = false;
  }
}

void profileset_controller_data_t::report_json_options( js::JsonOutput& root ) const
{
  auto output                            = root.add();
  auto splits                            = util::string_split( options, "," );
  output[ "profileset_controller_name" ] = key;
  for ( const auto& split : splits )
  {
    auto subsplit = util::string_split( split, "=" );
    assert( subsplit.size() == 2 );
    output[ subsplit[ 0 ] ] = subsplit[ 1 ];
  }
}

void profileset_controller_data_t::report_json_profileset( js::JsonOutput& root ) const
{
  for ( const auto& [ name, call_point, reason ] : exit_reasons )
  {
    auto output                 = root.add();
    output[ "profileset_name" ] = name;
    output[ "interrupted_by" ]  = key;
    output[ "exit_point" ]      = profileset_controller::call_point_string( call_point );
    output[ "exit_reason" ]     = reason;
  }
}

profileset_controller_data_wrapper_t::profileset_controller_data_wrapper_t( std::string key, std::string_view options )
  : mutex(), id( id_generator++ ), key( key ), options( options )
{
  if ( const auto& value = profileset_controller_t::factory.find( key );
       value != profileset_controller_t::factory.end() )
    data = value->second.second( key, options );
  assert( data );
}

void profileset_controller_data_wrapper_t::construct_controller( sim_t* sim )
{
  if ( const auto& value = profileset_controller_t::factory.find( key );
       value != profileset_controller_t::factory.end() )
  {
    auto controller = value->second.first( sim, id );
    controller->create_options();
    opts::parse( sim, "profileset_controller", controller->options, options,
                 [ this, &sim ]( opts::parse_status status, util::string_view name, util::string_view value ) {
                   // Fail parsing if strict parsing is used and the option is not found
                   if ( sim->strict_parsing && status == opts::parse_status::NOT_FOUND )
                     return opts::parse_status::FAILURE;
                   // .. otherwise, just warn that there's an unknown option
                   if ( status == opts::parse_status::NOT_FOUND )
                     sim->error(
                         "Warning: profileset controller '{}' provided unknown option '{}' with value '{}', ignoring.",
                         key, name, value );
                   return status;
                 } );
    sim->profileset_controller.emplace_back( std::move( controller ) );
    return;
  }
  assert( false && "No factory fn for key found." );
}

bool profileset_controller_t::register_controller( std::string key, profileset_controller_t::factory_fn_pair_t&& value )
{
  return factory.try_emplace( key, std::move( value ) ).second;
}

bool profileset_controller_t::controller_exists( std::string key )
{
  return factory.find( key ) != factory.end();
}

void profileset_controller_t::evaluate( sim_t* sim, call_point_e call_point )
{
  if ( !sim->profileset_enabled || !sim->parent || sim->profileset_controller.empty() )
    return;

  std::function<bool( std::unique_ptr<profileset_controller_t>& )> cb;
  switch ( call_point )
  {
    case POST_INIT:
      cb = []( std::unique_ptr<profileset_controller_t>& sc ) { return !sc->evaluate_post_init(); };
      break;
    case POST_ITER:
      cb = []( std::unique_ptr<profileset_controller_t>& sc ) { return !sc->evaluate_post_iter(); };
      break;
    default:
      assert( false );
      break;
  }
  auto pc = range::find_if( sim->profileset_controller, cb );
  if ( pc == sim->profileset_controller.end() )
    return;

  auto controller = pc->get();
  assert( controller->sim == sim );
  assert( controller->parent == sim->parent );

  controller->set_exit_reason(
      { sim->parent->profilesets->current_profileset_name(), call_point, controller->reason() } );

  sim->canceled = true;
  sim->error( error_level_e::TRIVIAL, "{}", controller->message( call_point ) );
  sim->interrupt();
}

void profileset_controller_t::add_option( std::unique_ptr<option_t>&& option )
{
  options.emplace_back( std::move( option ) );
}

profileset_controller_t::profileset_controller_t( sim_t* sim, unsigned int id )
  : parent( sim->parent ), sim( sim ), id( id )
{
  assert( sim && sim->parent );
}

const std::string profileset_controller_t::message( call_point_e call_point )
{
  std::string msg = fmt::format( "Profileset {} was canceled by Profileset Controller {} after {}",
                                 parent->profilesets->current_profileset_name(), name(),
                                 profileset_controller::call_point_string( call_point ) );
  if ( call_point == POST_ITER )
    msg += std::to_string( sim->current_iteration );

  if ( const auto r = reason(); !r.empty() )
    msg += fmt::format( " because {}.", r );
  else
    msg += ".";

  return msg;
}

void profileset_controller_t::set_exit_reason( exit_reason_t&& exit_reason )
{
  auto& pcd = parent->profileset_controller_data;
  assert( pcd.size() > id );
  pcd[ id ].data->exit_reasons.emplace_back( std::move( exit_reason ) );
}

namespace
{
template <typename T>
void report_html_table(
    std::ostream& out, std::vector<std::string> keys, const std::deque<profileset_controller_data_wrapper_t>& data,
    T ref, std::function<bool( const std::unique_ptr<profileset_controller_data_t>& )> cond = []( const auto& ) {
      return true;
    } )
{
  out << "<table class=\"details nowrap\" style=\"width:min-content\">\n"
      << "<tr>";
  bool first = true;
  for ( const auto& key : keys )
  {
    out << fmt::format( "<th class=\"small {}\">", first ? "left" : "center" ) << key << "</th>";
    first = false;
  }
  out << "</tr>\n";
  for ( const auto& datum_wrapper : data )
    if ( const auto& datum = datum_wrapper.data; datum && cond( datum ) )
      std::invoke( ref, datum, out );
  out << "</table>";
}
}  // namespace

namespace profileset_controller
{
const std::string call_point_string( call_point_e call_point )
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

void report_html( const sim_t& sim, std::ostream& out )
{
  if ( sim.profileset_controller_data.empty() )
    return;

  out << "<h3 class=\"toggle\">Profileset Sim Control</h3>\n";
  out << "<div class=\"toggle-content hide\">\n";

  out << "<div class=\"note\" style=\"margin:6px 0;\"><strong>Profileset Controllers</strong>\n";
  report_html_table( out, { "Type", "Count", "Options" }, sim.profileset_controller_data,
                     &profileset_controller_data_t::report_html_options );
  out << "</div>\n";

  // report source, location, and reason of interrupt for
  // all registered profileset profileset controllers
  bool has_culled_profileset = range::any_of( sim.profileset_controller_data,
                                              []( const auto& datum ) { return datum.data->exit_reasons.size(); } );

  if ( has_culled_profileset )
  {
    out << "<div class=\"note\" style=\"margin:6px 0;\"><strong>Cancelled Profilesets</strong>\n";
    report_html_table( out, { "Type", "Profileset Name", "Cancellation Point", "Reason" },
                       sim.profileset_controller_data, &profileset_controller_data_t::report_html_profileset,
                       []( const auto& datum ) { return datum->exit_reasons.size(); } );
    out << "</div>\n";
  }
  out << "</div>";
}

void report_json( const sim_t& sim, js::JsonOutput& output )
{
  if ( sim.profileset_controller_data.empty() )
    return;

  auto root = output[ "profileset_controller" ];

  auto exits = root[ "cancelled_profilesets" ].make_array();
  for ( const auto& datum_wrapper : sim.profileset_controller_data )
    if ( const auto& datum = datum_wrapper.data; datum )
      datum->report_json_profileset( exits );

  auto controllers = root[ "enabled_controllers" ].make_array();
  for ( const auto& datum_wrapper : sim.profileset_controller_data )
    if ( const auto& datum = datum_wrapper.data; datum )
      datum->report_json_options( controllers );
}
}  // namespace profileset_controller

bool set_bonus_enabled_t::evaluate_post_init()
{
  if ( target_player )
    return target_player->sets->has_set_bonus( target_player->specialization(), tier, count );
  return true;
}

const std::string set_bonus_enabled_t::reason() const
{
  // no to string for set bonus tier or count...
  // that should definitely exist :)
  auto set_bonuses = item_set_bonus_t::data( target_player ? target_player->dbc->ptr : false );
  std::string tier_name{};
  for ( const auto& set_bonus : set_bonuses )
    if ( set_bonus.enum_id == static_cast<unsigned int>( tier ) )
      tier_name = set_bonus.tier;
  return fmt::format( "player {} does not have set {} {}pc active", target_player->name(), tier_name,
                      static_cast<int>( count + 1 ) );
}

void set_bonus_enabled_t::create_options()
{
  add_option( opt_func( "tier", [ this ]( sim_t*, util::string_view, util::string_view value ) {
    auto set_bonuses = item_set_bonus_t::data( target_player ? target_player->dbc->ptr : false );
    for ( const auto& set_bonus : set_bonuses )
    {
      if ( util::str_compare_ci( set_bonus.tier, value ) )
      {
        this->tier = static_cast<set_bonus_type_e>( set_bonus.enum_id );
        return true;
      }
    }
    return false;
  } ) );
  add_option( opt_func( "pc", [ this ]( sim_t*, util::string_view, util::string_view value ) {
    auto bonus_value = util::to_unsigned( value );
    if ( bonus_value > B_MAX )
      return false;
    this->count = static_cast<set_bonus_e>( bonus_value - 1 );
    return true;
  } ) );
  add_option( opt_func( "player", [ this ]( sim_t* sim, util::string_view, util::string_view value ) {
    for ( auto& player : sim->player_list )
    {
      if ( util::str_compare_ci( player->name(), value ) )
      {
        this->target_player = player;
        return true;
      }
    }
    return false;
  } ) );
}
