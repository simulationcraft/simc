// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "report/sc_highchart.hpp"
#include "sc_profileset.hpp"
#ifdef SC_WINDOWS
#include <direct.h>
#endif

namespace { // UNNAMED NAMESPACE ============================================

// Comparator for iteration data entry sorting (see analyze_iteration_data)
bool iteration_data_cmp( const iteration_data_entry_t& a,
                         const iteration_data_entry_t& b )
{
  if ( a.metric > b.metric )
  {
    return true;
  }
  else if ( a.metric == b.metric )
  {
    if ( a.seed != b.seed )
    {
      return a.seed > b.seed;
    }
    else
    {
      return &a > &b;
    }
  }

  return false;
}

bool iteration_data_cmp_r( const iteration_data_entry_t& a,
                          const iteration_data_entry_t& b )
{
  if ( a.metric > b.metric )
  {
    return false;
  }
  else if ( a.metric == b.metric )
  {
    if ( a.seed != b.seed )
    {
      return a.seed > b.seed;
    }
    else
    {
      return &a > &b;
    }
  }

  return true;
}

struct seed_predicate_t
{
  uint64_t seed;

  seed_predicate_t( uint64_t seed_ ) : seed( seed_ )
  { }

  bool operator()( const iteration_data_entry_t& e ) const
  { return e.seed == seed; }
};

// parse_debug_seed =========================================================

bool parse_debug_seed( sim_t* sim, const std::string&, const std::string& value )
{
  auto split = util::string_split( value, ":/," );

  for ( const auto& seed_str : split )
  {

    uint64_t seed = std::stoll( seed_str );
    sim -> debug_seed.push_back( seed );
  }

  range::sort( sim -> debug_seed );

  if (sim->debug_seed.empty())
  {
    throw std::invalid_argument("Could not parse any debug seeds.");
  }

  return true;
}

// parse_ptr ================================================================

bool parse_ptr( sim_t*             sim,
                       const std::string& name,
                       const std::string& value )
{
  if ( name != "ptr" ) return false;

  if ( SC_USE_PTR )
    sim -> dbc.ptr = std::stoi( value ) != 0;
  else
    sim -> errorf( "SimulationCraft has not been built with PTR data.  The 'ptr=' option is ignored.\n" );

  return true;
}

// parse_active =============================================================

bool parse_active( sim_t*             sim,
                          const std::string& name,
                          const std::string& value )
{
  if ( name != "active" ) return false;

  if ( value == "owner" )
  {
    if ( sim -> active_player -> is_pet() )
    {
      sim -> active_player = sim -> active_player -> cast_pet() -> owner;
    }
    else
    {
      throw std::invalid_argument("Active Player is not a pet, cannot refer to 'owner'" );
    }
  }
  else if ( value == "none" || value == "0" )
  {
    sim -> active_player = nullptr;
  }
  else
  {
    if ( sim -> active_player )
    {
      sim -> active_player = sim -> active_player -> find_pet( value );
    }
    if ( ! sim -> active_player )
    {
      sim -> active_player = sim -> find_player( value );
    }
    if ( ! sim -> active_player )
    {
      throw std::invalid_argument(fmt::format("Unable to find player '{}' to make active.", value ));
    }
  }

  return true;
}

// parse_optimal_raid =======================================================

bool parse_optimal_raid( sim_t*             sim,
                                const std::string& name,
                                const std::string& value )
{
  if ( name != "optimal_raid" ) return false;

  sim -> use_optimal_buffs_and_debuffs( std::stoi( value ) );

  return true;
}

// parse_player =============================================================

bool parse_player( sim_t*             sim,
                          const std::string& name,
                          const std::string& value )
{

  if ( name[ 0 ] >= '0' && name[ 0 ] <= '9' )
  {
    throw std::invalid_argument(fmt::format("Invalid actor name '{}' - name cannot start with a digit.", name ));
  }

  if ( name == "pet" || name == "guardian" )
  {
    std::string::size_type cut_pt = value.find( ',' );
    std::string pet_type( value, 0, cut_pt );

    std::string pet_name;
    if ( cut_pt != value.npos )
      pet_name.assign( value, cut_pt + 1, value.npos );
    else
      pet_name = value;

    if ( ! sim -> active_player )
    {
      throw std::invalid_argument(fmt::format("Pet profile ({}) needs a player preceding it.", name ));
    }

    sim -> active_player = sim -> active_player -> create_pet( pet_name, pet_type );
  }
  else if ( name == "copy" )
  {
    std::string::size_type cut_pt = value.find( ',' );
    std::string player_name( value, 0, cut_pt );

    player_t* source;
    if ( cut_pt == value.npos )
    {
      source = sim -> active_player;
      if (!source)
      {
        throw std::invalid_argument("No active player as source for profile copy available - format is copy=target[,source]" );
      }
    }
    else
    {
      std::string source_name = value.substr( cut_pt + 1 );
      source = sim -> find_player( source_name );
      if (!source)
      {
        throw std::invalid_argument(fmt::format(
            "Invalid source for profile copy - format is copy=target[,source]: Cannot find player '{}'.", source_name ));
      }
    }

    assert(source);

    sim -> active_player = module_t::get( source -> type ) -> create_player( sim, player_name );

    if ( sim -> active_player != nullptr )
      sim -> active_player -> copy_from ( source );
  }
  else
  {
    sim -> active_player = nullptr;
    const module_t* module = module_t::get( name );

    if ( ! module || ! module -> valid() )
    {
      throw std::invalid_argument(fmt::format("Module for class '{}' is currently not available.", name ));
    }
    else
    {
      sim -> active_player = module -> create_player( sim, value );

      if ( ! sim -> active_player )
      {
        throw std::invalid_argument(fmt::format("Unable to create player '{}' with class '{}'.",
                       value, name ));
      }
    }
  }

  // Create options for player
  if ( sim -> active_player )
    sim -> active_player -> create_options();

  return sim -> active_player != nullptr;
}

// parse_proxy ==============================================================

bool parse_proxy( sim_t* , const std::string& /* name */, const std::string& value )
{

  std::vector<std::string> splits = util::string_split( value, "," );

  if ( splits.size() != 3 )
  {
    throw std::invalid_argument("Expected format is: proxy=type,host,port\n" );
  }

  if (splits[ 0 ] != "http")
  {
    throw std::invalid_argument(fmt::format("Proxy invalid type '{}'", splits[ 0 ] ));
  }

  unsigned port = std::stoul( splits[ 2 ] );
  if ( port <= 0 || port >= 65536 )
  {
    throw std::invalid_argument(fmt::format("Invalid proxy port, outside range", port ));
  }

  http::set_proxy( splits[ 0 ], splits[ 1 ], port );
  return true;
}

// parse_cache ==============================================================

bool parse_cache( sim_t*             /* sim */,
                         const std::string& name,
                         const std::string& value )
{
  if ( name == "cache_players" )
  {
    if ( value == "1" ) cache::players( cache::ANY );
    else if ( value == "0" ) cache::players( cache::CURRENT );
    else if ( util::str_compare_ci( value, "only" ) ) cache::players( cache::ONLY );
    else return false;

    return true;
  }

  else if ( name == "cache_items" )
  {
    if ( value == "1" ) cache::items( cache::ANY );
    else if ( value == "0" ) cache::items( cache::CURRENT );
    else if ( util::str_compare_ci( value, "only" ) ) cache::items( cache::ONLY );
    else return false;

    return true;
  }

  else
    return false;
}

// parse_talent_format ======================================================

bool parse_talent_format( sim_t*             sim,
                          const std::string& name,
                          const std::string& value )
{
  if ( name != "talent_format" ) return false;

  if ( util::str_compare_ci( value, "unchanged" ) )
  {
    sim -> talent_format = TALENT_FORMAT_UNCHANGED;
  }
  else if ( util::str_compare_ci( value, "armory" ) )
  {
    sim -> talent_format = TALENT_FORMAT_ARMORY;
  }
  else if ( util::str_compare_ci( value, "wowhead" ) )
  {
    sim -> talent_format = TALENT_FORMAT_WOWHEAD;
  }
  else if ( util::str_compare_ci( value, "numbers" ) || util::str_compare_ci( value, "default" ) )
  {
    sim -> talent_format = TALENT_FORMAT_NUMBERS;
  }

  return true;
}


// parse_armory =============================================================

class names_and_options_t
{
private:
  static bool is_valid_region( const std::string& s )
  {
    static const std::vector<std::string> REGIONS { "us", "eu", "kr", "tw", "cn" };
    return s.size() == 2 && range::find( REGIONS, s ) != REGIONS.end();
  }

public:
  typedef std::runtime_error error;
  struct option_error : public error {
    explicit option_error(const std::string& _Message)
      : runtime_error(_Message)
    {
    }

    explicit option_error(const char* _Message)
      : runtime_error(_Message)
    {
    }
  };

  std::vector<std::string> names;
  std::string region;
  std::string server;
  cache::behavior_e cache;

  names_and_options_t( sim_t* sim, const std::string& context,
                       std::vector<std::unique_ptr<option_t>> client_options, const std::string& input )
  {
    int use_cache = 0;

    std::vector<std::unique_ptr<option_t>> options;
    options = std::move(client_options);
    //options.insert( options.begin(), client_options.begin(), client_options.end() );
    options.push_back( opt_string( "region", region ) );
    options.push_back( opt_string( "server", server ) );
    options.push_back( opt_bool( "cache", use_cache ) );

    names = util::string_split( input, "," );

    std::vector<std::string> names2 = names;
    size_t count = 0;
    for ( size_t i = 0; i < names.size(); ++i )
    {
      if ( names[ i ].find( '=' ) != std::string::npos )
      {
        opts::parse( sim, context, options, names[ i ] );
      }
      else
      {
        names2[ count++ ] = names[ i ];
      }
    }

    names2.resize( count );
    names = names2;

    if ( region.empty() )
    {
      if ( names.size() > 2 )
      {
        region = names[ 0 ];
        // Lowercase regions, always
        std::transform( region.begin(), region.end(), region.begin(), tolower );
        server = names[ 1 ];
        names.erase( names.begin(), names.begin() + 2 );
      }
      else
      {
        region = sim -> default_region_str;
      }
    }
    if (!is_valid_region( region ))
    {
      throw std::invalid_argument(
          fmt::format( "Invalid region '{}', available regions are: us, eu, kr, tw, cn.", region ) );
    }

    if ( server.empty() )
    {
      if ( names.size() > 1 )
      {
        server = names[ 0 ];
        names.erase( names.begin(), names.begin() + 1 );
      }
      else
      {
        server = sim -> default_server_str;
      }
    }

    cache = use_cache ? cache::ANY : cache::players();
  }
};

bool parse_armory( sim_t*             sim,
                   const std::string& name,
                   const std::string& value )
{
    std::string spec = "active";

    std::vector<std::unique_ptr<option_t>> options;
    options.push_back( opt_string( "spec", spec ) );

    names_and_options_t stuff( sim, name, std::move(options), value );

    for ( size_t i = 0; i < stuff.names.size(); ++i )
    {
      // Format: name[|spec]
      std::string& player_name = stuff.names[ i ];
      std::string description = spec;

      if ( player_name[ 0 ] == '!' )
      {
        player_name.erase( 0, 1 );
        description = "inactive";
        sim -> errorf( "Warning: use of \"!%s\" to indicate a player's inactive talent spec is deprecated. Use \"%s,spec=inactive\" instead.\n",
                       player_name.c_str(), player_name.c_str() );
      }

      std::string::size_type pos = player_name.find( '|' );
      if ( pos != player_name.npos )
      {
        description.assign( player_name, pos + 1, player_name.npos );
        player_name.erase( pos );
      }

      player_t* p;
      try
        {
        if ( name == "local_json" )
          p = bcp_api::from_local_json( sim, player_name, stuff.server, description );
        else
          p = bcp_api::download_player( sim, stuff.region, stuff.server,
                                        player_name, description, stuff.cache );

        sim -> active_player = p;
        if ( ! p )
          throw std::runtime_error("Could not download player.");
        }
      catch (const std::exception& e )
      {
        std::throw_with_nested(std::runtime_error("BCP API"));
      }
    }

  // Create options for player
  if ( sim -> active_player )
    sim -> active_player -> create_options();

  return sim -> active_player != nullptr;
}

bool parse_guild( sim_t*             sim,
                  const std::string& name,
                  const std::string& value )
{
  // Save Raid Summary file when guilds are downloaded
  sim -> save_raid_summary = 1;

  try
  {
    std::string type_str;
    std::string ranks_str;
    int max_rank = 0;

    std::vector<std::unique_ptr<option_t>> options;
    options.push_back( opt_string( "class", type_str ) );
    options.push_back( opt_int( "max_rank", max_rank ) );
    options.push_back( opt_string( "ranks", ranks_str ) );

    names_and_options_t stuff( sim, name, std::move(options), value );

    std::vector<int> ranks_list;
    if ( ! ranks_str.empty() )
    {
      std::vector<std::string> ranks = util::string_split( ranks_str, "/" );

      for ( size_t i = 0; i < ranks.size(); i++ )
        ranks_list.push_back( std::stoi( ranks[i] ) );
    }

    player_e pt = PLAYER_NONE;
    if ( ! type_str.empty() )
      pt = util::parse_player_type( type_str );

    for ( size_t i = 0; i < stuff.names.size(); ++i )
    {
      std::string& guild_name = stuff.names[ i ];
      if ( ! bcp_api::download_guild( sim, stuff.region, stuff.server, guild_name,
                                      ranks_list, pt, max_rank, stuff.cache ) )
        return false;
    }
  }

  catch ( names_and_options_t::error& )
  { return false; }

  return true;
}


// parse_fight_style ========================================================

bool parse_fight_style( sim_t*             sim,
                        const std::string& /*name*/,
                        const std::string& value )
{
  static std::vector<std::string> FIGHT_STYLES {
    "Patchwerk", "Ultraxion", "CleaveAdd", "HelterSkelter", "LightMovement", "HeavyMovement",
    "HecticAddCleave", "Beastlord", "CastingPatchwerk", "DungeonSlice"
  };

  auto it = range::find_if( FIGHT_STYLES, [ &value ]( const std::string& n ) {
    return util::str_compare_ci( value, n );
  } );

  if ( it == FIGHT_STYLES.end() )
  {
    throw std::invalid_argument( fmt::format( "Unknown fight style {}, available values: {}",
      value, util::string_join( FIGHT_STYLES ) ) );
  }

  sim->fight_style = *it;

  return true;
}

// parse_override_spell_data ================================================

bool parse_override_spell_data( sim_t*             sim,
                                       const std::string& /* name */,
                                       const std::string& value )
{
  // Register overrides only once, for the main thread
  if ( sim -> parent )
  {
    return true;
  }

  size_t v_pos = value.find( '=' );

  if ( v_pos == std::string::npos )
  {
    throw std::invalid_argument("Invalid form. Spell data override takes the form <spell|effect|power>.<id>.<field>=value");
  }

  std::vector< std::string > splits = util::string_split( value.substr( 0, v_pos ), "." );

  if ( splits.size() != 3 )
  {
    throw std::invalid_argument("Invalid form. Spell data override takes the form <spell|effect|power>.<id>.<field>=value");
  }

  int parsed_id = std::stoi( splits[ 1 ] );
  if ( parsed_id <= 0 )
  {
    throw std::invalid_argument("Invalid spell id (negative or zero).");
  }
  unsigned id = as<unsigned>(parsed_id);

  double v = std::stod( value.substr( v_pos + 1 ) );

  if ( util::str_compare_ci( splits[ 0 ], "spell" ) )
  {
    dbc_override::register_spell( sim -> dbc, id, splits[ 2 ], v );
  }
  else if ( util::str_compare_ci( splits[ 0 ], "effect" ) )
  {
    dbc_override::register_effect( sim -> dbc, id, splits[ 2 ], v );
  }
  else if ( util::str_compare_ci( splits[ 0 ], "power" ) )
  {
    dbc_override::register_power( sim -> dbc, id, splits[ 2 ], v );
  }
  else
  {
    throw std::invalid_argument("Invalid form. Spell data override takes the form <spell|effect|power>.<id>.<field>=value");
  }

  return true;
}

// parse_override_target_health =============================================

bool parse_override_target_health( sim_t*             sim,
                                   const std::string& /* name */,
                                   const std::string& value )
{
  std::vector<std::string> healths = util::string_split( value, "/" );

  for ( size_t i = 0; i < healths.size(); ++i )
  {
    std::stringstream s;
    s << healths[ i ];
    uint64_t health_number;
    s >> health_number;
    if ( health_number > 0 )
    {
      sim -> overrides.target_health.push_back( health_number );
    }
  }

  return true;
}


// parse_spell_query ========================================================

bool parse_spell_query( sim_t*             sim,
                               const std::string& /* name */,
                               const std::string& value )
{
  std::string sq_str = value;
  size_t lvl_offset = std::string::npos;

  if ( ( lvl_offset = value.rfind( "@" ) ) != std::string::npos )
  {
    std::string lvl_offset_str = value.substr( lvl_offset + 1 );
    int sq_lvl = std::stoi( lvl_offset_str );
    if ( sq_lvl < 1 )
      return 0;

    if ( sq_lvl > MAX_ILEVEL )
    {
      throw std::invalid_argument(fmt::format("Maximum item level supported in Simulationcraft is {}.", MAX_ILEVEL ));
    }

    sim -> spell_query_level = as< unsigned >( sq_lvl );

    sq_str = sq_str.substr( 0, lvl_offset );
  }

  sim -> spell_query = std::unique_ptr<spell_data_expr_t>( spell_data_expr_t::parse( sim, sq_str ) );

  if (sim -> spell_query == nullptr)
  {
    throw std::invalid_argument("Could not build spell query");
  }
  return sim -> spell_query != nullptr;
}

// parse_item_sources =======================================================

// Specifies both the default search order for the various data sources
// and the complete set of valid data source names.
const char* const default_item_db_sources[] =
{
  "local", "bcpapi", "wowhead", "ptrhead"
#if SC_BETA
  , SC_BETA_STR "head"
#endif
};

bool parse_item_sources( sim_t*             sim,
                                const std::string& /* name */,
                                const std::string& value )
{
  sim -> item_db_sources.clear();

  std::vector<std::string> sources = util::string_split( value, ":/|" );

  for ( size_t j = 0; j < sources.size(); j++ )
  {
    for ( size_t i = 0; i < sizeof_array( default_item_db_sources ); ++i )
    {
      if ( util::str_compare_ci( sources[ j ], default_item_db_sources[ i ] ) )
      {
        sim -> item_db_sources.push_back( default_item_db_sources[ i ] );
        break;
      }
    }
  }

  if ( sim -> item_db_sources.empty() )
  {
    std::string all_known_sources;

    for ( size_t i = 0; i < sizeof_array( default_item_db_sources ); ++i )
    {
      all_known_sources += ' ';
      all_known_sources += default_item_db_sources[ i ];
    }

    throw std::invalid_argument(fmt::format("Your global data source string '{}' contained no valid data sources. "
        "Valid identifiers are: {}", value, all_known_sources ));
  }

  return true;
}

bool parse_process_priority( sim_t*             sim,
                                   const std::string& /* name */,
                                   const std::string& value )
{
  computer_process::priority_e pr = computer_process::BELOW_NORMAL;

  if ( util::str_compare_ci( value, "normal" ) )
  {
    pr = computer_process::NORMAL;
  }
  else if ( util::str_compare_ci( value, "above_normal" ) )
  {
    pr = computer_process::ABOVE_NORMAL;
  }
  else if ( util::str_compare_ci( value, "below_normal" ) )
  {
    pr = computer_process::BELOW_NORMAL;
  }
  else if ( util::str_compare_ci( value, "high" ) )
  {
    pr = computer_process::HIGH;
  }
  else if ( util::str_compare_ci( value, "low" ) )
  {
    pr = computer_process::LOW;
  }
  else
  {
    sim -> errorf( "Could not set thread priority to %s. Defaulting to below_normal priority.", value.c_str() );
  }

  sim -> process_priority = pr;

  return true;
}

bool parse_target_error_role( sim_t * sim,
                              const std::string& /* name */,
                              const std::string& value )
{
  sim -> target_error_role = util::parse_role_type( value );

  if (sim -> target_error_role == ROLE_NONE)
  {
    throw std::invalid_argument("Invalid target error role");
  }

  return true;
}

bool parse_maximize_reporting( sim_t*             sim,
                                   const std::string& /*name*/,
                                   const std::string& v )
{
  if ( v != "0" && v != "1" )
  {
    throw std::invalid_argument("Acceptable values are '1' or '0'.");
  }
  bool r = std::stoi( v ) != 0;
  if ( r )
  {
    sim -> maximize_reporting = true;
    sim -> statistics_level = 100;
    sim -> report_raid_summary = true;
    sim -> report_rng = true;
    sim -> report_details = true;
    sim -> report_targets = true;
    sim -> report_pets_separately = true;
    sim -> report_precision = 4;
    sim -> buff_uptime_timeline = true;
  }

  return true;
}

/**
 * Parse threads option, and if equal or lower than 0, adjust
 * the number of threads to the number of cpu cores minus the absolute value given as a thread option.
 * So eg. threads=-2 on a 8 core machine will result in 6 threads.
 */
void adjust_threads( int& threads )
{
  // This is to resolve https://github.com/simulationcraft/simc/issues/2304
  int max_threads = sc_thread_t::cpu_thread_count();
  if ( threads <= 0 && max_threads > 0 ) //max_threads will return 0 if it has no clue how many threads it can use.
  {
    threads *= -1;
    if ( threads <= max_threads )
      threads = max_threads - threads;
  }
}

// Proxy cast ===============================================================

struct proxy_cast_check_t : public event_t
{
  int uses;
  const int& _override;
  timespan_t start_time;
  timespan_t cooldown;
  timespan_t duration;

