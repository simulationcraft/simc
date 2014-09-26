// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE ============================================

// parse_ptr ================================================================

bool parse_ptr( sim_t*             sim,
                       const std::string& name,
                       const std::string& value )
{
  if ( name != "ptr" ) return false;

  if ( SC_USE_PTR )
    sim -> dbc.ptr = util::str_to_num<int>( value ) != 0;
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
      sim -> errorf( "Active Player is not a pet, cannot refer to 'owner'" );
      return false;
    }
  }
  else if ( value == "none" || value == "0" )
  {
    sim -> active_player = 0;
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
      sim -> errorf( "Unable to find player %s to make active.\n", value.c_str() );
      return false;
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

  sim -> use_optimal_buffs_and_debuffs( util::str_to_num<int>( value ) );

  return true;
}

// parse_player =============================================================

bool parse_player( sim_t*             sim,
                          const std::string& name,
                          const std::string& value )
{

  if ( name[ 0 ] >= '0' && name[ 0 ] <= '9' )
  {
    sim -> errorf( "Invalid actor name %s - name cannot start with a digit.", name.c_str() );
    return false;
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
      sim -> errorf( "Pet profile ( name %s ) needs a player preceding it.", name.c_str() );
      return false;
    }

    sim -> active_player = sim -> active_player -> create_pet( pet_name, pet_type );
  }
  else if ( name == "copy" )
  {
    std::string::size_type cut_pt = value.find( ',' );
    std::string player_name( value, 0, cut_pt );

    player_t* source;
    if ( cut_pt == value.npos )
      source = sim -> active_player;
    else
      source = sim -> find_player( value.substr( cut_pt + 1 ) );

    if ( source == 0 )
    {
      sim -> errorf( "Invalid source for profile copy - format is copy=target[,source], source defaults to active player." );
      return false;
    }

    sim -> active_player = module_t::get( source -> type ) -> create_player( sim, player_name );
    if ( sim -> active_player != 0 ) sim -> active_player -> copy_from ( source );
  }
  else
  {
    sim -> active_player = 0;
    const module_t* module = module_t::get( name );

    if ( ! module || ! module -> valid() )
    {
      sim -> errorf( "\nModule for class %s is currently not available.\n", name.c_str() );
    }
    else
    {
      sim -> active_player = module -> create_player( sim, value );

      if ( ! sim -> active_player )
      {
        sim -> errorf( "\nUnable to create player %s with class %s.\n",
                       value.c_str(), name.c_str() );
      }
    }
  }

  // Create options for player
  if ( sim -> active_player )
    sim -> active_player -> create_options();

  return sim -> active_player != 0;
}

// parse_proxy ==============================================================

