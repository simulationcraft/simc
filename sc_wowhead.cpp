// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

static js_node_t* download_profile( sim_t* sim, 
				    const std::string& id )
{
  std::string url = "http://profiler.wowhead.com/?profile=load&id=" + id;
  std::string result;

  if( http_t::download( result, url ) )
  {
    std::string::size_type start = result.find( "WowheadProfiler.registerProfile(" );
    if( start != std::string::npos ) start = result.find( "{", start );
    if( start != std::string::npos )
    {
      std::string::size_type finish = result.find( ";", start );
      if( finish != std::string::npos ) finish = result.rfind( "}", finish );
      if( finish != std::string::npos )
      {
	std::string buffer = result.substr( start, ( finish - start ) + 1 );
	if( sim -> debug ) printf( "%s\n", buffer.c_str() );
	return js_t::create( buffer );
      }
    }
  }

  return 0;
}

static const char* translate_class_id( int class_id )
{
  return "unknown";
}

} // ANONYMOUS NAMESPACE ====================================================

// wwhead_t::download_player ================================================

player_t* wowhead_t::download_player( sim_t* sim,
				      const std::string& region, 
				      const std::string& server, 
				      const std::string& name,
				      bool use_active_talents )
{
  std::string url = "http://profiler.wowhead.com/?profile=" + region + "." + server + "." + name;
  std::string result;

  if( http_t::download( result, url ) )
  {
    std::string::size_type start = result.find( "profilah.initialize(" );
    if( start != std::string::npos )
    {
      std::string::size_type finish = result.find( ";", start );
      if( finish != std::string::npos )
      {
	std::vector<std::string> splits;
	int num_splits = util_t::string_split( splits, result.substr( start, finish-start ), "(){}.;,: \t\n" );
	for( int i=0; i < num_splits-1; i++ )
	{
	  if( splits[ i ] == "id" )
	  {
	    return download_player( sim, splits[ i+1 ], use_active_talents );
	  }
	}
      }
    }
  }

  return 0;
}

// wwhead_t::download_player ================================================

player_t* wowhead_t::download_player( sim_t* sim,
				      const std::string& id, 
				      bool use_active_talents )
{
  js_node_t* profile_js = download_profile( sim, id );

  if( ! profile_js )
  {
    printf( "simcraft: Unable to download character profile %s from wowhead.\n", id.c_str() );
    return 0;
  }

  std::string name_str;
  if( ! js_t::get_value( name_str, profile_js, "character/name"  ) ) return 0;
  armory_t::format( name_str );

  int class_id;
  if( ! js_t::get_value( class_id, profile_js, "character/class" ) ) return 0;
  std::string type_str = translate_class_id( class_id );

  player_t* p = player_t::create( sim, type_str, name_str );
  if( ! p ) return 0;
  

  return 0;
}