  proxy_cast_check_t( sim_t& s, int u, timespan_t st, timespan_t i, timespan_t cd, timespan_t d, const int& o ) :
    event_t( s, i ),
    uses( u ), _override( o ), start_time( st ), cooldown( cd ), duration( d )
  {
  }
  virtual const char* name() const override
  { return "proxy_cast_check"; }
  virtual bool proxy_check() = 0;
  virtual void proxy_execute() = 0;
  virtual proxy_cast_check_t* proxy_schedule( timespan_t interval ) = 0;

  virtual void execute() override
  {
    timespan_t interval = timespan_t::from_seconds( 0.25 );

    if ( uses == _override && start_time > timespan_t::zero() )
      interval = cooldown - ( sim().current_time() - start_time );

    if ( proxy_check() )
    {
      // Cooldown over, reset uses
      if ( uses == _override && start_time > timespan_t::zero() && ( sim().current_time() - start_time ) >= cooldown )
      {
        start_time = timespan_t::zero();
        uses = 0;
      }

      if ( uses < _override )
      {
        proxy_execute();

        uses++;

        if ( start_time == timespan_t::zero() )
          start_time = sim().current_time();

        interval = duration + timespan_t::from_seconds( 1 );

        if ( sim().debug )
          sim().out_debug.printf( "Proxy-Execute uses=%d total=%d start_time=%.3f next_check=%.3f",
                      uses, _override, start_time.total_seconds(),
                      ( sim().current_time() + interval ).total_seconds() );
      }
    }

    proxy_schedule( interval );
  }
};

struct sim_end_event_t : event_t
{
  sim_end_event_t( sim_t& s, timespan_t end_time ) :
    event_t( s, end_time )
  {
  }
  virtual const char* name() const override
  { return "sim_end_expected_time"; }
  virtual void execute() override
  {
    sim().cancel_iteration();
  }
};

/* Forcefully cancel the iteration if it has unexpectedly taken too long
 * to end normally.
 */
struct sim_safeguard_end_event_t : public sim_end_event_t
{
  timespan_t et;

  sim_safeguard_end_event_t( sim_t& s, timespan_t end_time ) :
    sim_end_event_t( s, end_time ), et( end_time )
  { }

  const char* name() const override
  { return "sim_end_twice_expected_time"; }

  void execute() override
  {
    sim().errorf( "Simulation has been forcefully cancelled at %.3f (%.3f) because twice the "
                  "expected combat length has been exceeded.",
                  sim().current_time().total_seconds(),
                  et.total_seconds() );

    sim_end_event_t::execute();
  }
};

struct resource_timeline_collect_event_t : public event_t
{
  resource_timeline_collect_event_t( sim_t& s ) :
    event_t( s, timespan_t::from_seconds( 1 ) )
  {
  }
  virtual const char* name() const override
  { return "resource_timeline_collect_event_t"; }
  virtual void execute() override
  {
    if ( sim().iterations == 1 || sim().current_iteration > 0 )
    {
      if ( ! sim().single_actor_batch )
      {
        // Assumptions: Enemies do not have primary resource regeneration
        for ( size_t i = 0, actors = sim().player_non_sleeping_list.size(); i < actors; i++ )
        {
          player_t* p = sim().player_non_sleeping_list[ i ];
          if ( p -> primary_resource() == RESOURCE_NONE ) continue;

          p -> collect_resource_timeline_information();
        }
      }
      else
      {
        auto p = sim().player_no_pet_list[ sim().current_index ];
        if (p && p -> primary_resource() != RESOURCE_NONE)
        {
          p -> collect_resource_timeline_information();
          for ( auto pet : p -> pet_list )
          {
            if ( pet -> primary_resource() != RESOURCE_NONE )
            {
              pet -> collect_resource_timeline_information();
            }
          }
        }
      }

      // However, enemies do have health
      for ( size_t i = 0, actors = sim().target_non_sleeping_list.size(); i < actors; i++ )
      {
        player_t* p = sim().target_non_sleeping_list[ i ];
        p -> collect_resource_timeline_information();
      }
    }

    make_event<resource_timeline_collect_event_t>( sim(), sim() );
  }
};

struct regen_event_t : public event_t
{
  regen_event_t( sim_t& s ) :
    event_t( s, s.regen_periodicity )
  {
    if ( sim().debug )
    {
      sim().out_debug.printf( "New Regen Event" );
    }
  }

  virtual const char* name() const override
  { return "Regen Event"; }

  virtual void execute() override
  {
    if ( ! sim().single_actor_batch )
    {
      // targets do not get any resource regen for performance reasons
      for ( size_t i = 0, actors = sim().player_non_sleeping_list.size(); i < actors; i++ )
      {
        player_t* p = sim().player_non_sleeping_list[ i ];
        if ( p -> primary_resource() == RESOURCE_NONE ) continue;
        if ( p -> regen_type != REGEN_STATIC ) continue;

        p -> regen( sim().regen_periodicity );
      }
    }
    else
    {
      auto p = sim().player_no_pet_list[ sim().current_index ];
      if ( p && p -> primary_resource() != RESOURCE_NONE && p -> regen_type == REGEN_STATIC )
      {
        p -> regen( sim().regen_periodicity );
        for ( auto pet : p -> pet_list )
        {
          if ( ! pet -> is_sleeping() && p -> primary_resource() != RESOURCE_NONE &&
            p -> regen_type == REGEN_STATIC )
          {
            pet -> regen( sim().regen_periodicity );
          }
        }
      }
    }

    make_event<regen_event_t>( sim(), sim() );
  }
};

#ifndef SC_NO_NETWORKING
/// List of files from which to look for Blizzard API key
std::vector<std::string> get_api_key_locations()
{
  std::vector<std::string> key_locations;
  key_locations.push_back( "./apikey.txt" );

  // unix home
  if ( char* home_path = getenv( "HOME" ) )
  {
    std::string home_apikey(home_path);
    home_apikey += "/.simc_apikey";
    key_locations.push_back( home_apikey );
  }

  // windows home
  if ( char* home_drive = getenv( "HOMEDRIVE" ) )
  {
    if ( char* home_path = getenv( "HOMEPATH" ) )
    {
      std::string home_apikey(home_drive);
      home_apikey += home_path;
      home_apikey += "/apikey.txt";
      key_locations.push_back( home_apikey );
    }
  }

  return key_locations;
}

/// Check if api key is valid
bool validate_api_key( const std::string& key )
{
  // no better check for now than to measure its length.
  return key.size() == 65;
}

/**
 * @brief Try to get user-supplied api key
 *
 * We will include a simc default api key for people who do not wish to register their own.
 * This will be done at the time of compilation, as we do not want to post our key on git for everyone to see.
 * However, the default setting will always check to see if the user has put their own 32-character key in.
 * If this is true, then the apikey that they enter will overwrite the default.
 * in predetermined locations to see if they have saved the key to a file named 'api_key.txt'. The gui has a entry field setup
 * That will update the apikey variable whenever the user enters a key there.
 */
std::string get_api_key()
{
  auto key_locations = get_api_key_locations();

  for ( const auto& filename : key_locations )
  {
    std::ifstream myfile( filename );
    if ( ! myfile.is_open() )
    {
      continue;
    }

    std::string line;
    std::getline( myfile,line );
    if ( validate_api_key( line ) )
    {
      return line;
    }
    else
    {
      std::cerr << "Blizzard API credentials from file '" << filename << "' were not properly entered. (Size != 65)" << std::endl;
    }
  }

#if defined( SC_DEFAULT_APIKEY )
  return std::string(SC_DEFAULT_APIKEY);
#endif /* SC_DEFAULT_APIKEY */
  return std::string();
}
#endif /* SC_NO_NETWORKING */

/// Setup a periodic check for Bloodlust
struct bloodlust_check_t : public event_t
 {
   bloodlust_check_t( sim_t& sim ) :
     event_t( sim, timespan_t::from_seconds( 1.0 ) )
   {
   }

   virtual const char* name() const override
   { return "Bloodlust Check"; }

