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

const bool BCP_DEBUG_ITEMS = false;

// download_id ==============================================================

js_node_t* download_id( sim_t* sim, const std::string& region, const std::string& item_id, cache::behavior_t caching )
{
  if ( item_id.empty() || item_id == "0" ) return 0;

  std::string url = "http://" + region + ".battle.net/api/wow/item/" + item_id + "?locale=en_US";

  std::string result;
  if ( ! http_t::get( result, url, caching ) )
    return 0;

  return js_t::create( sim, result );
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
    p -> sim -> errorf( "BCP API: No talent encoding for player %s.\n", p -> name() );
    return false;
  }
  if ( talent_encoding.size() > MAX_TALENT_SLOTS )
    talent_encoding.resize( MAX_TALENT_SLOTS );
  if ( ! p -> parse_talents_armory( talent_encoding ) )
  {
    p -> sim -> errorf( "BCP API: Can't parse talent encoding '%s' for player %s.\n", talent_encoding.c_str(), p -> name() );
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
  static const char* const glyph_type_names[] =
  {
    "glyphs/prime", "glyphs/major", "glyphs/minor"
  };

  for ( std::size_t i = 0; i < sizeof_array( glyph_type_names ); ++i )
  {
    if ( js_node_t* glyphs = js_t::get_node( build, glyph_type_names[ i ] ) )
    {
      std::vector<js_node_t*> children;
      if ( js_t::get_children( children, glyphs ) > 0 )
      {
        for ( std::size_t j = 0; j < children.size(); ++j )
        {
          std::string glyph_name;
          if ( ! js_t::get_value( glyph_name, children[ j ], "name" ) )
          {
            std::string glyph_id;
            if ( js_t::get_value( glyph_id, children[ j ], "item" ) )
              item_t::download_glyph( p, glyph_name, glyph_id );
          }

          if ( ! glyph_name.empty() )
          {
            // FIXME: Move this common boilerplate stuff into util_t where all the data
            //        sources can use it instead of cut'n'pasting.
            if (      glyph_name.substr( 0, 9 ) == "Glyph of " ) glyph_name.erase( 0, 9 );
            else if ( glyph_name.substr( 0, 8 ) == "Glyph - "  ) glyph_name.erase( 0, 8 );
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

  static const char* const slot_map[] =
  {
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

  assert( sizeof_array( slot_map ) == SLOT_MAX );

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

struct player_spec_t
{
  std::string region, server, name, url, origin, talent_spec;
};

// parse_player =============================================================

player_t*
parse_player( sim_t*             sim,
              player_spec_t&     player,
              cache::behavior_t  caching )
{
  sim -> current_slot = 0;

  std::string result;
  if ( ! http_t::get( result, player.url, caching ) )
    return 0;
  // if ( sim -> debug ) util_t::fprintf( sim -> output_file, "%s\n%s\n", url.c_str(), result.c_str() );
  js_node_t* profile = js_t::create( sim, result );
  if ( ! profile )
  {
    sim -> errorf( "BCP API: Unable to download player from '%s'\n", player.url.c_str() );
    return 0;
  }
  if ( sim -> debug ) js_t::print( profile, sim -> output_file );

  std::string status, reason;
  if ( js_t::get_value( status, profile, "status" ) && util_t::str_compare_ci( status, "nok" ) )
  {
    js_t::get_value( reason, profile, "reason" );

    sim -> errorf( "BCP API: Unable to download player from '%s', reason: %s\n",
                   player.url.c_str(),
                   reason.c_str() );
    return 0;
  }

  std::string name;
  if ( js_t::get_value( name, profile, "name"  ) )
    player.name = name;
  else
    name = player.name;
  if ( player.talent_spec != "active" && ! player.talent_spec.empty() )
  {
    name += '_';
    name += player.talent_spec;
  }
  if ( ! name.empty() )
    sim -> current_name = name;

  int level;
  if ( ! js_t::get_value( level, profile, "level"  ) )
  {
    sim -> errorf( "BCP API: Unable to extract player level from '%s'.\n", player.url.c_str() );
    return 0;
  }

  int cid;
  if ( ! js_t::get_value( cid, profile, "class" ) )
  {
    sim -> errorf( "BCP API: Unable to extract player class from '%s'.\n", player.url.c_str() );
    return 0;
  }
  std::string class_name = util_t::player_type_string( util_t::translate_class_id( cid ) );

  int rid;
  if ( ! js_t::get_value( rid, profile, "race" ) )
  {
    sim -> errorf( "BCP API: Unable to extract player race from '%s'.\n", player.url.c_str() );
    return 0;
  }

  player_t* p = player_t::create( sim, class_name, name, util_t::translate_race_id( rid ) );
  sim -> active_player = p;
  if ( ! p )
  {
    sim -> errorf( "BCP API: Unable to build player with class '%s' and name '%s' from '%s'.\n",
                   class_name.c_str(), name.c_str(), player.url.c_str() );
    return 0;
  }

  p -> level = level;
  p -> region_str = player.region.empty() ? sim -> default_region_str : player.region;

  if ( ! js_t::get_value( p -> server_str, profile, "realm" ) && ! player.server.empty() )
    p -> server_str = player.server;

  if ( ! player.origin.empty() )
  {
    p -> origin_str = player.origin;
    http_t::format( p -> origin_str );
  }

  if ( js_t::get_value( p -> thumbnail_url, profile, "thumbnail" ) )
  {
    p -> thumbnail_url = "http://" + p -> region_str + ".battle.net/static-render/" +
                         p -> region_str + '/' + p -> thumbnail_url;
    http_t::format( p -> thumbnail_url );
  }

  parse_profession( p -> professions_str, profile, 0 );
  parse_profession( p -> professions_str, profile, 1 );

  js_node_t* build = js_t::get_node( profile, "talents" );
  if ( ! build )
  {
    sim -> errorf( "BCP API: Player '%s' from URL '%s' has no talents.\n",
                   name.c_str(), player.url.c_str() );
    return 0;
  }

  build = pick_talents( build, player.talent_spec );
  if ( ! build )
  {
    sim -> errorf( "BCP API: Invalid talent spec '%s' for player '%s'.\n",
                   player.talent_spec.c_str(), name.c_str() );
    return 0;
  }
  if ( ! parse_talents( p, build ) )
    return 0;

  if ( ! parse_glyphs( p, build ) )
    return 0;

  if ( ! parse_items( p, js_t::get_child( profile, "items" ) ) )
    return 0;

  if ( ! p -> server_str.empty() )
    p -> armory_extensions( p -> region_str, p -> server_str, player.name, caching );

  return p;
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
  sim -> current_name = name;

  player_spec_t player;

  std::string battlenet = "http://" + region + ".battle.net/";

  player.url = battlenet + "api/wow/character/" +
               server + '/' + name + "?fields=talents,items,professions&locale=en_US";
  player.origin = battlenet + "wow/en/character/" + server + '/' + name + "/advanced";

  player.region = region;
  player.server = server;
  player.name = name;

  player.talent_spec = talents;

  return parse_player( sim, player, caching );
}

namespace   // ANONYMOUS ====================================================
{

struct item_info_t : public item_data_t
{
  std::string name_str, icon_str;
  item_info_t() : item_data_t() {}
};

bool download_item_data( item_t& item, item_info_t& item_data,
                         const std::string& item_id, cache::behavior_t caching )
{
  // BCP API doesn't currently provide enough information to describe items completely.
  if ( ! BCP_DEBUG_ITEMS )
    return false;

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

  try
  {
    {
      int id;
      if ( ! js_t::get_value( id, js, "id" ) ) throw( "id" );
      item_data.id = id;
    }

    if ( ! js_t::get_value( item_data.name_str, js, "name" ) ) throw( "name" );
    item_data.name = item_data.name_str.c_str();

    if ( js_t::get_value( item_data.icon_str, js, "icon" ) )
      item_data.icon = item_data.icon_str.c_str();

    if ( ! js_t::get_value( item_data.level, js, "itemLevel" ) ) throw( "level" );

    js_t::get_value( item_data.req_level, js, "requiredLevel" );
    js_t::get_value( item_data.req_skill, js, "requiredSkill" );
    js_t::get_value( item_data.req_skill_level, js, "requiredSkillRank" );

    if ( ! js_t::get_value( item_data.quality, js, "quality" ) ) throw( "quality" );

    if ( ! js_t::get_value( item_data.inventory_type, js, "inventoryType" ) ) throw( "inventory type" );
    if ( ! js_t::get_value( item_data.item_class, js, "itemClass" ) ) throw( "item class" );
    if ( ! js_t::get_value( item_data.item_subclass, js, "itemSubClass" ) ) throw( "item subclass" );
    js_t::get_value( item_data.bind_type, js, "itemBind" );

    if ( js_node_t* w = js_t::get_child( js, "weaponInfo" ) )
    {
      int minDmg, maxDmg;
      double speed;
      if ( ! js_t::get_value( speed, w, "weaponSpeed" ) ) throw( "weapon speed" );
      if ( ! js_t::get_value( minDmg, w, "damage/minDamage" ) ) throw( "weapon minimum damage" );
      if ( ! js_t::get_value( maxDmg, w, "damage/maxDamage" ) ) throw( "weapon maximum damage" );
      item_data.delay = speed * 1000.0;
      item_data.dmg_range = maxDmg - minDmg;
    }

    if ( js_node_t* classes = js_t::get_child( js, "allowableClasses" ) )
    {
      std::vector<js_node_t*> nodes;
      js_t::get_children( nodes, classes );
      for ( size_t i = 0, n = nodes.size(); i < n; ++i )
      {
        int cid;
        if ( js_t::get_value( cid, nodes[ i ] ) )
          item_data.class_mask |= 1 << ( cid - 1 );
      }
    }
    else
      item_data.class_mask = -1;

    if ( js_node_t* races = js_t::get_child( js, "allowableRaces" ) )
    {
      std::vector<js_node_t*> nodes;
      js_t::get_children( nodes, races );
      for ( size_t i = 0, n = nodes.size(); i < n; ++i )
      {
        int rid;
        if ( js_t::get_value( rid, nodes[ i ] ) )
          item_data.race_mask |= 1 << ( rid - 1 );
      }
    }
    else
      item_data.race_mask = -1;

    if ( js_node_t* stats = js_t::get_child( js, "bonusStats" ) )
    {
      std::vector<js_node_t*> nodes;
      js_t::get_children( nodes, stats );
      for ( size_t i = 0, n = std::min( nodes.size(), sizeof_array( item_data.stat_type ) ); i < n; ++i )
      {
        if ( ! js_t::get_value( item_data.stat_type[ i ], nodes[ i ], "stat" ) ) throw( "bonus stat" );
        if ( ! js_t::get_value( item_data.stat_val[ i ], nodes[ i ], "amount" ) ) throw( "bonus stat amount" );
      }
    }

    if ( js_node_t* sockets = js_t::get_node( js, "socketInfo/sockets" ) )
    {
      std::vector<js_node_t*> nodes;
      js_t::get_children( nodes, sockets );
      for ( size_t i = 0, n = std::min( nodes.size(), sizeof_array( item_data.socket_color ) ); i < n; ++i )
      {
        std::string color;
        if ( js_t::get_value( color, nodes[ i ], "type" ) )
        {
          if ( color == "META" )
            item_data.socket_color[ i ] = SOCKET_COLOR_META;
          else if ( color == "RED" )
            item_data.socket_color[ i ] = SOCKET_COLOR_RED;
          else if ( color == "YELLOW" )
            item_data.socket_color[ i ] = SOCKET_COLOR_RED;
          else if ( color == "BLUE" )
            item_data.socket_color[ i ] = SOCKET_COLOR_BLUE;
          else if ( color == "PRISMATIC" )
            item_data.socket_color[ i ] = SOCKET_COLOR_PRISMATIC;
          else if ( color == "COGWHEEL" )
            item_data.socket_color[ i ] = SOCKET_COLOR_COGWHEEL;
        }
      }
    }

    js_t::get_value( item_data.id_set, js, "itemSet" );

    // heroic tag is not available from BCP API.
    {
      // FIXME: set item_data.flags_1 to ITEM_FLAG_HEROIC as appropriate.
    }

    // FIXME: LFR tag is not available from BCP API.

    // socket bonus is not available from BCP API.
    {
      // FIXME: set item_data.id_socket_bonus appropriately.
    }
  }
  catch ( const char* fieldname )
  {
    item.sim -> errorf( "BCP API: Player '%s' unable to parse item '%s' %s at slot '%s'.\n",
                        item.player -> name(), item_id.c_str(), fieldname, item.slot_name() );
    return false;
  }

  return item_database_t::load_item_from_data( item, &item_data );
}

} // ANONYMOUS namespace ====================================================

// bcp_api::download_item() =================================================

bool download_item( item_t& item, const std::string& item_id, cache::behavior_t caching )
{
  // BCP API doesn't currently provide enough information to describe items completely.
  if ( ! BCP_DEBUG_ITEMS )
    return false;

  item_info_t item_data;
  return download_item_data( item, item_data, item_id, caching );
}

// bcp_api::download_slot() =================================================

bool download_slot( item_t& item,
                    const std::string& item_id,
                    const std::string& enchant_id,
                    const std::string& addon_id,
                    const std::string& reforge_id,
                    const std::string& rsuffix_id,
                    const std::string gem_ids[ 3 ],
                    cache::behavior_t caching )
{
  // BCP API doesn't currently provide enough information to describe items completely.
  if ( ! BCP_DEBUG_ITEMS )
    return false;

  item_info_t item_data;
  if ( ! download_item_data( item, item_data, item_id, caching ) )
    return false;

  item_database_t::parse_gems( item, &item_data, gem_ids );

  if ( ! item_database_t::parse_enchant( item, enchant_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse enchant id %s for item \"%s\" at slot %s.\n",
                        item.player -> name(), enchant_id.c_str(), item.name(), item.slot_name() );
  }

  if ( ! enchant_t::download_addon( item, addon_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse addon id %s for item \"%s\" at slot %s.\n",
                        item.player -> name(), addon_id.c_str(), item.name(), item.slot_name() );
  }

  if ( ! enchant_t::download_reforge( item, reforge_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse reforge id %s for item \"%s\" at slot %s.\n",
                        item.player -> name(), reforge_id.c_str(), item.name(), item.slot_name() );
  }

  if ( ! enchant_t::download_rsuffix( item, rsuffix_id ) )
  {
    item.sim -> errorf( "Player %s unable to determine random suffix '%s' for item '%s' at slot %s.\n",
                        item.player -> name(), rsuffix_id.c_str(), item.name(), item.slot_name() );
  }

  return true;
}

namespace   // ANONYMOUS ====================================================
{

// download_roster ==========================================================

js_node_t* download_roster( sim_t* sim,
                            const std::string& region,
                            const std::string& server,
                            const std::string& name,
                            cache::behavior_t  caching )
{
  std::string url = "http://" + region + ".battle.net/api/wow/guild/" + server + '/' +
                    name + "?fields=members";

  std::string result;
  if ( ! http_t::get( result, url, caching, "\"members\"" ) )
  {
    sim -> errorf( "BCP API: Unable to download guild %s|%s|%s.\n", region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }
  js_node_t* js = js_t::create( sim, result );
  if ( ! js || ! ( js = js_t::get_child( js, "members" ) ) )
  {
    sim -> errorf( "BCP API: Unable to get members of guild %s|%s|%s.\n", region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }

  return js;
}

} // ANONYMOUS namespace ====================================================

// bcp_api::download_guild ==================================================

bool download_guild( sim_t* sim, const std::string& region, const std::string& server, const std::string& name,
                     const std::vector<int>& ranks, int player_filter, int max_rank, cache::behavior_t caching )
{
  js_node_t* js = download_roster( sim, region, server, name, caching );
  if ( !js ) return false;

  std::vector<std::string> names;
  std::vector<js_node_t*> characters;
  js_t::get_children( characters, js );

  for ( std::size_t i = 0, n = characters.size(); i < n; ++i )
  {
    int level;
    if ( ! js_t::get_value( level, characters[ i ], "character/level" ) || level < 85 )
      continue;

    int cid;
    if ( ! js_t::get_value( cid, characters[ i ], "character/class" ) ||
         ( player_filter != PLAYER_NONE && player_filter != util_t::translate_class_id( cid ) ) )
      continue;

    int rank;
    if ( ! js_t::get_value( rank, characters[ i ], "rank" ) ||
         ( ( max_rank > 0 ) && ( rank > max_rank ) ) ||
         ( ! ranks.empty() && range::find( ranks, rank ) == ranks.end() ) )
      continue;

    std::string cname;
    if ( ! js_t::get_value( cname, characters[ i ], "character/name" ) ) continue;
    names.push_back( cname );
  }

  if ( names.empty() ) return true;

  range::sort( names );

  for ( std::size_t i = 0, n = names.size(); i < n; ++i )
  {
    const std::string& cname = names[ i ];
    util_t::printf( "Downloading character: %s\n", cname.c_str() );
    player_t* player = download_player( sim, region, server, cname, "active", caching );
    if ( !player )
    {
      sim -> errorf( "BCP API: Failed to download player '%s' trying Wowhead instead\n", cname.c_str() );
      player = wowhead_t::download_player( sim, region, server, cname, "active", caching );
      if ( !player )
        sim -> errorf( "Wowhead: Failed to download player '%s'\n", cname.c_str() );
      // Just ignore invalid players
    }
  }

  return true;
}

// bcp_api::download_glyph ==================================================

bool download_glyph( player_t*          player,
                     std::string&       glyph_name,
                     const std::string& glyph_id,
                     cache::behavior_t  caching )
{
  const std::string& region =
    ( player -> region_str.empty() ? player -> sim -> default_region_str : player -> region_str );

  js_node_t* item = download_id( player -> sim, region, glyph_id, caching );
  if ( ! item || ! js_t::get_value( glyph_name, item, "name" ) )
  {
    if ( caching != cache::ONLY )
      player -> sim -> errorf( "BCP API: Unable to download glyph id '%s'\n", glyph_id.c_str() );
    return false;
  }

  static const std::string glyph_of = "Glyph of ";
  static const std::string glyph_dash = "Glyph - ";

  if ( util_t::str_prefix_ci( glyph_name, glyph_of ) )
    glyph_name.erase( 0, glyph_of.length() );
  else if ( util_t::str_prefix_ci( glyph_name, glyph_dash ) )
    glyph_name.erase( 0, glyph_dash.length() );
  armory_t::format( glyph_name );

  return true;
}

namespace   // ANONYMOUS ====================================================
{

// bcp_api::parse_gem_stats =================================================

std::string parse_gem_stats( const std::string& bonus )
{
  std::istringstream in( bonus );
  std::ostringstream out;

  int amount;
  std::string stat;

  in >> amount;
  in >> stat;

  stat_type st = util_t::parse_stat_type( stat );
  if ( st != STAT_NONE )
    out << amount << util_t::stat_type_abbrev( st );

  in >> stat;
  if ( in )
  {
    if ( util_t::str_compare_ci( stat, "Rating" ) )
      in >> stat;

    if ( in )
    {
      if ( util_t::str_compare_ci( stat, "and" ) )
      {
        in >> amount;
        in >> stat;

        st = util_t::parse_stat_type( stat );
        if ( st != STAT_NONE )
          out << '_' << amount << util_t::stat_type_abbrev( st );
      }
    }
  }

  return out.str();
}

} // ANONYMOUS namespace ====================================================

// bcp_api::parse_gem =======================================================

int parse_gem( item_t& item, const std::string& gem_id, cache::behavior_t caching )
{
  const std::string& region =
    item.player -> region_str.empty()
    ? item.sim -> default_region_str
    : item.player -> region_str;

  js_node_t* js = download_id( item.sim, region, gem_id, caching );
  if ( ! js )
    return GEM_NONE;

  std::string type_str;
  if ( ! js_t::get_value( type_str, js, "gemInfo/type/type" ) )
    return GEM_NONE;
  armory_t::format( type_str );

  int type = util_t::parse_gem_type( type_str );

  std::string result;
  if ( type == GEM_META )
  {
    if ( ! js_t::get_value( result, js, "name" ) )
      return GEM_NONE;

    std::string::size_type pos = result.rfind( " Diamond" );
    if ( pos != std::string::npos ) result.erase( pos );
  }
  else
  {
    std::string bonus;
    if ( ! js_t::get_value( bonus, js, "gemInfo/bonus/name" ) )
      return GEM_NONE;
    result = parse_gem_stats( bonus );
  }

  if ( ! result.empty() )
  {
    armory_t::format( result );
    if ( ! item.armory_gems_str.empty() )
      item.armory_gems_str += '_';
    item.armory_gems_str += result;
  }

  return type;
}

} // namespace bcp_api =====================================================

namespace wowreforge   // ==================================================
{

player_t* download_player( sim_t*             sim,
                           const std::string& profile_id,
                           cache::behavior_t  caching )
{
  sim -> current_name = profile_id;

  bcp_api::player_spec_t player;

  player.origin = "http://wowreforge.com/Profiles/" + profile_id;
  player.url = player.origin + "?json";
  player.name = "wowreforge_" + profile_id;

  return bcp_api::parse_player( sim, player, caching );
}

} // namespace wowreforge ==================================================
