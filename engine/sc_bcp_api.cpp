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
  static const char* const glyph_type_names[] = { "glyphs/prime", "glyphs/major", "glyphs/minor" };

  for ( std::size_t i = 0; i < sizeof_array( glyph_type_names ); ++i )
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
  if ( talents != "active" )
    name_str += '_' + talents;
  sim -> current_name = name_str;

  int level;
  if ( ! js_t::get_value( level, profile_js, "level"  ) )
  {
    sim -> errorf( "BCP API: Unable to extract player level from '%s'.\n", url.c_str() );
    return 0;
  }

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
  http_t::format( p -> origin_str );

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

namespace { // ANONYMOUS ===================================================

inline bool parse_item_id( item_t& item, js_node_t* js )
{ return js_t::get_value( item.armory_id_str, js, "id" ); }

bool parse_item_name( item_t& item, js_node_t* js )
{
  if ( js_t::get_value( item.armory_name_str, js, "name" ) )
  {
    armory_t::format( item.armory_name_str );
    return true;
  }
  else
    return false;
}

bool parse_item_quality( item_t& item, js_node_t* js )
{
  int quality;
  if ( js_t::get_value( quality, js, "quality" ) )
  {
    if ( quality > 1 )
      item.armory_quality_str = util_t::item_quality_string( quality );
    return true;
  }
  else
    return false;
}

inline bool parse_item_level( item_t& item, js_node_t* js )
{ return js_t::get_value( item.armory_ilevel_str, js, "itemLevel" ); }

bool parse_item_heroic( item_t&, js_node_t* )
{
  // "heroic" is not available from BCP API.
  return true;
}

bool parse_item_armor_type( item_t& item, js_node_t* js )
{
  item.armory_armor_type_str.clear();

  int itemclass, itemsubclass;
  if ( js_t::get_value( itemclass, js, "itemClass" ) &&
       js_t::get_value( itemsubclass, js, "itemSubClass" ) &&
       itemclass == ITEM_CLASS_ARMOR )
  {
    switch ( itemsubclass )
    {
    case ITEM_SUBCLASS_ARMOR_CLOTH:
      item.armory_armor_type_str = "cloth";
      break;
    case ITEM_SUBCLASS_ARMOR_LEATHER:
      item.armory_armor_type_str = "leather";
      break;
    case ITEM_SUBCLASS_ARMOR_MAIL:
      item.armory_armor_type_str = "mail";
      break;
    case ITEM_SUBCLASS_ARMOR_PLATE:
      item.armory_armor_type_str = "plate";
      break;
    default:
      break;
    }
  }

  return true;
}

} // ANONYMOUS namespace ===================================================

// bcp_api::download_item() ================================================

bool download_item( item_t& item, const std::string& item_id, cache::behavior_t caching )
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

  item_data_t item_data;
  zerofill( item_data );

  try
  {
    {
      int id;
      if ( ! js_t::get_value( id, js, "id" ) ) throw( "id" );
      item_data.id = id;
    }

    std::string item_name;
    if ( ! js_t::get_value( item_name, js, "name" ) ) throw( "name" );
    item_data.name = item_name.c_str();

    std::string item_icon;
    if ( js_t::get_value( item_name, js, "icon" ) )
      item_data.icon = item_icon.c_str();

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

    {
      std::vector<js_node_t*> nodes;
      if ( js_t::get_children( nodes, js, "allowableClasses" ) )
      {
        for ( unsigned i = 0, n = nodes.size(); i < n; ++i )
        {
          int cid;
          if ( js_t::get_value( cid, nodes[ i ] ) )
            item_data.class_mask |= 1 << ( cid - 1 );
        }
      }
      else
        item_data.class_mask = -1;
    }

    {
      std::vector<js_node_t*> nodes;
      if ( js_t::get_children( nodes, js, "allowableRaces" ) )
      {
        for ( unsigned i = 0, n = nodes.size(); i < n; ++i )
        {
          int rid;
          if ( js_t::get_value( rid, nodes[ i ] ) )
            item_data.race_mask |= 1 << ( rid - 1 );
        }
      }
      else
        item_data.race_mask = -1;
    }

    {
      std::vector<js_node_t*> nodes;
      js_t::get_children( nodes, js, "bonusStats" );
      for ( unsigned i = 0, n = std::min( nodes.size(), sizeof_array( item_data.stat_type ) ); i < n; ++i )
      {
        if ( ! js_t::get_value( item_data.stat_type[ i ], nodes[ i ], "stat" ) ) throw( "bonus stat" );
        if ( ! js_t::get_value( item_data.stat_val[ i ], nodes[ i ], "amount" ) ) throw( "bonus stat amount" );
      }
    }

    {
      std::string color;
      std::vector<js_node_t*> nodes;
      js_t::get_children( nodes, js, "socketInfo/sockets" );
      for ( unsigned i = 0, n = std::min( nodes.size(), sizeof_array( item_data.socket_color ) ); i < n; ++i )
      {
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
          else if ( color == "COGWHEEL" )
            item_data.socket_color[ i ] = SOCKET_COLOR_COGWHEEL;
        }
      }
    }

    js_t::get_value( item_data.id_set, js, "itemSet" );

    // heroic tag is not available from BCP API.
    if ( false ) item_data.flags_1 |= ITEM_FLAG_HEROIC;

    // socket bonus is not available from BCP API.
    if ( false ) item_data.id_socket_bonus = 7748234923;
  }
  catch ( const char* fieldname )
  {
    item.sim -> errorf( "BCP API: Player '%s' unable to parse item '%s' %s at slot '%s'.\n",
                       item.player -> name(), item_id.c_str(), fieldname, item.slot_name() );
    return false;
  }

  // return item.loadFrom( item_data );
  return false;
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
        ( ! ranks.empty() && std::find( ranks.begin(), ranks.end(), rank ) == ranks.end() ) )
      continue;

    std::string cname;
    if ( ! js_t::get_value( cname, characters[ i ], "character/name" ) ) continue;
    names.push_back( cname );
  }

  if ( names.empty() ) return true;

  std::sort( names.begin(), names.end() );

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
        return false;
    }
  }

  return true;
}

// bcp_api::download_glyph ================================================

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

  if(      glyph_name.substr( 0, 9 ) == "Glyph of " ) glyph_name.erase( 0, 9 );
  else if( glyph_name.substr( 0, 8 ) == "Glyph - "  ) glyph_name.erase( 0, 8 );
  armory_t::format( glyph_name );

  return true;
}

} // namespace bcp_api =====================================================
