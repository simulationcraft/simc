// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"
#define RANGE_BITS 24
#define RANGE_HIGH ((1<<RANGE_BITS)-1)
#define RANGE_HIGH_MASK (0x3f<<(RANGE_BITS-6))
#define RANGE_LOW_MASK ((1<<(RANGE_BITS-6))-1)

// static const char* base64 = "f-qR3eOHQ9dSIMwk8pabYt6yrJUFNXLTh4n2KWEoz0uC5j7xmAgDlZiPcs_BGV1v";  // sufficiently randomized 10+26+26+2 = 64

namespace   // ANONYMOUS NAMESPACE ==========================================
{

// download_id ==============================================================

static xml_node_t* download_id( sim_t*             sim,
                                const std::string& id_str,
                                cache::behavior_t  caching )
{
  if ( id_str.empty() || id_str == "0" ) return 0;
  std::string url = "http://db.mmo-champion.com/i/" + id_str + '/';

  xml_node_t* node = xml_t::get( sim, url, caching, "<h4>Item #" );

  if ( sim -> debug ) xml_t::print( node );
  return node;
}

// get_tti_node =============================================================

static xml_node_t* get_tti_node( xml_node_t*        root,
                                 const std::string& name )
{
  std::string class_str;
  if ( xml_t::get_value( class_str, root, "class" ) )
  {
    if ( ! strncmp( class_str.c_str(), name.c_str(), name.size() ) )
    {
      return root;
    }
  }

  std::vector<xml_node_t*> children;
  int num_children = xml_t::get_children( children, root );
  for ( int i=0; i < num_children; i++ )
  {
    xml_node_t* node = get_tti_node( children[ i ], name );
    if ( node ) return node;
  }

  return 0;
}

// get_tti_nodes ============================================================

static int get_tti_nodes( std::vector<xml_node_t*>& nodes,
                          xml_node_t*               root,
                          const std::string&        name )
{
  std::string class_str;
  if ( xml_t::get_value( class_str, root, "class" ) )
  {
    if ( ! strncmp( class_str.c_str(), name.c_str(), name.size() ) )
    {
      nodes.push_back( root );
    }
  }

  std::vector<xml_node_t*> children;
  int num_children = xml_t::get_children( children, root );
  for ( int i=0; i < num_children; i++ )
  {
    get_tti_nodes( nodes, children[ i ], name );
  }

  return ( int ) nodes.size();
}

// get_tti_value ============================================================

static bool get_tti_value( std::string&       value,
                           xml_node_t*        root,
                           const std::string& name )
{
  xml_node_t* node = get_tti_node( root, name );

  if ( node )
  {
    if ( xml_t::get_value( value, node, "." ) )
    {
      return true;
    }
    else if ( xml_t::get_value( value, node, "a/." ) )
    {
      return true;
    }
  }

  return false;
}

// get_tti_value ============================================================

static int get_tti_value( std::vector<std::string>& values,
                          xml_node_t*               root,
                          const std::string&        name )
{
  std::string class_str;
  if ( xml_t::get_value( class_str, root, "class" ) )
  {
    if ( ! strncmp( class_str.c_str(), name.c_str(), name.size() ) )
    {
      std::string value;
      if ( xml_t::get_value( value, root, "." ) )
      {
        values.push_back( value );
      }
      else if ( xml_t::get_value( value, root, "a/." ) )
      {
        values.push_back( value );
      }
    }
  }

  std::vector<xml_node_t*> children;
  int num_children = xml_t::get_children( children, root );
  for ( int i=0; i < num_children; i++ )
  {
    get_tti_value( values, children[ i ], name );
  }

  return ( int ) values.size();
}

// parse_gems ===============================================================

static bool parse_gems( item_t&           item,
                        xml_node_t*       node,
                        const std::string gem_ids[ 3 ] )
{
  item.armory_gems_str.clear();

  int sockets[ 3 ] = { GEM_NONE, GEM_NONE, GEM_NONE };

  std::vector<std::string> socket_colors;
  int num_sockets = get_tti_value( socket_colors, node, "tti-socket" );
  for ( int i=0; i < num_sockets && i < 3; i++ )
  {
    std::string& s = socket_colors[ i ];
    std::string::size_type pos = s.find( " " );
    if ( pos != std::string::npos ) s.erase( pos );
    armory_t::format( s );
    sockets[ i ] = util_t::parse_gem_type( s );
  }

  bool match = true;
  for ( int i=0; i < 3; i++ )
  {
    int gem = item_t::parse_gem( item, gem_ids[ i ] );

    if ( ! util_t::socket_gem_match( sockets[ i ], gem ) )
      match = false;
  }

  if ( match )
  {
    xml_node_t* socket_bonus_node = get_tti_node( node, "tti-socket_bonus" );
    if ( socket_bonus_node )
    {
      std::string socket_bonus_str;
      if ( xml_t::get_value( socket_bonus_str, socket_bonus_node, "a/." ) )
      {
        armory_t::fuzzy_stats( item.armory_gems_str, socket_bonus_str );
      }
    }
  }

  armory_t::format( item.armory_gems_str );

  return true;
}

// parse_weapon =============================================================

static bool parse_weapon( item_t&     item,
                          xml_node_t* node )
{
  std::string tti_speed, tti_dps, tti_dmg;

  if ( ! get_tti_value( tti_speed, node, "tti-speed" ) ||
       ! get_tti_value( tti_dps,   node, "tti-dps"   ) )
    return true;

  if ( ! get_tti_value( tti_dmg, node, "tti-dmg" ) )
    if ( ! get_tti_value( tti_dmg, node, "tti-damage" ) )
      if ( ! get_tti_value( tti_dmg, node, "tti-damage_1" ) )
        return true;

  std::string speed_str, dps_str, dmg_min_str, dmg_max_str;

  std::vector<std::string> tokens;
  int num_tokens = util_t::string_split( tokens, tti_speed, " " );
  if ( num_tokens == 2 ) speed_str = tokens[ 1 ];

  tokens.clear();
  num_tokens = util_t::string_split( tokens, tti_dmg, " " );
  if ( ( num_tokens == 4 ) || ( num_tokens == 5 ) )
  {
    dmg_min_str = tokens[ 0 ];
    dmg_max_str = tokens[ 2 ];
  }

  tokens.clear();
  num_tokens = util_t::string_split( tokens, tti_dps, " ()" );
  if ( num_tokens == 4 ) dps_str = tokens[ 0 ];

  if ( speed_str.empty() || dps_str.empty() || dmg_min_str.empty() || dmg_max_str.empty() ) return false;

  std::string subclass_str, slot_str;

  if ( ! get_tti_value( subclass_str, node, "tti-subclass" ) ||
       ! get_tti_value( slot_str,     node, "tti-slot"     ) )
    return false;

  if ( ( slot_str == "Main Hand" ) || ( slot_str == "Off-Hand" ) ) slot_str = "One-Hand";

  int weapon_type = WEAPON_NONE;
  if      ( subclass_str == "Axe" && slot_str == "One-Hand"   ) weapon_type = WEAPON_AXE;
  else if ( subclass_str == "Axe" && slot_str == "Two-Hand"   ) weapon_type = WEAPON_AXE_2H;
  else if ( subclass_str == "Dagger"                          ) weapon_type = WEAPON_DAGGER;
  else if ( subclass_str == "Fist Weapon"                     ) weapon_type = WEAPON_FIST;
  else if ( subclass_str == "Mace" && slot_str == "One-Hand"  ) weapon_type = WEAPON_MACE;
  else if ( subclass_str == "Mace" && slot_str == "Two-Hand"  ) weapon_type = WEAPON_MACE_2H;
  else if ( subclass_str == "Polearm"                         ) weapon_type = WEAPON_POLEARM;
  else if ( subclass_str == "Staff"                           ) weapon_type = WEAPON_STAFF;
  else if ( subclass_str == "Sword" && slot_str == "One-Hand" ) weapon_type = WEAPON_SWORD;
  else if ( subclass_str == "Sword" && slot_str == "Two-Hand" ) weapon_type = WEAPON_SWORD_2H;
  else if ( subclass_str == "Bow"                             ) weapon_type = WEAPON_BOW;
  else if ( subclass_str == "Crossbow"                        ) weapon_type = WEAPON_CROSSBOW;
  else if ( subclass_str == "Gun"                             ) weapon_type = WEAPON_GUN;
  else if ( subclass_str == "Thrown"                          ) weapon_type = WEAPON_THROWN;
  else if ( subclass_str == "Wand"                            ) weapon_type = WEAPON_WAND;
  else if ( subclass_str == "Fishing Pole"                    ) weapon_type = WEAPON_POLEARM;
  else if ( subclass_str == "Miscellaneous"                   ) weapon_type = WEAPON_POLEARM;

  if ( weapon_type == WEAPON_NONE ) return false;
  if ( weapon_type == WEAPON_WAND ) return true;

  item.armory_weapon_str = util_t::weapon_type_string( weapon_type );
  item.armory_weapon_str += "_" + speed_str + "speed" + "_" + dmg_min_str + "min" + "_" + dmg_max_str + "max";

  return true;
}

// parse_item_stats =========================================================

static bool parse_item_stats( item_t&     item,
                              xml_node_t* node )
{
  item.armory_stats_str.clear();

  std::vector<std::string> descriptions;
  get_tti_value( descriptions, node, "tti-stat"  );
  get_tti_value( descriptions, node, "tti-stats" );
  size_t num_descriptions = descriptions.size();
  for ( size_t i=0; i < num_descriptions; i++ )
  {
    armory_t::fuzzy_stats( item.armory_stats_str, descriptions[ i ] );
  }

  std::vector<xml_node_t*> spells;
  int num_spells = get_tti_nodes( spells, node, "tti-spells" );
  for ( int i=0; i < num_spells; i++ )
  {
    std::string description;
    if ( xml_t::get_value( description, spells[ i ], "a/." ) )
    {
      armory_t::fuzzy_stats( item.armory_stats_str, description );
    }
  }

  std::string armor_str;
  if ( get_tti_value( armor_str, node, "tti-armor" ) )
  {
    std::string::size_type pos = armor_str.find( " " );
    if ( pos != std::string::npos ) armor_str.erase( pos );
    item.armory_stats_str += "_";
    item.armory_stats_str += armor_str + "armor";
  }

  std::string block_str;
  if ( get_tti_value( armor_str, node, "tti-block" ) )
  {
    std::string::size_type pos = block_str.find( " " );
    if ( pos != std::string::npos ) block_str.erase( pos );
    item.armory_stats_str += "_";
    item.armory_stats_str += block_str + "blockv";
  }

  armory_t::format( item.armory_stats_str );

  return true;
}

// parse_item_stats =========================================================

static bool parse_item_reforge( item_t&     item,
                                xml_node_t* /* node */ )
{
  item.armory_reforge_str.clear();

  // TO-DO (if it even makes sense)

  armory_t::format( item.armory_reforge_str );

  return true;
}

// parse_item_name ==========================================================

static bool parse_item_name( item_t&            item,
                             xml_node_t*        node,
                             const std::string& item_id )
{
  std::string& s = item.armory_name_str;

  if ( ! xml_t::get_value( s, node, "title/." ) )
    return false;

  std::string::size_type pos = s.find( " - Items" ); // MMOC appends " - Items - Sigrie" to item names
  if ( pos != std::string::npos ) s.erase( pos );

  // The MMO-Champion names often have numbers embedded in the name.....
  for ( int i=0; i < ( int ) s.size(); i++ )
  {
    if ( isdigit( s[ i ] ) )
    {
      s.erase( i, 1 );
      i--;
    }
  }

  armory_t::format( s );

  item.armory_id_str = item_id;

  return true;
}

// parse_item_heroic ========================================================

static bool parse_item_heroic( item_t&     item,
                               xml_node_t* node )
{
  item.armory_heroic_str = "";

  std::vector<std::string> descriptions;
  get_tti_value( descriptions, node, "tti-heroic"  );

  if ( ! descriptions.empty() && ! descriptions[ 0 ].empty() )
  {
    item.armory_heroic_str = "1";
    armory_t::format( item.armory_heroic_str );
  }

  return true;
}

// parse_item_lfr ==========================================================

static bool parse_item_lfr( item_t& item, xml_node_t* node )
{
  // FIXME: MMOC currently doesn't have a flag for LFR
  return true;

  item.armory_lfr_str = "";

  std::vector<std::string> descriptions;
  get_tti_value( descriptions, node, "tti-raid-finder"  );

  if ( ! descriptions.empty() && ! descriptions[ 0 ].empty() )
  {
    item.armory_lfr_str = "1";
    armory_t::format( item.armory_lfr_str );
  }

  return true;
}

// parse_item_armor_type ====================================================

static bool parse_item_armor_type( item_t&     item,
                                   xml_node_t* node )
{
  item.armory_armor_type_str = "";

  std::vector<std::string> descriptions;
  get_tti_value( descriptions, node, "tti-subclass"  );

  if ( ! descriptions.empty() && ! descriptions[ 0 ].empty() )
  {
    item.armory_armor_type_str = descriptions[ 0 ];
    armory_t::format( item.armory_armor_type_str );
  }

  return true;
}

// parse_item_level =========================================================

static bool parse_item_level( item_t&     item,
                              xml_node_t* node )
{
  item.armory_ilevel_str.clear();

  std::string info_str;
  if ( ! get_tti_value( info_str, node, "tti-level" ) ) return false;

  std::size_t p_lvl = info_str.rfind( "Item Level " );

  if ( p_lvl == info_str.npos ) return false;

  p_lvl += strlen( "Item Level " );

  item.armory_ilevel_str = info_str.substr( p_lvl );

  return true;
}

// parse_quality ============================================================

static bool parse_quality( item_t&     item,
                           xml_node_t* node )
{
  item.armory_quality_str.clear();

  std::string info_str;
  if ( ! get_tti_value( info_str, node, "tti-quality" ) ) return false;

  item.armory_quality_str = tolower( info_str );

  return true;
}

} // ANONYMOUS NAMESPACE ====================================================