   virtual void execute() override
   {
     sim_t& sim = this -> sim();
     player_t* t = sim.target;
     if ( ( sim.bloodlust_percent  > 0                  && t -> health_percentage() <  sim.bloodlust_percent ) ||
          ( sim.bloodlust_time     < timespan_t::zero() && t -> time_to_percent( 0.0 ) < -sim.bloodlust_time ) ||
          ( sim.bloodlust_time     > timespan_t::zero() && sim.current_time() >  sim.bloodlust_time ) )
     {
       if ( ! sim.single_actor_batch )
       {
         for ( size_t i = 0; i < sim.player_non_sleeping_list.size(); ++i )
         {
           player_t* p = sim.player_non_sleeping_list[ i ];
           if ( p -> is_pet() || p -> buffs.exhaustion -> check() )
             continue;

           p -> buffs.bloodlust -> trigger();
           p -> buffs.exhaustion -> trigger();
         }
       }
       else
       {
         auto p = sim.player_no_pet_list[ sim.current_index ];
         if ( p )
         {
           p -> buffs.bloodlust -> trigger();
           p -> buffs.exhaustion -> trigger();
         }
       }
     }
     else
     {
      make_event<bloodlust_check_t>( sim, sim );
     }
   }
 };

// compare_dps ==============================================================

struct compare_dps
{
  bool operator()( player_t* l, player_t* r ) const
  {
    double lv = l->collected_data.dps.mean(), rv = r->collected_data.dps.mean();
    if ( lv == rv )
    {
      return l->actor_index < r->actor_index;
    }

    return lv > rv;
  }
};

// compare_priority_dps ==============================================================

struct compare_priority_dps
{
  bool operator()( player_t* l, player_t* r ) const
  {
    double lv = l->collected_data.prioritydps.mean(), rv = r->collected_data.prioritydps.mean();
    if ( lv == rv )
    {
      return l->actor_index < r->actor_index;
    }

    return lv > rv;
  }
};

// compare_apm ==============================================================

struct compare_apm
{
  bool operator()( player_t* l, player_t* r ) const
  {
    double lv = l->collected_data.fight_length.mean()
      ? 60.0 * l->collected_data.executed_foreground_actions.mean() / l->collected_data.fight_length.mean()
      : 0.0;

    double rv = r->collected_data.fight_length.mean()
      ? 60.0 * r->collected_data.executed_foreground_actions.mean() / r->collected_data.fight_length.mean()
      : 0.0;

    if ( lv == rv )
    {
      return l->actor_index < r->actor_index;
    }

    return lv > rv;
  }
};

// compare_hps ==============================================================

struct compare_hps
{
  bool operator()( player_t* l, player_t* r ) const
  {
    double lv = l->collected_data.hps.mean(), rv = r->collected_data.hps.mean();
    if ( lv == rv )
    {
      return l->actor_index < r->actor_index;
    }

    return lv > rv;
  }
};

// compare_hps_plus_aps ====================================================

struct compare_hps_plus_aps
{
  bool operator()( player_t* l, player_t* r ) const
  {
    double lv = l->collected_data.hps.mean() + l->collected_data.aps.mean(),
           rv = r->collected_data.hps.mean() + r->collected_data.aps.mean();
    if ( lv == rv )
    {
      return l->actor_index < r->actor_index;
    }

    return lv > rv;
  }
};

// compare_dtps ==============================================================

struct compare_dtps
{
  bool operator()( player_t* l, player_t* r ) const
  {
    double lv = l->collected_data.dtps.mean(), rv = r->collected_data.dtps.mean();
    if ( lv == rv )
    {
      return l->actor_index < r->actor_index;
    }

    return lv > rv;
  }
};

// compare_tmi==============================================================

struct compare_tmi
{
  bool operator()( player_t* l, player_t* r ) const
  {
    double lv = l->collected_data.theck_meloree_index.mean(),
           rv = r->collected_data.theck_meloree_index.mean();
    if ( lv == rv )
    {
      return l->actor_index < r->actor_index;
    }

    return lv > rv;
  }
};

// compare_name =============================================================

struct compare_name
{
  bool operator()( player_t* l, player_t* r ) const
  {
    if ( l -> type != r -> type )
    {
      return l -> type < r -> type;
    }
    if ( l -> specialization() != r -> specialization() )
    {
      return l -> specialization() < r -> specialization();
    }
    return l -> name_str < r -> name_str;
  }
};

} // UNNAMED NAMESPACE ===================================================

// ==========================================================================
// Simulator
// ==========================================================================

// sim_t::sim_t =============================================================

sim_t::sim_t() :
  event_mgr( this ),
  out_log( *this, &std::cout, sim_ostream_t::no_close() ),
  out_debug(*this, &std::cout, sim_ostream_t::no_close() ),
  debug( false ),
  max_time( timespan_t::zero() ),
  expected_iteration_time( timespan_t::zero() ),
  vary_combat_length( 0.0 ),
  current_iteration( -1 ),
  iterations( 0 ),
  canceled( 0 ),
  target_error( 0 ),
  target_error_role( ROLE_DPS ),
  current_error( 0 ),
  current_mean( 0 ),
  analyze_error_interval( 100 ),
  analyze_number( 0 ),
  cleanup_threads( false ),
  control( nullptr ),
  parent( nullptr ),
  initialized( false ),
  target( nullptr ),
  heal_target( nullptr ),
  target_list(),
  target_non_sleeping_list(),
  player_list(),
  player_no_pet_list(),
  player_non_sleeping_list(),
  active_player( nullptr ),
  current_index( 0 ),
  num_players( 0 ),
  num_enemies( 0 ), num_tanks( 0 ), enemy_targets( 0 ), healing( 0 ),
  global_spawn_index( 0 ),
  max_player_level( -1 ),
  queue_lag( timespan_t::from_seconds( 0.005 ) ), queue_lag_stddev( timespan_t::zero() ),
  gcd_lag( timespan_t::from_seconds( 0.150 ) ), gcd_lag_stddev( timespan_t::zero() ),
  channel_lag( timespan_t::from_seconds( 0.250 ) ), channel_lag_stddev( timespan_t::zero() ),
  queue_gcd_reduction( timespan_t::from_seconds( 0.1 ) ),
  default_cooldown_tolerance( timespan_t::from_millis( 250 ) ),
  strict_gcd_queue( 0 ),
  confidence( 0.95 ), confidence_estimator( 0.0 ),
  world_lag( timespan_t::from_seconds( 0.1 ) ), world_lag_stddev( timespan_t::min() ),
  travel_variance( 0 ), default_skill( 1.0 ), reaction_time( timespan_t::from_seconds( 0.5 ) ),
  regen_periodicity( timespan_t::from_seconds( 0.25 ) ),
  ignite_sampling_delta( timespan_t::from_seconds( 0.2 ) ),
  fixed_time( true ), optimize_expressions( false ),
  current_slot( -1 ),
  optimal_raid( 0 ), log( 0 ),
  debug_each( 0 ),
  save_profiles( false ),
  save_profile_with_actions( true ),
  default_actions( false ),
  normalized_stat( STAT_NONE ),
  default_region_str( "us" ),
  save_prefix_str( "save_" ),
  save_talent_str( 0 ),
  talent_format( TALENT_FORMAT_UNCHANGED ),
  stat_cache( 1 ),
  max_aoe_enemies( 20 ),
  show_etmi( 0 ),
  tmi_window_global( 0 ),
  tmi_bin_size( 0.5 ),
  requires_regen_event( false ), single_actor_batch( false ),
  progressbar_type( 0 ),
  armory_retries( 3 ),
  enemy_death_pct( 0 ), rel_target_level( -1 ), target_level( -1 ),
  target_adds( 0 ), desired_targets( 1 ), enable_taunts( false ),
  use_item_verification( true ),
  challenge_mode( false ), timewalk( -1 ), scale_to_itemlevel( -1 ), scale_itemlevel_down_only( false ),
  disable_set_bonuses( false ), disable_2_set( 1 ), disable_4_set( 1 ), enable_2_set( 1 ), enable_4_set( 1 ),
  pvp_crit( false ),
  auto_attacks_always_land( false ),
  active_enemies( 0 ), active_allies( 0 ),
  _rng(), seed( 0 ), deterministic( 0 ), strict_work_queue( 0 ),
  average_range( true ), average_gauss( false ),
  fight_style(), add_waves( 0 ), overrides( overrides_t() ),
  default_aura_delay( timespan_t::from_millis( 30 ) ),
  default_aura_delay_stddev( timespan_t::from_millis( 5 ) ),
  azerite_status( AZERITE_ENABLED ),
  progress_bar( *this ),
  scaling( new scaling_t( this ) ),
  plot( new plot_t( this ) ),
  reforge_plot( new reforge_plot_t( this ) ),
  elapsed_cpu( 0.0 ),
  elapsed_time( 0.0 ),
  work_done( 0 ),
  iteration_dmg( 0 ), priority_iteration_dmg( 0 ), iteration_heal( 0 ), iteration_absorb( 0 ),
  raid_dps(), total_dmg(), raid_hps(), total_heal(), total_absorb(), raid_aps(),
  simulation_length( "Simulation Length", false ),
  merge_time( 0 ), init_time( 0 ), analyze_time( 0 ),
  report_iteration_data( 0.025 ), min_report_iteration_data( -1 ),
  report_progress( 1 ),
  bloodlust_percent( 25 ), bloodlust_time( timespan_t::from_seconds( 0.5 ) ),
  // Report
  report_precision(2), report_pets_separately( 0 ), report_targets( 1 ), report_details( 1 ), report_raw_abilities( 1 ),
  report_rng( 0 ), hosted_html( 0 ),
  save_raid_summary( 0 ), save_gear_comments( 0 ), statistics_level( 1 ), separate_stats_by_actions( 0 ), report_raid_summary( 0 ), buff_uptime_timeline( 0 ),
  json_full_states( 0 ),
  decorated_tooltips( -1 ),
  allow_potions( true ),
  allow_food( true ),
  allow_flasks( true ),
  allow_augmentations( true ),
  solo_raid( false ),
  global_item_upgrade_level( 0 ),
  maximize_reporting( false ),
#ifndef SC_NO_NETWORKING
  apikey( get_api_key() ),
#endif
  distance_targeting_enabled( false ),
  ignore_invulnerable_targets( false ),
  enable_dps_healing( false ),
  scaling_normalized( 1.0 ),
  // Multi-Threading
  threads( 0 ), thread_index( 0 ), process_priority( computer_process::BELOW_NORMAL ),
  work_queue( new work_queue_t() ),
  spell_query(), spell_query_level( MAX_LEVEL ),
  pause_mutex( nullptr ),
  paused( false ),
  chart_show_relative_difference( false ),
  chart_boxplot_percentile( .25 ),
  display_hotfixes( false ),
  disable_hotfixes( false ),
  display_bonus_ids( false ),
  profileset_metric( { SCALE_METRIC_DPS } ),
  profileset_output_data(),
  profileset_enabled( false ),
  profileset_work_threads( 0 ),
  profileset_init_threads( 1 )
{
  item_db_sources.assign( std::begin( default_item_db_sources ),
                          std::end( default_item_db_sources ) );

  max_time = timespan_t::from_seconds( 300 );
  vary_combat_length = 0.2;
  use_optimal_buffs_and_debuffs( 1 );

  create_options();

  profileset::create_options( this );
}

sim_t::sim_t( sim_t* p, int index ) : sim_t()
{
  assert( p );

  parent = p;
  thread_index = index;

  // Inherit setup
  setup( parent -> control );

  // Inherit 'scaling' settings from parent because these are set outside of the config file
  assert( parent -> scaling );
  scaling -> scale_stat  = parent -> scaling -> scale_stat;
  scaling -> scale_value = parent -> scaling -> scale_value;

  // Inherit reporting directives from parent
  report_progress = parent -> report_progress;

  // Inherit 'plot' settings from parent because are set outside of the config file
  enchant = parent -> enchant;

  // While we inherit the parent seed, it may get overwritten in sim_t::init
  seed = parent -> seed;

  parent -> add_relative( this );
}

sim_t::sim_t( sim_t* p, int index, sim_control_t* control ) : sim_t()
{
  assert( p && control );

  parent = p;
  thread_index = index;

  // Use specialized control for setup
  setup( control );

  // Inherit 'scaling' settings from parent because these are set outside of the config file
  assert( parent -> scaling );
  scaling -> scale_stat  = parent -> scaling -> scale_stat;
  scaling -> scale_value = parent -> scaling -> scale_value;

  // Inherit reporting directives from parent
  report_progress = parent -> report_progress;

  // Inherit 'plot' settings from parent because are set outside of the config file
  enchant = parent -> enchant;

  // While we inherit the parent seed, it may get overwritten in sim_t::init
  seed = parent -> seed;

  parent -> add_relative( this );
}

// sim_t::~sim_t ============================================================

sim_t::~sim_t()
{
  assert( ( requires_cleanup() && relatives.empty() ) || ! requires_cleanup() );
  if( parent )
    parent -> remove_relative( this );
}

// sim_t::iteration_time_adjust =============================================

double sim_t::iteration_time_adjust() const
{
  if ( iterations <= 1 )
    return 1.0;

  if ( current_iteration == 0 )
    return 1.0;

  // Approximate uniform distribution for fight lengths through randomization when target error is
  // used. Will generate more fair fight length distribution when reasonable (<0.5) target_error
  // values are chosen, and removes issues with pathological cases where high values are used (high
  // enough for analyze_error_interval to bound the number of iterations done, instead of the
  // target_error).
  if ( target_error != 0 )
  {
    return rng().range( 1.0 - vary_combat_length, 1.0 + vary_combat_length );
  }
  else
  {
    auto progress = work_queue -> progress();
    return 1.0 + vary_combat_length * ( ( current_iteration % 2 ) ? 1 : -1 ) * progress.pct();
  }
}

// sim_t::expected_max_time =================================================

double sim_t::expected_max_time() const
{
  return max_time.total_seconds() * ( 1.0 + vary_combat_length );
}

// sim_t::add_relative ======================================================

void sim_t::add_relative( sim_t* cousin )
{
  if( parent )
  {
    parent -> add_relative( cousin );
  }
  else
  {
    AUTO_LOCK( relatives_mutex );
    relatives.push_back( cousin );
  }
}

// sim_t::remove_relative ===================================================

void sim_t::remove_relative( sim_t* cousin )
{
  if( parent )
  {
    parent -> remove_relative( cousin );
  }
  else
  {
    AUTO_LOCK( relatives_mutex );
    auto it = range::find( relatives, cousin );
    assert( it != relatives.end() && "Could not find relative to remove" );
    *it = relatives.back();
    relatives.pop_back();
  }
}

/// Cancel simulation.
void sim_t::cancel()
{
  if ( canceled ) return;

  if ( current_iteration >= 0 )
  {
    errorf( "\nSimulation has been canceled after %d iterations! (thread=%d) %20s\n", current_iteration + 1, thread_index, "" );
  }
  else
  {
    errorf( "\nSimulation has been canceled during player setup! (thread=%d) %20s\n", thread_index, "" );
  }

  work_queue -> flush();

  canceled = 1;

  for (auto & relative : relatives)
  {
    relative -> cancel();
  }

  if ( ! parent )
  {
    profilesets.cancel();
  }
}

// sim_t::interrupt =========================================================

void sim_t::interrupt()
{
  work_queue -> flush();

  for (auto & child : children)
  {
    child -> interrupt();
  }
}

// sim_t::is_canceled =======================================================

bool sim_t::is_canceled() const
{
  return canceled;
}

// sim_t::cancel_iteration ==================================================

void sim_t::cancel_iteration()
{
  if ( debug )
    out_debug << "Iteration canceled.";

  event_mgr.cancel();
}

// sim_t::combat ============================================================

void sim_t::combat()
{
  if ( debug ) out_debug << "Starting Simulator";

  // The sequencing of event manager seed and flush is very tricky.
  // DO NOT MESS WITH THIS UNLESS YOU ARE EXTREMELY CONFIDENT.
  // combat_begin will seed the event manager with "player_ready" events
  // combat_end   will flush any events remaining due to early termination
  // In the future, flushing may occur in event manager execute().

  combat_begin();
  event_mgr.execute();
  combat_end();
}

/// Reset simulation.
void sim_t::reset()
{
  if ( debug )
    out_debug << "Resetting Simulator";

  if( deterministic )
    seed = rng().reseed();

  event_mgr.reset();

  expected_iteration_time = max_time * iteration_time_adjust();

  analyze_number = 0;

  for ( auto& buff : buff_list )
    buff -> reset();

  for ( auto& target : target_list )
  {
    target -> reset();
    range::for_each( target->pet_list, []( pet_t* pet ) { pet->reset(); } );
  }

  if ( single_actor_batch )
  {
    player_no_pet_list[ current_index ] -> reset();
    // make sure to reset pets after owner, or otherwards they may access uninitialized things from the owner
    for ( auto pet : player_no_pet_list[ current_index ] -> pet_list )
    {
      pet -> reset();
    }
  }
  else
  {
    for ( auto& player : player_no_pet_list )
    {
      player -> reset();
      // Make sure to reset pets after owner, or otherwards they may access uninitialized things from the owner
      for ( auto& pet : player -> pet_list )
      {
        pet -> reset();
      }
    }
  }

  raid_event_t::reset( this );

  if ( legion_data.pantheon_proxy )
  {
    legion_data.pantheon_proxy -> reset();
  }
}