bool parse_proxy( sim_t*             sim,
                         const std::string& /* name */,
                         const std::string& value )
{

  std::vector<std::string> splits = util::string_split( value, "," );

  if ( splits.size() != 3 )
  {
    sim -> errorf( "Expected format is: proxy=type,host,port\n" );
    return false;
  }

  unsigned port = util::str_to_num<unsigned>( splits[ 2 ] );
  if ( splits[ 0 ] == "http" && port > 0 && port < 65536 )
  {
    http::set_proxy( splits[ 0 ], splits[ 1 ], port );
    return true;
  }

  return false;
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


  return true;
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
  bool is_valid_region( const std::string& s )
  { return s.size() == 2; }

public:
  struct error {};
  struct option_error : public error {};

  std::vector<std::string> names;
  std::string region;
  std::string server;
  cache::behavior_e cache;

  names_and_options_t( sim_t* sim, const std::string& context,
                       const option_t* client_options, const std::string& input )
  {
    int use_cache = 0;

    option_t base_options[] =
    {
      opt_string( "region", region ),
      opt_string( "server", server ),
      opt_bool( "cache", use_cache ),
      opt_null()
    };

    std::vector<option_t> options;
    option_t::merge( options, base_options, client_options );

    names = util::string_split( input, "," );

    std::vector<std::string> names2 = names;
    size_t count = 0;
    for ( size_t i = 0; i < names.size(); ++i )
    {
      if ( names[ i ].find( '=' ) != std::string::npos )
      {
        if ( ! option_t::parse( sim, context.c_str(), options, names[ i ] ) )
        {
          throw option_error();
        }
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
      if ( names.size() > 2 && is_valid_region( names[ 0 ] ) )
      {
        region = names[ 0 ];
        server = names[ 1 ];
        names.erase( names.begin(), names.begin() + 2 );
      }
      else
      {
        region = sim -> default_region_str;
      }
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
  try
  {
    std::string spec = "active";

    option_t options[] =
    {
      opt_string( "spec", spec ),
      opt_null()
    };

    names_and_options_t stuff( sim, name, options, value );

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
      if ( name == "wowhead" )
      {
        sim -> errorf( "Wowhead profiler currently not support. "
                       "Wowhead profiler does not provide spec, talent or glyph data.\n" );
        return false;

        //p = wowhead::download_player( sim, stuff.region, stuff.server, player_name, description, wowhead::LIVE, stuff.cache );
      }
      else if ( name == "local_json" )
        p = bcp_api::from_local_json( sim, player_name, stuff.server, description );
      else
        p = bcp_api::download_player( sim, stuff.region, stuff.server,
                                      player_name, description, stuff.cache );

      sim -> active_player = p;
      if ( ! p )
        return false;
    }
  }

  catch ( names_and_options_t::error& )
  { return false; }

  // Create options for player
  if ( sim -> active_player )
    sim -> active_player -> create_options();

  return sim -> active_player != 0;
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

    option_t options[] =
    {
      opt_string( "class", type_str ),
      opt_int( "max_rank", max_rank ),
      opt_string( "ranks", ranks_str ),
      opt_null()
    };

    names_and_options_t stuff( sim, name, options, value );

    std::vector<int> ranks_list;
    if ( ! ranks_str.empty() )
    {
      std::vector<std::string> ranks = util::string_split( ranks_str, "/" );

      for ( size_t i = 0; i < ranks.size(); i++ )
        ranks_list.push_back( atoi( ranks[i].c_str() ) );
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
                               const std::string& name,
                               const std::string& value )
{
  if ( name != "fight_style" ) return false;

  if ( util::str_compare_ci( value, "Patchwerk" ) )
  {
    sim -> fight_style = "Patchwerk";
    sim -> raid_events_str.clear();
  }
  else if ( util::str_compare_ci( value, "Ultraxion" ) )
  {
    sim -> fight_style = "Ultraxion";
    sim -> max_time    = timespan_t::from_seconds( 366.0 );
    sim -> fixed_time  = true;
    sim -> vary_combat_length = 0.0;
    sim -> raid_events_str =  "flying,first=0,duration=500,cooldown=500";
    sim -> raid_events_str +=  "/position_switch,first=0,duration=500,cooldown=500";
    sim -> raid_events_str += "/stun,duration=1.0,first=45.0,period=45.0";
    sim -> raid_events_str += "/stun,duration=1.0,first=57.0,period=57.0";
    sim -> raid_events_str += "/damage,first=6.0,period=6.0,last=59.5,amount=44000,type=shadow";
    sim -> raid_events_str += "/damage,first=60.0,period=5.0,last=119.5,amount=44855,type=shadow";
    sim -> raid_events_str += "/damage,first=120.0,period=4.0,last=179.5,amount=44855,type=shadow";
    sim -> raid_events_str += "/damage,first=180.0,period=3.0,last=239.5,amount=44855,type=shadow";
    sim -> raid_events_str += "/damage,first=240.0,period=2.0,last=299.5,amount=44855,type=shadow";
    sim -> raid_events_str += "/damage,first=300.0,period=1.0,amount=44855,type=shadow";
  }
  else if ( util::str_compare_ci( value, "HelterSkelter" ) || util::str_compare_ci( value, "Helter_Skelter" ) )
  {
    sim -> fight_style = "HelterSkelter";
    sim -> raid_events_str = "casting,cooldown=30,duration=3,first=15";
    sim -> raid_events_str += "/movement,cooldown=30,distance=20";
    sim -> raid_events_str += "/stun,cooldown=60,duration=2";
    sim -> raid_events_str += "/invulnerable,cooldown=120,duration=3";
  }
  else if ( util::str_compare_ci( value, "LightMovement" ) )
  {
    sim -> fight_style = "LightMovement";
    sim -> raid_events_str = "/movement,players_only=1,first=";
    sim -> raid_events_str += util::to_string( int( sim -> max_time.total_seconds() * 0.1 ) );
    sim -> raid_events_str += ",cooldown=85,distance=50,last=";
    sim -> raid_events_str += util::to_string( int( sim -> max_time.total_seconds() * 0.8 ) );
  }
  else if ( util::str_compare_ci( value, "HeavyMovement" ) )
  {
    sim -> fight_style = "HeavyMovement";
    sim -> raid_events_str = "/movement,players_only=1,first=10,cooldown=10,distance=25";
  }
  else if ( util::str_compare_ci( value, "WoD_KillableRaidDummy" ) || util::str_compare_ci( value, "WoD_KillableMythicDummy" ) )
  {
    sim -> fight_style = value;
    sim -> overrides.bloodlust = 0;
    if ( util::str_compare_ci( value, "WoD_KillableRaidDummy" ) )
      sim -> overrides.target_health = 20000000;
    else
      sim -> overrides.target_health = 300000000;
    sim -> target_death_pct = 0;
    sim -> allow_potions = false;
    sim -> vary_combat_length = 0;
    sim -> max_time = timespan_t::from_seconds( 1800 );
    sim -> average_range = false;
    sim -> solo_raid = true;
  }
  else if ( util::str_compare_ci( value, "WoD_RaidDamageDummyCluster" ) )
  {
    sim -> fight_style = value;
    sim -> overrides.bloodlust = 0;
    sim -> allow_potions = false;
    sim -> average_range = false;
    sim -> solo_raid = true;

    // Set the main target to "RaidDamageDummy" and spawn 2 lower level mobs too
    sim -> target = module_t::enemy() -> create_player( sim, "RaidDamageDummy" );

    player_t* add_1 = module_t::enemy() -> create_player( sim, "DungeonDamageDummy1" );
    add_1 -> level = 102;
    player_t* add_2 = module_t::enemy() -> create_player( sim, "DungeonDamageDummy2" );
    add_2 -> level = 102;
  }
  else if ( util::str_compare_ci( value, "WoD_WeakDamageDummyCluster" ) )
  {
    sim -> fight_style = value;
    sim -> overrides.bloodlust = 0;
    sim -> allow_potions = false;
    sim -> average_range = false;
    sim -> solo_raid = true;

    for ( size_t i = 0; i < 5; i++ )
    {
      player_t* dummy = module_t::enemy() -> create_player( sim, "WeakDamageDummy" + util::to_string( i + 1 ) );
      dummy -> level = 100;
      if ( i == 0 )
        sim -> target = dummy;
    }
  }
  else if ( util::str_compare_ci( value, "HecticAddCleave" ) )
  {
    sim -> fight_style = "HecticAddCleave";

    sim -> raid_events_str += "/adds,count=5,first=" + util::to_string( int( sim -> max_time.total_seconds() * 0.05 ) ) + ",cooldown=" + util::to_string( int( sim -> max_time.total_seconds() * 0.075 ) ) + ",duration=" + util::to_string( int( sim -> max_time.total_seconds() * 0.05 ) ) + ",last=" + util::to_string( int( sim -> max_time.total_seconds() * 0.75 ) ); //P1
    sim -> raid_events_str += "/movement,players_only=1,first=" + util::to_string( int( sim -> max_time.total_seconds() * 0.05 ) ) + ",cooldown=" + util::to_string( int( sim -> max_time.total_seconds() * 0.075 ) ) + ",distance=" + util::to_string( int( sim -> max_time.total_seconds() * 0.056 ) ) + ",last=" + util::to_string( int( sim -> max_time.total_seconds() * 0.75 ) ); //move to new position of adds
    sim -> raid_events_str += "/movement,players_only=1,first=" + util::to_string( int( sim -> max_time.total_seconds() * 0.03 ) ) + ",cooldown=" + util::to_string( int( sim -> max_time.total_seconds() * 0.03 ) ) + ",distance=" + util::to_string( std::max( int( sim -> max_time.total_seconds() * 0.021 ), 1 ) ); //move out of stuff

  }
  else
  {
    sim -> out_std.printf( "Custom fight style specified: %s", value.c_str() );
    sim -> fight_style = value;
  }

  return true;
}
// parse_override_spell_data ================================================

bool parse_override_spell_data( sim_t*             sim,
                                       const std::string& /* name */,
                                       const std::string& value )
{
  size_t v_pos = value.find( '=' );

  if ( v_pos == std::string::npos )
    return false;

  std::vector< std::string > splits = util::string_split( value.substr( 0, v_pos ), "." );

  if ( splits.size() != 3 )
    return false;

  unsigned long int id = strtoul( splits[ 1 ].c_str(), 0, 10 );
  if ( id == 0 || id == std::numeric_limits<unsigned long>::max() )
    return false;

  double v = strtod( value.substr( v_pos + 1 ).c_str(), 0 );
  if ( v == std::numeric_limits<double>::min() || v == std::numeric_limits<double>::max() )
    return false;

  if ( util::str_compare_ci( splits[ 0 ], "spell" ) )
  {
    spell_data_t* s = const_cast< spell_data_t* >( sim -> dbc.spell( id ) );
    if ( s == spell_data_t::nil() )
      return false;

    return s -> override_field( splits[ 2 ], v );
  }
  else if ( util::str_compare_ci( splits[ 0 ], "effect" ) )
  {
    spelleffect_data_t* s = const_cast< spelleffect_data_t* >( sim -> dbc.effect( id ) );
    if ( s == spelleffect_data_t::nil() )
      return false;

    return s -> override_field( splits[ 2 ], v );
  }
  else
    return false;
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
    int sq_lvl = strtol( lvl_offset_str.c_str(), 0, 10 );
    if ( sq_lvl < 1 || sq_lvl > MAX_LEVEL )
      return 0;

    sim -> spell_query_level = as< unsigned >( sq_lvl );

    sq_str = sq_str.substr( 0, lvl_offset );
  }

  sim -> spell_query = spell_data_expr_t::parse( sim, sq_str );
  return sim -> spell_query != 0;
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

    sim -> errorf( "Your global data source string \"%s\" contained no valid data sources. Valid identifiers are:%s",
                   value.c_str(), all_known_sources.c_str() );
    return false;
  }

  return true;
}

bool parse_thread_priority( sim_t*             sim,
                                   const std::string& /* name */,
                                   const std::string& value )
{
  sc_thread_t::priority_e pr = sc_thread_t::NORMAL;

  if ( util::str_compare_ci( value, "normal" ) )
  {
    pr = sc_thread_t::NORMAL;
  }
  else if ( util::str_compare_ci( value, "above_normal" ) || util::str_compare_ci( value, "high" ) )
  {
    pr = sc_thread_t::ABOVE_NORMAL;
  }
  else if ( util::str_compare_ci( value, "below_normal" ) || util::str_compare_ci( value, "low" ) )
  {
    pr = sc_thread_t::BELOW_NORMAL;
  }
  else if ( util::str_compare_ci( value, "highest" ) )
  {
    pr = sc_thread_t::HIGHEST;
  }
  else if ( util::str_compare_ci( value, "lowest" ) )
  {
    pr = sc_thread_t::LOWEST;
  }
  else
  {
    sim -> errorf( "Could not set thread priority to %s. Using Normal priority.", value.c_str() );
  }

  sim -> thread_priority = pr;

  return true;
}

bool parse_maximize_reporting( sim_t*             sim,
                                   const std::string& name,
                                   const std::string& v )
{
  if ( v != "0" && v != "1" )
  {
    sim -> errorf( "Acceptable values for '%s' are '1' or '0'\n", name.c_str() );
    return false;
  }
  bool r = atoi( v.c_str() ) != 0;
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
// Proxy cast ===============================================================

struct proxy_cast_check_t : public event_t
{
  int uses;
  const int& _override;
  timespan_t start_time;
  timespan_t cooldown;
  timespan_t duration;

  proxy_cast_check_t( sim_t& s, int u, timespan_t st, timespan_t i, timespan_t cd, timespan_t d, const int& o ) :
    event_t( s, "proxy_cast_check" ),
    uses( u ), _override( o ), start_time( st ), cooldown( cd ), duration( d )
  {
    sim().add_event( this, i );
  }

  virtual bool proxy_check() = 0;
  virtual void proxy_execute() = 0;
  virtual proxy_cast_check_t* proxy_schedule( timespan_t interval ) = 0;

  virtual void execute()
  {
    timespan_t interval = timespan_t::from_seconds( 0.25 );

    if ( uses == _override && start_time > timespan_t::zero() )
      interval = cooldown - ( sim().current_time - start_time );

    if ( proxy_check() )
    {
      // Cooldown over, reset uses
      if ( uses == _override && start_time > timespan_t::zero() && ( sim().current_time - start_time ) >= cooldown )
      {
        start_time = timespan_t::zero();
        uses = 0;
      }

      if ( uses < _override )
      {
        proxy_execute();

        uses++;

        if ( start_time == timespan_t::zero() )
          start_time = sim().current_time;

        interval = duration + timespan_t::from_seconds( 1 );

        if ( sim().debug )
          sim().out_debug.printf( "Proxy-Execute uses=%d total=%d start_time=%.3f next_check=%.3f",
                      uses, _override, start_time.total_seconds(),
                      ( sim().current_time + interval ).total_seconds() );
      }
    }

    proxy_schedule( interval );
  }
};

struct sim_end_event_t : event_t
{
  sim_end_event_t( sim_t& s, const char* n, timespan_t end_time ) :
    event_t( s, n )
  {
    sim().add_event( this, end_time );
  }
  virtual void execute()
  {
    sim().cancel_iteration();
  }
};

/* Forcefully cancel the iteration if it has unexpectedly taken too long
 * to end normally.
 */
struct sim_safeguard_end_event_t : public sim_end_event_t
{
  sim_safeguard_end_event_t( sim_t& s, const char* n, timespan_t end_time ) :
    sim_end_event_t( s, n, end_time )
  { }

  virtual void execute()
  {
    sim().errorf( "Simulation has been forcefully cancelled at %.2f because twice the expected combat length has been exceeded.", sim().current_time.total_seconds() );

    sim_end_event_t::execute();
  }
};

struct resource_timeline_collect_event_t : public event_t
{
  resource_timeline_collect_event_t( sim_t& s ) :
    event_t( s, "resource_timeline_collect_event_t" )
  {
    sim().add_event( this, timespan_t::from_seconds( 1 ) );
  }

  virtual void execute()
  {
    if ( sim().iterations == 1 || sim().current_iteration > 0 )
    {
      // Assumptions: Enemies do not have primary resource regeneration
      for ( size_t i = 0, actors = sim().player_non_sleeping_list.size(); i < actors; i++ )
      {
        player_t* p = sim().player_non_sleeping_list[ i ];
        if ( p -> primary_resource() == RESOURCE_NONE ) continue;

        p -> collect_resource_timeline_information();
      }
      // However, enemies do have health
      for ( size_t i = 0, actors = sim().target_non_sleeping_list.size(); i < actors; i++ )
      {
        player_t* p = sim().target_non_sleeping_list[ i ];
        p -> collect_resource_timeline_information();
      }
    }

    new ( sim() ) resource_timeline_collect_event_t( sim() );
  }
};

struct regen_event_t : public event_t
{
  regen_event_t( sim_t& s ) :
    event_t( s, "Regen Event" )
  {
    if ( sim().debug ) sim().out_debug.printf( "New Regen Event" );

    add_event( sim().regen_periodicity );
  }

  virtual void execute()
  {
    // targets do not get any resource regen for performance reasons
    for ( size_t i = 0, actors = sim().player_non_sleeping_list.size(); i < actors; i++ )
    {
      player_t* p = sim().player_non_sleeping_list[ i ];
      if ( p -> primary_resource() == RESOURCE_NONE ) continue;
      if ( p -> regen_type != REGEN_STATIC ) continue;

      p -> regen( sim().regen_periodicity );
    }

    new ( sim() ) regen_event_t( sim() );
  }
};

} // UNNAMED NAMESPACE ===================================================

// ==========================================================================
// Simulator
// ==========================================================================

// sim_t::sim_t =============================================================

sim_t::sim_t( sim_t* p, int index ) :
  control( 0 ),
  parent( p ),
  initialized( false ),
  paused( false ),
  target( NULL ),
  heal_target( NULL ),
  target_list(),
  target_non_sleeping_list(),
  player_list(),
  player_no_pet_list(),
  player_non_sleeping_list(),
  active_player( 0 ),
  num_players( 0 ),
  num_enemies( 0 ), healing( 0 ),
  global_spawn_index( 0 ),
  max_player_level( -1 ),
  queue_lag( timespan_t::from_seconds( 0.005 ) ), queue_lag_stddev( timespan_t::zero() ),
  gcd_lag( timespan_t::from_seconds( 0.150 ) ), gcd_lag_stddev( timespan_t::zero() ),
  channel_lag( timespan_t::from_seconds( 0.250 ) ), channel_lag_stddev( timespan_t::zero() ),
  queue_gcd_reduction( timespan_t::from_seconds( 0.032 ) ), strict_gcd_queue( 0 ),
  confidence( 0.95 ), confidence_estimator( 0.0 ),
  world_lag( timespan_t::from_seconds( 0.1 ) ), world_lag_stddev( timespan_t::min() ),
  travel_variance( 0 ), default_skill( 1.0 ), reaction_time( timespan_t::from_seconds( 0.5 ) ),
  regen_periodicity( timespan_t::from_seconds( 0.25 ) ),
  ignite_sampling_delta( timespan_t::from_seconds( 0.2 ) ),
  fixed_time( false ),
  seed( 0 ), current_slot( -1 ),
  optimal_raid( 0 ), log( 0 ), debug_each( 0 ), save_profiles( 0 ), default_actions( 0 ),
  normalized_stat( STAT_NONE ),
  default_region_str( "us" ),
  save_prefix_str( "save_" ),
  save_talent_str( 0 ),
  talent_format( TALENT_FORMAT_UNCHANGED ),
  auto_ready_trigger( 0 ), stat_cache( 1 ), max_aoe_enemies( 20 ), show_etmi( 0 ), tmi_window_global( 0 ), tmi_bin_size( 0.5 ),
  target_death_pct( 0 ), rel_target_level( 0 ), target_level( -1 ), target_adds( 0 ), desired_targets( 0 ), enable_taunts( false ),
  challenge_mode( false ), scale_to_itemlevel ( -1 ),
  active_enemies( 0 ), active_allies( 0 ),
  deterministic_rng( false ),
  average_range( true ), average_gauss( false ),
  convergence_scale( 2 ),
  fight_style( "Patchwerk" ), overrides( overrides_t() ), auras( auras_t() ),
  default_aura_delay( timespan_t::from_millis( 30 ) ),
  default_aura_delay_stddev( timespan_t::from_millis( 5 ) ),
  progress_bar( *this ),
  scaling( new scaling_t( this ) ),
  plot( new plot_t( this ) ),
  reforge_plot( new reforge_plot_t( this ) ),
  elapsed_cpu( 0.0 ),
  elapsed_time( 0.0 ),
  iteration_dmg( 0 ), iteration_heal( 0 ), iteration_absorb( 0 ),
  raid_dps(), total_dmg(), raid_hps(), total_heal(), total_absorb(), raid_aps(),
  simulation_length( "Simulation Length", false ),
  report_progress( 1 ),
  bloodlust_percent( 25 ), bloodlust_time( timespan_t::from_seconds( 0.5 ) ),
  // Report
  report_precision( 2 ), report_pets_separately( 0 ), report_targets( 1 ), report_details( 1 ), report_raw_abilities( 1 ),
  report_rng( 0 ), hosted_html( 0 ), print_styles( 0 ),
  save_raid_summary( 0 ), save_gear_comments( 0 ), statistics_level( 1 ), separate_stats_by_actions( 0 ), report_raid_summary( 0 ), buff_uptime_timeline( 0 ),
  wowhead_tooltips( -1 ),
  allow_potions( true ),
  allow_food( true ),
  allow_flasks( true ),
  solo_raid( false ),
  global_item_upgrade_level( 0 ),
  maximize_reporting( false ),
  report_information(),
  // Multi-Threading
  threads( 0 ), thread_index( index ), thread_priority( sc_thread_t::NORMAL ), work_queue(),
  spell_query( 0 ), spell_query_level( MAX_LEVEL ),
  pause_cvar( &pause_mutex ),
  requires_regen_event( false )
{
  item_db_sources.assign( range::begin( default_item_db_sources ),
                          range::end( default_item_db_sources ) );


  max_time = timespan_t::from_seconds( 450 );
  vary_combat_length = 0.2;

  use_optimal_buffs_and_debuffs( 1 );

  create_options();

  if ( parent )
  {
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

    seed = parent -> seed;
  }
}

// sim_t::~sim_t ============================================================

sim_t::~sim_t()
{
  delete scaling;
  delete plot;
  delete reforge_plot;
  delete spell_query;
}

// sim_t::combat ============================================================

void sim_t::combat( int iteration )
{
  if ( debug_each )
  {
    // On Debug Each, we collect debug information for each iteration, but clear it before each one
    std::shared_ptr<io::ofstream> o(new io::ofstream());
    o -> open( output_file_str );
    if ( o -> is_open() )
    {
      out_std = o;
      out_debug = o;
      out_log = o;
      out_error = o;

      out_std.printf( "------ Iteration #%i ------", iteration + 1 );
      std::flush( *out_std.get_stream() );
    }
    else
    {
      errorf( "Unable to open output file '%s'\n", output_file_str.c_str() );
      cancel();
    }
  }

  if ( ! canceled )
    core_sim_t::combat( iteration );

  if ( debug_each && ! canceled )
    static_cast<io::ofstream*>(out_std.get_stream()) -> close();
}

// sim_t::reset =============================================================

void sim_t::reset()
{
  core_sim_t::reset();

  for ( size_t i = 0; i < buff_list.size(); ++i )
    buff_list[ i ] -> reset();

  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    player_t* t = target_list[ i ];
    t -> reset();
  }
  for ( size_t i = 0; i < player_no_pet_list.size(); ++i )
  {
    player_t& p = *player_no_pet_list[ i ];
    p.reset();
    // Make sure to reset pets after owner, or otherwards they may access uninitialized things from the owner
    for ( size_t j = 0; j < p.pet_list.size(); ++j )
    {
      pet_t& pet = *p.pet_list[ j ];
      pet.reset();
    }
  }
  raid_event_t::reset( this );
}

