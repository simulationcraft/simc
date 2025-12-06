#include "sim_controller.hpp"

#include "dbc/dbc.hpp"
#include "dbc/item_set_bonus.hpp"
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
 *  - as work is created, child option data is copied and sim controller is constructed by iterating over scd vec,
 * passing along id as well
 *
 * NOTES:
 *  - implement sim opts as function options, which induces opt order dependency, no preventing option parse without
 * filtering profileset sim control
 *  - implement sim opts as map list and execute processing in setup or init if sim is parent
 * OTHER CHANGES:
 *  - move exit_reasons and options (?) from data wrapper to data
 *  - provide getters and setters for exit_reasons and options as appropriate
 *  - no key-value pairs, everything should be a vec
 *  - initialize controllers and scd with atomic id generated via scd
 *  - rename to profileset_controller, as they control profilesets not sims per se
 */

/*
 * Global profileset_controller_t factories. If you create a `profileset_controller_t`
 * subtype specific to some context, have that context register it early in sim init
 * via the static function `profileset_controller_t::register_controller`.
 */
std::unordered_map<std::string, profileset_controller_t::factory_fn_pair_t> profileset_controller_t::factory = {
    { "set_bonus_enabled",
      { []( sim_t* sim, unsigned int id ) { return std::make_unique<set_bonus_enabled_t>( sim, id ); },
        []( std::string_view key, std::string_view options ) {
          return std::make_unique<set_bonus_enabled_t::data_t>( key, options );
        } } } };

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
  std::string msg =
      fmt::format( "Profileset {} was canceled by {} after {}", parent->profilesets->current_profileset_name(), name(),
                   profileset_controller::call_point_string( call_point ) );
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

namespace
{
// how to do this with reference wrapper instead of template?
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

void report_json()
{
}
}  // namespace profileset_controller

bool min_player_stat_t::evaluate_post_init()
{
  return true;
}

const std::string min_player_stat_t::reason() const
{
  return fmt::format( "player {} does not exceed {} rating for {}", target_player->name(), min_rating,
                      util::stat_type_string( rating ) );
}

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