/// Start combat.
void sim_t::combat_begin()
{
  if ( debug_each )
  {
    // On Debug Each, we collect debug information for each iteration, but clear it before each one
    std::shared_ptr<io::ofstream> o(new io::ofstream());
    o -> open( output_file_str );
    if ( o -> is_open() )
    {
      out_debug = o;
      out_log = o;

      fmt::print( "------ Iteration #{} ------", current_iteration + 1 );
      std::fflush( stdout );
    }
    else
    {
      throw std::runtime_error(fmt::format("Unable to open output file '{}'.", output_file_str ));
    }
  }

  if ( debug )
    out_debug << "Combat Begin";

  reset();

  // Debug seed needs to be done _after_ sim reset, because deterministic=1 will reseed in
  // sim_t::reset()
  if ( debug_seed.size() > 0 )
  {
    enable_debug_seed();
  }

  iteration_dmg = priority_iteration_dmg = iteration_heal = 0;

  // Always call begin() to ensure various counters are initialized.
  datacollection_begin();

  for ( auto& target : target_list )
    target -> combat_begin();

  if ( overrides.arcane_intellect ) auras.arcane_intellect -> override_buff();
  if ( overrides.battle_shout ) auras.battle_shout->override_buff();
  if ( overrides.power_word_fortitude ) auras.power_word_fortitude -> override_buff();

  for ( player_e i = PLAYER_NONE; i < PLAYER_MAX; ++i )
  {
    const module_t* m = module_t::get( i );
    if ( m ) m -> combat_begin( this );
  }

  raid_event_t::combat_begin( this );

  if ( single_actor_batch )
  {
    player_no_pet_list[ current_index ] -> combat_begin();
    for ( auto pet: player_no_pet_list[ current_index ] -> pet_list )
    {
      pet -> combat_begin();
    }
  }
  else
  {
    for ( size_t i = 0; i < player_list.size(); ++i )
    {
      player_t* p = player_list[ i ];
      p -> combat_begin();
    }
  }

  if ( requires_regen_event )
    make_event<regen_event_t>( *this, *this );

  if ( overrides.bloodlust )
  {
     make_event<bloodlust_check_t>( *this, *this );
  }

  if ( fixed_time || ( target -> resources.base[ RESOURCE_HEALTH ] == 0 ) )
  {
    make_event<sim_end_event_t>( *this, *this, expected_iteration_time );
    target -> death_pct = enemy_death_pct;
  }
  else
  {
    target -> death_pct = enemy_death_pct;
  }
  make_event<sim_safeguard_end_event_t>( *this, *this, expected_iteration_time + expected_iteration_time );

  if ( legion_data.pantheon_proxy )
  {
    legion_data.pantheon_proxy -> start();
  }
}

// sim_t::combat_end ========================================================

void sim_t::combat_end()
{
  if ( debug ) out_debug << "Combat End";

  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    player_t* t = target_list[ i ];
    if ( t -> is_add() ) continue;
    t -> combat_end();
  }

  for ( player_e i = PLAYER_NONE; i < PLAYER_MAX; ++i )
  {
    const module_t* m = module_t::get( i );
    if ( m ) m -> combat_end( this );
  }

  raid_event_t::combat_end( this );

  if ( single_actor_batch )
  {
    player_no_pet_list[ current_index ] -> combat_end();
  }
  else
  {
    for ( size_t i = 0; i < player_no_pet_list.size(); ++i )
    {
      player_t* p = player_no_pet_list[ i ];
      p -> combat_end();
    }
  }

  for ( size_t i = 0; i < buff_list.size(); ++i )
  {
    buff_t* b = buff_list[ i ];
    b -> expire();
  }

  if ( iterations == 1 || current_iteration >= 1 )
    datacollection_end();

  //assert( active_enemies == 0 );
  //assert( active_allies == 0 );

  if ( debug ) out_debug << "Flush Events";

  event_mgr.flush();

  analyze_error();

  if ( debug_seed.size() > 0 )
  {
    disable_debug_seed();
  }
}

// sim_t::datacollection_begin ==============================================

void sim_t::datacollection_begin()
{
  if ( debug ) out_debug << "Sim Data Collection Begin";

  iteration_dmg = priority_iteration_dmg = iteration_heal = iteration_absorb = 0.0;

  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    player_t* t = target_list[ i ];
    if ( t -> is_add() ) continue;
    t -> datacollection_begin();
  }

  for ( size_t i = 0; i < buff_list.size(); ++i )
    buff_list[ i ] -> datacollection_begin();

  if ( single_actor_batch )
  {
    player_no_pet_list[ current_index ] -> datacollection_begin();
  }
  else
  {
    for ( size_t i = 0; i < player_no_pet_list.size(); ++i )
    {
      player_t* p = player_no_pet_list[ i ];
      p -> datacollection_begin();
    }
  }
  make_event<resource_timeline_collect_event_t>( *this, *this );
}

// sim_t::datacollection_end ================================================

void sim_t::datacollection_end()
{
  if ( debug ) out_debug << "Sim Data Collection End";

  simulation_length.add( current_time().total_seconds() );

  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    player_t* t = target_list[ i ];
    if ( t -> is_add() ) continue;
    t -> datacollection_end();
  }

  if ( single_actor_batch )
  {
    player_no_pet_list[ current_index ] -> datacollection_end();
  }
  else
  {
    for ( size_t i = 0; i < player_no_pet_list.size(); ++i )
    {
      player_t* p = player_no_pet_list[ i ];
      p -> datacollection_end();
    }
  }

  for ( size_t i = 0; i < buff_list.size(); ++i )
  {
    buff_t* b = buff_list[ i ];
    b -> datacollection_end();
  }

  total_dmg.add( iteration_dmg );
  raid_dps.add( current_time() != timespan_t::zero() ? iteration_dmg / current_time().total_seconds() : 0 );
  total_heal.add( iteration_heal );
  raid_hps.add( current_time() != timespan_t::zero() ? iteration_heal / current_time().total_seconds() : 0 );
  total_absorb.add( iteration_absorb );
  raid_aps.add( current_time() != timespan_t::zero() ? iteration_absorb / current_time().total_seconds() : 0 );

  if ( deterministic && report_iteration_data > 0 && current_iteration > 0 &&
       current_time() > timespan_t::zero() )
  {
    // TODO: Metric should be selectable
    iteration_data_entry_t entry( iteration_dmg / current_time().total_seconds(),
        current_time().total_seconds(), seed, current_iteration );
    for ( size_t i = 0, end = target_list.size(); i < end; ++i )
    {
      const player_t* t = target_list[ i ];
      // Once we start hitting adds (instead of real enemies), break out as those don't have real
      // hitpoints.
      if ( t -> is_add() )
      {
        break;
      }

      entry.add_health( static_cast< uint64_t >( t -> resources.initial[ RESOURCE_HEALTH ] ) );
    }

    if ( std::find_if( iteration_data.begin(), iteration_data.end(),
                       seed_predicate_t( seed ) ) != iteration_data.end() )
    {
      errorf( "[Thread-%d] Duplicate seed %llu found on iteration %u, skipping ...",
          thread_index, seed, current_iteration );
    }
    else
    {
      iteration_data.push_back( entry );
    }
  }
}

// sim_t::analyze_error =====================================================

void sim_t::analyze_error()
{
  if ( thread_index != 0 ) return;
  if ( target_error <= 0 ) return;
  if ( current_iteration < 1 ) return;

  work_queue -> lock();

  int n_iterations = work_queue -> progress().current_iterations;
  if ( strict_work_queue )
  {
    range::for_each( children, [ &n_iterations ]( sim_t* c ) {
      n_iterations += c -> work_queue -> progress().current_iterations;
    } );
  }

  if ( n_iterations < analyze_error_interval * ( analyze_number + 1 ) )
  {
    work_queue -> unlock();
    return;
  }

  analyze_number++;

  double mean_total=0;
  int mean_count=0;

  current_error = 0;

  if ( single_actor_batch )
  {
    auto p = player_no_pet_list[ current_index ];
    auto& cd = p -> collected_data;
    AUTO_LOCK( cd.target_metric_mutex );
    if ( cd.target_metric.size() != 0 )
    {
      cd.target_metric.analyze_basics();
      cd.target_metric.analyze_variance();
      current_mean = cd.target_metric.mean();
      if ( current_mean != 0 )
      {
        current_error = sim_t::distribution_mean_error( *this, cd.target_metric ) / current_mean;
      }
    }
  }
  else
  {
    for ( size_t i = 0; i < actor_list.size(); i++ )
    {
      player_t* p = actor_list[i];
      player_collected_data_t& cd = p -> collected_data;
      AUTO_LOCK( cd.target_metric_mutex );
      if ( cd.target_metric.size() != 0 )
      {
        cd.target_metric.analyze_basics();
        cd.target_metric.analyze_variance();
        double mean = cd.target_metric.mean();
        if ( mean != 0 )
        {
          double error = sim_t::distribution_mean_error( *this, cd.target_metric ) / mean;
          if ( error > current_error ) current_error = error;
          mean_total += mean;
          mean_count++;
        }
      }
    }
  }

  if( mean_count > 0 )
  {
    current_mean = mean_total / mean_count;
  }

  current_error *= 100;

  if ( current_error > 0 )
  {
    if ( current_error < target_error )
    {
      interrupt();
    }
    else
    {
      auto projected_iterations = static_cast<int>( n_iterations * ( ( current_error * current_error ) /
          ( target_error *  target_error ) ) );
      if ( ! strict_work_queue )
      {
        work_queue -> project( projected_iterations );
      }
      else
      {
        // Divide work evenly between threads
        projected_iterations /= threads;
        work_queue -> project( projected_iterations );
        range::for_each( children, [ projected_iterations ]( sim_t* c ) {
          c -> work_queue -> project( projected_iterations );
        } );
      }
    }
  }

  work_queue -> unlock();
}

/**
 * @brief check for active player
 *
 * This function just checks that the sim has at least 1 active player, and sets
 * fixed_time automatically if the only role present is a ROLE_HEAL.  Called in sim_t::init_actors()
 */
void sim_t::check_actors()
{
  bool too_quiet = true; // Check for at least 1 active player
  bool zero_dds = true; // Check for at least 1 player != HEAL

  for ( const auto& p : actor_list )
  {
    if ( p -> is_pet() || p -> is_enemy() )
      continue;
    if ( p -> type == HEALING_ENEMY )
      continue;
    if ( ! p -> quiet )
    {
      too_quiet = false;
    }
    if ( p -> primary_role() != ROLE_HEAL && ! p -> is_pet() )
    {
      zero_dds = false;
    }
  }

  if ( too_quiet && ! debug )
  {
    throw std::runtime_error("No active players in sim!");
  }

  // Set Fixed_Time when there are no DD's present
  if ( zero_dds && ! debug )
  {
    fixed_time = true;
  }
}

/**
 * @brief configure fight style
 *
 * Parses fight style information into specific simulator defines. Performed as a separate
 * initialization step (early) in the simulator init process to avoid positional dependencies
 * between options (first and foremost, the max_time option).
 *
 * Note that there are side-effects of this, namely that where fight styles define other options
 * (e.g., Ultraxion), those can no longer be overwritten on the command line.
 */
void sim_t::init_fight_style()
{
  if ( util::str_compare_ci( fight_style, "Patchwerk" ) )
  {
    raid_events_str.clear();
  }
  else if ( util::str_compare_ci( fight_style, "Ultraxion" ) )
  {
    max_time    = timespan_t::from_seconds( 366.0 );
    fixed_time  = true;
    vary_combat_length = 0.0;
    raid_events_str += "/flying,first=0,duration=500,cooldown=500";
    raid_events_str += "/position_switch,first=0,duration=500,cooldown=500";
    raid_events_str += "/stun,duration=1.0,first=45.0,period=45.0";
    raid_events_str += "/stun,duration=1.0,first=57.0,period=57.0";
    raid_events_str += "/damage,first=6.0,period=6.0,last=59.5,amount=44000,type=shadow";
    raid_events_str += "/damage,first=60.0,period=5.0,last=119.5,amount=44855,type=shadow";
    raid_events_str += "/damage,first=120.0,period=4.0,last=179.5,amount=44855,type=shadow";
    raid_events_str += "/damage,first=180.0,period=3.0,last=239.5,amount=44855,type=shadow";
    raid_events_str += "/damage,first=240.0,period=2.0,last=299.5,amount=44855,type=shadow";
    raid_events_str += "/damage,first=300.0,period=1.0,amount=44855,type=shadow";
  }
  else if ( util::str_compare_ci( fight_style, "CleaveAdd" ) || util::str_compare_ci(fight_style, "Cleave_Add" ) )
  {
    auto first_and_duration = static_cast<unsigned>( max_time.total_seconds() * 0.05 );
    auto cooldown = static_cast<unsigned>( max_time.total_seconds() * 0.075 );
    auto last = static_cast<unsigned>( max_time.total_seconds() * 0.90 );
    
    raid_events_str += fmt::format( "/adds,count=1,first={},cooldown={},duration={},last={}",
                                     first_and_duration, cooldown, first_and_duration, last );
  }
  else if ( util::str_compare_ci( fight_style, "HelterSkelter" ) || util::str_compare_ci( fight_style, "Helter_Skelter" ) )
  {
    raid_events_str += "/casting,cooldown=30,duration=3,first=15";
    raid_events_str += "/movement,cooldown=30,distance=20";
    raid_events_str += "/stun,cooldown=60,duration=2";
    raid_events_str += "/invulnerable,cooldown=120,duration=3";
  }
  else if ( util::str_compare_ci( fight_style, "LightMovement" ) )
  {
    auto first_time = static_cast<unsigned>( max_time.total_seconds() * 0.1 );
    auto last_time = static_cast<unsigned>( max_time.total_seconds() * 0.8 );

    raid_events_str += fmt::format( "/movement,players_only=1,cooldown=85,distance=50,first={},last={}",
                                   first_time, last_time );
  }
  else if ( util::str_compare_ci( fight_style, "HeavyMovement" ) )
  {
    raid_events_str += "/movement,players_only=1,first=10,cooldown=10,distance=25";
  }
  else if ( util::str_compare_ci( fight_style, "HecticAddCleave" ) )
  {
    // Phase 1 - Adds and move into position to fight adds
    auto first_and_duration = std::max( static_cast<unsigned>( max_time.total_seconds() * 0.05 ), 1u );
    auto cooldown = std::max( static_cast<unsigned>( max_time.total_seconds() * 0.075 ), 1u );
    auto last = static_cast<unsigned>( max_time.total_seconds() * 0.75 );

    raid_events_str += fmt::format( "/adds,count=5,first={},cooldown={},duration={},last={}",
                                    first_and_duration, cooldown, first_and_duration, last );

    raid_events_str += fmt::format( "/movement,distance=25,first={},cooldown={},last={}",
                                    first_and_duration, cooldown, last );

    // Phase2 - Move out of stuff
    auto first2 = static_cast<unsigned>( max_time.total_seconds() * 0.03 );
    auto cooldown2 = std::max( static_cast<unsigned>( max_time.total_seconds() * 0.04 ), 1u );

    raid_events_str += fmt::format( "/movement,players_only=1,distance=8,first={},cooldown={}",
                                    first2, cooldown2 );
  }
  else if ( util::str_compare_ci( fight_style, "Beastlord" ) )
  {
    raid_events_str += "/adds,name=Pack_Beast,count=6,"
                       "first=15,duration=10,cooldown=30,angle_start=0,angle_end=360,distance=3";
    raid_events_str += "/adds,name=Heavy_Spear,count=2,"
                       "first=15,duration=15,cooldown=20,spawn_x=-15,spawn_y=0,distance=15";
    raid_events_str += "/movement,first=13,distance=5,cooldown=20,players_only=1,player_chance=0.1";

    auto beast_duration = static_cast<int>( max_time.total_seconds() * 0.15 );
    auto beast_cooldown = static_cast<int>( max_time.total_seconds() * 0.25 );
    // Ensure min cooldown (cd - 6*stddev) is larger than duration
    auto beast_cooldown_stddev = std::max(
        static_cast<int>( ( beast_duration - beast_cooldown ) / 6.0 ) - 1, 0 );
    auto beast_last = static_cast<unsigned>( max_time.total_seconds() * 0.65 );

    raid_events_str += fmt::format( "/adds,name=Beast,count=1,first=10,duration_stddev=5,"
                                    "duration={},cooldown={},cooldown_stddev={},last={}",
                                    beast_duration, beast_cooldown, beast_cooldown_stddev,
                                    beast_last );
  }
  else if ( util::str_compare_ci( fight_style, "CastingPatchwerk" ) )
  {
    raid_events_str += "/casting,cooldown=500,duration=500";
  }
  else if ( util::str_compare_ci( fight_style, "DungeonSlice" ) )
  { //Based on the Hero Dungeon setup
    max_time                       = timespan_t::from_seconds( 360.0 );
    //Disables all raidbuffs, except those provided by scrolls or the character itself.
    optimal_raid                   = 0;
    overrides.arcane_intellect     = 1;
    overrides.battle_shout         = 1;
    overrides.power_word_fortitude = 1;
    overrides.bloodlust            = 1;

    ignore_invulnerable_targets = true;

    raid_events_str +=
        "/invulnerable,cooldown=500,duration=500,retarget=1"
        "/adds,name=Boss,count=1,cooldown=500,duration=140,duration_stddev=2"
        "/adds,name=SmallAdd,count=5,count_range=1,first=140,cooldown=45,duration=15,duration_stddev=2"
        "/adds,name=BigAdd,count=2,count_range=1,first=155,cooldown=45,duration=30,duration_stddev=2";
  }
  else
  {
    fight_style = "Patchwerk";
  }
}

