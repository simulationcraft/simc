// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"
#define RANGE_BITS 24
#define RANGE_HIGH ((1<<RANGE_BITS)-1)
#define RANGE_HIGH_MASK (0x3f<<(RANGE_BITS-6))
#define RANGE_LOW_MASK ((1<<(RANGE_BITS-6))-1)

static const char* base64 = "f-qR3eOHQ9dSIMwk8pabYt6yrJUFNXLTh4n2KWEoz0uC5j7xmAgDlZiPcs_BGV1v";  // sufficiently randomized 10+26+26+2 = 64

namespace   // ANONYMOUS NAMESPACE ==========================================
{

// download_id ==============================================================

static xml_node_t* download_id( sim_t*             sim,
                                const std::string& id_str,
                                int                cache_only=0 )
{
  if ( id_str.empty() || id_str == "" || id_str == "0" ) return 0;
  std::string url = "http://db.mmo-champion.com/i/" + id_str + "/";

  xml_node_t* node;

  if ( cache_only )
    node = xml_t::download_cache( sim, url );
  else
    node = xml_t::download( sim, url, "<h4>Item #", 0 );

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

// get_tti_value ===========================================================

static bool get_tti_value( std::string&       value,
                           xml_node_t*        root,
                           const std::string& name )
{
  xml_node_t* node = get_tti_node( root, name );

  if ( node )
  {
    if( xml_t::get_value( value, node, "." ) )
    {
      return true;
    }
    else if( xml_t::get_value( value, node, "a/." ) )
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
    xml_node_t* socket_bonus_node = get_tti_node( node, "tti-socketbonus" );
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
      return true;

  std::string speed_str, dps_str, dmg_min_str, dmg_max_str;

  std::vector<std::string> tokens;
  int num_tokens = util_t::string_split( tokens, tti_speed, " " );
  if ( num_tokens == 2 ) speed_str = tokens[ 1 ];

  tokens.clear();
  num_tokens = util_t::string_split( tokens, tti_dmg, " " );
  if ( num_tokens == 4 )
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

  if( slot_str == "Main Hand" ) slot_str = "One-Hand";

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
  int num_descriptions = descriptions.size();
  for ( int i=0; i < num_descriptions; i++ )
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

  std::string armor_str;;
  if ( get_tti_value( armor_str, node, "tti-armor" ) )
  {
    std::string::size_type pos = armor_str.find( " " );
    if ( pos != std::string::npos ) armor_str.erase( pos );
    item.armory_stats_str += "_";
    item.armory_stats_str += armor_str + "armor";
  }

  std::string block_str;;
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

// parse_item_name ==========================================================

static bool parse_item_name( item_t&            item,
                             xml_node_t*        node,
                             const std::string& item_id )
{
  std::string& s = item.armory_name_str;

  if ( ! xml_t::get_value( s, node, "title/." ) )
    return false;

  std::string::size_type pos = s.find( " - " );
  if ( pos != std::string::npos ) s.erase( pos );

  // The MMO-Champion names often have numbers embedded in the name.....
  for ( int i=0; i < s.size(); i++ )
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

// parse_item_heroic =========================================================

static bool parse_item_heroic( item_t&     item,
                               xml_node_t* node )
{
  std::string heroic_str;
  item.armory_heroic_str = "";

  if ( xml_t::get_value( heroic_str, node, "tti-heroic" ) )
  {
    item.armory_heroic_str = "1";
  }

  armory_t::format( item.armory_heroic_str );

  return true;
}


} // ANONYMOUS NAMESPACE ====================================================


// mmo_champion_t::parse_gem ================================================================

int mmo_champion_t::parse_gem( item_t&            item,
                               const std::string& gem_id,
                               int cache_only )
{
  if ( gem_id.empty() || gem_id == "" || gem_id == "0" )
    return GEM_NONE;

  xml_node_t* node = download_id( item.sim, gem_id, cache_only );
  if ( ! node )
  {
    if ( ! cache_only )
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
      xml_node_t* property_node = get_tti_node( node, "tti-gemproperties" );
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

bool mmo_champion_t::download_glyph( sim_t*             sim,
                                     std::string&       glyph_name,
                                     const std::string& glyph_id,
                                     int cache_only )
{
  xml_node_t* node = download_id( sim, glyph_id, cache_only );
  if ( ! node || ! xml_t::get_value( glyph_name, node, "title/." ) )
  {
    if ( ! cache_only )
      sim -> errorf( "Unable to download glyph id %s from mmo-champion\n", glyph_id.c_str() );
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
                                    int cache_only )
{
  player_t* p = item.player;

  xml_node_t* node = download_id( item.sim, item_id, cache_only );
  if ( ! node )
  {
    if ( ! cache_only )
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
    item.sim -> errorf( "Player %s unable to determine heroic flag for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
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
                                    const std::string  gem_ids[ 3 ],
                                    int cache_only )
{
  player_t* p = item.player;

  xml_node_t* node = download_id( item.sim, item_id, cache_only );
  if ( ! node )
  {
    if ( ! cache_only )
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

  return true;
}

bool mmo_champion_t::parse_talents( player_t* player, const std::string& talent_string)
{
  unsigned int i;
  unsigned int visible[MAX_TALENT_SLOTS] = {0};
  int trees[3] = {0};
  int numleft = MAX_TALENT_POINTS;
  int thismax;
  unsigned int rangelow = 0;
  unsigned int rangehigh = RANGE_HIGH;
  unsigned int rangeval = 0;
  unsigned int div1;
  int mul1;
  int mul2;
  unsigned int len;
  char c;
  int newvisible = 1;
  int talents[MAX_TALENT_SLOTS] = {0};

  std::vector<talent_translation_t> translations = player->get_talent_list();

  // need to have empty talent in the front of vector
  std::vector<talent_translation_t> talent_list;
  talent_translation_t empty_talent = {0,0,0,0,0,0,0};
  talent_list.push_back(empty_talent);
  for(i=0;i<translations.size();i++)
	talent_list.push_back(translations[i]);

  char* src = (char*)talent_string.c_str();

  for(i=0;i<RANGE_BITS/6;i++) get_next_range_byte(&rangeval, &src);    // get 24 bits

    // 0 = disabled
    // 1 = enabled this pass
    // 2 = already scanned over, not considering again

  visible[0] = 2;
  while(newvisible) {
    newvisible = 0;

      // step 1: enable (consider) next row, if requirements met

    for(i=1; i < talent_list.size();i++) {
      if(visible[i] == 0 && trees[talent_list[i].tree] >= 5*talent_list[i].row && visible[talent_list[i].req] == 2 && talents[talent_list[i].req] == talent_list[talent_list[i].req].max) visible[i] = 1;
    }

      // step 2: step through each enabled node, convert

	  for(i=1; i < talent_list.size();i++) {
		  if(!numleft) break;

		  if(visible[i] == 1) 
		  {
			newvisible = 1;
			visible[i] = 2;

		  thismax = numleft;
		  if(talent_list[i].max < thismax) thismax = talent_list[i].max;

		  div1 = thismax+1;        // # choices
		  mul2 = 1;                // prob of just this one

			len = rangehigh - rangelow;

			for(mul1=0;mul1<thismax+1;mul1++) {
			  if(rangeval >= rangelow + len * mul1 / div1 && rangeval < rangelow + len * (mul1 + mul2) / div1) break;
			}

			talents[i] = mul1;
			if(talents[i] > talent_list[i].max) {    // probably corrupt string or wrong version, we can't really continue decode
			  return 1;
			}

			rangehigh = rangelow + len * (mul1 + mul2) / div1;// - 1;
			rangelow += len * mul1 / div1;

			while( (rangelow & RANGE_HIGH_MASK) == (rangehigh & RANGE_HIGH_MASK) ) {
			  c = (rangelow >> (RANGE_BITS-6)) & 0x3f;
			  len = rangehigh - rangelow;
			  rangelow = ((rangelow & RANGE_LOW_MASK) << 6);
			  rangehigh = rangelow + len*0x40 - 1;
			  get_next_range_byte(&rangeval, &src);    // get 6 bits
			}

			// update
			trees[talent_list[i].tree] += talents[i];
			numleft -= talents[i];
		  }
	  }
  }
  for(i=0;i<translations.size();i++)
	  talents[i] = talents[i+1];
  return  player->parse_talent_trees(talents);
}

void mmo_champion_t::get_next_range_byte(unsigned int* rangeval, char** src) {
  *rangeval = ((*rangeval)&(RANGE_HIGH>>6))<<6;
  if(**src && strchr(base64, **src)) {
    *rangeval |= (strchr(base64, **src)-base64);
    *src = (*src) + 1;
  }
}
