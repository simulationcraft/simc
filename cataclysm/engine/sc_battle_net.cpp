// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace {  // ANONYMOUS NAMESPACE ==========================================

// download_character_sheet =================================================

static xml_node_t* download_character_sheet( sim_t* sim,
                                             const std::string& region,
                                             const std::string& server,
                                             const std::string& name )
{
  std::string url = "http://" + region + ".battle.net/wow/en/character/" + server + "/" + name + "/advanced";
  xml_node_t* node = xml_t::download( sim, url, "", -1 );
  if ( sim -> debug ) xml_t::print( node, sim -> output_file );
  return node;
}

// download_character_talents ===============================================

static xml_node_t* download_character_talents( sim_t* sim,
					       const std::string& region,
					       const std::string& server,
					       const std::string& name,
					       const char* p_s )
{
  std::string url = "http://" + region + ".battle.net/wow/en/character/" + server + "/" + name + "/talent/" + p_s;
  xml_node_t* node = xml_t::download( sim, url, "", -1 );
  if ( sim -> debug ) xml_t::print( node, sim -> output_file );
  return node;
}

} // ANONYMOUS NAMESPACE ===================================================

// battle_net_t::download_player ===========================================

player_t* battle_net_t::download_player( sim_t* sim,
					 const std::string& region,
					 const std::string& server,
					 const std::string& name,
					 const std::string& talents_description,
					 int cache )
{
  sim -> current_slot = 0;
  sim -> current_name = name;

  xml_node_t* sheet_xml     = download_character_sheet  ( sim, region, server, name );
  xml_node_t* p_talents_xml = download_character_talents( sim, region, server, name, "primary" );
  xml_node_t* s_talents_xml = download_character_talents( sim, region, server, name, "secondary" );

  if ( ! sheet_xml || ! p_talents_xml || ! s_talents_xml )
  {
    sim -> errorf( "Unable to download character %s|%s|%s from the Armory.\n", region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }

  xml_node_t* profile_info = xml_t::get_node( sheet_xml, "div", "class", "profile-info" );
  if ( ! profile_info )
  {
    sim -> errorf( "Unable to get profile info for character %s|%s|%s from the Armory.\n", region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }

  xml_node_t* name_node = xml_t::get_node( profile_info, "div", "class", "name" );

  std::string type_str, race_str, name_str;
  int level;

  if ( ! xml_t::get_value( name_str, xml_t::get_child( name_node, "a" ), "href" ) ||
       ! xml_t::get_value( type_str, xml_t::get_node( profile_info, "span", "class", "class" ), "." ) ||
       ! xml_t::get_value( race_str, xml_t::get_node( profile_info, "span", "class", "race"  ), "." ) ||
       ! xml_t::get_value(    level, xml_t::get_node( xml_t::get_node( profile_info, "span", "class", "level" ), "strong" ), "." ) )
  {
    sim -> errorf( "Unable to determine name/class/race/level from armory xml for %s|%s|%s.\n",
      region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }

  size_t pos = 0;
  name_str.erase( name_str.end() - 1 );
  
  if ( ( pos = name_str.rfind( '/' ) ) == std::string::npos )
  {
    sim -> errorf("Could not find '/' in %s, no how to parse the player name.\n", name_str.c_str() );
    return 0;
  }
  else
  {
    name_str = name_str.substr( pos + 1 );
    name_str = util_t::format_text( util_t::urldecode( name_str ), sim -> input_is_utf8 );
  }

  std::string cid_str = util_t::class_id_string( util_t::parse_player_type( type_str ) );
  
  armory_t::format( type_str );
  armory_t::format( race_str );

  race_type r = util_t::parse_race_type( race_str );

  if( ! talents_description.empty() && ( talents_description != "active" ) )
  {
    name_str += "_" + talents_description;
  }

  player_t* p = player_t::create( sim, type_str, name_str, r );
  sim -> active_player = p;
  if ( ! p )
  {
    sim -> errorf( "Unable to build player with class '%s' and name '%s' from armory %s|%s|%s.\n",
		   type_str.c_str(), name_str.c_str(), region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }

  p -> level      = level;
  p -> region_str = region;
  p -> server_str = server;

  std::string origin_str = "http://" + region + ".battle.net/wow/en/character/" + server + "/" + name + "/advanced";
  http_t::format( p -> origin_str, origin_str );

  std::string last_modified;
  if ( xml_t::get_value( last_modified, xml_t::get_node( sheet_xml, "div", "class", "summary-lastupdate" ), "." ) )
  {
    p -> last_modified = util_t::parse_date( last_modified.substr( 16 ) );
  }

  p -> professions_str = "";
  xml_node_t* profession_list_node = xml_t::get_node( sheet_xml, "div", "class", "summary-professions" );
  std::vector<xml_node_t*> profession_nodes;
  int num_professions = xml_t::get_nodes( profession_nodes, profession_list_node, "li" );

  for( int i=0; i < num_professions; i++ )
  {
    std::string key_str, value_str;

    if ( xml_t::get_value(   key_str, xml_t::get_node( profession_nodes[ i ], "span", "class", "name"  ), "." ) &&
         xml_t::get_value( value_str, xml_t::get_node( profession_nodes[ i ], "span", "class", "value" ), "." ) )
    {
      if ( i ) p -> professions_str += "/";
      p -> professions_str += key_str + "=" + value_str;
    }
  }

  xml_node_t* talents_xml = 0;

  if( util_t::str_compare_ci( talents_description, "primary"   ) ) talents_xml = p_talents_xml;
  if( util_t::str_compare_ci( talents_description, "secondary" ) ) talents_xml = s_talents_xml;

  if( ! talents_xml )
  {
    xml_node_t* summary_talents_node = xml_t::get_node( sheet_xml, "div", "class", "summary-talents" );
    std::vector<xml_node_t*> talent_nodes;
    int num_talents = xml_t::get_nodes( talent_nodes, summary_talents_node, "li" );
    for( int i=0; i < num_talents; i++ )
    {
      xml_node_t* anchor_node = xml_t::get_node( talent_nodes[ i ], "a" );
      if( ! anchor_node ) continue;

      std::string href_str;
      xml_t::get_value( href_str, anchor_node, "href" );
      bool primary = ( href_str.find( "talent/primary" ) != std::string::npos );

      std::string active_str;
      bool active = ( xml_t::get_value( active_str, anchor_node, "class" ) && active_str == "active" );

      if( ( util_t::str_compare_ci( talents_description,   "active" ) &&   active ) ||
	  ( util_t::str_compare_ci( talents_description, "inactive" ) && ! active ) )
      {
	talents_xml = ( primary ? p_talents_xml : s_talents_xml );
	break;
      }
      
      std::string build_str;
      if( xml_t::get_value( build_str, xml_t::get_node( anchor_node, "span", "class", "name" ), "." ) )
      {
	if( util_t::str_compare_ci( talents_description, build_str ) )
	{
	  talents_xml = ( primary ? p_talents_xml : s_talents_xml );
	  break;
	}
      }
    }
  }

  if( ! talents_xml )
  {
    sim -> errorf( "Unable to get talent summary for character %s|%s|%s (%s) from the Armory.\n", 
		   region.c_str(), server.c_str(), name.c_str(), talents_description.c_str() );
    return 0;
  }

  xml_node_t* character_talents_node = xml_t::get_node( talents_xml, "div", "class", "character-talents" );
  xml_node_t* build_script_node      = xml_t::get_node( character_talents_node, "script", "type", "text/javascript" );

  std::string talents_encoding;

  std::string cdata_str;
  if( xml_t::get_value( cdata_str, build_script_node, "cdata" ) )
  {
    std::string::size_type start = cdata_str.find( "build:" ) + 8;
    std::string::size_type end   = cdata_str.find( "\"", start );
    talents_encoding = cdata_str.substr( start, end-start );
  }

  if ( ! p -> parse_talents_armory( talents_encoding ) )
  {
    sim -> errorf( "Player %s unable to parse talents '%s'.\n", p -> name(), talents_encoding.c_str() );
    return 0;
  }
  p -> talents_str = "http://www.wowarmory.com/talent-calc.xml?cid=" + cid_str + "&tal=" + talents_encoding;

  
  p -> glyphs_str = "";

  xml_node_t* character_glyphs_node = xml_t::get_node( talents_xml, "div", "class", "character-glyphs" );
  xml_node_t* glyph_script_node     = xml_t::get_node( character_glyphs_node, "script", "type", "text/javascript" );

  if( xml_t::get_value( cdata_str, glyph_script_node, "cdata" ) )
  {
    std::string::size_type index=0;

    while( ( index = cdata_str.find( "\"Glyph", index ) ) != std::string::npos )
    {
      std::string::size_type start = index + 1;
      index = cdata_str.find( "\"", start );

      if( cdata_str.compare( start, 9, "Glyph of " ) )
      {
	start += 9;
      }
      else if( cdata_str.compare( start, 8, "Glyph - " ) )
      {
	start += 9;
      }
      else continue;

      std::string glyph_name = cdata_str.substr( start, ( index - start ) );
      armory_t::format( glyph_name );
      if( p -> glyphs_str.size() > 0 ) p -> glyphs_str += "/";
      p -> glyphs_str += glyph_name;      
    }
  }
  
  xml_node_t* inventory_node = xml_t::get_node( sheet_xml, "div", "id", "summary-inventory" );

  for ( int i=0; i < SLOT_MAX; i++ )
  {
    if( sim -> canceled ) return 0;
    sim -> current_slot = i;
    item_t& item = p -> items[ i ];

    char slot_id[ 80 ];
    sprintf( slot_id, "%d", i );

    xml_node_t* slot_node = xml_t::get_node( inventory_node, "div", "data-id", slot_id );
    xml_node_t* anchor_node = xml_t::get_node( slot_node, "a" );
    
    std::string data_item_str;
    xml_t::get_value( data_item_str, anchor_node, "data-item" );
    
    std::vector<std::string> tokens;
    int num_tokens = util_t::string_split( tokens, data_item_str, "&=" );

    std::string id_str, enchant_id, addon_id, reforge_id, gem_ids[ 3 ];
    for( int i=0; i < num_tokens-1; i += 2 )
    {
      std::string& name  = tokens[ i ];
      std::string& value = tokens[ i+1 ];

      if     ( name == "i"  ) id_str       = value;
      else if( name == "e"  ) enchant_id   = value;
      else if( name == "ee" ) addon_id     = value;
      else if( name == "re" ) reforge_id   = value;
    }

    xml_node_t* sockets_root_node = xml_t::get_node( slot_node, "span", "class", "sockets" );
    std::vector<xml_node_t*> socket_nodes;
    int num_sockets = xml_t::get_children( socket_nodes, sockets_root_node );
    
    for( int i=0; i < num_sockets; i++ )
    {
      std::string href_str;
      xml_t::get_value( href_str, xml_t::get_node( socket_nodes[ i ], "a" ), "href" );
      std::string::size_type index = href_str.rfind( "/" );
      if( index != std::string::npos ) gem_ids[ i ] = href_str.substr( index+1 );
    }
    
    if( ! id_str.empty() )
    {
      if( ! item_t::download_slot( item, id_str, enchant_id, addon_id, reforge_id, gem_ids ) )
      {
	return 0;
      }
    }
  }

  return p;
}

