#include "sim_controller.hpp"

#include "player/set_bonus.hpp"
#include "profileset.hpp"
#include "sc_enums.hpp"
#include "sim.hpp"
/*
 * TODO:
 *  - initialize contents of factory map
 *  - pass in option values to pc_t, sim_controller.cpp:21
 *
 * PROCEDURE:
 *  1) register controllers via `profileset_controller_t::register_controller(...)`,
 *     which emplaces controller(s) into the factory map for later construction.
 *  2) parent sim parses options, constructing `pcd_w_t` for each type via factory map.
 *  3) child sim iterates over parent->profileset_controller_data, constructing `pc_t`
 *     for each `pcd_w_t`
 */

/*
 * sim_controller_t:
 *  - scope: profileset
 *  - evaluates whether or not profileset should exit
 *  - reports why an exit occurs
 *  - provides options, as in many cases a custom `sim_controller_data_t` type is
 *    not required for storage reasons
 *
 * sim_controller_data_t
 *  - scope: parent sim
 *  - contains custom data, such as current "winner" for binary controllers
 *    including profileset culling
 *  - contains option values
 *
 * sim_controller_data_wrapper_t
 *  - scope: parent sim
 *  - contains mutex for thread safety when operating on sim_controller_data_t
 *  - contains sim_controller_data_t pointer
 *  - contains exit_reasons ()
 *  - contains registered option definitions
 *
 * data_wrapper_t
 *  - scope: sim_controller_data_t getter
 *  - contains scoped recursive lock on sim_controller_data_t
 *  - contains reference to sim_controller_data_t data
 *
 * notes:
 *
 * profilesets_t objects are configured and init via
 *  - sim_t::execute() > sim_t::iterate() > sim_t::init() > profilesets->initialize( sim )
 *  - profilesets_t::parse( ... ) > m_profilesets.push_back( ... )
 * profilesets_t is executed via
 *  - profilesets->iterate( sim ) > profilesets_t::generate_work( ... )
 * profileset sims/workers are constructed and executed via
 *  - SEQUENTIAL: new sim_t( ... )
 *  - PARALLEL: push_back( new worker_t( profileset_control ) )
 * profileset-creation options are stripped from profileset_control prior to construction
 *  - profilesets_t::create_sim_options, profilesets_t::initialize, filter_control
 *
 * sim where ( !parent ) parses values stored in sim_t::sim_controller_options,
 * static sim_controller_data_t::register([ key, opts ]) -> scd_w_t { key, id, scd_t { parsed_options_values... } }
 * sim where ( parent ) iterates over sim_t::sim_controller_data
 * sim_controller_data_t::register_controller() -> sc_t { id, parsed_options_values... }
 *
 * how can i copy parsed_option_values from scd_t -> sc_t without a type that
 * contains the option_values and without reparsing the options?
 *
 * use atomic incrementing id for scds
 *
 * no storage as a map, as scd id and vector position should be identical
 *
 * move implementation of reporting strictly to scd_t
 *
 * implement reporting under a static member to handle all reporting with minimal
 * implementation in html/json files
 */

// profilesets object is configured/init via sim_t
// - execute() -:> iterate() -:> init() -:> profilesets->initialize(self)
// - parse(...) -:> m.profilesets.push_back(...)
// profilesets are executed via profilesets->iterate(self) -> generate_work(...)
// profileset sims are created and executed via
// - profilesets->iterate(self) -:> generate_work(...) -:>
// - SEQUENTIAL: new sim_t
// - PARALLEL: push_back new worker_t(pset_data)
// strip profileset-specific options from opts used to initialize sims
// profilesets_t::create_sim_options, profilesets_t::initialize, filter_control

/*
  * implementation:
  *  - parent sim parses sim controller instantiation option and child options
  *    - sc_main.cpp:273 arg parse -> sc_main.cpp:287 opt parse and actor creation
  *  - parent sim constructs sim controller data for auto incrementing index id
  *  - as work is created, child option data is copied and sim controller is constructed by iterating over scd vec, passing along id as well
  *
  * NOTES:
  *  - implement sim opts as function options, which induces opt order dependency, no preventing option parse without filtering profileset sim control
  *  - implement sim opts as map list and execute processing in setup or init if sim is parent
  * OTHER CHANGES:
  *  - move exit_reasons and options (?) from data wrapper to data
  *  - provide getters and setters for exit_reasons and options as appropriate
  *  - no key-value pairs, everything should be a vec
  *  - initialize controllers and scd with atomic id generated via scd
  *  - rename to profileset_controller, as they control profilesets not sims per se
  */

std::unordered_map<std::string, profileset_controller_t::factory_fn_pair_t> profileset_controller_t::factory;
std::atomic_uint profileset_controller_data_wrapper_t::id_generator;

profileset_controller_data_t::profileset_controller_data_t( std::string_view options )
{
}

profileset_controller_data_wrapper_t::profileset_controller_data_wrapper_t( std::string key, std::string_view options )
  : mutex(), id( id_generator++ ), key( key )
{
  if ( const auto& value = profileset_controller_t::factory.find( key );
       value != profileset_controller_t::factory.end() )
    data = value->second.second( options );
  assert( data );
}