/**
 * @brief configure parties
 *
 * This function... builds parties? I guess it assigns each player in player_list to
 * a party based on party_encoding. Called in sim_t::init_actors()
 */
void sim_t::init_parties()
{
  if ( debug )
    out_debug.printf( "Building Parties." );

  int party_index = 0;
  for ( const auto& party_str : party_encoding )
  {
    if (util::str_compare_ci(party_str, "reset"))
    {
      party_index = 0;
      for ( auto& player : player_list )
      {
        player -> party = 0;
      }
    }
    else if (util::str_compare_ci(party_str, "all"))
    {
      for ( auto& player : player_list )
      {
        player -> party = 1;
      }
    }
    else
    {
      party_index++;

      auto player_names = util::string_split( party_str, ",;/" );

      for ( const auto& player_name : player_names )
      {
        player_t* p = find_player( player_name );
        if ( ! p )
        {
          throw std::invalid_argument( fmt::format("Unable to find player '{}' for party creation.", player_name) );
        }
        p -> party = party_index;
        for ( auto& pet : p -> pet_list )
        {
          pet -> party = party_index;
        }
      }
    }
  }
}

/// Initialize actors
void sim_t::init_actors()
{
  if ( debug )
  {
    out_debug.printf( "Initializing actors." );
  }

  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    init_actor( target_list[ i ] );
  }

  if ( debug )
    out_debug.printf( "Initializing Players." );

  if ( decorated_tooltips == -1 )
    decorated_tooltips = 1;

  auto name_number = 1;
  for ( size_t j = 0; j < player_no_pet_list.size(); ++j )
  {
    for ( size_t i = 0; i < player_no_pet_list.size(); ++i )
    {
      if ( i == j )
        continue;
      if ( player_no_pet_list[j] -> name_str == player_no_pet_list[i] -> name_str )
      {
        errorf( "%s has duplicate name, renaming to %s%s. Please use unique names for all players in simulation.",
                player_no_pet_list[j] -> name_str.c_str(),
                player_no_pet_list[j] -> name_str.c_str(),
                std::to_string( name_number ).c_str() );

        player_no_pet_list[j] -> name_str += std::to_string( name_number );
        name_number++;
      }
    }
  }

  for ( size_t i = 0; i < player_no_pet_list.size(); ++i )
  {
    init_actor( player_no_pet_list[ i ] );
  }

  // create actor entries for pets
  init_actor_pets();

  // check that we have at least one active player
  check_actors();

  // organize parties if necessary
  init_parties();
}

// sim_t::init_actor ========================================================

// This method handles the bulk of player initialization. Order is pretty
// critical here. Called in sim_t::init()
void sim_t::init_actor( player_t* p )
{
  try
  {
    // initialize class/enemy modules
    for ( player_e i = PLAYER_NONE; i < PLAYER_MAX; ++i )
    {
      const module_t* m = module_t::get( i );
      if ( m ) m -> init( p );
    }

    if ( default_actions && !p -> is_pet() )
    {
      p -> clear_action_priority_lists();
      p -> action_list_str.clear();
    }

    p -> init();
    p -> initialized = true;

    // This next section handles all the ugly details of initialization. Ideally, each of these
    // init_* methods will eventually return a bool to indicate success or failure, from which
    // we can either continue or halt initialization.
    // For now, we're only enforcing this condition for the particular init_* methods that can
    // lead to a sim -> cancel() result ( player_t::init_items() and player_t::init_actions() ).

    p -> init_target();
    p -> init_character_properties();

    // Initialize each actor's items, construct gear information & stats
    p -> init_items();

    // Must be done after init_items (processes item options, so we know selected azerite powers in
    // each item), and before init_spells (class modules "find_azerite_spell" in these).
    p -> init_azerite();
    p -> init_spells();
    p -> init_base_stats();
    p -> create_buffs();

    // First-phase creation of special effects from various sources. Needed to be able to create
    // actions (APLs, really) based on the presence of special effects on items.
    p -> create_special_effects();

    // First, create all the action objects and set up action lists properly
    p -> create_actions();

    // Create persistent actors from dynamic spawners
    spawner::create_persistent_actors( *p );

    // Create all actor pets before special effects get initialized. This ensures that we can use
    // stuff like the presence of an action (created with create_actions()) to determine if a pet
    // needs to be created or not. Similarly, talent, artifact, spec, and item based qualifiers would
    // work.
    p -> create_pets();

    // Second-phase initialize all special effects and register them to actors
    p -> init_special_effects();

    // Finally, initialize all action objects
    p -> init_actions();

    // Once all transient properties are initialized (e.g., base stats, spells, special effects,
    // items), initialize the initial stats of the actor.
    p -> init_initial_stats();
    // And once initial stats are initialized, derive the passive defensive properties of the actor.
    p -> init_defense();

    p -> init_scaling();
    p -> init_gains();
    p -> init_procs();
    p -> init_uptimes();
    p -> init_benefits();
    p -> init_rng();
    p -> init_stats();
    p -> init_distance_targeting();
    p -> init_absorb_priority();
    p -> init_assessors();
  }
  catch (const std::exception& e)
  {
    std::throw_with_nested(std::runtime_error(fmt::format("Actor '{}'", p->name())));
  }
}

// sim_t::init_actor_pets ===================================================

void sim_t::init_actor_pets()
{
  if ( debug )
    out_debug.printf( "Creating and initializing pets." );

  for ( size_t i = 0, end = target_list.size(); i < end; ++i )
  {
    player_t* p = target_list[ i ];

    for (auto & elem : p -> pet_list)
    {
      init_actor( elem );
    }
  }

  for ( size_t i = 0, end = player_no_pet_list.size(); i < end; ++i )
  {
    player_t* p = player_no_pet_list[ i ];

    for (auto & elem : p -> pet_list)
    {
      init_actor( elem );
    }
  }
}

// sim_t::init ==============================================================

void sim_t::init()
{
  auto start = std::chrono::high_resolution_clock::now();

  if ( initialized )
    return;

  event_mgr.init();

  unique_gear::register_target_data_initializers( this );

  // Seed RNG
  if ( seed == 0 )
  {
    if( deterministic )
    {
      seed = 31459;
    }
    else
    {
      std::random_device rd;
      seed  = uint64_t(rd()) | (uint64_t(rd()) << 32);
    }
  }
  _rng = rng::create( rng::parse_type( rng_str ) );
  _rng -> seed( seed + thread_index );

  if (   queue_lag_stddev == timespan_t::zero() )   queue_lag_stddev =   queue_lag * 0.25;
  if (     gcd_lag_stddev == timespan_t::zero() )     gcd_lag_stddev =     gcd_lag * 0.25;
  if ( channel_lag_stddev == timespan_t::zero() ) channel_lag_stddev = channel_lag * 0.25;
  if ( world_lag_stddev    < timespan_t::zero() ) world_lag_stddev   =   world_lag * 0.1;

  confidence_estimator = rng::stdnormal_inv( 1.0 - ( 1.0 - confidence ) / 2.0 );

  if ( challenge_mode && scale_to_itemlevel < 0 )
  {
    scale_to_itemlevel = 630;
    scale_itemlevel_down_only = true;
  }
  else if ( timewalk > 0 )
  {
    if ( scale_to_itemlevel == -1 )
    {
      switch ( timewalk )
      {
        case 85: scale_to_itemlevel = 300; break;
        case 80: scale_to_itemlevel = 160; break;
        case 70: scale_to_itemlevel = 95;  break;
      }
    }
    scale_itemlevel_down_only = true;
  }

  // set scaling metric
  if ( ! scaling -> scale_over.empty() )
  {
    scaling -> scaling_metric = util::parse_scale_metric( scaling -> scale_over );
    if ( scaling -> scaling_metric == SCALE_METRIC_NONE )
    {
      throw std::invalid_argument(fmt::format("Unknown scaling metric '{}'", scaling -> scale_over));
    }
  }

  auras.arcane_intellect = make_buff( this, "arcane_intellect", dbc::find_spell( this, 1459 ) )
                           ->set_default_value( dbc::find_spell( this, 1459 ) -> effectN( 1 ).percent() )
                           ->add_invalidate( CACHE_INTELLECT );

  auras.battle_shout = make_buff( this, "battle_shout", dbc::find_spell( this, 6673 ) )
                        ->set_default_value( dbc::find_spell( this, 6673 )->effectN( 1 ).percent() )
                        ->add_invalidate( CACHE_ATTACK_POWER );

  auras.power_word_fortitude = make_buff( this, "power_word_fortitude", dbc::find_spell( this, 21562 ) )
                           ->set_default_value( dbc::find_spell( this, 21562 ) -> effectN( 1 ).percent() )
                           ->add_invalidate( CACHE_STAMINA );

  // Find Already defined target, otherwise create a new one.
  if ( debug )
    out_debug << "Creating Enemies.";

  if ( !target_list.empty() )
  {
    target = target_list.data().front();
  }
  else if ( ! main_target_str.empty() )
  {
    player_t* p = find_player( main_target_str );
    if ( p )
      target = p;
  }
  else
    target = module_t::enemy() -> create_player( this, "Fluffy_Pillow" );

  // create additional enemies here
  while ( as<int>(target_list.size()) < desired_targets )
  {
    active_player = nullptr;
    active_player = module_t::enemy() -> create_player( this, "enemy" + util::to_string( target_list.size() + 1 ) );
    if ( ! active_player )
    {
      throw std::invalid_argument(fmt::format("Unable to create enemy {}.", target_list.size() ));
    }
  }

  if ( max_player_level < 0 )
  {
    for ( size_t i = 0; i < player_no_pet_list.size(); ++i )
    {
      player_t* p = player_no_pet_list[ i ];
      if ( max_player_level < p -> level() )
        max_player_level = p -> level();
    }
  }

  {
    // Determine whether we have healers or tanks.
    unsigned int healers = 0, tanks = 0;
    for ( size_t i = 0; i < player_no_pet_list.size(); ++i )
    {
      player_t& p = *player_no_pet_list[ i ];
      if ( p.primary_role() == ROLE_HEAL )
        ++healers;
      else if ( p.primary_role() == ROLE_TANK )
        ++tanks;
    }
    if ( healers > 0 || healing > 0 )
      heal_target = module_t::heal_enemy() -> create_player( this, "Healing_Target", RACE_NONE );
    if ( healing > 1 )
    {
      int targets_create = healing;
      do
      {
        heal_target = module_t::heal_enemy() -> create_player( this, "Healing_Target_" + util::to_string( targets_create ), RACE_NONE );
        targets_create--;
      }
      while ( targets_create > 1 );
    }
  }

  // Fight style initialization must be performed before raid event initialization, since fight
  // styles may define raid events.
  init_fight_style();

  raid_event_t::init( this );

  // Initialize actors
  init_actors();

  if ( report_precision < 0 ) report_precision = 2;

  simulation_length.reserve( std::min( iterations, 10000 ) );

  for ( const auto& player : player_list )
  {
    if ( player -> regen_type == REGEN_STATIC )
    {
      requires_regen_event = true;
      break;
    }
  }

  // We are committed to simulating something. Tell actors that the sim init is now complete if they
  // need to do something.
  if ( ! canceled )
  {
    bool verify_use_items_state = true;

    for ( auto& actor : actor_list )
    {
      try
      {
        actor -> init_finished();
      }
      catch (const std::exception& e)
      {
        std::throw_with_nested(std::runtime_error(fmt::format("Actor '{}'", actor->name())));
      }
      // Some verification stuff to avoid user mistakes

      // .. nag if the user has not added an use_item line for each on-use item
      if ( ! actor -> verify_use_items() )
      {
        verify_use_items_state = false;
      }

    }

    if ( ! verify_use_items_state )
    {
      errorf( "Disable this warning by adding 'use_item' actions into the action priority list "
              "for the actor(s), or add \"use_item_verification=0\" to your list of options "
              "passed to Simulationcraft." );
    }
  }

  // If save= option is used, don't bother initializing profilesets as the main thread is going to
  // exit in any case
  if ( active_player && active_player->report_information.save_str.empty() )
  {
    profilesets.initialize( this );
  }

  initialized = true;

  init_time = util::duration_fp_seconds( start );

  if (canceled)
  {

    throw std::runtime_error("Canceled");
  }
}


// sim_t::analyze ===========================================================

void sim_t::analyze()
{
  auto start = std::chrono::high_resolution_clock::now();

  simulation_length.analyze();
  if ( simulation_length.mean() == 0 ) return;

  for ( size_t i = 0; i < buff_list.size(); ++i )
    buff_list[ i ] -> analyze();

  if ( scaling -> scale_stat == STAT_NONE &&
       scaling -> calculate_scale_factors == 0 &&
       plot -> dps_plot_stat_str.empty() &&
       reforge_plot -> reforge_plot_stat_str.empty() &&
       profileset_map.size() == 0 && ! profileset_enabled )
  {
    std::cout << "Analyzing actor data ..." << std::endl;
  }

  for ( size_t i = 0; i < actor_list.size(); i++ )
    actor_list[ i ] -> analyze( *this );

  range::sort( players_by_dps,  compare_dps() );
  range::sort( players_by_priority_dps, compare_priority_dps() );
  range::sort( players_by_hps,  compare_hps() );
  range::sort( players_by_hps_plus_aps, compare_hps_plus_aps() );
  range::sort( players_by_dtps, compare_dtps() );
  range::sort( players_by_tmi,  compare_tmi() );
  range::sort( players_by_name, compare_name() );
  range::sort( players_by_apm, compare_apm() );
  range::sort( targets_by_name, compare_name() );

  analyze_iteration_data();

  analyze_time = util::duration_fp_seconds( start );
}

/**
 * Build a N-highest/lowest iteration table for deterministic so they can be
 * replayed
 */
void sim_t::analyze_iteration_data()
{
  // Only enabled for deterministic simulations for now
  if ( ! deterministic || report_iteration_data == 0 )
  {
    return;
  }

  range::sort( iteration_data, iteration_data_cmp_r );

  size_t min_entries = ( min_report_iteration_data == -1 ) ? 5 : static_cast<size_t>( min_report_iteration_data );
  double n_pct = report_iteration_data / ( report_iteration_data > 1 ? 100.0 : 1.0 );
  size_t n_entries = std::max( min_entries, static_cast<size_t>( std::ceil( iteration_data.size() * n_pct ) ) );

  // If low + high entries is more than we have data for, we will just print
  // all data out
  if ( n_entries * 2 > iteration_data.size() )
  {
    return;
  }

  std::copy( iteration_data.begin(), iteration_data.begin() + n_entries, std::back_inserter( low_iteration_data ) );
  range::sort( low_iteration_data, iteration_data_cmp_r );
  std::copy( iteration_data.end() - n_entries, iteration_data.end(), std::back_inserter( high_iteration_data ) );
  range::sort( high_iteration_data, iteration_data_cmp );
}