// mmo_champion_t::parse_gem ================================================

int mmo_champion_t::parse_gem( item_t&            item,
                               const std::string& gem_id,
                               cache::behavior_t  caching )
{
  if ( gem_id.empty() || gem_id == "0" )
    return GEM_NONE;

  xml_node_t* node = download_id( item.sim, gem_id, caching );
  if ( ! node )
  {
    if ( caching != cache::ONLY )
      item.sim -> errorf( "Player %s unable to download gem id %s from mmo-champion.\n",
                          item.player -> name(), gem_id.c_str() );
    return GEM_NONE;
  }

  if ( node )
  {
    std::string color_str;
    if ( get_tti_value( color_str, node, "tti-subclass" ) )
    {
      armory_t::format( color_str );
      int gem_type = util_t::parse_gem_type( color_str );

      std::string property_str;
      xml_node_t* property_node = get_tti_node( node, "tti-gem_properties" );
      if ( property_node ) xml_t::get_value( property_str, property_node, "a/." );

      if ( gem_type == GEM_NONE || property_str.empty() )
        return GEM_NONE;

      std::string& s = item.armory_gems_str;

      if ( gem_type == GEM_META )
      {
        int meta_gem_type = armory_t::parse_meta_gem( property_str );

        if ( meta_gem_type != META_GEM_NONE )
        {
          s += "_";
          s += util_t::meta_gem_type_string( meta_gem_type );
        }
        else
        {
          armory_t::fuzzy_stats( s, property_str );
        }
      }
      else
      {
        armory_t::fuzzy_stats( s, property_str );
      }

      return gem_type;
    }
  }

  return GEM_NONE;
}