void profileset_controller_data_wrapper_t::construct_controller( sim_t* sim )
{
  if ( const auto& value = profileset_controller_t::factory.find( key );
       value != profileset_controller_t::factory.end() )
  {
    sim->profileset_controller.emplace_back( value->second.first( sim, id ) );
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

const std::string profileset_controller_t::call_point_string( call_point_e call_point )
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

void profileset_controller_t::evaluate( sim_t* sim, call_point_e call_point )
{
  if ( !sim->profileset_enabled || !sim->parent )
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

void profileset_controller_t::html_report( const sim_t& sim, std::ostream& out )
{
  if ( sim.profileset_controller_data.empty() )
    return;

  out << "<h3 class=\"toggle\">Profileset Sim Control</h3>\n";
  out << "<div class=\"toggle-content hide\">\n";

  out << "<div class=\"note\" style=\"margin:6px 0;\"><strong>Sim Controllers</strong><ul>\n";
  for ( const auto& controller_data_wrapper : sim.profileset_controller_data )
    if ( const auto& controller_data = controller_data_wrapper.data; controller_data )
      controller_data->report_html_options( out );
  out << "</ul></div>\n";

  // report source, location, and reason of interrupt for
  // all registered profileset profileset controllers
  bool has_culled_profileset = range::any_of( sim.profileset_controller_data,
                                              []( const auto& entry ) { return entry.data->exit_reasons.size(); } );

  if ( has_culled_profileset )
  {
    out << "<div class=\"note\" style=\"margin:6px 0;\"><strong>Interrupted Profilesets</strong><ul>\n";
    for ( const auto& controller_data_wrapper : sim.profileset_controller_data )
      if ( const auto& controller_data = controller_data_wrapper.data;
           controller_data && controller_data->exit_reasons.size() )
        controller_data->report_html_profileset( out );
    out << "</ul></div>\n";
  }
  out << "</div>";
}

profileset_controller_t::profileset_controller_t( sim_t* sim, unsigned int id )
  : parent( sim->parent ), sim( sim ), id( id )
{
  assert( sim && sim->parent );
}

const std::string profileset_controller_t::message( call_point_e call_point )
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

void profileset_controller_t::set_exit_reason( exit_reason_t&& exit_reason )
{
  auto& pcd = parent->profileset_controller_data;
  assert( pcd.size() > id );
  pcd[ id ].data->exit_reasons.emplace_back( std::move( exit_reason ) );
}

bool min_player_stat_t::evaluate_post_init()
{
  return true;
}

const std::string min_player_stat_t::reason() const
{
  return fmt::format( "player {} does not exceed {} rating for {}", target_player->name(), min_rating,
                      util::stat_type_string( rating ) );
}

bool tier_set_count_t::evaluate_post_init()
{
  tier  = TWW2;
  count = B2;
  return target_player->sets->has_set_bonus( target_player->specialization(), tier, count );
}

const std::string tier_set_count_t::reason() const
{
  // no to string for set bonus tier or count...
  // that should definitely exist :)
  return fmt::format( "player {} does not have tier {} {} active", target_player->name(), static_cast<int>( tier ),
                      static_cast<int>( count ) );
}

// void tier_set_count_t::create_options()
// {
//   add_option( opt_int( "test", test ) );
// }

// void sim_controller_t::add_option( std::unique_ptr<option_t>&& option )
// {
//   auto& scd = sim->parent->sim_controller_data.at( name() );
//   std::scoped_lock<std::recursive_mutex> L( scd.mutex );

//   scd.options.emplace_back( std::move( option ) );
// }

// void sim_controller_data_wrapper_t::report_json_profileset( js::JsonOutput& output ) const
// {
//   for ( const exit_reason_t& exit_reason : exit_reasons )
//   {
//     output[ "interrupted_by" ] = exit_reason.profileset_name;
//     output[ "exit_point" ]     = sim_controller_t::call_point_string( exit_reason.exit_point );
//     output[ "exit_reason" ]    = exit_reason.exit_reason;
//   }
// }

// void sim_controller_data_wrapper_t::report_json_options( js::JsonOutput& ) const
// {
//   // TODO: implement opt parsing and automatic generation of report json from opts
// }

// void sim_controller_data_wrapper_t::report_html_profileset( std::ostream& output ) const
// {
//   for ( const exit_reason_t& exit_reason : exit_reasons )
//     output << "<li>"
//           << util::encode_html( exit_reason.profileset_name ) << " "
//           << util::encode_html( sim_controller_t::call_point_string( exit_reason.exit_point ) ) << " "
//           << util::encode_html( exit_reason.exit_reason )
//           << "</li>\n";
// }

// void sim_controller_data_wrapper_t::report_html_options( std::ostream& output ) const
// {
//   output << "<li>"
//          << util::encode_html( key ) << " ";
//   for ( const auto& opt : options )
//     output << fmt::format( "{} ", opt );
//   output << "</li>\n";
// }
