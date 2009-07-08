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

// translate_inventory_id ===================================================

static const char* translate_inventory_id( int slot )
{
  return "unknown";
}

} // ANONYMOUS NAMESPACE ====================================================

// wowhead_t::download_glyph ================================================

bool wowhead_t::download_glyph( std::string&       glyph_name,
				const std::string& glyph_id )
{
  return false;
}

// wowhead_t::download_gem ==================================================

bool wowhead_t::download_gem( item_t&            item, 
			      const std::string& gem_id )
{
  return false;
}

// wowhead_t::download_item =================================================

bool wowhead_t::download_item( item_t&            item, 
			       const std::string& item_id )
{
  return false;
}

// wowhead_t::download_slot =================================================

bool wowhead_t::download_slot( item_t&            item, 
			       const std::string& item_id,
			       const std::string& enchant_id, 
			       const std::string  gem_ids[ 3 ] )
{
  return false;
}

// wowhead_t::download_player ===============================================

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

// wowhead_t::download_player ===============================================

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

  std::vector<std::string> region_data;
  int num_region = js_t::get_value( region_data, profile_js, "region" );
  if( num_region > 0 ) p -> region_str = region_data[ 0 ];

  std::vector<std::string> realm_data;
  int num_realm = js_t::get_value( realm_data, profile_js, "realm" );
  if( num_realm > 0 ) p -> server_str = realm_data[ 0 ];

  if( p -> region_str.empty() || p -> server_str.empty() )
  {
    p -> origin_str = "http://profiler.wowhead.com/?profile=" + id;
  }
  else
  {
    p -> origin_str = "http://profiler.wowhead.com/?profile=" + p -> region_str + "." + p -> server_str + "." + name_str;
  }

  int active_talents = 0;
  js_t::get_value( active_talents, profile_js, "talents/active" );
  if( ! use_active_talents ) active_talents = ( active_talents ? 0 : 1 );

  std::vector<std::string> talent_encodings;
  int num_builds = js_t::get_value( talent_encodings, profile_js, "talents/build" );
  if( num_builds == 2 )
  {
    if( ! p -> parse_talents( talent_encodings[ active_talents ] ) ) return 0;
    p -> talents_str = "http://www.wowarmory.com/talent-calc.xml?cid=" + cid_str + "&tal=" + talent_encodings[ active_talents ];
  }
  
  std::vector<std::string> glyph_encodings;
  num_builds = js_t::get_value( glyph_encodings, profile_js, "talents/glyphs" );
  if( num_builds == 2 )
  {
    p -> glyphs_str = "";
    std::vector<std::string> glyph_ids;
    int num_glyphs = util_t::string_split( glyph_ids, glyph_encodings[ active_talents ], ":" );
    for( int i=0; i < num_glyphs; i++ )
    {
      std::string glyph_name;
      if( ! download_glyph( glyph_name, glyph_ids[ i ] ) ) return 0;
      if( i ) p -> glyphs_str += "/";
      p -> glyphs_str += glyph_name;
    }
  }

  for( int i=0; i < SLOT_MAX; i++ )
  {
    std::vector<std::string> inventory_data;
    if( js_t::get_value( inventory_data, profile_js, translate_inventory_id( i ) ) )
    {
      std::string    item_id = inventory_data[ 0 ];
      std::string enchant_id = inventory_data[ 2 ];

      std::string gem_ids[ 3 ];
      gem_ids[ 0 ] = inventory_data[ 4 ];
      gem_ids[ 1 ] = inventory_data[ 5 ];
      gem_ids[ 2 ] = inventory_data[ 6 ];

      if( ! download_slot( p -> items[ i ], item_id, enchant_id, gem_ids ) )
	return 0;
    }
  }

  return p;
}