// sim_t::combat_begin ======================================================

void sim_t::combat_begin()
{
  core_sim_t::combat_begin();

  iteration_dmg = iteration_heal = 0;

  // Always call begin() to ensure various counters are initialized.
  datacollection_begin();

  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    player_t* t = target_list[ i ];
    t -> combat_begin();
  }

  if ( overrides.attack_power_multiplier ) auras.attack_power_multiplier -> override_buff();
  if ( overrides.critical_strike         ) auras.critical_strike         -> override_buff();

  if ( overrides.mastery                 )
    auras.mastery -> override_buff( 1, dbc.effect_average( dbc.spell( 116956 ) -> effectN( 1 ).id(), max_player_level ) );

  if ( overrides.haste                   ) auras.haste                   -> override_buff();
  if ( overrides.multistrike             ) auras.multistrike             -> override_buff();
  if ( overrides.spell_power_multiplier  ) auras.spell_power_multiplier  -> override_buff();
  if ( overrides.stamina                 ) auras.stamina                 -> override_buff();
  if ( overrides.str_agi_int             ) auras.str_agi_int             -> override_buff();
  if ( overrides.versatility             ) auras.versatility             -> override_buff();

  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    player_t* t = target_list[ i ];
    if ( overrides.mortal_wounds          ) t -> debuffs.mortal_wounds          -> override_buff();
    if ( overrides.bleeding               ) t -> debuffs.bleeding               -> override_buff( 1, 1.0 );
  }

  for ( player_e i = PLAYER_NONE; i < PLAYER_MAX; ++i )
  {
    const module_t* m = module_t::get( i );
    if ( m ) m -> combat_begin( this );
  }

  raid_event_t::combat_begin( this );

  for ( size_t i = 0; i < player_list.size(); ++i )
  {
    player_t* p = player_list[ i ];
    p -> combat_begin();
  }

  if ( requires_regen_event )
    new ( *this ) regen_event_t( *this );

  if ( overrides.bloodlust )
  {
    // Setup a periodic check for Bloodlust

    struct bloodlust_check_t : public event_t
    {
      bloodlust_check_t( sim_t& sim ) :
        event_t( sim, "Bloodlust Check" )
      {
        add_event( timespan_t::from_seconds( 1.0 ) );
      }
      virtual void execute()
      {
        sim_t& sim = this -> sim();
        player_t* t = sim.target;
        if ( ( sim.bloodlust_percent  > 0                  && t -> health_percentage() <  sim.bloodlust_percent ) ||
             ( sim.bloodlust_time     < timespan_t::zero() && t -> time_to_die()       < -sim.bloodlust_time ) ||
             ( sim.bloodlust_time     > timespan_t::zero() && sim.current_time      >  sim.bloodlust_time ) )
        {
          for ( size_t i = 0; i < sim.player_non_sleeping_list.size(); ++i )
          {
            player_t* p = sim.player_non_sleeping_list[ i ];
            if ( p -> buffs.exhaustion -> check() || p -> is_pet() || p -> is_enemy() )
              continue;

            p -> buffs.bloodlust -> trigger();
            p -> buffs.exhaustion -> trigger();
          }
        }
        else
        {
          new ( sim ) bloodlust_check_t( sim );
        }
      }
    };

    new ( *this ) bloodlust_check_t( *this );
  }

  cancel_iteration( false );

  if ( fixed_time || ( target -> resources.base[ RESOURCE_HEALTH ] == 0 ) )
  {
    new ( *this ) sim_end_event_t( *this, "sim_end_expected_time", expected_iteration_time );
    target -> death_pct = target_death_pct;
  }
  else
  {
    target -> death_pct = target_death_pct;
  }
  new ( *this ) sim_safeguard_end_event_t( *this, "sim_end_twice_expected_time", expected_iteration_time + expected_iteration_time );
}

