// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

// download_id ==============================================================

static xml_node_t* download_id( const std::string& id_str )
{
  if( id_str.empty() || id_str == "" || id_str == "0" ) return 0;
  std::string url = "http://db.mmo-champion.com/i/" + id_str;
  xml_node_t* node = xml_t::download( url, "<h4>Item #", 0 );
  xml_t::print( node );
  if( ! node ) printf( "\nsimcraft: Unable to download id %s from mmo-champion\n", id_str.c_str() );
  return node;
}

// get_tti_node =============================================================

static xml_node_t* get_tti_node( xml_node_t*        node,
				 const std::string& name )
{
  std::string tti_name = "tti-"; tti_name += name;

  std::vector<xml_node_t*> div_nodes;
  int num_nodes = xml_t::get_nodes( div_nodes, node, "div" );

  for( int i=0; i < num_nodes; i++ )
  {
    std::string class_str;
    if( xml_t::get_value( class_str, div_nodes[ i ], "class" ) )
    {
      if( ! strncmp( class_str.c_str(), tti_name.c_str(), tti_name.size() ) )
      {
	return div_nodes[ i ];
      }
    }
  }

  return 0;
}

// get_tti_value ===========================================================

static bool get_tti_value( std::string&       value,
			   xml_node_t*        node,
			   const std::string& name )
{
  std::string tti_name = "tti-"; tti_name += name;

  std::vector<xml_node_t*> div_nodes;
  int num_nodes = xml_t::get_nodes( div_nodes, node, "div" );

  for( int i=0; i < num_nodes; i++ )
  {
    std::string class_str;
    if( xml_t::get_value( class_str, div_nodes[ i ], "class" ) )
    {
      if( ! strncmp( class_str.c_str(), tti_name.c_str(), tti_name.size() ) )
      {
	if( xml_t::get_value( value, div_nodes[ i ], "." ) )
	{
	  return true;
	}
      }
    }
  }

  return false;
}

// get_tti_value ============================================================

static int get_tti_value( std::vector<std::string>& values,
			  xml_node_t*               node,
			  const std::string&        name )
{
  values.clear();

  std::string tti_name = "tti-"; tti_name += name;

  std::vector<xml_node_t*> div_nodes;
  int num_nodes = xml_t::get_nodes( div_nodes, node, "div" );

  for( int i=0; i < num_nodes; i++ )
  {
    std::string class_str;
    if( xml_t::get_value( class_str, div_nodes[ i ], "class" ) )
    {
      if( ! strncmp( class_str.c_str(), tti_name.c_str(), tti_name.size() ) )
      {
	std::string value;
	if( xml_t::get_value( value, div_nodes[ i ], "." ) )
	{
	  values.push_back( value );
	}
      }
    }
  }

  return values.size();
}

// parse_gem ================================================================