// sim_t::iterate ===========================================================

bool sim_t::iterate()
{
  try
  {
    init();
  }
  catch( const std::exception& e ){
    if (parent == nullptr)
    {
      std::throw_with_nested( std::runtime_error("Initializing"));
      fmt::print("Error initializing: {}\n", e.what());
    }
    return false;
  }

  progress_bar.init();

  activate_actors();

  bool more_work = true;
  do
  {
    ++current_iteration;
    ++work_done;

    combat();

    if ( progress_bar.update( false, as<int>(current_index) ) )
    {
      progress_bar.output( false );
    }

    do_pause();
    auto old_active = current_index;
    if ( ! canceled )
    {
      current_index = work_queue -> pop();
      more_work = work_queue -> more_work();

      if ( more_work && current_index != old_active )
      {
        if ( ! parent ||
             scaling -> scale_stat != STAT_NONE ||
             ( parent && parent -> reforge_plot -> current_stat_combo > -1 ) )
        {
          progress_bar.update( true, static_cast<int>( old_active ) );
          progress_bar.output( true );
          progress_bar.restart();
        }

        activate_actors();
      }
    }
  } while ( more_work && ! canceled );

  if ( ! canceled && progress_bar.update( true, as<int>(current_index) ) )
  {
    progress_bar.output( true );
  }

  // Deactivate the final actor after simulation is done in single_actor_batch
  if ( single_actor_batch )
  {
    player_no_pet_list[ player_no_pet_list.size() - 1 ] -> deactivate();
  }
  else
  {
    progress_bar.restart();
  }

  reset();

  iterations = current_iteration + 1;

  return iterations > 0;
}

/**
 * @brief pause simulator
 *
 * Sit in an external pause mutex (lock) of the first simulator thread until
 * it's our turn to lock/unlock it. In theory can have racing issues, but in
 * practice all iterations do enough work for it not to matter.
 *
 * Lock/unlock is done per iteration, so processing cost should be minimal.
 */
void sim_t::do_pause()
{
  if ( parent )
  {
    parent -> do_pause();
  }
  else
  {
    if ( pause_mutex && paused )
    {
      pause_mutex -> lock();
      pause_mutex -> unlock();
    }
  }
}

/// merge sims
void sim_t::merge( sim_t& other_sim )
{
  auto_lock_t auto_lock( merge_mutex );
  auto start = std::chrono::high_resolution_clock::now();

  if ( scaling -> scale_stat == STAT_NONE &&
       scaling -> calculate_scale_factors == 0 &&
       plot -> dps_plot_stat_str.empty() &&
       reforge_plot -> reforge_plot_stat_str.empty() &&
       profileset_map.size() == 0 && ! profileset_enabled )
  {
    std::cout << "Merging data from thread-" << other_sim.thread_index << " ..." << std::endl;
  }

  iterations += other_sim.iterations;
  work_per_thread[ other_sim.thread_index ] = other_sim.work_done;

  simulation_length.merge( other_sim.simulation_length );
  total_dmg.merge( other_sim.total_dmg );
  raid_dps.merge( other_sim.raid_dps );
  total_heal.merge( other_sim.total_heal );
  raid_hps.merge( other_sim.raid_hps );
  total_absorb.merge( other_sim.total_absorb );
  raid_aps.merge( other_sim.raid_aps );
  event_mgr.merge( other_sim.event_mgr );

  for ( auto & buff : buff_list )
  {
    if ( buff_t* otherbuff = buff_t::find( &other_sim, buff -> name_str.c_str() ) )
    {
      buff -> merge( *otherbuff );
    }
  }

  for ( auto & player : actor_list )
  {
    // If the player is spawned by a separate wrapper class, it will handle the merging process
    if ( player -> spawner != nullptr )
    {
      continue;
    }

    player_t* other_p = other_sim.find_player( player -> index );
    assert( other_p );
    player -> merge( *other_p );
  }

  // After normal player merging, merge all dynamically spawned players. This is done after the
  // normal player merging because the dynamic spawner merging process may need to create new actors
  // into the parent (e.g., thread 0) sim to accommodate child sims managing to create more actors
  // than the parent
  spawner::merge( *this, other_sim );

  range::append( iteration_data, other_sim.iteration_data );
  merge_time += util::duration_fp_seconds( start );
  init_time += other_sim.init_time;
}

/// merge all sims together
void sim_t::merge()
{
  work_per_thread[ thread_index ] = work_done;

  if ( children.empty() )
    return;

  merge_mutex.unlock();

  for ( size_t i = 0; i < children.size(); i++ )
  {
    sim_t* child = children[ i ];
    if ( child )
    {
      child -> join();
      children[ i ] = nullptr;
      if ( requires_cleanup() )
      {
        delete child;
      }
    }
  }

  children.clear();
}

// sim_t::run ===============================================================

void sim_t::run()
{
  try
  {
    if( iterate() )
    {
      parent -> merge( *this );
    }
  }
  catch (const std::exception& e )
  {
    if (parent)
      parent -> error("Error in child simulation ({}): {}", thread_index, e.what());
    cancel();
  }
}

// sim_t::partition =========================================================

void sim_t::partition()
{
  iterations = work_queue -> size();

  if ( threads <= 1 )
    return;
  if ( iterations < threads )
    return;

  thread::set_main_thread_priority();

  merge_mutex.lock(); // parent sim is locked until parent merge() is called

  int remainder = iterations % threads;
  iterations /= threads;

  // Normally we use a shared work-queue to ensure proper load balancing among threads.
  // However, when we desire deterministic runs (for debugging) we need to force the
  // sims to each use a specific number of iterations as opposed to using shared pool of work.

  if ( deterministic || strict_work_queue )
  {
    work_queue -> init( iterations );
  }

  int num_children = threads - 1;

  sim_control_t* child_control = nullptr;
  // Filter out profileset-related options from the child sim control, since they are not going to
  // use them anyhow. This significantly speeds up child creation in situations where the input
  // profile is a very large set of profileset sims.
  if ( profileset_map.size() > 0 )
  {
    child_control = profileset::filter_control( control );
  }
  else
  {
    child_control = control;
  }

  for ( int i = 0; i < num_children; i++ )
  {
    auto  child = new sim_t( this, i + 1, child_control );

    assert( child );
    children.push_back( child );

    child -> iterations = iterations;
    if ( remainder )
    {
      child -> iterations += 1;
      remainder--;
    }

    if( deterministic || strict_work_queue )
    {
      child -> work_queue -> init( child -> iterations );
    }
    else // share the work queue
    {
      child -> work_queue = work_queue;
    }
    child -> report_progress = 0;
  }

  computer_process::set_priority( process_priority ); // Set main thread priority

  for ( auto & child : children )
    child -> launch();

  // Safe to do for now, since control is only referenced by sim_t::setup, which is called in the
  // sim_t constructor.
  if ( profileset_map.size() > 0 )
  {
    delete child_control;
  }
}

// sim_t::execute ===========================================================

bool sim_t::execute()
{
  double start_cpu_time  = util::cpu_time();
  double start_wall_time = util::wall_time();

  bool success = false;
  {
    auto merge_final_action = gsl::finally([&](){ merge(); }); // Always merge, even in cases of unsuccessful simulation!
    partition();
    success = iterate();
  }

  if( success )
    analyze();

  elapsed_cpu  = util::cpu_time()  - start_cpu_time;
  elapsed_time = util::wall_time() - start_wall_time;

  return success;
}

/// find player in sim by name
player_t* sim_t::find_player( const std::string& name ) const
{
  auto it = range::find_if( actor_list, [name](const player_t* p) { return name == p -> name(); });
  if ( it != actor_list.end())
    return *it;
  return nullptr;
}

/// find player in sim by actor index
player_t* sim_t::find_player( int index ) const
{
  auto it = range::find_if( actor_list, [index](const player_t* p) { return index == p -> index; });
  if ( it != actor_list.end())
    return *it;
  return nullptr;
}

// sim_t::get_cooldown ======================================================

cooldown_t* sim_t::get_cooldown( const std::string& name )
{
  cooldown_t* c;

  for ( size_t i = 0; i < cooldown_list.size(); ++i )
  {
    c = cooldown_list[ i ];
    if ( c -> name_str == name )
      return c;
  }

  c = new cooldown_t( name, *this );

  cooldown_list.push_back( c );

  return c;
}

// sim_t::use_optimal_buffs_and_debuffs =====================================

void sim_t::use_optimal_buffs_and_debuffs( int value )
{
  optimal_raid = value;

  overrides.arcane_intellect        = optimal_raid;
  overrides.battle_shout            = optimal_raid;
  overrides.power_word_fortitude    = optimal_raid;

  overrides.chaos_brand             = optimal_raid;
  overrides.mystic_touch            = optimal_raid;
  overrides.mortal_wounds           = optimal_raid;
  overrides.bleeding                = optimal_raid;

  overrides.bloodlust               = optimal_raid;
}

// sim_t::time_to_think =====================================================

bool sim_t::time_to_think( timespan_t proc_time )
{
  if ( proc_time == timespan_t::zero() ) return false;
  if ( proc_time < timespan_t::zero() ) return true;
  return current_time() - proc_time > reaction_time;
}

// sim_t::create_expression =================================================

expr_t* sim_t::create_expression( const std::string& name_str )
{
  if ( name_str == "desired_targets" )
    return expr_t::create_constant( name_str, desired_targets );

  if ( name_str == "initial_targets" )
    return expr_t::create_constant( name_str, target_list.size() );

  if ( name_str == "time" )
    return make_ref_expr( name_str, event_mgr.current_time );

  if ( util::str_compare_ci( name_str, "expected_combat_length" ) )
    return make_ref_expr( name_str, expected_iteration_time );

  if ( name_str == "channel_lag" )
    return expr_t::create_constant( name_str, channel_lag );

  if ( name_str == "channel_lag_stddev" )
    return expr_t::create_constant( name_str, channel_lag_stddev );

  if ( util::str_compare_ci( name_str, "enemies" ) )
    return expr_t::create_constant( name_str, enemy_targets );

  if ( util::str_compare_ci( name_str, "active_enemies" ) )
  {
    if ( target_list.size() == 1u && !raid_event_t::has_raid_event( this, "adds" ) )
    {
      return expr_t::create_constant( name_str, 1.0 );
    }
    else
    {
      return make_ref_expr( name_str, active_enemies );
    }
  }

  if ( util::str_compare_ci( name_str, "active_allies" ) )
    return make_ref_expr( name_str, active_allies );

  // Get the number of actors in the simulation that do not have execute abilities
  if (name_str == "nonexecute_actors_pct")
  {
    struct nonexecute_actors_pct_expr : public expr_t
    {
      double nonexecute_actors_pct;  // Cached value, created on expression
                                     // creation

      nonexecute_actors_pct_expr( const sim_t* sim )
        : expr_t( "nonexecute_actors_pct" )
      {
        double execute    = 0.0;
        double nonexecute = 0.0;

        for ( const player_t* p : sim->player_list )
        {
          if ( p->role != ROLE_NONE )
          {
            switch ( p->specialization() )
            {
              case HUNTER_MARKSMANSHIP:
                if ( p->true_level > 100 )  // Assume Bullseye artifact trait
                {
                  execute += 1.0;
                }
                else
                {
                  nonexecute += 1.0;
                }
                break;
              case PRIEST_SHADOW:
              case WARRIOR_ARMS:
              case WARRIOR_FURY:
                execute += 1.0;
                break;
              default:
                nonexecute += 1.0;
            }
          }
        }

        auto divisor = ( nonexecute + execute );
        nonexecute_actors_pct = divisor != 0 ? nonexecute / divisor : 0.0;
      }

      double evaluate() override
      {
        return nonexecute_actors_pct;
      }
    };
    return new nonexecute_actors_pct_expr( this );
  }

  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( splits.size() == 3 )
  {
    if ( splits[ 0 ] == "aura" )
    {
      buff_t* buff = buff_t::find( this, splits[ 1 ] );
      if ( ! buff ) return nullptr;
      return buff_t::create_expression( splits[ 1 ], splits[ 2 ], *buff );
    }
  }
  if ( splits.size() >= 3 && splits[ 0 ] == "actors" )
  {
    player_t* actor = sim_t::find_player( splits[ 1 ] );
    if ( ! target ) return nullptr;
    std::string rest = splits[ 2 ];
    for ( size_t i = 3; i < splits.size(); ++i )
      rest += '.' + splits[ i ];
    return actor -> create_expression( rest );
  }

  if ( splits.size() == 1 && splits[ 0 ] == "target" )
  {
    return make_ref_expr( name_str, target -> actor_index );
  }

  // conditionals that handle upcoming raid events

  if ( splits.size() >= 3 && util::str_compare_ci( splits[ 0 ], "raid_event" ) )
  {
    std::string type_or_name = splits[ 1 ];
    std::string filter = splits[ 2 ];

    // Call once to see if we have a valid raid expression.
    raid_event_t::evaluate_raid_event_expression( this, type_or_name, filter, true );

    if ( optimize_expressions && util::str_compare_ci( filter, "exists" ) )
      return expr_t::create_constant( name_str, raid_event_t::evaluate_raid_event_expression( this, type_or_name, filter ) );

    struct raid_event_expr_t : public expr_t
    {
      sim_t* s;
      std::string type;
      std::string filter;

      raid_event_expr_t( sim_t* s, std::string& type, std::string& filter ) :
        expr_t( "raid_event" ), s( s ), type( type ), filter( filter )
      {}

      double evaluate() override
      {
        return raid_event_t::evaluate_raid_event_expression( s, type, filter );
      }

    };

    return new raid_event_expr_t( this, type_or_name, filter );
  }

  // If nothing else works, check to see if the string matches an actor in the sim.
  // If so, return their actor index
  if ( splits.size() == 1 )
    for ( size_t i = 0; i < actor_list.size(); i++ )
      if ( name_str == actor_list[ i ] -> name_str )
        return make_ref_expr( name_str, actor_list[ i ] -> actor_index );

  return nullptr;
}

// sim_t::print_options =====================================================

void sim_t::print_options()
{
  out_log.raw() << "\nWorld of Warcraft Raid Simulator Options:\n";

  out_log.raw() << "\nSimulation Engine:\n";
  for ( size_t i = 0; i < options.size(); ++i )
  {
    if ( options[i] -> name() != "apikey" ) // Don't print out sensitive information.
      out_log.raw() << options[i];
  }

  for ( size_t i = 0; i < player_list.size(); ++i )
  {
    player_t* p = player_list[ i ];

    out_log.raw().printf( "\nPlayer: %s (%s)\n", p -> name(), util::player_type_string( p -> type ) );
    for ( size_t j = 0; j < p -> options.size(); ++j )
      out_log.raw() << p -> options[ j ];
  }

  out_log.raw() << "\n";
}

void sim_t::add_option( std::unique_ptr<option_t> opt )
{
  options.insert( options.begin(), std::move(opt) );
}

// sim_t::create_options ====================================================