// sim_t::combat_end ========================================================

void sim_t::combat_end()
{
  core_sim_t::combat_end();

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

  for ( size_t i = 0; i < player_no_pet_list.size(); ++i )
  {
    player_t* p = player_no_pet_list[ i ];
    p -> combat_end();
  }

  for ( size_t i = 0; i < buff_list.size(); ++i )
  {
    buff_t* b = buff_list[ i ];
    b -> expire();
  }

  if ( iterations == 1 || current_iteration >= 1 )
    datacollection_end();

  assert( active_enemies == 0 );
  assert( active_allies == 0 );
}

// sim_t::datacollection_begin ==============================================

void sim_t::datacollection_begin()
{
  if ( debug ) out_debug << "Sim Data Collection Begin";

  iteration_dmg = iteration_heal = iteration_absorb = 0.0;

  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    player_t* t = target_list[ i ];
    if ( t -> is_add() ) continue;
    t -> datacollection_begin();
  }

  for ( size_t i = 0; i < buff_list.size(); ++i )
    buff_list[ i ] -> datacollection_begin();

  for ( size_t i = 0; i < player_no_pet_list.size(); ++i )
  {
    player_t* p = player_no_pet_list[ i ];
    p -> datacollection_begin();
  }

  new ( *this ) resource_timeline_collect_event_t( *this );
}

// sim_t::datacollection_end ================================================

void sim_t::datacollection_end()
{
  if ( debug ) out_debug << "Sim Data Collection End";

  simulation_length.add( current_time.total_seconds() );

  for ( size_t i = 0; i < target_list.size(); ++i )
  {
    player_t* t = target_list[ i ];
    if ( t -> is_add() ) continue;
    t -> datacollection_end();
  }
  raid_event_t::combat_end( this );

  for ( size_t i = 0; i < player_no_pet_list.size(); ++i )
  {
    player_t* p = player_no_pet_list[ i ];
    p -> datacollection_end();
  }

  for ( size_t i = 0; i < buff_list.size(); ++i )
  {
    buff_t* b = buff_list[ i ];
    b -> datacollection_end();
  }

  total_dmg.add( iteration_dmg );
  raid_dps.add( current_time != timespan_t::zero() ? iteration_dmg / current_time.total_seconds() : 0 );
  total_heal.add( iteration_heal );
  raid_hps.add( current_time != timespan_t::zero() ? iteration_heal / current_time.total_seconds() : 0 );
  total_absorb.add( iteration_absorb );
  raid_aps.add( current_time != timespan_t::zero() ? iteration_absorb / current_time.total_seconds() : 0 );
}

// sim_t::check_actors() ========================================================
// This function just checks that the sim has at least 1 active player, and sets
// fixed_time automatically if the only role present is a ROLE_HEAL.  Called in sim_t::init_actors()

bool sim_t::check_actors()
{
  bool too_quiet = true; // Check for at least 1 active player
  bool zero_dds = true; // Check for at least 1 player != TANK/HEAL

  for ( size_t i = 0; i < actor_list.size(); i++ )
  {
    player_t* p = actor_list[ i ];
    if ( p -> is_pet() || p -> is_enemy() ) continue;
    if ( p -> type == HEALING_ENEMY ) continue;
    if ( ! p -> quiet ) too_quiet = false;
    if ( p -> primary_role() != ROLE_HEAL && ! p -> is_pet() ) zero_dds = false;
  }
  
  if ( too_quiet && ! debug )
  {
    errorf( "No active players in sim!" );
    return false;
  }

  // Set Fixed_Time when there are no DD's present
  if ( zero_dds && ! debug )
    fixed_time = true;

  return true;
}

// sim_t::init_parties =============================================================
// This function... builds parties? I guess it assigns each player in player_list to
// a party based on party_encoding. Called in sim_t::init_actors()

bool sim_t::init_parties()
{
  // Parties
  if ( debug )
    out_debug.printf( "Building Parties." );

  int party_index = 0;
  for ( size_t i = 0; i < party_encoding.size(); i++ )
  {
    std::string& party_str = party_encoding[ i ];

    if ( party_str == "reset" )
    {
      party_index = 0;
      for ( size_t j = 0; j < player_list.size(); ++j )
        player_list[ j ] -> party = 0;
    }
    else if ( party_str == "all" )
    {
      for ( size_t j = 0; j < player_list.size(); ++j )
      {
        player_t* p = player_list[ j ];
        p -> party = 1;
      }
    }
    else
    {
      party_index++;

      std::vector<std::string> player_names = util::string_split( party_str, ",;/" );

      for ( size_t j = 0; j < player_names.size(); j++ )
      {
        player_t* p = find_player( player_names[ j ] );
        if ( ! p )
        {
          errorf( "Unable to find player %s for party creation.\n", player_names[ j ].c_str() );
          return false;
        }
        p -> party = party_index;
        for ( size_t k = 0; k < p -> pet_list.size(); ++k )
        {
          pet_t* pet = p -> pet_list[ k ];
          pet -> party = party_index;
        }
      }
    }
  }

  return true;
}

// init_items ===============================================================
// This is a helper function that loops through each actor and calls its respective
// player_t::init_items() method. 

bool sim_t::init_items()
{  
  bool success = true;

  for ( size_t i = 0; i < actor_list.size(); i++ )
  {
    if ( ! actor_list[ i ] -> init_items() )
      success = false;
  }
  return success;
}

// init_actions =============================================================
// This is a helper function that loops through each actor and calls its respective
// player_t::init_actions() method.

bool sim_t::init_actions()
{
  bool success = true;

  for ( size_t i = 0; i < actor_list.size(); i++ )
  {
    if ( ! actor_list[ i ] -> init_actions() )
      success = false;
  }
  return success;
  
}

// sim_t::init_actors =======================================================
// This method handles the bulk of player initialization. Order is pretty 
// critical here. Called in sim_t::init()

