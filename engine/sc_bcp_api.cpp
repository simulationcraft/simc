// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Blizzard Community Platform API
// ==========================================================================

namespace { // ANONYMOUS NAMESPACE ==========================================

std::string urlencode( const std::string& s )
{
  std::string encoded = s;
  util_t::urlencode( encoded );
  return encoded;
}

std::string get_region_host( const std::string& region )
{
  if ( region == "cn" )
    return "http://battlenet.com.cn/";
  else
    return "http://" + urlencode( region ) + ".battle.net/";
}

js_node_t* download_guild( sim_t* sim,
                           const std::string& region,
                           const std::string& server,
                           const std::string& name,
                           bool allow_cache )
{
  std::string url = get_region_host( region ) + "api/wow/guild/" + urlencode( server ) + '/' + urlencode ( name ) + "?fields=members";

  std::string result;
  if ( allow_cache ) http_t::get( result, url, "members" );
  else http_t::download( result, url );
  if ( result.empty() )
  {
    sim -> errorf( "Unable to download guild %s|%s|%s from BCP API.\n", region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }
  js_node_t* js = js_t::create( sim, result );
  if ( ! js || ! ( js = js_t::get_child( js, "members" ) ) )
  {
    sim -> errorf( "Unable to determine members for guild %s|%s|%s from BCP API.\n", region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }

  return js;
}

} // ANONYMOUS NAMESPACE ====================================================

namespace bcp_api { // ======================================================

// bcp_api::download_player =================================================

player_t* download_player( sim_t* sim,
                           const std::string& region,
                           const std::string& server,
                           const std::string& name,
                           bool active )
{
  std::string battlenet = get_region_host( region );

  std::string url = battlenet + "api/wow/character/" + urlencode( server ) + '/' + urlencode ( name ) + "?fields=talents,items,professions,pets"; // ,stats

  std::string result;
  if ( ! http_t::download( result, url ) ) return 0;

  if ( sim -> debug ) util_t::fprintf( sim -> output_file, "%s\n%s\n", url.c_str(), result.c_str() );
  js_node_t* profile_js = js_t::create( sim, result );
  if ( ! profile_js )
  {
    sim -> errorf( "Unable to download player %s from BCP API.\n", name.c_str() );
    return 0;
  }

  if ( sim -> debug ) js_t::print( profile_js, sim -> output_file );

  std::string name_str;
  if ( ! js_t::get_value( name_str, profile_js, "name"  ) )
    name_str = name;

  sim -> current_name = name_str;

  util_t::format_text ( name_str, sim -> input_is_utf8 );

  std::string level_str;
  if ( ! js_t::get_value( level_str, profile_js, "level"  ) )
  {
    sim -> errorf( "Unable to extract player level from BCP API '%s'.\n", url.c_str() );
    return 0;
  }
  int level = atoi( level_str.c_str() );
  if ( level < 60 )
  {
    level = 60;
  }
  else if ( level > 85 )
  {
    level = 85;
  }

  std::string cid_str;
  if ( ! js_t::get_value( cid_str, profile_js, "class" ) )
  {
    sim -> errorf( "Unable to extract player class from BCP API '%s'.\n", url.c_str() );
    return 0;
  }
  int cid = atoi( cid_str.c_str() );
  int player_type = util_t::translate_class_id( cid );
  std::string type_str = util_t::player_type_string( player_type );

  std::string rid_str;
  if ( ! js_t::get_value( rid_str, profile_js, "race" ) )
  {
    sim -> errorf( "Unable to extract player race from BCP API '%s'.\n", url.c_str() );
    return 0;
  }
  race_type r = util_t::translate_race_id( atoi( rid_str.c_str() ) );

  player_t* p = player_t::create( sim, type_str, name_str, r );
  sim -> active_player = p;
  if ( ! p )
  {
    sim -> errorf( "Unable to build player with class '%s' and name '%s' from BCP API '%s'.\n",
                   type_str.c_str(), name_str.c_str(), url.c_str() );
    return 0;
  }

  p -> level = level;

  p -> region_str = region;

  if ( ! js_t::get_value( p -> server_str, profile_js, "realm" ) )
    p -> server_str = server;

  p -> origin_str = battlenet + "wow/en/character/" + urlencode( server ) + "/" + urlencode( name ) + "/advanced";

  if ( js_node_t* profession = js_t::get_node( profile_js, "professions/primary/0" ) )
  {
    std::string name;
    if ( js_t::get_value( name, profession, "name" ) )
    {
      int rank;
      if ( js_t::get_value( rank, profession, "rank" ) )
        p -> professions_str = name + "=" + util_t::to_string( rank );
    }
  }

  if ( js_node_t* profession = js_t::get_node( profile_js, "professions/primary/1" ) )
  {
    std::string name;
    if ( js_t::get_value( name, profession, "name" ) )
    {
      int rank;
      if ( js_t::get_value( rank, profession, "rank" ) )
      {
        if ( p -> professions_str.length() > 0 )
          p -> professions_str += '/';
        p -> professions_str += name + "=" + util_t::to_string( rank );
      }
    }
  }

  if ( js_node_t* build = js_t::get_node( profile_js, "talents" ) )
  {
    build = js_t::get_child( build, ( js_t::get_node( build, "0/selected" ) && active ) ? "0" : "1" );
    if ( build  )
    {
      std::string talent_encoding;
      if ( ! js_t::get_value( talent_encoding, build, "build" ) )
      {
        sim -> errorf( "Player %s unable to access talent encoding from profile.\n", p -> name() );
        return 0;
      }
      if ( ! p -> parse_talents_armory( talent_encoding ) )
      {
        sim -> errorf( "Player %s unable to parse talent encoding '%s'.\n", p -> name(), talent_encoding.c_str() );
        return 0;
      }
      p -> talents_str = "http://www.wowhead.com/talent#" + type_str + "-" + talent_encoding;

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
    }
  }

  if ( js_node_t* items = js_t::get_child( profile_js, "items" ) )
  {
    static const char* const slot_map[ SLOT_MAX ] = {
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

    for ( int i = 0; i < SLOT_MAX; ++i )
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
        return 0;
    }
  }

  return p;
}

#if UNFINISHED

// bcp_api::download_item() ================================================

bool download_item( item_t& item, const std::string& item_id, bool cache_only )
{
  if ( item_id.empty() || item_id == "0" ) return false;

  std::string url = get_region_host( item.player -> region_str ) + "api/wow/data/item/" + urlencode( item_id );

  std::string result;
  js_node_t* js = 0;
  if ( cache_only )
    http_t::cache_get( result, url );
  else
    http_t::get( result, url, "itemLevel" );

  if ( ! result.empty() )
    js = js_t::create( item.sim, result );

  if ( ! js )
  {
    if ( ! cache_only )
      item.sim -> errorf( "Player %s unable to download item id %s from BCP API at slot %s.\n", item.player -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( item.sim -> debug ) js_t::print( js, item.sim -> output_file );

  if ( ! js_t::get_value( item.armory_name_str, js, "name" ) )
    goto error;
  armory_t::format( item.armory_name_str );

  if ( ! js_t::get_value( item.armory_id_str, js, "id" ) )
    goto error;

  // Err, success?
  return false;

error:
  item.sim -> errorf( "Player %s unable to parse item id %s from BCP API at slot %s.\n", item.player -> name(), item_id.c_str(), item.slot_name() );
  return false;
}
#endif

bool download_guild( sim_t* sim, const std::string& region, const std::string& server, const std::string& name,
                     const std::vector<int>& ranks, int player_filter, int max_rank, bool allow_cache )
{
  js_node_t* js = ::download_guild( sim, region, server, name, allow_cache );
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
    player_t* player = download_player( sim, region, server, cname );
    if ( !player )
    {
      sim -> errorf( "BCP API failed for '%s' ...\n", cname.c_str() );
      player = wowhead_t::download_player( sim, region, server, cname );
    }
    if ( !player )
      return false;
  }

  return true;
}

} // namespace bcp_api =====================================================