void sim_t::create_options()
{
  // General
  add_option( opt_int( "iterations", iterations ) );
  add_option( opt_bool( "cleanup_threads", cleanup_threads ) );
  add_option( opt_float( "target_error", target_error ) );
  add_option( opt_func( "target_error_role", parse_target_error_role ) );
  add_option( opt_int( "analyze_error_interval", analyze_error_interval ) );
  add_option( opt_func( "process_priority", parse_process_priority ) );
  add_option( opt_timespan( "max_time", max_time, timespan_t::zero(), timespan_t::max() ) );
  add_option( opt_bool( "fixed_time", fixed_time ) );
  add_option( opt_float( "vary_combat_length", vary_combat_length, 0.0, 1.0 ) );
  add_option( opt_func( "ptr", parse_ptr ) );
  add_option( opt_int( "threads", threads ) );
  add_option( opt_float( "confidence", confidence, 0.0, 1.0 ) );
  add_option( opt_func( "spell_query", parse_spell_query ) );
  add_option( opt_string( "spell_query_xml_output_file", spell_query_xml_output_file_str ) );
  add_option( opt_func( "item_db_source", parse_item_sources ) );
  add_option( opt_func( "proxy", parse_proxy ) );
  add_option( opt_int( "stat_cache", stat_cache ) );
  add_option( opt_int( "max_aoe_enemies", max_aoe_enemies ) );
  add_option( opt_bool( "optimize_expressions", optimize_expressions ) );
  add_option( opt_bool( "single_actor_batch", single_actor_batch ) );
  add_option( opt_bool( "progressbar_type", progressbar_type ) );
  // Raid buff overrides
  add_option( opt_func( "optimal_raid", parse_optimal_raid ) );
  add_option( opt_int( "override.arcane_intellect", overrides.arcane_intellect ) );
  add_option( opt_int( "override.battle_shout", overrides.battle_shout ) );
  add_option( opt_int( "override.power_word_fortitude", overrides.power_word_fortitude ) );
  add_option( opt_int( "override.chaos_brand", overrides.chaos_brand ) );
  add_option( opt_int( "override.mystic_touch", overrides.mystic_touch ) );
  add_option( opt_int( "override.mortal_wounds", overrides.mortal_wounds ) );
  add_option( opt_int( "override.bleeding", overrides.bleeding ) );
  add_option( opt_func( "override.spell_data", parse_override_spell_data ) );
  add_option( opt_func( "override.target_health", parse_override_target_health ) );
  // Lag
  add_option( opt_timespan( "channel_lag", channel_lag ) );
  add_option( opt_timespan( "channel_lag_stddev", channel_lag_stddev ) );
  add_option( opt_timespan( "gcd_lag", gcd_lag ) );
  add_option( opt_timespan( "gcd_lag_stddev", gcd_lag_stddev ) );
  add_option( opt_timespan( "queue_lag", queue_lag ) );
  add_option( opt_timespan( "queue_lag_stddev", queue_lag_stddev ) );
  add_option( opt_timespan( "queue_gcd_reduction", queue_gcd_reduction ) );
  add_option( opt_bool( "strict_gcd_queue", strict_gcd_queue ) );
  add_option( opt_timespan( "default_cooldown_tolerance", default_cooldown_tolerance ) );
  add_option( opt_timespan( "default_world_lag", world_lag ) );
  add_option( opt_timespan( "default_world_lag_stddev", world_lag_stddev ) );
  add_option( opt_timespan( "default_aura_delay", default_aura_delay ) );
  add_option( opt_timespan( "default_aura_delay_stddev", default_aura_delay_stddev ) );
  add_option( opt_float( "default_skill", default_skill ) );
  add_option( opt_timespan( "reaction_time", reaction_time ) );
  add_option( opt_float( "travel_variance", travel_variance ) );
  add_option( opt_timespan( "ignite_sampling_delta", ignite_sampling_delta ) );
  // Output
  add_option( opt_bool( "save_profiles", save_profiles ) );
  add_option( opt_bool( "save_profile_with_actions", save_profile_with_actions ) );
  add_option( opt_bool( "default_actions", default_actions ) );
  add_option( opt_bool( "debug", debug ) );
  add_option( opt_bool( "debug_each", debug_each ) );
  add_option( opt_func( "debug_seed", parse_debug_seed ) );
  add_option( opt_string( "html", html_file_str ) );
  add_option( opt_string( "json", json_file_str ) );
  add_option( opt_string( "json2", json_file_str ) );
  add_option( opt_bool( "hosted_html", hosted_html ) );
  add_option( opt_int( "healing", healing ) );
  add_option( opt_bool( "log", log ) );
  add_option( opt_string( "output", output_file_str ) );
  add_option( opt_bool( "save_raid_summary", save_raid_summary ) );
  add_option( opt_bool( "save_gear_comments", save_gear_comments ) );
  add_option( opt_bool( "buff_uptime_timeline", buff_uptime_timeline ) );
  add_option( opt_bool( "json_full_states", json_full_states ) );
  // Bloodlust
  add_option( opt_int( "bloodlust_percent", bloodlust_percent ) );
  add_option( opt_timespan( "bloodlust_time", bloodlust_time ) );
  // Overrides"
  add_option( opt_bool( "override.allow_potions", allow_potions ) );
  add_option( opt_bool( "override.allow_food", allow_food ) );
  add_option( opt_bool( "override.allow_flasks", allow_flasks ) );
  add_option( opt_bool( "override.allow_augmentations", allow_augmentations ) );
  add_option( opt_bool( "override.bloodlust", overrides.bloodlust ) );
  // Regen
  add_option( opt_timespan( "regen_periodicity", regen_periodicity ) );
  // RNG
  add_option( opt_string( "rng", rng_str ) );
  add_option( opt_bool( "deterministic", deterministic ) );
  add_option( opt_bool( "strict_work_queue", strict_work_queue ) );
  add_option( opt_float( "report_iteration_data", report_iteration_data ) );
  add_option( opt_int( "min_report_iteration_data", min_report_iteration_data ) );
  add_option( opt_bool( "average_range", average_range ) );
  add_option( opt_bool( "average_gauss", average_gauss ) );
  // Misc
  add_option( opt_list( "party", party_encoding ) );
  add_option( opt_func( "active", parse_active ) );
  add_option( opt_uint64( "seed", seed ) );
  add_option( opt_float( "wheel_granularity", event_mgr.wheel_granularity ) );
  add_option( opt_int( "wheel_seconds", event_mgr.wheel_seconds ) );
  add_option( opt_int( "wheel_shift", event_mgr.wheel_shift ) );
  add_option( opt_string( "reference_player", reference_player_str ) );
  add_option( opt_string( "raid_events", raid_events_str ) );
  add_option( opt_append( "raid_events+", raid_events_str ) );
  add_option( opt_func( "fight_style", parse_fight_style ) );
  add_option( opt_string( "main_target", main_target_str ) );
  add_option( opt_float( "enemy_death_pct", enemy_death_pct ) );
  add_option( opt_int( "target_level", target_level ) );
  add_option( opt_int( "target_level+", rel_target_level ) );
  add_option( opt_string( "target_race", target_race ) );
  add_option( opt_bool( "challenge_mode", challenge_mode ) );
  add_option( opt_int( "timewalk", timewalk, -1, MAX_SCALING_LEVEL ) );
  add_option( opt_int( "scale_to_itemlevel", scale_to_itemlevel ) );
  add_option( opt_bool( "scale_itemlevel_down_only", scale_itemlevel_down_only ) );
  add_option( opt_bool( "disable_set_bonuses", disable_set_bonuses ) );
  add_option( opt_uint( "disable_2_set", disable_2_set ) );
  add_option( opt_uint( "disable_4_set", disable_4_set ) );
  add_option( opt_uint( "enable_2_set", enable_2_set ) );
  add_option( opt_uint( "enable_4_set", enable_4_set ) );
  add_option( opt_bool( "pvp", pvp_crit ) );
  add_option( opt_bool( "auto_attacks_always_land", auto_attacks_always_land ) );
  add_option( opt_int( "desired_targets", desired_targets ) );
  add_option( opt_bool( "show_etmi", show_etmi ) );
  add_option( opt_float( "tmi_window_global", tmi_window_global ) );
  add_option( opt_float( "tmi_bin_size", tmi_bin_size ) );
  add_option( opt_bool( "enable_taunts", enable_taunts ) );
  add_option( opt_bool( "use_item_verification", use_item_verification ) );
  // Character Creation
  add_option( opt_func( "deathknight", parse_player ) );
  add_option( opt_func( "demonhunter", parse_player ) );
  add_option( opt_func( "druid", parse_player ) );
  add_option( opt_func( "hunter", parse_player ) );
  add_option( opt_func( "mage", parse_player ) );
  add_option( opt_func( "monk", parse_player ) );
  add_option( opt_func( "priest", parse_player ) );
  add_option( opt_func( "paladin", parse_player ) );
  add_option( opt_func( "rogue", parse_player ) );
  add_option( opt_func( "shaman", parse_player ) );
  add_option( opt_func( "warlock", parse_player ) );
  add_option( opt_func( "warrior", parse_player ) );
  add_option( opt_func( "enemy", parse_player ) );
  add_option( opt_func( "tmi_boss", parse_player ) );
  add_option( opt_func( "tank_dummy", parse_player ) );
  add_option( opt_func( "pet", parse_player ) );
  add_option( opt_func( "guardian", parse_player ) );
  add_option( opt_func( "copy", parse_player ) );
  add_option( opt_func( "armory", parse_armory ) );
  add_option( opt_func( "guild", parse_guild ) );
  add_option( opt_func( "local_json", parse_armory ) );
  add_option( opt_func( "http_clear_cache", http::clear_cache ) );
  add_option( opt_func( "cache_items", parse_cache ) );
  add_option( opt_func( "cache_players", parse_cache ) );
  add_option( opt_string( "default_region", default_region_str ) );
  add_option( opt_string( "default_server", default_server_str ) );
  add_option( opt_string( "save_prefix", save_prefix_str ) );
  add_option( opt_string( "save_suffix", save_suffix_str ) );
  add_option( opt_bool( "save_talent_str", save_talent_str ) );
  add_option( opt_func( "talent_format", parse_talent_format ) );
  add_option( opt_int( "armory_retries", armory_retries ) );
  // Stat Enchants
  add_option( opt_float( "default_enchant_strength", enchant.attribute[ATTR_STRENGTH] ) );
  add_option( opt_float( "default_enchant_agility", enchant.attribute[ATTR_AGILITY] ) );
  add_option( opt_float( "default_enchant_stamina", enchant.attribute[ATTR_STAMINA] ) );
  add_option( opt_float( "default_enchant_intellect", enchant.attribute[ATTR_INTELLECT] ) );
  add_option( opt_float( "default_enchant_spirit", enchant.attribute[ATTR_SPIRIT] ) );
  add_option( opt_float( "default_enchant_spell_power", enchant.spell_power ) );
  add_option( opt_float( "default_enchant_attack_power", enchant.attack_power ) );
  add_option( opt_float( "default_enchant_haste_rating", enchant.haste_rating ) );
  add_option( opt_float( "default_enchant_mastery_rating", enchant.mastery_rating ) );
  add_option( opt_float( "default_enchant_crit_rating", enchant.crit_rating ) );
  add_option( opt_float( "default_enchant_versatility_rating", enchant.versatility_rating ) );
  add_option( opt_float( "default_enchant_health", enchant.resource[RESOURCE_HEALTH] ) );
  add_option( opt_float( "default_enchant_mana", enchant.resource[RESOURCE_MANA] ) );
  add_option( opt_float( "default_enchant_rage", enchant.resource[RESOURCE_RAGE] ) );
  add_option( opt_float( "default_enchant_energy", enchant.resource[RESOURCE_ENERGY] ) );
  add_option( opt_float( "default_enchant_focus", enchant.resource[RESOURCE_FOCUS] ) );
  add_option( opt_float( "default_enchant_runic", enchant.resource[RESOURCE_RUNIC_POWER] ) );
  // Report
  add_option( opt_int( "report_precision", report_precision ) );
  add_option( opt_bool( "report_pets_separately", report_pets_separately ) );
  add_option( opt_bool( "report_targets", report_targets ) );
  add_option( opt_bool( "report_details", report_details ) );
  add_option( opt_bool( "report_raw_abilities", report_raw_abilities ) );
  add_option( opt_bool( "report_rng", report_rng ) );
  add_option( opt_int( "statistics_level", statistics_level ) );
  add_option( opt_bool( "separate_stats_by_actions", separate_stats_by_actions ) );
  add_option( opt_bool( "report_raid_summary", report_raid_summary ) ); // Force reporting of raid summary
  add_option( opt_string( "reforge_plot_output_file", reforge_plot_output_file_str ) );
  add_option( opt_bool( "monitor_cpu", event_mgr.monitor_cpu ) );
  add_option( opt_func( "maximize_reporting", parse_maximize_reporting ) );
  add_option( opt_string( "apikey", apikey ) );
  add_option( opt_string( "apitoken", user_apitoken ) );
  add_option( opt_bool( "distance_targeting_enabled", distance_targeting_enabled ) );
  add_option( opt_bool( "ignore_invulnerable_targets", ignore_invulnerable_targets ) );
  add_option( opt_bool( "enable_dps_healing", enable_dps_healing ) );
  add_option( opt_float( "scaling_normalized", scaling_normalized ) );
  add_option( opt_int( "global_item_upgrade_level", global_item_upgrade_level ) );
  add_option( opt_int( "decorated_tooltips", decorated_tooltips ) );
  // Charts
  add_option( opt_bool( "chart_show_relative_difference", chart_show_relative_difference ) );
  add_option( opt_float( "chart_boxplot_percentile", chart_boxplot_percentile ) );
  // Hotfix
  add_option( opt_bool( "show_hotfixes", display_hotfixes ) );
  // Bonus ids
  add_option( opt_bool( "show_bonus_ids", display_bonus_ids ) );

  // Expansion-specific options

  // Legion
  add_option( opt_int( "legion.infernal_cinders_users", legion_opts.infernal_cinders_users, 1, 20 ) );
  add_option( opt_int( "legion.engine_of_eradication_orbs", legion_opts.engine_of_eradication_orbs, 0, 4 ) );
  add_option( opt_int( "legion.void_stalkers_contract_targets", legion_opts.void_stalkers_contract_targets ) );
  add_option( opt_float( "legion.specter_of_betrayal_overlap", legion_opts.specter_of_betrayal_overlap, 0, 1 ) );
  add_option( opt_float( "legion.archimondes_hatred_reborn_damage", legion_opts.archimondes_hatred_reborn_damage, 0, 1 ) );
  add_option( opt_string( "legion.pantheon_trinket_users", legion_opts.pantheon_trinket_users ) );
  add_option( opt_timespan( "legion.pantheon_trinket_interval", legion_opts.pantheon_trinket_interval ) );
  add_option( opt_float( "legion.pantheon_trinket_interval_stddev", legion_opts.pantheon_trinket_interval_stddev ) );
  add_option( opt_func( "legion.cradle_of_anguish_resets", []( sim_t* sim, const std::string&, const std::string& value ) {
    auto split = util::string_split( value, ":/," );
    range::for_each( split, [ sim ]( const std::string& str ) {
      auto v = std::atof( str.c_str() );
      if ( v <= 0.0 )
      {
        return;
      }

      auto it = range::find( sim -> legion_opts.cradle_of_anguish_resets, v );
      if ( it != sim -> legion_opts.cradle_of_anguish_resets.end() )
      {
        return;
      }

      sim -> legion_opts.cradle_of_anguish_resets.push_back( v );
    } );
    return true;
  } ) );

  // Battle for Azeroth
  add_option( opt_func( "disable_azerite", []( sim_t* sim, const std::string&, const std::string& value ) {
    if ( value == "1" )
    {
      sim -> azerite_status = AZERITE_DISABLED_ALL;
    }
    else if ( util::str_compare_ci( value, "items" ) )
    {
      sim -> azerite_status = AZERITE_DISABLED_ITEMS;
    }
    else if ( util::str_compare_ci( value, "all" ) )
    {
      sim -> azerite_status = AZERITE_DISABLED_ALL;
    }
    else
    {
      sim -> errorf( "Unknown disable_azerite value \"%s\", valid values are 'items' or 'all'",
          value.c_str() );
      return false;
    }
    return true;
  } ) );

  add_option( opt_uint( "bfa.jes_howler_allies", bfa_opts.jes_howler_allies, 0, 4 ) );
  add_option( opt_float( "bfa.secrets_of_the_deep_chance",
        bfa_opts.secrets_of_the_deep_chance, 0, 1 ) );
  add_option( opt_float( "bfa.secrets_of_the_deep_collect_chance",
        bfa_opts.secrets_of_the_deep_collect_chance, 0, 1 ) );
  add_option( opt_int( "bfa.initial_archive_of_the_titans_stacks",
        bfa_opts.initial_archive_of_the_titans_stacks, 0, 20 ) );
  add_option( opt_int( "bfa.reorigination_array_stacks",
        bfa_opts.reorigination_array_stacks, 0, 10 ) );
  add_option( opt_bool( "bfa.reorigination_array_ignore_scale_factors",
        bfa_opts.reorigination_array_ignore_scale_factors ) );
  add_option( opt_float( "bfa.seductive_power_pickup_chance",
        bfa_opts.seductive_power_pickup_chance, 0.0, 1.0 ) );
  add_option( opt_int( "bfa.initial_seductive_power_stacks",
        bfa_opts.initial_seductive_power_stacks, 0, 5 ) );
  add_option( opt_bool( "bfa.randomize_oscillation",
        bfa_opts.randomize_oscillation ) );
  add_option( opt_bool( "bfa.auto_oscillating_overload",
        bfa_opts.auto_oscillating_overload ) );
  add_option( opt_bool( "bfa.zuldazar",
        bfa_opts.zuldazar ) );
  add_option( opt_timespan( "bfa.covenant_period",
        bfa_opts.covenant_period, 1_ms, timespan_t::max() ) );
  add_option( opt_float( "bfa.covenant_chance",
        bfa_opts.covenant_chance, 0.0, 1.0 ) );
  add_option( opt_float( "bfa.incandescent_sliver_chance",
        bfa_opts.incandescent_sliver_chance, 0.0, 1.0 ) );
  add_option( opt_timespan( "bfa.fight_or_flight_period", 
        bfa_opts.fight_or_flight_period, 1_ms, timespan_t::max() ) );
  add_option( opt_float( "bfa.fight_or_flight_chance", 
        bfa_opts.fight_or_flight_chance, 0.0, 1.0 ) );

  // applies to: "lavish_suramar_feast", battle for azeroth feasts
  add_option( opt_bool( "feast_as_dps", feast_as_dps ) );
}