// mmo_champion_t::download_glyph ===========================================

bool mmo_champion_t::download_glyph( player_t*          player,
                                     std::string&       glyph_name,
                                     const std::string& glyph_id,
                                     cache::behavior_t  caching )
{
  xml_node_t* node = download_id( player -> sim, glyph_id, caching );
  if ( ! node || ! xml_t::get_value( glyph_name, node, "title/." ) )
  {
    if ( caching != cache::ONLY )
      player -> sim -> errorf( "Unable to download glyph id %s from mmo-champion\n", glyph_id.c_str() );
    return false;
  }

  glyph_name.erase( 0, 9 ); // remove "Glyph of "
  std::string::size_type pos = glyph_name.find( " - " );
  if ( pos != std::string::npos ) glyph_name.erase( pos );
  armory_t::format( glyph_name );
  return true;
}

// mmo_champion_t::download_item ============================================

bool mmo_champion_t::download_item( item_t&            item,
                                    const std::string& item_id,
                                    cache::behavior_t  caching )
{
  player_t* p = item.player;

  xml_node_t* node = download_id( item.sim, item_id, caching );
  if ( ! node )
  {
    if ( caching != cache::ONLY )
      item.sim -> errorf( "Player %s unable to download item id '%s' from mmo-champion at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_name( item, node, item_id ) )
  {
    item.sim -> errorf( "Player %s unable to determine item name for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_heroic( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine heroic flag for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_lfr( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine LFR flag for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_level( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine item level for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_quality( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine item quality for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_armor_type( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine armor type for id %s at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_stats( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine stats for item '%s' at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if ( ! parse_weapon( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine weapon info for item '%s' at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  return true;
}

// mmo_champion_t::download_slot ============================================

bool mmo_champion_t::download_slot( item_t&            item,
                                    const std::string& item_id,
                                    const std::string& enchant_id,
                                    const std::string& addon_id,
                                    const std::string& reforge_id,
                                    const std::string& rsuffix_id,
                                    const std::string  gem_ids[ 3 ],
                                    cache::behavior_t  caching )
{
  player_t* p = item.player;

  xml_node_t* node = download_id( item.sim, item_id, caching );
  if ( ! node )
  {
    if ( caching != cache::ONLY )
      item.sim -> errorf( "Player %s nable to download item id '%s' from mmo-champion at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_name( item, node, item_id ) )
  {
    item.sim -> errorf( "Player %s unable to determine item name for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_heroic( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine heroic flag for item '%s' at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_lfr( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine LFR flag for item '%s' at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_level( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine item level for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_quality( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine item quality for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_armor_type( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine armor type for id %s at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_stats( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine stats for item '%s' at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_reforge( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine reforge for item '%s' at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if ( ! parse_weapon( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine weapon info for item '%s' at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if ( ! parse_gems( item, node, gem_ids ) )
  {
    item.sim -> errorf( "Player %s unable to determine gems for item '%s' at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if ( ! enchant_t::download( item, enchant_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse enchant id %s for item \"%s\" at slot %s.\n", p -> name(), enchant_id.c_str(), item.name(), item.slot_name() );
    //return false;
  }

  if ( ! enchant_t::download_addon( item, addon_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse addon id %s for item \"%s\" at slot %s.\n", p -> name(), addon_id.c_str(), item.name(), item.slot_name() );
    //return false;
  }

  if ( ! enchant_t::download_reforge( item, reforge_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse reforge id %s for item \"%s\" at slot %s.\n", p -> name(), reforge_id.c_str(), item.name(), item.slot_name() );
    //return false;
  }

  if ( ! enchant_t::download_rsuffix( item, rsuffix_id ) )
  {
    item.sim -> errorf( "Player %s unable to determine random suffix '%s' for item '%s' at slot %s.\n", p -> name(), rsuffix_id.c_str(), item.name(), item.slot_name() );
    return false;
  }

  return true;
}

