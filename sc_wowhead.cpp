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

// download_id ==============================================================

static xml_node_t* download_id( const std::string& id_str )
{
  if( id_str.empty() || id_str == "" || id_str == "0" ) return 0;
  std::string url = "http://www.wowhead.com/?item=" + id_str + "&xml";
  xml_node_t* node = xml_t::download( url, "</json>", 0 );
  if( ! node ) printf( "simcraft: Unable to download glyph id %s from wowhead\n", id_str.c_str() );
  return node;
}

// parse_stats ==============================================================

static void parse_stats( std::string& encoding,
			 const std::string& stats_str )
{
  std::vector<std::string> splits;
  int num_splits = util_t::string_split( splits, stats_str, "," );

  for( int i=0; i < num_splits; i++ )
  {
    std::string type_str, value_str;

    if( 2 == util_t::string_split( splits[ i ], ":", "S S", &type_str, &value_str ) )
    {
      int type = util_t::parse_stat_type( type_str );
      if( type != STAT_NONE )
      {
	encoding += "_";
	encoding += value_str;
	encoding += util_t::stat_type_abbrev( type );
      }
    }
  }
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
  switch( slot )
  {
  case SLOT_HEAD:      return "inventory/1";
  case SLOT_NECK:      return "inventory/2";
  case SLOT_SHOULDERS: return "inventory/3";
  case SLOT_SHIRT:     return "inventory/4";
  case SLOT_CHEST:     return "inventory/5";
  case SLOT_WAIST:     return "inventory/6";
  case SLOT_LEGS:      return "inventory/7";
  case SLOT_FEET:      return "inventory/8";
  case SLOT_WRISTS:    return "inventory/9";
  case SLOT_HANDS:     return "inventory/10";
  case SLOT_FINGER_1:  return "inventory/11";
  case SLOT_FINGER_2:  return "inventory/12";
  case SLOT_TRINKET_1: return "inventory/13";
  case SLOT_TRINKET_2: return "inventory/14";
  case SLOT_BACK:      return "inventory/15";
  case SLOT_MAIN_HAND: return "inventory/16";
  case SLOT_OFF_HAND:  return "inventory/17";
  case SLOT_RANGED:    return "inventory/18";
  }

  return "unknown";
}

} // ANONYMOUS NAMESPACE ====================================================

// wowhead_t::download_glyph ================================================

bool wowhead_t::download_glyph( std::string&       glyph_name,
				const std::string& glyph_id )
{
  xml_node_t* node = download_id( glyph_id );
  if( node )
  {
    if( xml_t::get_value( glyph_name, node, "name/cdata" ) )
    {
      glyph_name.erase( 0, 9 ); // remove "Glyph of "
      armory_t::format( glyph_name );
      return true;
    }
  }
  return false;
}

// wowhead_t::download_enchant ==============================================

bool wowhead_t::download_enchant( item_t&            item, 
				  const std::string& enchant_id )
{
  item.armory_enchant_str.clear();

  if( enchant_id.empty() || enchant_id == "" || enchant_id == "0" )
    return true;

  std::string enchant_name;
  if( enchant_t::get_encoding( enchant_name, item.armory_enchant_str, enchant_id ) )
  {
    if     ( enchant_name == "Lightweave Embroidery"    ) { item.armory_enchant_str = "lightweave";  }
    else if( enchant_name == "Hand-Mounted Pyro Rocket" ) { item.armory_enchant_str = "pyrorocket";  }
    else if( enchant_name == "Berserking"               ) { item.armory_enchant_str = "berserking";  }
    else if( enchant_name == "Mongoose"                 ) { item.armory_enchant_str = "mongoose";    }
    else if( enchant_name == "Executioner"              ) { item.armory_enchant_str = "executioner"; }
    else if( enchant_name == "Spellsurge"               ) { item.armory_enchant_str = "spellsurge";  }

    armory_t::format( item.armory_enchant_str );

    return true;
  }

  return false;
}

// wowhead_t::download_gem ==================================================

int wowhead_t::download_gem( item_t&            item, 
			     const std::string& gem_id )
{
  if( gem_id.empty() || gem_id == "" || gem_id == "0" )
    return true;

  xml_node_t* node = download_id( gem_id );

  if( node )
  {
    std::string color_str;
    if( xml_t::get_value( color_str, node, "subclass/cdata" ) )
    {
      std::string::size_type pos = color_str.find( ' ' );
      if( pos != std::string::npos ) color_str.erase( pos );
      armory_t::format( color_str );
      int gem_type = util_t::parse_gem_type( color_str );
    
      if( gem_type == GEM_META )
      {
	std::string name_str;
	if( xml_t::get_value( name_str, node, "name/cdata" ) )
	{
	  std::string::size_type pos = name_str.find( " Diamond" );
	  if( pos != std::string::npos ) name_str.erase( pos );
	  armory_t::format( name_str );
	  item.armory_gems_str += "_";
	  item.armory_gems_str += name_str;
	}	
      }
      else
      {
	std::string stats_str;
	if( xml_t::get_value( stats_str, node, "jsonEquip/cdata" ) )
	{
	  parse_stats( item.armory_gems_str, stats_str );
	}
      }

      return gem_type;
    }
  }

  return GEM_NONE;
}

// wowhead_t::download_item =================================================

bool wowhead_t::download_item( item_t&            item, 
			       const std::string& item_id )
{
  xml_node_t* node = download_id( item_id );

  if( node )
  {
    if( xml_t::get_value( item.armory_name_str, node, "name/cdata" ) )
    {
      armory_t::format( item.armory_name_str );

      item.armory_stats_str.clear();
      std::string stats_str;
      if( xml_t::get_value( stats_str, node, "jsonEquip/cdata" ) )
      {
	parse_stats( item.armory_stats_str, stats_str );
	armory_t::format( item.armory_stats_str );
      }

      return true;
    }
  }

  return false;
}

// wowhead_t::download_slot =================================================

bool wowhead_t::download_slot( item_t&            item, 
			       const std::string& item_id,
			       const std::string& enchant_id, 
			       const std::string  gem_ids[ 3 ] )
{
  player_t* p = item.player;

  if( ! download_item( item, item_id ) )
  {
    printf( "simcraft: Player %s unable to parse name for item %s at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  item.armory_gems_str.clear();
  for( int i=0; i < 3; i++ )
  {
    if( ! download_gem( item, gem_ids[ i ] ) )
    {
      printf( "simcraft: Player %s unable to parse gem id %s for item \"%s\" at slot %s.\n", p -> name(), gem_ids[ i ].c_str(), item.name(), item.slot_name() );
      return false;
    }
  }
  armory_t::format( item.armory_gems_str );

  if( ! download_enchant( item, enchant_id ) )
  {
    printf( "simcraft: Player %s unable to parse enchant id %s for item \"%s\" at slot %s.\n", p -> name(), enchant_id.c_str(), item.name(), item.slot_name() );
    return false;
  }

  return true;
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

  if( sim -> debug ) js_t::print( profile_js );

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

  int user_id=0;
  if( js_t::get_value( user_id, profile_js, "user" ) && ( user_id != 0 ) )
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
  num_builds = js_t::get_value( glyph_encodings, profile_js, "glyphs" );
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