bool sim_t::init_actors()
{
  
  // create actor entries for pets
  if ( debug )
    out_debug.printf( "Creating Pets." );

  for ( size_t i = 0; i < actor_list.size(); i++ )
  {
    player_t* p = actor_list[i];
    p -> create_pets();    
  }

  // initialize class/enemy modules
  for ( player_e i = PLAYER_NONE; i < PLAYER_MAX; ++i )
  {
    const module_t* m = module_t::get( i );
    if ( m ) m -> init( this );
  }

  if ( debug )
    out_debug.printf( "Initializing Players." );

  // call each actor's init() function; mostly does APL stuff, see player_t::init()
  for ( size_t i = 0; i < actor_list.size(); i++ )
  {
    player_t* p = actor_list[ i ];
    if ( default_actions && ! p -> is_pet() )
    {
      p -> clear_action_priority_lists();
      p -> action_list_str.clear();
    };
    p -> init();
    p -> initialized = true;
  }

  if ( wowhead_tooltips == -1 )
    wowhead_tooltips = player_no_pet_list.size() <= 10;

  // This next section handles all the ugly details of initialization. Ideally, each of these
  // init_* methods will eventually return a bool to indicate success or failure, from which
  // we can either continue or halt initialization.
  // For now, we're only enforcing this condition for the particular init_* methods that can
  // lead to a sim -> cancel() result ( player_t::init_items() and player_t::init_actions() ).

  // Determine Spec, Talents, Professions, Glyphs
  range::for_each( actor_list, std::mem_fn( &player_t::init_target ) );
  range::for_each( actor_list, std::mem_fn( &player_t::init_character_properties ) );

  // Initialize each actor's items, construct gear information & stats
  if ( ! init_items() )
    return false;

  // Initialize spells, base/initial stats, defense, buffs, scaling, special effects
  range::for_each( actor_list, std::mem_fn( &player_t::init_spells ) );
  range::for_each( actor_list, std::mem_fn( &player_t::init_base_stats ) );
  range::for_each( actor_list, std::mem_fn( &player_t::init_initial_stats ) );
  range::for_each( actor_list, std::mem_fn( &player_t::init_defense ) );
  range::for_each( actor_list, std::mem_fn( &player_t::create_buffs ) ); // keep here for now
  range::for_each( actor_list, std::mem_fn( &player_t::init_scaling ) );
  range::for_each( actor_list, std::mem_fn( &player_t::init_special_effects ) ); // Must be before init_actions
  range::for_each( actor_list, std::mem_fn( &player_t::register_callbacks ) ); // Must be before init_actions

  // Initialize each actor's actions
  if ( ! init_actions() )
    return false;

  // Initialize gains, procs, uptimes, benefits, rng, stats
  range::for_each( actor_list, std::mem_fn( &player_t::init_gains ) );
  range::for_each( actor_list, std::mem_fn( &player_t::init_procs ) );
  range::for_each( actor_list, std::mem_fn( &player_t::init_uptimes ) );
  range::for_each( actor_list, std::mem_fn( &player_t::init_benefits ) );
  range::for_each( actor_list, std::mem_fn( &player_t::init_rng ) );
  range::for_each( actor_list, std::mem_fn( &player_t::init_stats ) );

  // check that we have at least one active player
  if ( ! check_actors() )
    return false;

  // organize parties if necessary
  if ( ! init_parties() )
    return false;
/*
  // Callbacks
  if ( debug )
    out_debug.printf( "Registering Callbacks." );

  for ( size_t i = 0; i < actor_list.size(); i++ )
  {
    player_t* p = actor_list[ i ];
    p -> register_callbacks();
  }
*/
  // If we make it here, everything initialized properly and we can return true to sim_t::init()
  return true;
}

// sim_t::init ==============================================================

bool sim_t::init()
{
  if ( initialized )
    return true;

  // Seed RNG
  if ( seed == 0 )
  {
    int64_t sec, usec;
    stopwatch_t sw( STOPWATCH_WALL );
    sw.now( &sec, &usec );
    int seed_ = static_cast< int >( sec * 1000 );
    seed_ += static_cast< int >( usec / 1000.0 );
    seed = deterministic_rng ? 31459 : seed_;
  }
  _rng.seed( seed + thread_index );

  if ( ! core_sim_t::init() )
    return false;

  if (   queue_lag_stddev == timespan_t::zero() )   queue_lag_stddev =   queue_lag * 0.25;
  if (     gcd_lag_stddev == timespan_t::zero() )     gcd_lag_stddev =     gcd_lag * 0.25;
  if ( channel_lag_stddev == timespan_t::zero() ) channel_lag_stddev = channel_lag * 0.25;
  if ( world_lag_stddev    < timespan_t::zero() ) world_lag_stddev   =   world_lag * 0.1;

  if ( challenge_mode && scale_to_itemlevel < 0 ) scale_to_itemlevel = 620; //Check later


  // WoD aura initialization

  // Attack Power Multiplier, value from Trueshot Aura (id=19506) (Hunter)
  auras.attack_power_multiplier = buff_creator_t( this, "attack_power_multiplier" )
                                  .max_stack( 100 )
                                  .default_value( dbc.spell( 19506 ) -> effectN( 1 ).percent() )
                                  .add_invalidate( CACHE_ATTACK_POWER );

  // Critical Strike, value from Arcane Brilliance (id=1459) (Mage)
  auras.critical_strike = buff_creator_t( this, "critical_strike" )
                          .max_stack( 100 )
                          .default_value( dbc.spell( 1459 ) -> effectN( 2 ).percent() )
                          .add_invalidate( CACHE_CRIT );

  // Mastery, value from Grace of Air (id=116956) (Shaman)
  auras.mastery = buff_creator_t( this, "mastery" )
                  .max_stack( 100 )
                  .default_value( dbc.spell( 116956 ) -> effectN( 1 ).base_value() )
                  .add_invalidate( CACHE_MASTERY );

  // Haste, value from Mind Quickening (id=49868) (Priest)
  auras.haste = buff_creator_t( this, "haste" )
                      .max_stack( 100 )
                      .default_value( dbc.spell( 49868 ) -> effectN( 1 ).percent() )
                      .add_invalidate( CACHE_HASTE );

  // Multistrike, value from Mind Quickening (id=49868) (Priest)
  auras.multistrike = buff_creator_t( this, "multistrike" )
                      .max_stack( 100 )
                      .default_value( dbc.spell( 49868 ) -> effectN( 2 ).percent() )
                      .add_invalidate( CACHE_MULTISTRIKE );

  // Spell Power Multiplier, value from Arcane Brilliance (id=1459) (Mage)
  auras.spell_power_multiplier = buff_creator_t( this, "spell_power_multiplier" )
                                 .max_stack( 100 )
                                 .default_value( dbc.spell( 1459 ) -> effectN( 1 ).percent() )
                                 .add_invalidate( CACHE_SPELL_POWER );

  // Stamina, value from fortitude (id=21562) (Priest)
  auras.stamina = buff_creator_t( this, "stamina" )
                  .max_stack( 100 )
                  .default_value( dbc.spell( 21562 ) -> effectN( 1 ).percent() )
                  .add_invalidate( CACHE_STAMINA );

  // Strength, Agility, and Intellect, value from Blessing of Kings (id=20217) (Paladin)
  auras.str_agi_int = buff_creator_t( this, "str_agi_int" )
                      .max_stack( 100 )
                      .default_value( dbc.spell( 20217 ) -> effectN( 1 ).percent() )
                      .add_invalidate( CACHE_STRENGTH )
                      .add_invalidate( CACHE_AGILITY )
                      .add_invalidate( CACHE_INTELLECT );

  // Versatility from Inspiring Presence - ID=167188 (Warrior)
  // Warriors will have it.
  auras.versatility = buff_creator_t( this, "versatility" )
                      .max_stack( 100 )
                      .default_value( dbc.spell( 167188 ) -> effectN ( 1 ).percent() )
                      .add_invalidate( CACHE_VERSATILITY );

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
    active_player = 0;
    active_player = module_t::enemy() -> create_player( this, "enemy" + util::to_string( target_list.size() + 1 ) );
    if ( ! active_player )
    {
      errorf( "\nUnable to create enemy %d.\n", (unsigned)target_list.size() );
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
      heal_target = module_t::heal_enemy() -> create_player( this, "Healing_Target" );
    if ( healing > 1 )
    {
      int targets_create = healing;
      do
      {
        heal_target = module_t::heal_enemy() -> create_player( this, "Healing_Target_" + util::to_string( targets_create ) );
        targets_create--;
      }
      while ( targets_create > 1 );
    }
  }

  if ( max_player_level < 0 )
  {
    for ( size_t i = 0; i < player_no_pet_list.size(); ++i )
    {
      player_t* p = player_no_pet_list[ i ];
      if ( max_player_level < p -> level )
        max_player_level = p -> level;
    }
  }

  raid_event_t::init( this );

  // Initialize actors
  if ( ! init_actors() ) return false;

  if ( report_precision < 0 ) report_precision = 2;

  simulation_length.reserve( iterations );

  initialized = true;

  for ( size_t i = 0, end = player_list.size(); i < end; i++ )
  {
    if ( player_list[ i ] -> regen_type == REGEN_STATIC )
    {
      requires_regen_event = true;
      break;
    }
  }

  return canceled ? false : true;
}

// compare_dps ==============================================================

struct compare_dps
{
  bool operator()( player_t* l, player_t* r ) const
  {
    return l -> collected_data.dps.mean() > r -> collected_data.dps.mean();
  }
};

// compare_hps ==============================================================

struct compare_hps
{
  bool operator()( player_t* l, player_t* r ) const
  {
    return l -> collected_data.hps.mean() > r -> collected_data.hps.mean();
  }
};

// compare_hps_plus_aps ====================================================