// sim_t::parse_option ======================================================

bool sim_t::parse_option( const std::string& name,
                          const std::string& value )
{
  if ( active_player )
    if ( opts::parse( this, active_player -> options, name, value ) )
      return true;

  if ( opts::parse( this, options, name, value ) )
    return true;

  return false;
}

// sim_t::setup =============================================================

void sim_t::setup( sim_control_t* c )
{
  // Limitation: setup+execute is a one-way action that cannot be repeated or reset

  control = c;

  if ( ! parent ) cache::advance_era();

  // Global Options
  for ( const auto& option : control -> options )
  {
    if ( option.scope != "global" ) continue;
    if ( ! parse_option( option.name, option.value ) )
    {
      throw std::invalid_argument(
          fmt::format("Unknown option '{}' with value '{}'.", option.name, option.value ));
    }
  }

  // Combat
  // Try very hard to limit this to just what would be displayed on the gui.
  // Super-users can use misc options.
  // xyz = control -> combat.xyz;

  // Players
  for ( size_t i = 0; i < control -> players.size(); i++ )
  {
    player_t::create( this, control -> players[ i ] );
  }

  // Player Options
  for ( size_t i = 0; i < control -> options.size(); i++ )
  {
    option_tuple_t& o = control -> options[ i ];
    if ( o.scope == "global" ) continue;
    player_t* p = find_player( o.scope );
    if ( !p )
    {
      throw std::invalid_argument(
                fmt::format("Unable to locate player '{}' for option '{}' with value '{}'.",
                    o.scope, o.name, o.value));
    }
    if ( ! opts::parse(this, p->options, o.name, o.value))
    {
      throw std::invalid_argument(fmt::format("Unable to parse option '{}' with value '{}' for player '{}'.",
          o.name, o.value, p->name()));
    }

  }

  if ( player_list.empty() && spell_query == nullptr && ! display_bonus_ids )
  {
    throw std::runtime_error( "Nothing to sim!" );
  }

  if ( parent )
  {
    debug = 0;
    log = 0;
  }
  else if ( ! output_file_str.empty() )
  {
    std::shared_ptr<io::ofstream> o(new io::ofstream());
    o -> open( output_file_str );
    if ( o -> is_open() )
    {
      out_debug = o;
      out_log = o;
    }
    else
    {
      throw std::runtime_error(fmt::format("Unable to open output file '{}'.", output_file_str));
    }
  }
  if ( debug_each )
    debug = 1;

  if ( debug )
  {
    log = 1;
    print_options();
  }

  adjust_threads( threads );

  if ( log )
  {
    if ( ! debug_each )
      iterations = 1;

    threads = 1;
  }

  if ( iterations <= 0 )
  {
    iterations = 1000000; // limited by relative standard error

    if ( target_error <= 0 )
    {
      if ( scaling -> calculate_scale_factors )
      {
        target_error = 0.05;
      }
      else
      {
        target_error = 0.2;
      }
    }
    if ( plot -> dps_plot_iterations <= 0 )
    {
      if ( plot -> dps_plot_target_error <= 0 )
      {
        plot -> dps_plot_target_error = 0.5;
      }
    }
    if ( reforge_plot -> reforge_plot_iterations <= 0 )
    {
      if ( reforge_plot -> reforge_plot_target_error <= 0 )
      {
        reforge_plot -> reforge_plot_target_error = 0.5;
      }
    }
  }

  if ( single_actor_batch )
  {
    work_queue -> batches( player_no_pet_list.size() );
  }
  work_queue -> init( iterations );
  if ( thread_index == 0 )
  {
    work_per_thread.resize( threads );
  }

  if( deterministic && ( target_error != 0 ) )
  {
    throw std::invalid_argument("deterministic=1 cannot be used with non-zero target_error values!");
  }
}

// sim_t::progress ==========================================================

sim_progress_t sim_t::progress( std::string* detailed, int index )
{
  auto total_progress = work_queue -> progress( index );

  // If strict work queue is used with target error, estimate that total iterations will be roughly
  // the total iterations of the main thread, multiplied by the total number of threads.
  if ( target_error > 0 && strict_work_queue )
  {
    total_progress.total_iterations *= threads;
  }

  // For work queues that are independent, collect all work done so far for the progressbar.
  if ( deterministic || strict_work_queue )
  {
    AUTO_LOCK( relatives_mutex );
    for ( const auto& child : children )
    {
      if ( child )
      {
        auto progress = child -> work_queue -> progress( index );
        total_progress.current_iterations += progress.current_iterations;
        // If no target error, include total iterations from the child threads as well
        if ( target_error <= 0 )
        {
          total_progress.total_iterations += progress.total_iterations;
        }
      }
    }
  }

  detailed_progress( detailed, total_progress.current_iterations, total_progress.total_iterations );

  return total_progress;
}

double sim_t::progress( std::string& phase, std::string* detailed, int index )
{
  if ( canceled )
  {
    phase = "Canceled";
    return 1.0;
  }

  if ( plot -> num_plot_stats > 0 &&
       plot -> remaining_plot_stats > 0 )
  {
    return plot -> progress( phase, detailed );
  }
  else if ( scaling -> calculate_scale_factors &&
            scaling -> num_scaling_stats > 0 &&
            scaling -> remaining_scaling_stats > 0 )
  {
    return scaling -> progress( phase, detailed );
  }
  else if ( reforge_plot -> num_stat_combos > 0 )
  {
    return reforge_plot -> progress( phase, detailed );
  }
  else if ( current_iteration >= 0 )
  {
    phase = "Simulating";
    return progress( detailed, index ).pct();
  }
  else if ( current_slot >= 0 )
  {
    phase = current_name;
    return current_slot / ( double ) SLOT_MAX;
  }

  return 0.0;
}

void sim_t::detailed_progress( std::string* detail, int current_iterations, int total_iterations )
{
  if ( detail )
  {
    detail -> clear();
    *detail = fmt::format("Iteration {:d}/{:d}", current_iterations, total_iterations );
  }
}

void sim_t::abort()
{
  std::stringstream s;
  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    s << std::fixed << target_list[ i ] -> resources.initial[ RESOURCE_HEALTH ];
    if ( i < target_list.size() - 1 )
    {
      s << '/';
    }
  }

  errorf( "Force abort, seed=%llu target_health=%s", seed, s.str().c_str() );
  std::terminate();
}

/// add chart to sim for end of report processing
void sim_t::add_chart_data( const highchart::chart_t& chart )
{
  if ( chart.toggle_id_str_.empty() )
  {
    on_ready_chart_data.push_back( chart.to_aggregate_string( false ) );
  }
  else
  {
    chart_data[ chart.toggle_id_str_ ].push_back( chart.to_data() );
  }
}

void sim_t::print_spell_query()
{
  if ( ! spell_query_xml_output_file_str.empty() )
  {
    io::cfile file( spell_query_xml_output_file_str.c_str(), "w" );
    if ( ! file )
    {
      std::cerr << "Unable to open spell query xml output file '" << spell_query_xml_output_file_str << "', using stdout instead\n";
      file = io::cfile( stdout, io::cfile::no_close() );
    }
    std::shared_ptr<xml_node_t> root( new xml_node_t( "spell_query" ) );

    report::print_spell_query( root.get(), file, *this, *spell_query, spell_query_level );
  }
  else
  {
    report::print_spell_query( std::cout, *this, *spell_query, spell_query_level );
  }
}

/* Build a divisor timeline vector appropriate to a given timeline
 * bucket size, from given simulation length data.
 */
std::vector<double> sc_timeline_t::build_divisor_timeline( const extended_sample_data_t& simulation_length, double bin_size )
{
  std::vector<double> divisor_timeline;
  // divisor_timeline is necessary because not all iterations go the same length of time
  size_t max_buckets = static_cast<size_t>( floor( simulation_length.max() / bin_size ) + 1);
  divisor_timeline.assign( max_buckets, 0.0 );

  size_t num_timelines = simulation_length.data().size();
  for ( size_t i = 0; i < num_timelines; i++ )
  {
    size_t last = static_cast<size_t>( floor( simulation_length.data()[ i ] / bin_size ) );
    assert( last < divisor_timeline.size() ); // We created it with max length

    static const bool use_old_behaviour = true;
    if ( use_old_behaviour )
    {
      // Add all visited buckets.
      for ( size_t j = 0; j <= last; j++ )
      {
        divisor_timeline[ j ] += 1.0;
      }
    }
    else
    {
      // First add fully visited buckets.
      for ( size_t j = 0; j < last; j++ )
      {
        divisor_timeline[ j ] += 1.0;
      }

      // Now add partial amount for the last incomplete bucket.
      double remainder = simulation_length.data()[ i ] / bin_size - last;
      if ( remainder > 0.0 )
      {
        assert( last < divisor_timeline.size() );
        divisor_timeline[ last ] += remainder;
      }
    }
  }

  return divisor_timeline;
}

/* This function adjusts the timeline by a appropriate divisor_timeline.
 * Each bucket of the timeline is divided by the "amount of simulation time spent in that bucket"
 */
void sc_timeline_t::adjust( sim_t& sim )
{
  // Check if we have divisor timeline cached
  auto it = sim.divisor_timeline_cache.find( bin_size );
  if ( it == sim.divisor_timeline_cache.end() )
  {
    // If we don't have a cached divisor timeline, build one
    sim.divisor_timeline_cache[ bin_size ] = build_divisor_timeline( sim.simulation_length, bin_size );
  }

  // Do the timeline adjustement
  base_t::adjust( sim.divisor_timeline_cache[ bin_size ] );
}

void sc_timeline_t::adjust( const extended_sample_data_t& adjustor )
{
  // Do the timeline adjustement
  base_t::adjust( build_divisor_timeline( adjustor, bin_size ) );
}

// FIXME!  Move this to util at some point.

void sim_t::enable_debug_seed()
{
  auto enabled = false;

  if ( debug_seed.size() == 1 && seed == debug_seed[ 0 ] )
  {
    enabled = true;
  }
  else
  {
    auto it = std::lower_bound( debug_seed.begin(), debug_seed.end(), seed );
    enabled = it != debug_seed.end() && *it == seed;
  }

  if ( enabled )
  {
    if ( output_file_str.empty() )
    {
      errorf( "No 'output' option specified for debug_seed, not generating debug output ..." );
      return;
    }

    std::shared_ptr<io::ofstream> o(new io::ofstream());
    std::string fname = output_file_str + "." + util::to_string( seed );
    o -> open( fname );
    if ( o -> is_open() )
    {
      out_debug = o;
      out_log = o;

      fmt::print( "------ Iteration #{} (seed={}) ------", current_iteration, seed );
      std::fflush( stdout );
    }
    else
    {
      errorf( "Unable to open output file '%s'\n", fname.c_str() );
      return;
    }

    debug = true;
    log = 1;
  }
}

void sim_t::disable_debug_seed()
{
  if ( output_file_str.empty() )
  {
    return;
  }

  auto enabled = false;

  if ( debug_seed.size() == 1 && seed == debug_seed[ 0 ] )
  {
    enabled = true;
  }
  else
  {
    auto it = std::lower_bound( debug_seed.begin(), debug_seed.end(), seed );
    enabled = it != debug_seed.end() && *it == seed;
  }

  if ( enabled )
  {
    debug = false;
    log = 0;
  }
}

// Activates the relevant actors in the simulator just before simulating, based on the relevant
// simulation mode (single vs multi actor).
void sim_t::activate_actors()
{
  // Clear out all callbacks
  target_list.reset_callbacks();
  target_non_sleeping_list.reset_callbacks();
  player_list.reset_callbacks();
  player_no_pet_list.reset_callbacks();
  player_non_sleeping_list.reset_callbacks();
  healing_no_pet_list.reset_callbacks();
  healing_pet_list.reset_callbacks();

  // Normal sim mode activates all actors .. and this method is only called once at the beginning of
  // the simulation run.
  if ( ! single_actor_batch )
  {
    range::for_each( player_list, []( player_t* p ) { p -> activate(); } );
  }
  // Single-actor batch mode activates the current active actor
  else
  {
    range::for_each( target_list, []( player_t* t ) { t -> actor_changed(); } );

    // Deactivate old actor
    if ( current_index > 0 )
    {
      player_no_pet_list[ current_index - 1 ] -> deactivate();
    }

    // Activate new actor
    player_no_pet_list[ current_index ] -> activate();
    if ( ! profileset_enabled )
    {
      progress_bar.set_phase( player_no_pet_list[ current_index ] -> name_str );
    }
  }

  progress_bar.progress();

  current_iteration = -1;
}

bool sim_t::has_raid_event( const std::string& type ) const
{
  auto it = range::find_if( raid_events, [ &type ]( const std::unique_ptr<raid_event_t>& event ) {
      return util::str_compare_ci( type, event -> type );
  } );

  return it != raid_events.end();
}

// Determine when the main thread must clean up child threads (i.e., delete them)
bool sim_t::requires_cleanup() const
{
  // .. if we are a profileset simulation
  if ( profileset_enabled )
  {
    return true;
  }

  // .. if we are simulating a scale factor calculation
  if ( scaling -> scale_stat != STAT_NONE )
  {
    return true;
  }

  // .. if we are simulating a reforge plot
  if ( ! reforge_plot -> reforge_plot_stat_str.empty() )
  {
    return true;
  }

  // .. or finally, clean up child threads based on the "cleanup_threads" option value
  return cleanup_threads;
}