static int parse_gem( item_t&            item, 
                      const std::string& gem_id )
{
  if( gem_id.empty() || gem_id == "" || gem_id == "0" )
    return GEM_NONE;

  xml_node_t* node = download_id( gem_id );

  if( node )
  {
    std::string color_str;
    if( get_tti_value( color_str, node, "subclass" ) )
    {
      armory_t::format( color_str );
      int gem_type = util_t::parse_gem_type( color_str );

      std::string property_str;
      xml_node_t* property_node = get_tti_node( node, "gemproperties" );
      if( property_node ) xml_t::get_value( property_str, property_node, "a/." );
    
      if( gem_type == GEM_NONE || property_str.empty() ) 
	return GEM_NONE;

      std::string& s = item.armory_gems_str;

      if( gem_type == GEM_META )
      {
	int meta_gem_type = armory_t::parse_meta_gem( property_str );

	if( meta_gem_type != META_GEM_NONE )
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

// parse_gems ===============================================================

static bool parse_gems( item_t&           item, 
                        xml_node_t*       node,
                        const std::string gem_ids[ 3 ] )
{
  item.armory_gems_str.clear();

  int sockets[ 3 ] = { GEM_NONE, GEM_NONE, GEM_NONE };

  std::vector<std::string> socket_colors;
  int num_sockets = get_tti_value( socket_colors, node, "socket" );
  for( int i=0; i < num_sockets && i < 3; i++ )
  {
    std::string& s = socket_colors[ i ];
    std::string::size_type pos = s.find( " " );
    if( pos != std::string::npos ) s.erase( pos );
    armory_t::format( s );
    sockets[ i ] = util_t::parse_gem_type( s );
  }

  bool match = true;
  for( int i=0; i < 3; i++ )
  {
    int gem = parse_gem( item, gem_ids[ i ] );

    if( ! util_t::socket_gem_match( sockets[ i ], gem ) )
      match = false;
  }

  if( match )
  {
    xml_node_t* socket_bonus_node = get_tti_node( node, "socketbonus" );
    if( socket_bonus_node )
    {
      std::string socket_bonus_str;
      if( xml_t::get_value( socket_bonus_str, socket_bonus_node, "a/." ) )
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
  std::string tti_speed, tti_dps;

  if( ! get_tti_value( tti_speed, node, "speed" ) ||
      ! get_tti_value( tti_dps,   node, "dps"   ) )
    return true;

  std::string speed_str, dps_str;

  std::vector<std::string> tokens;
  int num_tokens = util_t::string_split( tokens, tti_speed, " " );
  if( num_tokens == 2 ) speed_str = tokens[ 1 ];

  num_tokens = util_t::string_split( tokens, tti_dps, " ()" );
  if( num_tokens == 4 ) dps_str = tokens[ 0 ];

  if( speed_str.empty() || dps_str.empty() ) return false;

  std::string subclass_str, slot_str;

  if( ! get_tti_value( subclass_str, node, "subclass" ) ||
      ! get_tti_value( slot_str,     node, "slot"     ) )
    return false;

  int weapon_type = WEAPON_NONE;
  if     ( subclass_str == "Axe" && slot_str == "One-Hand"   ) weapon_type = WEAPON_AXE;
  else if( subclass_str == "Axe" && slot_str == "Two-Hand"   ) weapon_type = WEAPON_AXE_2H;
  else if( subclass_str == "Dagger"                          ) weapon_type = WEAPON_DAGGER;
  else if( subclass_str == "Fist Weapon"                     ) weapon_type = WEAPON_FIST;
  else if( subclass_str == "Mace" && slot_str == "One-Hand"  ) weapon_type = WEAPON_MACE;
  else if( subclass_str == "Mace" && slot_str == "Two-Hand"  ) weapon_type = WEAPON_MACE_2H;
  else if( subclass_str == "Polearm"                         ) weapon_type = WEAPON_POLEARM;
  else if( subclass_str == "Staff"                           ) weapon_type = WEAPON_STAFF;
  else if( subclass_str == "Sword" && slot_str == "One-Hand" ) weapon_type = WEAPON_SWORD;
  else if( subclass_str == "Sword" && slot_str == "Two-Hand" ) weapon_type = WEAPON_SWORD_2H;
  else if( subclass_str == "Bow"                             ) weapon_type = WEAPON_BOW;
  else if( subclass_str == "Crossbow"                        ) weapon_type = WEAPON_CROSSBOW;
  else if( subclass_str == "Gun"                             ) weapon_type = WEAPON_GUN;
  else if( subclass_str == "Thrown"                          ) weapon_type = WEAPON_THROWN;
  else if( subclass_str == "Wand"                            ) weapon_type = WEAPON_WAND;
  else if( subclass_str == "Fishing Pole"                    ) weapon_type = WEAPON_POLEARM;
  else if( subclass_str == "Miscellaneous"                   ) weapon_type = WEAPON_POLEARM;

  if( weapon_type == WEAPON_NONE ) return false;
  if( weapon_type == WEAPON_WAND ) return true;
    
  item.armory_weapon_str = util_t::weapon_type_string( weapon_type );
  item.armory_weapon_str += "_" + speed_str + "speed" + "_" + dps_str + "dps";

  return true;
}

// parse_item_stats =========================================================

static bool parse_item_stats( item_t&     item,
                              xml_node_t* node )
{
  item.armory_stats_str.clear();

  std::vector<std::string> descriptions;
  int num_descriptions = get_tti_value( descriptions, node, "stats" );

  for( int i=0; i < num_descriptions; i++ )
  {
    armory_t::fuzzy_stats( item.armory_stats_str, descriptions[ i ] );
  }

  armory_t::format( item.armory_stats_str );

  return true;
}

// parse_item_name ==========================================================

static bool parse_item_name( item_t&     item,
                             xml_node_t* node )
{
  std::string& s = item.armory_name_str;

  if( ! xml_t::get_value( s, node, "title/." ) )
    return false;

  std::string::size_type pos = s.find( " - " );
  if( pos != std::string::npos ) s.erase( pos );
  armory_t::format( s );

  return true;
}

} // ANONYMOUS NAMESPACE ====================================================

// mmo_champion_t::download_glyph ===========================================

bool mmo_champion_t::download_glyph( std::string&       glyph_name,
				     const std::string& glyph_id )
{
  xml_node_t* node = download_id( glyph_id );
  if( node )
  {
    if( xml_t::get_value( glyph_name, node, "title/." ) )
    {
      glyph_name.erase( 0, 9 ); // remove "Glyph of "
      std::string::size_type pos = glyph_name.find( " - " );
      if( pos != std::string::npos ) glyph_name.erase( pos );
      armory_t::format( glyph_name );
      return true;
    }
  }
  return false;
}

// mmo_champion_t::download_item ============================================

bool mmo_champion_t::download_item( item_t&            item, 
                               const std::string& item_id )
{
  player_t* p = item.player;

  xml_node_t* node = download_id( item_id );
  if( ! node ) 
  {
    printf( "\nsimcraft: Player %s nable to download item id '%s' from mmo-champion at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if( ! parse_item_name( item, node ) )
  {
    printf( "\nsimcraft: Player %s unable to determine item name for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if( ! parse_item_stats( item, node ) )
  {
    printf( "\nsimcraft: Player %s unable to determine stats for item '%s' at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if( ! parse_weapon( item, node ) )
  {
    printf( "\nsimcraft: Player %s unable to determine weapon info for item '%s' at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  return true;
}

// mmo_champion_t::download_slot ============================================

bool mmo_champion_t::download_slot( item_t&            item, 
                               const std::string& item_id,
                               const std::string& enchant_id, 
                               const std::string  gem_ids[ 3 ] )
{
  player_t* p = item.player;

  xml_node_t* node = download_id( item_id );
  if( ! node ) 
  {
    printf( "\nsimcraft: Player %s nable to download item id '%s' from mmo-champion at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if( ! parse_item_name( item, node ) )
  {
    printf( "\nsimcraft: Player %s unable to determine item name for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if( ! parse_item_stats( item, node ) )
  {
    printf( "\nsimcraft: Player %s unable to determine stats for item '%s' at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if( ! parse_weapon( item, node ) )
  {
    printf( "\nsimcraft: Player %s unable to determine weapon info for item '%s' at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if( ! parse_gems( item, node, gem_ids ) )
  {
    printf( "\nsimcraft: Player %s unable to determine gems for item '%s' at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if( ! enchant_t::download( item, enchant_id ) )
  {
    printf( "\nsimcraft: Player %s unable to parse enchant id %s for item \"%s\" at slot %s.\n", p -> name(), enchant_id.c_str(), item.name(), item.slot_name() );
    //return false;
  }

  return true;
}

