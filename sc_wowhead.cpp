// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

// download_profile =========================================================

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

// translate_class_id =======================================================

static const char* translate_class_id( const std::string& cid_str )
{
  switch( atoi( cid_str.c_str() ) )
  {
  case 1: return "warrior";
  case 2: return "paladin";
  case 3: return "hunter";
  case 4: return "rogue";
  case 5: return "priest";
  case 6: return "death_knight";
  case 7: return "shaman";
  case 8: return "mage";
  case 9: return "warlock";
  case 11: return "druid";
  }

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
  if( ! js_t::get_value( name_str, profile_js, "name"  ) ) return 0;
  armory_t::format( name_str );

  std::string cid_str;
  if( ! js_t::get_value( cid_str, profile_js, "classs" ) ) return 0;
  std::string type_str = translate_class_id( cid_str );

  player_t* p = player_t::create( sim, type_str, name_str );
  if( ! p ) return 0;

  p -> origin_str = "http://profiler.wowhead.com/?profile==" + id;

  std::vector<std::string> talent_encodings;
  int num_builds = js_t::get_value( talent_encodings, profile_js, "talents/build" );
  if( num_builds == 2 )
  {
    int active_talents = 0;
    js_t::get_value( active_talents, profile_js, "talents/active" );
    if( ! use_active_talents ) active_talents = ( active_talents ? 0 : 1 );

    if( ! p -> parse_talents( talent_encodings[ active_talents ] ) ) return 0;
    p -> talents_str = "http://www.wowarmory.com/talent-calc.xml?cid=" + cid_str + "&tal=" + talent_encodings[ active_talents ];
  }
  

  return p;
}