struct compare_hps_plus_aps
{
  bool operator()( player_t* l, player_t* r ) const
  {
    return ( l -> collected_data.hps.mean() + l -> collected_data.aps.mean() ) > ( r -> collected_data.hps.mean() + r -> collected_data.aps.mean() );
  }
};

// compare_dtps ==============================================================

struct compare_dtps
{
  bool operator()( player_t* l, player_t* r ) const
  {
    return l -> collected_data.dtps.mean() < r -> collected_data.dtps.mean();
  }
};

// compare_tmi==============================================================

struct compare_tmi
{
  bool operator()( player_t* l, player_t* r ) const
  {
    return l -> collected_data.theck_meloree_index.mean() < r -> collected_data.theck_meloree_index.mean();
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


// sim_t::analyze ===========================================================

void sim_t::analyze()
{
  simulation_length.analyze_all();
  if ( simulation_length.mean() == 0 ) return;

  // divisor_timeline is necessary because not all iterations go the same length of time
  int max_buckets = ( int ) simulation_length.max() + 1;
  divisor_timeline.assign( max_buckets, 0 );

  size_t num_timelines = simulation_length.data().size();
  for ( size_t i = 0; i < num_timelines; i++ )
  {
    int last = ( int ) floor( simulation_length.data()[ i ] );
    assert( last < ( int ) divisor_timeline.size() ); // We created it with max length
    for ( int j = 0; j <= last; j++ ) divisor_timeline[ j ] += 1;
  }

  for ( size_t i = 0; i < buff_list.size(); ++i )
    buff_list[ i ] -> analyze();

  confidence_estimator = rng::stdnormal_inv( 1.0 - ( 1.0 - confidence ) / 2.0 );

  for ( size_t i = 0; i < actor_list.size(); i++ )
    actor_list[ i ] -> analyze( *this );

  range::sort( players_by_dps,  compare_dps() );
  range::sort( players_by_hps,  compare_hps() );
  range::sort( players_by_hps_plus_aps, compare_hps_plus_aps() );
  range::sort( players_by_dtps, compare_dtps() );
  range::sort( players_by_tmi,  compare_tmi() );
  range::sort( players_by_name, compare_name() );
  range::sort( targets_by_name, compare_name() );
}

// progress_bar_t ===========================================================

progress_bar_t::progress_bar_t( sim_t& s ) :
  sim( s ), steps( 20 ), updates( 100 ), interval( 0 ), start_time( 0 ) {}

void progress_bar_t::init()
{
  start_time = util::wall_time();
  interval = sim.iterations / updates;
  if ( interval == 0 ) interval = 1;
}

bool progress_bar_t::update( bool finished )
{
  if ( sim.thread_index != 0 ) return false;
  if ( ! sim.report_progress ) return false;
  if ( ! sim.current_iteration ) return false;

  if ( sim.current_iteration < ( sim.iterations - 1 ) )
    if ( sim.current_iteration % interval ) return false;

  int current, _final;
  double pct = sim.progress( &current, &_final );
  if ( pct <= 0 ) return false;
  if ( finished ) pct = 1.0;

  size_t prev_size = status.size();

  status = "[";
  status.insert( 1, steps, '.' );
  status += "]";

  int length = ( int ) ( steps * pct + 0.5 );
  for ( int i = 1; i < length + 1; i++ ) status[ i ] = '=';
  if ( length > 0 ) status[ length ] = '>';

  double current_time = util::wall_time() - start_time;
  double total_time = current_time / pct;

  int remaining_sec = int( total_time - current_time );
  int remaining_min = remaining_sec / 60;
  remaining_sec -= remaining_min * 60;

  char buffer[80];
  snprintf( buffer, sizeof( buffer ), " %d/%d", finished ? _final : current, _final );
  status += buffer;

  if ( remaining_min > 0 )
  {
    snprintf( buffer, sizeof( buffer ), " %dmin", remaining_min );
    status += buffer;
  }

  if ( remaining_sec > 0 )
  {
    snprintf( buffer, sizeof( buffer ), " %dsec", remaining_sec );
    status += buffer;
  }

  if ( prev_size > status.size()  ) status.insert( status.end(), ( prev_size - status.size() ), ' ' );

  return true;
}

// sim_t::iterate ===========================================================

bool sim_t::iterate()
{
  if ( ! init() ) return false;

  progress_bar.init();

  bool use_lb = use_load_balancing();

  for( int i = 0; use_lb ? (true) : (i < iterations); ++i )
  {
    do_pause();

    if ( canceled )
    {
      iterations = current_iteration + 1;
      break;
    }

    if ( use_lb ) // Load Balancing
    {
      // Select the work queue on the main thread
      work_queue_t& work_queue = (thread_index != 0) ? parent -> work_queue : this -> work_queue;

      auto_lock_t( work_queue.mutex );


      // Check whether we have work left to continue or not
      if ( work_queue.iterations_to_process > 0 )
      {
        // We're good to go for another iteration
        --work_queue.iterations_to_process;
      }
      else
      {
        // No more work left to do, break
        break;
      }
    }

    if ( progress_bar.update() )
    {
      util::fprintf( stdout, "%s %s\r", sim_phase_str.c_str(), progress_bar.status.c_str() );
      fflush( stdout );
    }
    combat( i );
  }

  if ( progress_bar.update( true ) )
  {
    util::fprintf( stdout, "%s %s\n", sim_phase_str.c_str(), progress_bar.status.c_str() );
    fflush( stdout );
  }

  if ( ! canceled )
    reset();

  return ! canceled;
}

// sim_t::merge =============================================================

void sim_t::merge( sim_t& other_sim )
{
  auto_lock_t auto_lock( mutex );

  iterations             += other_sim.iterations;
  total_events_processed += other_sim.total_events_processed;

  simulation_length.merge( other_sim.simulation_length );
  total_dmg.merge( other_sim.total_dmg );
  raid_dps.merge( other_sim.raid_dps );
  total_heal.merge( other_sim.total_heal );
  raid_hps.merge( other_sim.raid_hps );
  total_absorb.merge( other_sim.total_absorb );
  raid_aps.merge( other_sim.raid_aps );

  if ( max_events_remaining < other_sim.max_events_remaining ) max_events_remaining = other_sim.max_events_remaining;

  for ( size_t i = 0; i < buff_list.size(); ++i )
  {
    if ( buff_t* otherbuff = buff_t::find( &other_sim, buff_list[ i ] -> name_str.c_str() ) )
      buff_list[ i ] -> merge( *otherbuff );
  }

  for ( size_t i = 0; i < actor_list.size(); i++ )
  {
    player_t* p = actor_list[ i ];
    player_t* other_p = other_sim.find_player( p -> index );
    assert( other_p );
    p -> merge( *other_p );
  }
}

// sim_t::merge =============================================================

void sim_t::merge()
{
  if ( children.empty() ) return;

  mutex.unlock();

  for ( size_t i = 0; i < children.size(); i++ )
  {
    sim_t* child = children[ i ];
    if ( child )
    {
      child -> wait();
      children[ i ] = 0;
      delete child;
    }
  }

  children.clear();
}

// sim_t::run ===============================================================

void sim_t::run()
{
  iterate();
  if ( total_events_processed > 0 )
    parent -> merge( *this );
}

// sim_t::partition =========================================================

void sim_t::partition()
{
  if ( threads <= 1 ) return;
  if ( iterations < threads ) return;

#if defined( NO_THREADS )
  errorf( "simulationcraft: This executable was built without thread support, please remove 'threads=N' from config file.\n" );
  exit( 0 );
#endif

  mutex.lock(); // parent sim is locked until parent merge() is called

  int remainder = iterations % threads;
  iterations /= threads;

  int num_children = threads - 1;

  for ( int i = 0; i < num_children; i++ )
  {
    sim_t* child = new sim_t( this, i + 1 );
    if( child )
    {
      children.push_back( child );

      child -> iterations = iterations;
      if ( remainder )
      {
        child -> iterations += 1;
        remainder--;
      }

      child -> report_progress = 0;
    }
  }

  sc_thread_t::set_calling_thread_priority( thread_priority ); // Set main thread priority

  for ( int i = 0; i < num_children; i++ )
    children[ i ] -> launch( thread_priority );
}

// sim_t::execute ===========================================================

bool sim_t::execute()
{
  {
    auto_lock_t( work_queue.mutex );
    work_queue.iterations_to_process = iterations;
  }

  double start_cpu_time = util::cpu_time();
  double start_time = util::wall_time();

  partition();

  bool iterate_successfull = iterate();
  merge(); // Always merge, even in cases of unsuccessful simulation!
  if ( !iterate_successfull )
    return false;

  analyze();

  elapsed_cpu =  util::cpu_time() - start_cpu_time;
  elapsed_time =  util::wall_time() - start_time;

  return true;
}

// sim_t::find_player =======================================================

player_t* sim_t::find_player( const std::string& name )
{
  for ( size_t i = 0, actors = actor_list.size(); i < actors; ++i )
  {
    player_t* p = actor_list[ i ];
    if ( name == p -> name() )
      return p;
  }
  return nullptr;
}

// sim_t::find_player =======================================================

player_t* sim_t::find_player( int index )
{
  for ( size_t i = 0, actors = actor_list.size(); i < actors; ++i )
  {
    player_t* p = actor_list[ i ];
    if ( index == p -> index )
      return p;
  }
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

  overrides.attack_power_multiplier = optimal_raid;
  overrides.critical_strike         = optimal_raid;
  overrides.mastery                 = optimal_raid;
  overrides.haste                   = optimal_raid;
  overrides.multistrike             = optimal_raid;
  overrides.spell_power_multiplier  = optimal_raid;
  overrides.stamina                 = optimal_raid;
  overrides.str_agi_int             = optimal_raid;
  overrides.versatility             = optimal_raid;

  overrides.mortal_wounds           = optimal_raid;
  overrides.bleeding                = optimal_raid;

  overrides.bloodlust               = optimal_raid;
}

// sim_t::time_to_think =====================================================

bool sim_t::time_to_think( timespan_t proc_time )
{
  if ( proc_time == timespan_t::zero() ) return false;
  if ( proc_time < timespan_t::zero() ) return true;
  return current_time - proc_time > reaction_time;
}

// sim_t::create_expression =================================================

expr_t* sim_t::create_expression( action_t* a,
                                  const std::string& name_str )
{
  if ( name_str == "time" )
    return make_ref_expr( name_str, current_time );

  if ( util::str_compare_ci( name_str, "enemies" ) )
    return make_ref_expr( name_str, num_enemies );

  if ( util::str_compare_ci( name_str, "active_enemies" ) )
    return make_ref_expr( name_str, active_enemies );

  if ( util::str_compare_ci( name_str, "active_allies" ) )
    return make_ref_expr( name_str, active_allies );

  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( splits.size() == 3 )
  {
    if ( splits[ 0 ] == "aura" )
    {
      buff_t* buff = buff_t::find( this, splits[ 1 ] );
      if ( ! buff ) return 0;
      return buff_t::create_expression( splits[ 1 ], a, splits[ 2 ], buff );
    }
  }
  if ( splits.size() >= 3 && splits[ 0 ] == "actors" )
  {
    player_t* actor = sim_t::find_player( splits[ 1 ] );
    if ( ! target ) return 0;
    std::string rest = splits[ 2 ];
    for ( size_t i = 3; i < splits.size(); ++i )
      rest += '.' + splits[ i ];
    return actor -> create_expression( a, rest );
  }
  if ( splits.size() >= 2 && splits[ 0 ] == "target" )
  {
    std::string rest = splits[1];
    for ( size_t i = 2; i < splits.size(); ++i )
      rest += '.' + splits[ i ];
    return target -> create_expression( a, rest );
  }

  if ( splits.size() == 1 && splits[ 0 ] == "target" )
  {
    return make_ref_expr( name_str, target -> actor_index );
  }

  // conditionals that handle upcoming raid events

  if ( splits.size() >= 3 && util::str_compare_ci( splits[ 0 ], "raid_event" ) )
  {
    std::string type = splits[ 1 ];
    std::string filter = splits[ 2 ];

    struct raid_event_expr_t : public expr_t
    {
      sim_t* s;
      std::string type;
      std::string filter;

      raid_event_expr_t( sim_t* s, std::string& type, std::string& filter ) :
        expr_t( "raid_event" ), s( s ), type( type ), filter( filter )
      {}

      double evaluate()
      {
        return raid_event_t::evaluate_raid_event_expression( s, type, filter );
      }

    };

    return new raid_event_expr_t( this, type, filter );
  }

  // If nothing else works, check to see if the string matches an actor in the sim.
  // If so, return their actor index
  if ( splits.size() == 1 )
    for ( size_t i = 0; i < actor_list.size(); i++ )
      if ( name_str == actor_list[ i ] -> name_str )
        return make_ref_expr( name_str, actor_list[ i ] -> actor_index );    

  return 0;
}

// sim_t::print_options =====================================================

void sim_t::print_options()
{
  out_log.raw() << "\nWorld of Warcraft Raid Simulator Options:\n";


  out_log.raw() << "\nSimulation Engine:\n";
  for ( size_t i = 0; i < options.size(); ++i )
    out_log.raw() << options[ i ];

  for ( size_t i = 0; i < player_list.size(); ++i )
  {
    player_t* p = player_list[ i ];

    out_log.raw().printf( "\nPlayer: %s (%s)\n", p -> name(), util::player_type_string( p -> type ) );
    for ( size_t j = 0; j < p -> options.size(); ++j )
      out_log.raw() << p -> options[ j ];
  }

  out_log.raw() << "\n";
}

// sim_t::create_options ====================================================

void sim_t::create_options()
{
  option_t global_options[] =
  {
    // Program control
    opt_int( "iterations", iterations ),
    opt_func( "thread_priority", parse_thread_priority ),
    // General
    opt_timespan( "max_time", max_time ),
    opt_bool( "fixed_time", fixed_time ),
    opt_float( "vary_combat_length", vary_combat_length ),
    opt_func( "ptr", parse_ptr ),
    opt_int( "threads", threads ),
    opt_float( "confidence", confidence ),
    opt_func( "spell_query", parse_spell_query ),
    opt_string( "spell_query_xml_output_file", spell_query_xml_output_file_str ),
    opt_func( "item_db_source", parse_item_sources ),
    opt_func( "proxy", parse_proxy ),
    opt_int( "auto_ready_trigger", auto_ready_trigger ),
    opt_int( "stat_cache", stat_cache ),
    opt_int( "max_aoe_enemies", max_aoe_enemies ),
    // Raid buff overrides
    opt_func( "optimal_raid", parse_optimal_raid ),
    opt_int( "override.attack_power_multiplier", overrides.attack_power_multiplier ),
    opt_int( "override.critical_strike", overrides.critical_strike ),
    opt_int( "override.mastery", overrides.mastery ),
    opt_int( "override.haste", overrides.haste ),
    opt_int( "override.multistrike", overrides.multistrike ),
    opt_int( "override.spell_power_multiplier", overrides.spell_power_multiplier ),
    opt_int( "override.stamina", overrides.stamina ),
    opt_int( "override.str_agi_int", overrides.str_agi_int ),
    opt_int( "override.versatility", overrides.versatility ),
    opt_int( "override.mortal_wounds", overrides.mortal_wounds ),
    opt_int( "override.bleeding", overrides.bleeding ),
    opt_func( "override.spell_data", parse_override_spell_data ),
    opt_float( "override.target_health", overrides.target_health ),
    // Lag
    opt_timespan( "channel_lag", channel_lag ),
    opt_timespan( "channel_lag_stddev", channel_lag_stddev ),
    opt_timespan( "gcd_lag", gcd_lag ),
    opt_timespan( "gcd_lag_stddev", gcd_lag_stddev ),
    opt_timespan( "queue_lag", queue_lag ),
    opt_timespan( "queue_lag_stddev", queue_lag_stddev ),
    opt_timespan( "queue_gcd_reduction", queue_gcd_reduction ),
    opt_bool( "strict_gcd_queue", strict_gcd_queue ),
    opt_timespan( "default_world_lag", world_lag ),
    opt_timespan( "default_world_lag_stddev", world_lag_stddev ),
    opt_timespan( "default_aura_delay", default_aura_delay ),
    opt_timespan( "default_aura_delay_stddev", default_aura_delay_stddev ),
    opt_float( "default_skill", default_skill ),
    opt_timespan( "reaction_time", reaction_time ),
    opt_float( "travel_variance", travel_variance ),
    opt_timespan( "ignite_sampling_delta", ignite_sampling_delta ),
    // Output
    opt_bool( "save_profiles", save_profiles ),
    opt_bool( "default_actions", default_actions ),
    opt_bool( "debug", debug ),
    opt_bool( "debug_each", debug_each ),
    opt_string( "html", html_file_str ),
    opt_bool( "hosted_html", hosted_html ),
    opt_int( "healing", healing ),
    opt_string( "xml", xml_file_str ),
    opt_string( "xml_style", xml_stylesheet_file_str ),
    opt_bool( "log", log ),
    opt_string( "output", output_file_str ),
    opt_bool( "save_raid_summary", save_raid_summary ),
    opt_bool( "save_gear_comments", save_gear_comments ),
    opt_bool( "buff_uptime_timeline", buff_uptime_timeline ),
    // Bloodlust
    opt_int( "bloodlust_percent", bloodlust_percent ),
    opt_timespan( "bloodlust_time", bloodlust_time ),
    // Overrides"
    opt_bool( "override.allow_potions", allow_potions ),
    opt_bool( "override.allow_food", allow_food ),
    opt_bool( "override.allow_flasks", allow_flasks ),
    opt_bool( "override.bloodlust", overrides.bloodlust ),
    // Regen
    opt_timespan( "regen_periodicity", regen_periodicity ),
    // RNG
    opt_bool( "deterministic_rng", deterministic_rng ),
    opt_bool( "average_range", average_range ),
    opt_bool( "average_gauss", average_gauss ),
    opt_int( "convergence_scale", convergence_scale ),
    // Misc
    opt_list( "party", party_encoding ),
    opt_func( "active", parse_active ),
    opt_int( "seed", seed ),
    opt_float( "wheel_granularity", em.wheel_granularity ),
    opt_int( "wheel_seconds", em.wheel_seconds ),
    opt_int( "wheel_shift", em.wheel_shift ),
    opt_string( "reference_player", reference_player_str ),
    opt_string( "raid_events", raid_events_str ),
    opt_append( "raid_events+", raid_events_str ),
    opt_func( "fight_style", parse_fight_style ),
    opt_string( "main_target", main_target_str ),
    opt_float( "target_death_pct", target_death_pct ),
    opt_int( "target_level", target_level ),
    opt_int( "target_level+", rel_target_level ),
    opt_string( "target_race", target_race ),
    opt_bool( "challenge_mode", challenge_mode ),
    opt_int( "scale_to_itemlevel", scale_to_itemlevel ),
    opt_int( "desired_targets", desired_targets ),
    opt_bool( "show_etmi", show_etmi ),
    opt_float( "tmi_window_global", tmi_window_global ),
    opt_float( "tmi_bin_size", tmi_bin_size ),
    opt_bool( "enable_taunts", enable_taunts ),
    // Character Creation
    opt_func( "death_knight", parse_player ),
    opt_func( "deathknight", parse_player ),
    opt_func( "druid", parse_player ),
    opt_func( "hunter", parse_player ),
    opt_func( "mage", parse_player ),
    opt_func( "monk", parse_player ),
    opt_func( "priest", parse_player ),
    opt_func( "paladin", parse_player ),
    opt_func( "rogue", parse_player ),
    opt_func( "shaman", parse_player ),
    opt_func( "warlock", parse_player ),
    opt_func( "warrior", parse_player ),
    opt_func( "enemy", parse_player ),
    opt_func( "pet", parse_player ),
    opt_func( "guardian", parse_player ),
    opt_func( "copy", parse_player ),
    opt_func( "armory", parse_armory ),
    opt_func( "guild", parse_guild ),
    opt_func( "wowhead", parse_armory ),
    opt_func( "mopdev", parse_armory ),
    opt_func( "mophead", parse_armory ),
    opt_func( "local_json", parse_armory ),
    opt_func( "http_clear_cache", http::clear_cache ),
    opt_func( "cache_items", parse_cache ),
    opt_func( "cache_players", parse_cache ),
    opt_string( "default_region", default_region_str ),
    opt_string( "default_server", default_server_str ),
    opt_string( "save_prefix", save_prefix_str ),
    opt_string( "save_suffix", save_suffix_str ),
    opt_bool( "save_talent_str", save_talent_str ),
    opt_func( "talent_format", parse_talent_format ),
    // Stat Enchants
    opt_float( "default_enchant_strength", enchant.attribute[ ATTR_STRENGTH  ] ),
    opt_float( "default_enchant_agility", enchant.attribute[ ATTR_AGILITY   ] ),
    opt_float( "default_enchant_stamina", enchant.attribute[ ATTR_STAMINA   ] ),
    opt_float( "default_enchant_intellect", enchant.attribute[ ATTR_INTELLECT ] ),
    opt_float( "default_enchant_spirit", enchant.attribute[ ATTR_SPIRIT    ] ),
    opt_float( "default_enchant_spell_power", enchant.spell_power ),
    opt_float( "default_enchant_attack_power", enchant.attack_power ),
    opt_float( "default_enchant_haste_rating", enchant.haste_rating ),
    opt_float( "default_enchant_mastery_rating", enchant.mastery_rating ),
    opt_float( "default_enchant_crit_rating", enchant.crit_rating ),
    opt_float( "default_enchant_multistrike_rating", enchant.multistrike_rating ),
    opt_float( "default_enchant_versatility_rating", enchant.versatility_rating ),
    opt_float( "default_enchant_health", enchant.resource[ RESOURCE_HEALTH ] ),
    opt_float( "default_enchant_mana", enchant.resource[ RESOURCE_MANA   ] ),
    opt_float( "default_enchant_rage", enchant.resource[ RESOURCE_RAGE   ] ),
    opt_float( "default_enchant_energy", enchant.resource[ RESOURCE_ENERGY ] ),
    opt_float( "default_enchant_focus", enchant.resource[ RESOURCE_FOCUS  ] ),
    opt_float( "default_enchant_runic", enchant.resource[ RESOURCE_RUNIC_POWER  ] ),
    // Report
    opt_int( "print_styles", print_styles ),
    opt_int( "report_precision", report_precision ),
    opt_bool( "report_pets_separately", report_pets_separately ),
    opt_bool( "report_targets", report_targets ),
    opt_bool( "report_details", report_details ),
    opt_bool( "report_raw_abilities", report_raw_abilities ),
    opt_bool( "report_rng", report_rng ),
    opt_int( "statistics_level", statistics_level ),
    opt_bool( "separate_stats_by_actions", separate_stats_by_actions ),
    opt_bool( "report_raid_summary", report_raid_summary ), // Force reporting of raid summary
    opt_string( "reforge_plot_output_file", reforge_plot_output_file_str ),
    opt_string( "csv_output_file_str", csv_output_file_str ),
    opt_bool( "monitor_cpu", monitor_cpu ),
    opt_func( "maximize_reporting", parse_maximize_reporting ),
    opt_int( "global_item_upgrade_level", global_item_upgrade_level ),
    opt_int( "wowhead_tooltips", wowhead_tooltips ),
    opt_null()
  };

  option_t::copy( options, global_options );
}

// sim_t::parse_option ======================================================

bool sim_t::parse_option( const std::string& name,
                          const std::string& value )
{
  if ( canceled ) return false;

  if ( active_player )
    if ( option_t::parse( this, active_player -> options, name, value ) )
      return true;

  if ( option_t::parse( this, options, name, value ) )
    return true;

  return false;
}

// sim_t::setup =============================================================

bool sim_t::setup( sim_control_t* c )
{
  // Limitation: setup+execute is a one-way action that cannot be repeated or reset

  control = c;

  if ( ! parent ) cache::advance_era();

  // Global Options
  for ( size_t i = 0; i < control -> options.size(); i++ )
  {
    option_tuple_t& o = control -> options[ i ];
    if ( o.scope != "global" ) continue;
    if ( ! parse_option( o.name, o.value ) )
    {
      errorf( "Unknown option \"%s\" with value \"%s\"\n", o.name.c_str(), o.value.c_str() );
      return false;
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
    if ( p )
    {
      if ( ! option_t::parse( this, p -> options, o.name, o.value ) )
        return false;
    }
    else
    {
      errorf( "sim_t::setup: Unable to locate player %s for option %s with value %s\n", o.scope.c_str(), o.name.c_str(), o.value.c_str() );
      return false;
    }
  }

  if ( player_list.empty() && spell_query == NULL )
  {
    errorf( "Nothing to sim!\n" );
    cancel();
    return false;
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
      out_std = o;
      out_debug = o;
      out_log = o;
      out_error = o;
    }
    else
    {
      errorf( "Unable to open output file '%s'\n", output_file_str.c_str() );
      cancel();
      return false;
    }
  }
  if ( debug_each )
    debug = 1;

  if ( debug )
  {
    log = 1;
    print_options();
  }
  if ( log )
  {
    if ( ! debug_each )
      iterations = 1;

    threads = 1;
  }

  return true;
}

// sim_t::cancel ============================================================

void sim_t::cancel()
{
  if ( canceled ) return;

  if ( current_iteration >= 0 )
  {
    errorf( "Simulation has been canceled after %d iterations! (thread=%d)\n", current_iteration + 1, thread_index );
  }
  else
  {
    errorf( "Simulation has been canceled during player setup! (thread=%d)\n", thread_index );
  }

  canceled = 1;

  for ( size_t i = 0; i < children.size(); i++ )
  {
    children[ i ] -> cancel();
  }
}

// sim_t::progress ==========================================================

double sim_t::progress( int* current,
                        int* _final,
                        std::string* detailed )
{
  int total_iterations = iterations;
  int total_current_iterations = current_iteration + 1;
  for ( size_t i = 0; i < children.size(); i++ )
  {
    if ( ! children[ i ] )
      continue;

    total_iterations += children[ i ] -> iterations;
    total_current_iterations += children[ i ] -> current_iteration + 1;
  }

  if ( current ) *current = total_current_iterations;
  if ( _final   ) *_final   = total_iterations;
  detailed_progress( detailed, total_current_iterations, total_iterations );

  return total_current_iterations / ( double ) total_iterations;
}

double sim_t::progress( std::string& phase, std::string* detailed )
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
    return progress( 0, 0, detailed );
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
    char buf[512];
    util::snprintf( buf, 512, "%d/%d", current_iterations, total_iterations );
    (*detail) = buf;
  }
}

// sim_t::errorf ============================================================

void sim_t::errorf( const char* format, ... )
{
  if ( thread_index != 0 )
    return;

  char buffer[ 1024 ];

  va_list fmtargs;
  va_start( fmtargs, format );
  int rval = ::vsnprintf( buffer, sizeof( buffer ), format, fmtargs );
  va_end( fmtargs );

  assert( rval < 0 || ( static_cast<size_t>( rval ) < sizeof( buffer ) ) );
  (void) rval;

  std::string s( buffer );
  (void) buffer;

  util::replace_all( s, "\n", "" );

  out_error.raw() << s << "\n";
  error_list.push_back( s );
}

bool sim_t::use_load_balancing() const
{
  if ( deterministic_rng )
    return false;

  return true;
}

void sim_t::toggle_pause()
{
  if ( parent )
    return;

  pause_mutex.lock();
  if ( ! paused )
    paused = true;
  else
  {
    paused = false;
    pause_cvar.broadcast();
  }
  pause_mutex.unlock();
}

