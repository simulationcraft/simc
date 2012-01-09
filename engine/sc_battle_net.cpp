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
                                             const std::string& name,
                                             cache::behavior_t caching )
{
  std::string url = "http://" + region + ".battle.net/wow/en/character/" + server + "/" + name + "/advanced";
  xml_node_t* node = xml_t::get( sim, url, caching, std::string(), sim -> current_throttle );
  if ( sim -> debug ) xml_t::print( node, sim -> output_file );
  return node;
}

// download_character_talents ===============================================

static xml_node_t* download_character_talents( sim_t* sim,
                                               const std::string& region,
                                               const std::string& server,
                                               const std::string& name,
                                               const char* p_s,
                                               cache::behavior_t caching )
{
  std::string url = "http://" + region + ".battle.net/wow/en/character/" + server + "/" + name + "/talent/" + p_s;
  xml_node_t* node = xml_t::get( sim, url, caching, std::string(), sim -> current_throttle );
  if ( sim -> debug ) xml_t::print( node, sim -> output_file );
  return node;
}

} // ANONYMOUS NAMESPACE ===================================================

// battle_net_t::download_player ============================================

player_t* battle_net_t::download_player( sim_t* sim,
                                         const std::string& region,
                                         const std::string& server,
                                         const std::string& name,
                                         const std::string& talents_description,
                                         cache::behavior_t caching )
{
  sim -> current_slot = 0;
  sim -> current_name = name;

  xml_node_t* sheet_xml   = download_character_sheet( sim, region, server, name, caching );
  xml_node_t* talents_xml = 0;

  if ( ! sheet_xml )
  {
    sim -> errorf( "Unable to download character %s|%s|%s from the Armory: Is the Armory down or throttled?\n",
                   region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }

  if ( xml_t::get_node( sheet_xml, "h3", ".", "Character Not Available" ) )
  {
    sim -> errorf( "Unable to download character %s|%s|%s from the Armory: Character Not Available\n",
                   region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }

  if ( util_t::str_compare_ci( talents_description, "primary" ) )
  {
    talents_xml = download_character_talents( sim, region, server, name, "primary", caching );
  }
  else if ( util_t::str_compare_ci( talents_description, "secondary" ) )
  {
    talents_xml = download_character_talents( sim, region, server, name, "secondary", caching );
  }

  xml_node_t* profile_info = xml_t::get_node( sheet_xml, "div", "class", "profile-info" );
  if ( ! profile_info )
  {
    sim -> errorf( "Unable to get profile info for character %s|%s|%s from the Armory.\n", region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }

  xml_node_t* name_node = xml_t::get_node( profile_info, "div", "class", "name" );
  xml_node_t* server_node = xml_t::get_node( profile_info, "span", "class", "realm tip" );

  std::string type_str, race_str, name_str, server_str;
  int level;

  if ( ! xml_t::get_value( name_str, xml_t::get_child( name_node, "a" ), "href" ) ||
       ! xml_t::get_value( type_str, xml_t::get_node( profile_info, "a", "class", "class" ), "href" ) ||
       ! xml_t::get_value( race_str, xml_t::get_node( profile_info, "a", "class", "race"  ), "href" ) ||
       ! xml_t::get_value(    level, xml_t::get_node( xml_t::get_node( profile_info, "span", "class", "level" ), "strong" ), "." ) ||
       ! xml_t::get_value( server_str, server_node, "." ) )
  {
    sim -> errorf( "Unable to determine name/class/race/level/server from armory xml for %s|%s|%s.\n",
                   region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }

  std::size_t pos = 0;
  name_str.erase( name_str.end() - 1 );

  if ( ( pos = name_str.rfind( '/' ) ) == std::string::npos )
  {
    sim -> errorf( "Could not find '/' in %s, no how to parse the player name.\n", name_str.c_str() );
    return 0;
  }
  else
  {
    name_str = name_str.substr( pos + 1 );
    name_str = util_t::format_text( util_t::urldecode( name_str ), sim -> input_is_utf8 );
  }

  if ( ( pos = type_str.rfind( '/' ) ) == std::string::npos )
  {
    sim -> errorf( "Could not find '/' in %s, no how to parse the player class.\n", type_str.c_str() );
    return 0;
  }
  else
  {
    type_str = type_str.substr( pos + 1 );
    type_str = util_t::format_text( util_t::urldecode( type_str ), sim -> input_is_utf8 );
  }

  if ( ( pos = race_str.rfind( '/' ) ) == std::string::npos )
  {
    sim -> errorf( "Could not find '/' in %s, no how to parse the player race.\n", race_str.c_str() );
    return 0;
  }
  else
  {
    race_str = race_str.substr( pos + 1 );
    race_str = util_t::format_text( util_t::urldecode( race_str ), sim -> input_is_utf8 );
    size_t size = race_str.size();
    for ( size_t i=0; i < size; i++ ) if ( race_str[ i ] == '-' ) race_str[ i ] = '_';
  }

  armory_t::format( type_str );
  armory_t::format( race_str );

  if ( race_str == "forsaken" )
    race_str = "undead";

  race_type r = util_t::parse_race_type( race_str );

  if ( ! talents_description.empty() && ( talents_description != "active" ) )
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
  p -> server_str = server_str;

  p -> origin_str = "http://" + region + ".battle.net/wow/en/character/" + server + "/" + name + "/advanced";
  http_t::format( p -> origin_str );

  p -> professions_str = "";
  xml_node_t* profession_list_node = xml_t::get_node( sheet_xml, "div", "class", "summary-professions" );
  std::vector<xml_node_t*> profession_nodes;
  int num_professions = xml_t::get_nodes( profession_nodes, profession_list_node, "li" );

  for ( int i=0; i < num_professions; i++ )
  {
    std::string key_str, value_str;

    if ( xml_t::get_value(   key_str, xml_t::get_node( profession_nodes[ i ], "span", "class", "name"  ), "." ) &&
         xml_t::get_value( value_str, xml_t::get_node( profession_nodes[ i ], "span", "class", "value" ), "." ) )
    {
      if ( i ) p -> professions_str += "/";
      p -> professions_str += key_str + "=" + value_str;
    }
  }

  if ( ! talents_xml )
  {
    xml_node_t* summary_talents_node = xml_t::get_node( sheet_xml, "div", "class", "summary-talents" );
    std::vector<xml_node_t*> talent_nodes;
    int num_talents = xml_t::get_nodes( talent_nodes, summary_talents_node, "li" );
    for ( int i=0; i < num_talents; i++ )
    {
      xml_node_t* anchor_node = xml_t::get_node( talent_nodes[ i ], "a" );
      if ( ! anchor_node ) continue;

      std::string href_str;
      xml_t::get_value( href_str, anchor_node, "href" );
      bool primary = ( href_str.find( "talent/primary" ) != std::string::npos );

      std::string active_str;
      bool active = ( xml_t::get_value( active_str, anchor_node, "class" ) && active_str == "active" );

      if ( ( util_t::str_compare_ci( talents_description,   "active" ) &&   active ) ||
          ( util_t::str_compare_ci( talents_description, "inactive" ) && ! active ) )
      {
        talents_xml = download_character_talents( sim, region, server, name, ( primary ? "primary" : "secondary" ), caching );
        break;
      }

      std::string build_str;
      if ( xml_t::get_value( build_str, xml_t::get_node( anchor_node, "span", "class", "name" ), "." ) )
      {
        armory_t::format( build_str );
        if ( util_t::str_compare_ci( talents_description, build_str ) )
        {
          talents_xml = download_character_talents( sim, region, server, name, ( primary ? "primary" : "secondary" ), caching );
          break;
        }
      }
    }
  }

  if ( ! talents_xml )
  {
    sim -> errorf( "Unable to get talent summary for character %s|%s|%s (%s) from the Armory: Is the Armory down or throttled?\n",
                   region.c_str(), server.c_str(), name.c_str(), talents_description.c_str() );
    return 0;
  }

  if ( xml_t::get_node( talents_xml, "h3", ".", "Character Not Available" ) )
  {
    sim -> errorf( "Unable to get talent summary for character %s|%s|%s (%s) from the Armory: Character Not Available\n",
                   region.c_str(), server.c_str(), name.c_str(), talents_description.c_str() );
    return 0;
  }

  xml_node_t* character_talents_node = xml_t::get_node( talents_xml, "div", "class", "character-talents-wrapper" );
  xml_node_t* build_script_node      = xml_t::get_node( character_talents_node, "script", "type", "text/javascript" );

  std::string talents_encoding;

  std::string cdata_str;
  if ( xml_t::get_value( cdata_str, build_script_node, "cdata" ) )
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
  else
  {
    // HACK: We need primary_tree() to return something sensible, so we can filter out tanks/healers in guild download
    p -> player_t::init_talents();
  }

  p -> talents_str = "http://www.wowhead.com/talent#";
  p -> talents_str += util_t::player_type_string( p -> type );
  p -> talents_str += "-" + talents_encoding;
  p ->  glyphs_str = "";

  xml_node_t* character_glyphs_node = xml_t::get_node( talents_xml, "div", "class", "character-glyphs" );
  std::vector<xml_node_t*> glyph_nodes;
  std::string glyph_id, glyph_name;
  // Hack to get a non-item id based glyph identification working, at least for the english
  // speaking locale. Not much we can do about koKR for example.
  int num_glyphs = xml_t::get_nodes( glyph_nodes, character_glyphs_node, "li", "class", "filled" );
  for ( int i=0; i < num_glyphs; i++ )
  {
    if ( ! xml_t::get_value( glyph_id, xml_t::get_node( glyph_nodes[ i ], "a" ), "href" ) )
    {
      // Revert to name based lookup
      if ( ! xml_t::get_value( glyph_name, xml_t::get_node( glyph_nodes[ i ], "span", "class", "name" ), "." ) )
      {
        sim -> errorf( "Could not fetch a valid glyph id or name string." );
      }
      else
      {
        if (      glyph_name.substr( 0, 9 ) == "Glyph of " ) glyph_name.erase( 0, 9 );
        else if ( glyph_name.substr( 0, 8 ) == "Glyph - "  ) glyph_name.erase( 0, 8 );
        armory_t::format( glyph_name );

        if ( p -> glyphs_str.size() > 0 ) p -> glyphs_str += "/";
        p -> glyphs_str += glyph_name;
      }

      continue;
    }

    if ( ( pos = glyph_id.rfind( '/' ) ) == std::string::npos )
    {
      sim -> errorf( "Could not parse glyph_id string \"%s\".\n", glyph_id.c_str() );
      continue;
    }
    else
    {
      glyph_id = glyph_id.substr( pos + 1 );
      glyph_id = util_t::format_text( util_t::urldecode( glyph_id ), sim -> input_is_utf8 );
    }

    if ( item_t::download_glyph( p, glyph_name, glyph_id ) )
    {
      if ( p -> glyphs_str.size() > 0 ) p -> glyphs_str += "/";
      p -> glyphs_str += glyph_name;
    }
  }

  xml_node_t* inventory_node = xml_t::get_node( sheet_xml, "div", "id", "summary-inventory" );

  for ( int i=0; i < SLOT_MAX; i++ )
  {
    if ( sim -> canceled ) return 0;
    sim -> current_slot = i;
    item_t& item = p -> items[ i ];

    char slot_id[ 80 ];
    snprintf( slot_id, sizeof( slot_id ), "%d", i );

    xml_node_t* slot_node = xml_t::get_node( inventory_node, "div", "data-id", slot_id );
    xml_node_t* anchor_node = xml_t::get_node( slot_node, "a" );

    std::string data_item_str, id_str;
    xml_t::get_value( data_item_str, anchor_node, "data-item" );
    xml_t::get_value( id_str, anchor_node, "href" );
    if ( id_str.rfind( '/' ) != std::string::npos )
      id_str = id_str.substr( id_str.rfind( '/' ) + 1 );
    else
      id_str = "";

    std::vector<std::string> tokens;
    int num_tokens = util_t::string_split( tokens, data_item_str, "&=" );

    std::string enchant_id, addon_id, reforge_id, rsuffix_id, gem_ids[ 3 ];
    for ( int j=0; j < num_tokens-1; j += 2 )
    {
      std::string& key  = tokens[ j ];
      std::string& value = tokens[ j+1 ];

      if      ( key == "i"  ) id_str       = value;
      else if ( key == "e"  ) enchant_id   = value;
      else if ( key == "ee" ) addon_id     = value;
      else if ( key == "re" ) reforge_id   = value;
      else if ( key == "r"  ) rsuffix_id   = value;
    }

    xml_node_t* sockets_root_node = xml_t::get_node( slot_node, "span", "class", "sockets" );
    std::vector<xml_node_t*> socket_nodes;
    int num_sockets = xml_t::get_children( socket_nodes, sockets_root_node );

    for ( int j=0; j < num_sockets; j++ )
    {
      std::string href_str;
      xml_t::get_value( href_str, xml_t::get_node( socket_nodes[ j ], "a" ), "href" );
      std::string::size_type index = href_str.rfind( "/" );
      if ( index != std::string::npos ) gem_ids[ j ] = href_str.substr( index+1 );
    }

    if ( ! id_str.empty() )
    {
      if ( ! item_t::download_slot( item, id_str, enchant_id, addon_id, reforge_id, rsuffix_id, gem_ids ) )
      {
        return 0;
      }
    }
  }

  p -> armory_extensions( region, server, name, caching );

  return p;
}

// battle_net_t::download_guild =============================================

bool battle_net_t::download_guild( sim_t* sim,
                                   const std::string& region,
                                   const std::string& server,
                                   const std::string& name,
                                   const std::vector<int>& ranks,
                                   int player_filter,
                                   int max_rank,
                                   cache::behavior_t caching )
{
  std::string url_name = name;
  std::string url_server = server;
  util_t::urlencode( url_name );
  util_t::urlencode( url_server );
  std::string url = "http://" + region + ".battle.net/wow/en/guild/" + url_server + "/" + url_name + "/roster";

  xml_node_t* node = xml_t::get( sim, url, caching, std::string(), sim -> current_throttle );
  xml_node_t* guild_info = xml_t::get_node( xml_t::get_node( node, "div", "id", "roster" ), "tbody" );
  if ( ! guild_info ) return false;

  if ( sim -> debug ) xml_t::print( guild_info, sim -> output_file );

  std::vector<xml_node_t*> characters;
  std::vector<std::string> character_names;
  xml_t::get_nodes( characters, guild_info, "tr" );

  // size() - 1 because last node is a "no results row" in the table
  for ( unsigned i = 0; i < characters.size() - 1; i++ )
  {
    int c_level, c_rank, c_class;
    std::string c_name, c_url, c_class_str;

    if ( ! xml_t::get_value( c_level, characters[ i ], "data-level" ) )
      return false;

    if ( ! xml_t::get_value( c_rank, xml_t::get_node( characters[ i ], "td", "class", "rank" ), "data-raw" ) )
      return false;

    if ( ! xml_t::get_value( c_class_str, xml_t::get_node( characters[ i ], "td", "class", "cls" ), "data-raw" ) )
      return false;
    else
      c_class = util_t::translate_class_str( c_class_str );

    if ( ! xml_t::get_value( c_url, xml_t::get_node( xml_t::get_node( characters[ i ], "td", "class", "name" ), "a" ), "href" ) )
      return false;
    else // Urldecode the name, as xml_t::get_value seems to have issues with utf-8
    {
      std::size_t pos = 0;
      c_url.erase( c_url.end() - 1 );

      if ( ( pos = c_url.rfind( '/' ) ) == std::string::npos )
      {
        sim -> errorf( "Could not find '/' in %s, no way to parse the player name.\n", c_url.c_str() );
        return 0;
      }
      else
      {
        c_url = c_url.substr( pos + 1 );
        c_name = util_t::format_text( util_t::urldecode( c_url ), sim -> input_is_utf8 );
      }
    }

    // Then perform filtering based on level, rank and class filter
    if ( c_level < 85 )
      continue;

    if ( ( max_rank > 0 ) && ( c_rank > max_rank ) )
      continue;

    if ( ! ranks.empty() && ( range::find( ranks, c_rank ) == ranks.end() ) )
      continue;

    if ( player_filter != PLAYER_NONE )
      if ( player_filter != c_class )
        continue;

    character_names.push_back( c_name );
  }

  int num_characters = ( int ) character_names.size();
  if ( num_characters > 0 )
  {
    range::sort( character_names );

    for ( int i = 0; i < num_characters; i++ )
    {
      std::string& character_name = character_names[ i ];

      sim -> errorf( "Downloading character: %s\n", character_name.c_str() );
      player_t* p = battle_net_t::download_player( sim, region, server, character_name, "active", caching );

      if ( ! p )
      {
        sim -> errorf( "simulationcraft: Battle.net armory failed for '%s' ...\n", character_name.c_str() );
        continue;
      }

      int tree = p -> primary_tree();
      if ( tree == TREE_RESTORATION || tree == TREE_HOLY || tree == TREE_DISCIPLINE )
      {
        sim -> errorf( "Setting quiet=1 on healer %s\n", character_name.c_str() );
        p -> quiet = true;
      }

      if ( tree == TREE_PROTECTION || tree == TREE_BLOOD )
      {
        sim -> errorf( "Setting quiet=1 on tank %s\n", character_name.c_str() );
        p -> quiet = true;
      }
    }
  }

  return true;
}
