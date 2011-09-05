// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Blizzard Community Platform API
// ==========================================================================

namespace bcp_api { // ======================================================

namespace { // ANONYMOUS NAMESPACE ==========================================

template <typename T>
inline T clamp( T x, T low, T high )
{
  return x < low ? low : ( high < x ? high : x );
}

// download_id ==============================================================

js_node_t* download_id( sim_t* sim, const std::string& region, const std::string& item_id, cache::behavior_t caching )
{
  if ( item_id.empty() || item_id == "0" ) return 0;

  std::string url = "http://" + region + ".battle.net/api/wow/item/" + item_id + "?locale=en_US";

  std::string result;
  if ( ! http_t::get( result, url, std::string(), caching ) )
    return 0;

  return js_t::create( sim, result );
}

// download_guild ===========================================================

js_node_t* download_guild( sim_t* sim,
                           const std::string& region,
                           const std::string& server,
                           const std::string& name,
                           cache::behavior_t  caching )
{
  std::string url = "http://" + region + ".battle.net/api/wow/guild/" + server + '/' +
      name + "?fields=members";

  std::string result;
  if ( ! http_t::get( result, url, "\"members\"", caching ) )
  {
    sim -> errorf( "Unable to download guild %s|%s|%s from BCP API.\n", region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }
  js_node_t* js = js_t::create( sim, result );
  if ( ! js || ! ( js = js_t::get_child( js, "members" ) ) )
  {
    sim -> errorf( "Unable to determine members of guild %s|%s|%s from BCP API.\n", region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }

  return js;
}

// parse_profession =========================================================

void parse_profession( std::string& professions_str, js_node_t* profile, int index )
{
  std::string key = "professions/primary/" + util_t::to_string( index );
  if ( js_node_t* profession = js_t::get_node( profile, key ) )
  {
    int id;
    std::string rank;
    if ( js_t::get_value( id, profession, "id" ) && js_t::get_value( rank, profession, "rank" ) )
    {
      if ( professions_str.length() > 0 )
        professions_str += '/';
      professions_str += util_t::profession_type_string( util_t::translate_profession_id( id ) );
      professions_str += '=' + rank;
    }
  }
}

// pick_talents =============================================================

js_node_t* pick_talents( js_node_t* talents, const std::string& specifier )
{
  js_node_t* spec1 = js_t::get_child( talents, "0" );
  js_node_t* spec2 = js_t::get_child( talents, "1" );

  assert( spec1 );

  bool spec1_is_active = js_t::get_child( spec1, "selected" ) != 0;

  if ( specifier == "active" )
    return spec1_is_active ? spec1 : spec2;

  if ( specifier == "inactive" )
    return spec1_is_active ? spec2 : spec1;

  if ( specifier == "primary" )
    return spec1;

  if ( specifier == "secondary" )
    return spec2;

  std::string spec_name;
  if ( ! js_t::get_value( spec_name, spec1, "name" ) )
    return 0;
  if ( util_t::str_prefix_ci( spec_name, specifier ) )
    return spec1;

  if ( spec2 && js_t::get_value( spec_name, spec2, "name" ) )
  {
    if ( util_t::str_prefix_ci( spec_name, specifier ) )
      return spec2;
  }

  return 0;
}

// parse_talents ============================================================

bool parse_talents( player_t* p, js_node_t* talents )
{
  std::string talent_encoding;
  if ( ! js_t::get_value( talent_encoding, talents, "build" ) )
  {
    p -> sim -> errorf( "Player %s unable to access talent encoding from profile.\n", p -> name() );
    return false;
  }

  if ( ! p -> parse_talents_armory( talent_encoding ) )
  {
    p -> sim -> errorf( "Player %s unable to parse talent encoding '%s'.\n", p -> name(), talent_encoding.c_str() );
    return false;
  }

  p -> talents_str  = "http://www.wowhead.com/talent#";
  p -> talents_str += util_t::class_id_string( p -> type );
  p -> talents_str += '-';
  p -> talents_str += talent_encoding;

  return true;
}

// parse_glyphs =============================================================

bool parse_glyphs( player_t* p, js_node_t* build )
{
  static const char* const glyph_type_names[] = { "glyphs/prime", "glyphs/major", "glyphs/minor" };

  for ( std::size_t i = 0; i < sizeof( glyph_type_names ) / sizeof( glyph_type_names[ 0 ]); ++i )
  {
    if ( js_node_t* glyphs = js_t::get_node( build, glyph_type_names[ i ] ) )
    {
      std::vector<js_node_t*> children;
      if ( js_t::get_children( children, glyphs ) > 0 )
      {
        for( std::size_t i = 0; i < children.size(); ++i )
        {
          std::string glyph_name;
          if ( js_t::get_value( glyph_name, children[ i ], "name" ) )
          {
            // FIXME: Move this common boilerplate stuff into util_t where all the data
            //        sources can use it instead of cut'n'pasting.
            if(      glyph_name.substr( 0, 9 ) == "Glyph of " ) glyph_name.erase( 0, 9 );
            else if( glyph_name.substr( 0, 8 ) == "Glyph - "  ) glyph_name.erase( 0, 8 );
            armory_t::format( glyph_name );

            if ( p -> glyphs_str.length() )
              p -> glyphs_str += '/';
            p -> glyphs_str += glyph_name;
          }
        }
      }
    }
  }

  return true;
}

// parse_items ==============================================================

bool parse_items( player_t* p, js_node_t* items )
{
  if ( !items ) return true;

  static const char* const slot_map[] = {
    "head",
    "neck",
    "shoulder",
    "shirt",
    "chest",
    "waist",
    "legs",
    "feet",
    "wrist",
    "hands",
    "finger1",
    "finger2",
    "trinket1",
    "trinket2",
    "back",
    "mainHand",
    "offHand",
    "ranged",
    "tabard"
  };

  assert( sizeof( slot_map ) / sizeof ( slot_map[ 0 ] ) == SLOT_MAX );

  for ( unsigned i = 0; i < SLOT_MAX; ++i )
  {
    js_node_t* item = js_t::get_child( items, slot_map[ i ] );
    if ( ! item ) continue;

    std::string item_id;
    if ( ! js_t::get_value( item_id, item, "id" ) ) continue;

    std::string gem_ids[3];
    js_t::get_value( gem_ids[0], item, "tooltipParams/gem0" );
    js_t::get_value( gem_ids[1], item, "tooltipParams/gem1" );
    js_t::get_value( gem_ids[2], item, "tooltipParams/gem2" );

    std::string enchant_id;
    js_t::get_value( enchant_id, item, "tooltipParams/enchant" );

    std::string reforge_id;
    js_t::get_value( reforge_id, item, "tooltipParams/reforge" );

    std::string tinker_id;
    js_t::get_value( tinker_id, item, "tooltipParams/tinker" );

    std::string suffix_id;
    js_t::get_value( suffix_id, item, "tooltipParams/suffix" );

    if ( ! item_t::download_slot( p -> items[ i ], item_id, enchant_id, tinker_id, reforge_id, suffix_id, gem_ids ) )
      return false;
  }

  return true;
}

} // ANONYMOUS NAMESPACE ====================================================

// bcp_api::download_player =================================================

player_t* download_player( sim_t*             sim,
                           const std::string& region,
                           const std::string& server,
                           const std::string& name,
                           const std::string& talents,
                           cache::behavior_t  caching )
{
  std::string battlenet = "http://" + region + ".battle.net/";
  std::string url = battlenet + "api/wow/character/" +
    server + '/' + name + "?fields=talents,items,professions&locale=en_US";

  sim -> current_slot = 0;
  sim -> current_name = name;

  std::string result;
  if ( ! http_t::get( result, url, std::string(), caching ) )
    return 0;
  // if ( sim -> debug ) util_t::fprintf( sim -> output_file, "%s\n%s\n", url.c_str(), result.c_str() );
  js_node_t* profile_js = js_t::create( sim, result );
  if ( ! profile_js )
  {
    sim -> errorf( "BCP API: Unable to download player '%s'\n", name.c_str() );
    return 0;
  }
  if ( sim -> debug ) js_t::print( profile_js, sim -> output_file );

  std::string name_str;
  if ( ! js_t::get_value( name_str, profile_js, "name"  ) )
    name_str = name;
  sim -> current_name = name_str;
  if ( talents != "active" )
    name_str += '_' + talents;

  int level;
  if ( ! js_t::get_value( level, profile_js, "level"  ) )
  {
    sim -> errorf( "BCP API: Unable to extract player level from '%s'.\n", url.c_str() );
    return 0;
  }
  //level = clamp( level, 60, 85 );
  // Is there a reason not to import low level characters? Besides the 20pt base stats in player_t::init_resource
  if ( level > 85 ) level = 85;

  int cid;
  if ( ! js_t::get_value( cid, profile_js, "class" ) )
  {
    sim -> errorf( "BCP API: Unable to extract player class from '%s'.\n", url.c_str() );
    return 0;
  }
  std::string class_str = util_t::player_type_string( util_t::translate_class_id( cid ) );

  int rid;
  if ( ! js_t::get_value( rid, profile_js, "race" ) )
  {
    sim -> errorf( "BCP API: Unable to extract player race from '%s'.\n", url.c_str() );
    return 0;
  }

  player_t* p = player_t::create( sim, class_str, name_str, util_t::translate_race_id( rid ) );
  sim -> active_player = p;
  if ( ! p )
  {
    sim -> errorf( "BCP API: Unable to build player with class '%s' and name '%s' from '%s'.\n",
                   class_str.c_str(), name_str.c_str(), url.c_str() );
    return 0;
  }

  p -> level = level;
  p -> region_str = region;
  if ( ! js_t::get_value( p -> server_str, profile_js, "realm" ) )
    p -> server_str = server;

  p -> origin_str = battlenet + "wow/en/character/" + server + '/' + name + "/advanced";
  http_t::format( p ->origin_str );

  if ( js_t::get_value( p -> thumbnail_url, profile_js, "thumbnail" ) )
  {
    p -> thumbnail_url = battlenet + "static-render/" + region + '/' + p -> thumbnail_url;
    http_t::format( p -> thumbnail_url );
  }

  parse_profession( p -> professions_str, profile_js, 0 );
  parse_profession( p -> professions_str, profile_js, 1 );

  js_node_t* build = js_t::get_node( profile_js, "talents" );
  if ( ! build )
  {
    sim -> errorf( "BCP API: Player '%s' from URL '%s' has no talents.\n",
                   name_str.c_str(), url.c_str() );
    return 0;
  }

  build = pick_talents( build, talents );
  if ( ! build )
  {
    sim -> errorf( "BCP API: Invalid talent spec '%s' for player '%s'.\n",
                   talents.c_str(), name_str.c_str() );
    return 0;
  }
  if ( ! parse_talents( p, build ) )
    return 0;

  if ( ! parse_glyphs( p, build ) )
    return 0;

  if ( ! parse_items( p, js_t::get_child( profile_js, "items" ) ) )
    return 0;

  p -> armory_extensions( region, server, name, caching );

  return p;
}

// bcp_api::download_item() ================================================

bool download_item( item_t& item, const std::string& item_id, cache::behavior_t caching )
{
  js_node_t* js = download_id( item.sim, item.sim -> default_region_str, item_id, caching );
  if ( ! js )
  {
    if ( caching != cache::ONLY )
    {
      item.sim -> errorf( "BCP API: Player '%s' unable to download item id '%s' at slot %s.\n",
                          item.player -> name(), item_id.c_str(), item.slot_name() );
    }
    return false;
  }
  if ( item.sim -> debug ) js_t::print( js, item.sim -> output_file );

  if ( ! js_t::get_value( item.armory_name_str, js, "name" ) )
  {
    item.sim -> errorf( "BCP API: Player '%s' unable to parse item '%s' name at slot '%s'.\n",
                        item.player -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }
  armory_t::format( item.armory_name_str );

  if ( ! js_t::get_value( item.armory_id_str, js, "id" ) )
  {
    item.sim -> errorf( "BCP API: Player '%s' unable to parse item '%s' id at slot '%s'.\n",
                        item.player -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  return true;
}

// bcp_api::download_guild ==================================================

bool download_guild( sim_t* sim, const std::string& region, const std::string& server, const std::string& name,
                     const std::vector<int>& ranks, int player_filter, int max_rank, cache::behavior_t caching )
{
  js_node_t* js = download_guild( sim, region, server, name, caching );
  if ( !js ) return false;

  std::vector<std::string> names;
  std::vector<js_node_t*> characters;
  js_t::get_children( characters, js );

  for ( std::size_t i = 0, n = characters.size(); i < n; ++i )
  {
    std::string cname;
    if ( ! js_t::get_value( cname, characters[ i ], "character/name" ) ) continue;

    int cid;
    if ( ! js_t::get_value( cid, characters[ i ], "character/class" ) ) continue;

    int level;
    if ( ! js_t::get_value( level, characters[ i ], "character/level" ) ) continue;

    int rank;
    if ( ! js_t::get_value( rank, characters[ i ], "rank" ) ) continue;

    if ( level < 85 || ( ( max_rank > 0 ) && ( rank > max_rank ) ) ||
         ( ( ranks.size() > 0 ) && std::find( ranks.begin(), ranks.end(), rank ) == ranks.end() ) )
      continue;

    if ( player_filter != PLAYER_NONE && player_filter != util_t::translate_class_id( cid ) )
      continue;

    names.push_back( cname );
  }

  if ( names.empty() ) return true;

  std::sort( names.begin(), names.end() );

  for ( std::size_t i = 0, n = names.size(); i < n; ++i )
  {
    const std::string& cname = names[ i ];
    sim -> errorf( "Downloading character: %s\n", cname.c_str() );
    player_t* player = download_player( sim, region, server, cname, "active", caching );
    if ( !player )
    {
      sim -> errorf( "BCP API: Failed to download player '%s' ...\n", cname.c_str() );
      player = wowhead_t::download_player( sim, region, server, cname, "active", caching );
    }
    if ( !player )
      return false;
  }

  return true;
}

// bcp_api::download_glyph ================================================

bool download_glyph( player_t*          player,
                     std::string&       glyph_name,
                     const std::string& glyph_id,
                     cache::behavior_t  caching )
{
  js_node_t* item = download_id( player -> sim,
                                 player -> region_str.empty() ? player -> sim -> default_region_str : player -> region_str,
                                 glyph_id, caching );
  if ( ! item || ! js_t::get_value( glyph_name, item, "name" ) )
  {
    if ( caching != cache::ONLY )
      player -> sim -> errorf( "BCP API: Unable to download glyph id '%s'\n", glyph_id.c_str() );
    return false;
  }

  if(      glyph_name.substr( 0, 9 ) == "Glyph of " ) glyph_name.erase( 0, 9 );
  else if( glyph_name.substr( 0, 8 ) == "Glyph - "  ) glyph_name.erase( 0, 8 );
  armory_t::format( glyph_name );

  return true;
}

} // namespace bcp_api =====================================================
