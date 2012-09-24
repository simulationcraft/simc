// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#define RANGE_BITS 24
#define RANGE_HIGH ((1<<RANGE_BITS)-1)
#define RANGE_HIGH_MASK (0x3f<<(RANGE_BITS-6))
#define RANGE_LOW_MASK ((1<<(RANGE_BITS-6))-1)

// static const char* base64 = "f-qR3eOHQ9dSIMwk8pabYt6yrJUFNXLTh4n2KWEoz0uC5j7xmAgDlZiPcs_BGV1v";  // sufficiently randomized 10+26+26+2 = 64

// download_id ==============================================================

static xml_node_t* download_id( sim_t*             sim,
                                const std::string& id_str,
                                cache::behavior_e  caching )
{
  if ( id_str.empty() || id_str == "0" ) return 0;
  std::string url = "http://db.mmo-champion.com/i/" + id_str + '/';

  xml_node_t* node = xml_node_t::get( sim, url, caching, "<h4>Item #" );

  if ( sim -> debug && node ) node -> print();
  return node;
}

// get_tti_node =============================================================

static xml_node_t* get_tti_node( xml_node_t*        root,
                                 const std::string& name )
{
  if ( ! root ) return 0;

  std::string class_str;
  if ( root->get_value( class_str, "class" ) )
  {
    if ( ! strncmp( class_str.c_str(), name.c_str(), name.size() ) )
    {
      return root;
    }
  }

  std::vector<xml_node_t*> children;
  int num_children = root -> get_children( children );
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
  if ( ! root ) return ( int ) nodes.size();

  std::string class_str;
  if ( root -> get_value( class_str, "class" ) )
  {
    if ( ! strncmp( class_str.c_str(), name.c_str(), name.size() ) )
    {
      nodes.push_back( root );
    }
  }

  std::vector<xml_node_t*> children;
  int num_children = root -> get_children( children );
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
  if ( ! root ) return false;

  xml_node_t* node = get_tti_node( root, name );

  if ( node )
  {
    if ( node -> get_value( value, "." ) )
    {
      return true;
    }
    else if ( node -> get_value( value, "a/." ) )
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
  if ( ! root ) return ( int ) values.size();

  std::string class_str;
  if ( root -> get_value( class_str, "class" ) )
  {
    if ( ! strncmp( class_str.c_str(), name.c_str(), name.size() ) )
    {
      std::string value;
      if ( root -> get_value( value, "." ) )
      {
        values.push_back( value );
      }
      else if ( root -> get_value( value, "a/." ) )
      {
        values.push_back( value );
      }
    }
  }

  std::vector<xml_node_t*> children;
  int num_children = root -> get_children( children );
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

  gem_e sockets[ 3 ] = { GEM_NONE, GEM_NONE, GEM_NONE };

  std::vector<std::string> socket_colors;
  int num_sockets = get_tti_value( socket_colors, node, "tti-socket" );
  for ( int i=0; i < num_sockets && i < 3; i++ )
  {
    std::string& s = socket_colors[ i ];
    std::string::size_type pos = s.find( " " );
    if ( pos != std::string::npos ) s.erase( pos );
    util::tokenize( s );
    sockets[ i ] = util::parse_gem_type( s );
  }

  bool match = true;
  for ( int i=0; i < 3; i++ )
  {
    if ( ! util::socket_gem_match( sockets[ i ],
                                   item_t::parse_gem( item, gem_ids[ i ] ) ) )
      match = false;
  }

  if ( match )
  {
    xml_node_t* socket_bonus_node = get_tti_node( node, "tti-socket_bonus" );
    if ( socket_bonus_node )
    {
      std::string socket_bonus_str;
      if ( socket_bonus_node -> get_value( socket_bonus_str, "a/." ) )
      {
        util::fuzzy_stats( item.armory_gems_str, socket_bonus_str );
      }
    }
  }

  util::tokenize( item.armory_gems_str );

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
  int num_tokens = util::string_split( tokens, tti_speed, " " );
  if ( num_tokens == 2 ) speed_str = tokens[ 1 ];

  tokens.clear();
  num_tokens = util::string_split( tokens, tti_dmg, " " );
  if ( ( num_tokens == 4 ) || ( num_tokens == 5 ) )
  {
    dmg_min_str = tokens[ 0 ];
    dmg_max_str = tokens[ 2 ];
  }

  tokens.clear();
  num_tokens = util::string_split( tokens, tti_dps, " ()" );
  if ( num_tokens == 4 ) dps_str = tokens[ 0 ];

  if ( speed_str.empty() || dps_str.empty() || dmg_min_str.empty() || dmg_max_str.empty() ) return false;

  std::string subclass_str, slot_str;

  if ( ! get_tti_value( subclass_str, node, "tti-subclass" ) ||
       ! get_tti_value( slot_str,     node, "tti-slot"     ) )
    return false;

  if ( ( slot_str == "Main Hand" ) || ( slot_str == "Off-Hand" ) ) slot_str = "One-Hand";

  weapon_e type = WEAPON_NONE;
  if      ( subclass_str == "Axe" && slot_str == "One-Hand"   ) type = WEAPON_AXE;
  else if ( subclass_str == "Axe" && slot_str == "Two-Hand"   ) type = WEAPON_AXE_2H;
  else if ( subclass_str == "Dagger"                          ) type = WEAPON_DAGGER;
  else if ( subclass_str == "Fist Weapon"                     ) type = WEAPON_FIST;
  else if ( subclass_str == "Mace" && slot_str == "One-Hand"  ) type = WEAPON_MACE;
  else if ( subclass_str == "Mace" && slot_str == "Two-Hand"  ) type = WEAPON_MACE_2H;
  else if ( subclass_str == "Polearm"                         ) type = WEAPON_POLEARM;
  else if ( subclass_str == "Staff"                           ) type = WEAPON_STAFF;
  else if ( subclass_str == "Sword" && slot_str == "One-Hand" ) type = WEAPON_SWORD;
  else if ( subclass_str == "Sword" && slot_str == "Two-Hand" ) type = WEAPON_SWORD_2H;
  else if ( subclass_str == "Bow"                             ) type = WEAPON_BOW;
  else if ( subclass_str == "Crossbow"                        ) type = WEAPON_CROSSBOW;
  else if ( subclass_str == "Gun"                             ) type = WEAPON_GUN;
  else if ( subclass_str == "Thrown"                          ) type = WEAPON_THROWN;
  else if ( subclass_str == "Wand"                            ) type = WEAPON_WAND;
  else if ( subclass_str == "Fishing Pole"                    ) type = WEAPON_POLEARM;
  else if ( subclass_str == "Miscellaneous"                   ) type = WEAPON_POLEARM;

  if ( type == WEAPON_NONE ) return false;
  if ( type == WEAPON_WAND ) return true;

  item.armory_weapon_str = util::weapon_type_string( type );
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
    util::fuzzy_stats( item.armory_stats_str, descriptions[ i ] );
  }

  std::vector<xml_node_t*> spells;
  int num_spells = get_tti_nodes( spells, node, "tti-spells" );
  for ( int i=0; i < num_spells; i++ )
  {
    std::string description;
    if ( spells[ i ] && spells[ i ] -> get_value( description, "a/." ) )
    {
      util::fuzzy_stats( item.armory_stats_str, description );
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

  util::tokenize( item.armory_stats_str );

  return true;
}

// parse_item_stats =========================================================

static bool parse_item_reforge( item_t&     item,
                                xml_node_t* /* node */ )
{
  item.armory_reforge_str.clear();

  // TO-DO (if it even makes sense)

  util::tokenize( item.armory_reforge_str );

  return true;
}

// parse_item_name ==========================================================

static bool parse_item_name( item_t&            item,
                             xml_node_t*        node,
                             const std::string& item_id )
{
  std::string& s = item.armory_name_str;

  if ( ! node || ! node -> get_value( s, "title/." ) )
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

  util::tokenize( s );

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
    util::tokenize( item.armory_heroic_str );
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
    util::tokenize( item.armory_lfr_str );
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
    util::tokenize( item.armory_armor_type_str );
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

  item.armory_quality_str = util::tolower( info_str );

  return true;
}


// armory_t::parse_meta_gem =================================================

static meta_gem_e parse_meta_gem( const std::string& description )
{
  if ( description == "+54 Agility and 3% Increased Critical Damage"                      ) return META_AGILE_SHADOWSPIRIT;
  if ( description == "+32 Stamina and 2% Increased Armor Value from Items"               ) return META_AUSTERE_EARTHSIEGE;
  if ( description == "+81 Stamina and 2% Increased Armor Value from Items"               ) return META_AUSTERE_SHADOWSPIRIT;
  if ( description == "+21 Critical Strike Rating and +2% Mana"                           ) return META_BEAMING_EARTHSIEGE;
  if ( description == "+21 Intellect and +2% Reduced Threat"                              ) return META_BRACING_EARTHSIEGE;
  if ( description == "+12 Intellect and +2% Reduced Threat"                              ) return META_BRACING_EARTHSTORM;
  if ( description == "+54 Intellect and +2% Reduced Threat"                              ) return META_BRACING_SHADOWSPIRIT;
  if ( description == "+54 Intellect and 3% Increased Critical Damage"                    ) return META_BURNING_SHADOWSPIRIT;
  if ( description == "+54 Critical Strike Rating and 3% Increased Critical Damage"       ) return META_CHAOTIC_SHADOWSPIRIT;
  if ( description == "+21 Critical Strike Rating and 3% Increased Critical Damage"       ) return META_CHAOTIC_SKYFLARE;
  if ( description == "+12 Critical Strike Rating and 3% Increased Critical Damage"       ) return META_CHAOTIC_SKYFIRE;
  if ( description == "+54 Critical Strike Rating and 1% Spell Reflect"                   ) return META_DESTRUCTIVE_SHADOWSPIRIT;
  if ( description == "+14 Critical Strike Rating and 1% Spell Reflect"                   ) return META_DESTRUCTIVE_SKYFIRE;
  if ( description == "+25 Critical Strike Rating and 1% Spell Reflect"                   ) return META_DESTRUCTIVE_SKYFLARE;
  if ( description == "+81 Stamina and Reduce Spell Damage Taken by 2%"                   ) return META_EFFULGENT_SHADOWSPIRIT;
  if ( description == "+54 Intellect and +2% Maximum Mana"                                ) return META_EMBER_SHADOWSPIRIT;
  if ( description == "+12 Intellect and +2% Maximum Mana"                                ) return META_EMBER_SKYFIRE;
  if ( description == "+21 Intellect and +2% Maximum Mana"                                ) return META_EMBER_SKYFLARE;
  if ( description == "+54 Critical Strike Rating and Reduces Snare/Root Duration by 10%" ) return META_ENIGMATIC_SHADOWSPIRIT;
  if ( description == "+21 Critical Strike Rating and Reduces Snare/Root Duration by 10%" ) return META_ENIGMATIC_SKYFLARE;
  if ( description == "+17 Critical Strike Rating and Reduces Snare/Root Duration by 10%" ) return META_ENIGMATIC_STARFLARE;
  if ( description == "+12 Critical Strike Rating and Reduces Snare/Root Duration by 10%" ) return META_ENIGMATIC_SKYFIRE;
  if ( description == "+21 Dodge Rating and +1% Shield Block Value"                       ) return META_ETERNAL_EARTHSIEGE;
  if ( description == "+81 Stamina and +5% Shield Block Value"                            ) return META_ETERNAL_SHADOWSPIRIT;
  if ( description == "+54 Mastery Rating and Minor Run Speed Increase"                   ) return META_FLEET_SHADOWSPIRIT;
  if ( description == "+54 Intellect and Silence Duration Reduced by 10%"                 ) return META_FORLORN_SHADOWSPIRIT;
  if ( description == "+21 Intellect and Silence Duration Reduced by 10%"                 ) return META_FORLORN_SKYFLARE;
  if ( description == "+17 Intellect and Silence Duration Reduced by 10%"                 ) return META_FORLORN_STARFLARE;
  if ( description == "+54 Critical Strike Rating and Fear Duration Reduced by 10%"       ) return META_IMPASSIVE_SHADOWSPIRIT;
  if ( description == "+21 Critical Strike Rating and Fear Duration Reduced by 10%"       ) return META_IMPASSIVE_SKYFLARE;
  if ( description == "+17 Critical Strike Rating and Fear Duration Reduced by 10%"       ) return META_IMPASSIVE_STARFLARE;
  if ( description == "+21 Intellect and Chance to restore mana on spellcast"             ) return META_INSIGHTFUL_EARTHSIEGE;
  if ( description == "+12 Intellect and Chance to restore mana on spellcast"             ) return META_INSIGHTFUL_EARTHSTORM;
  if ( description == "+21 Haste Rating and Sometimes Heal on Your Crits"                 ) return META_INVIGORATING_EARTHSIEGE;
  if ( description == "+21 Agility and 3% Increased Critical Damage"                      ) return META_RELENTLESS_EARTHSIEGE;
  if ( description == "+12 Agility and 3% Increased Critical Damage"                      ) return META_RELENTLESS_EARTHSTORM;
  if ( description == "+21 Critical Strike Rating and Stun Duration Reduced by 10%"       ) return META_PERSISTENT_EARTHSIEGE;
  if ( description == "+17 Critical Strike Rating and Stun Duration Reduced by 10%"       ) return META_PERSISTENT_EARTHSHATTER;
  if ( description == "+32 Stamina and Stun Duration Reduced by 10%"                      ) return META_POWERFUL_EARTHSIEGE;
  if ( description == "+26 Stamina and Stun Duration Reduced by 10%"                      ) return META_POWERFUL_EARTHSHATTER;
  if ( description == "+18 Stamina and Stun Duration Reduced by 10%"                      ) return META_POWERFUL_EARTHSTORM;
  if ( description == "+81 Stamina and Stun Duration Reduced by 10%"                      ) return META_POWERFUL_SHADOWSPIRIT;
  if ( description == "+81 Spirit and 3% Increased Critical Healing Effect"               ) return META_REVITALIZING_SHADOWSPIRIT;
  if ( description == "+54 Strength and 3% Increased Critical Damage"                     ) return META_REVERBERATING_SHADOWSPIRIT;
  if ( description == "+22 Spirit and 3% Increased Critical Healing Effect"               ) return META_REVITALIZING_SKYFLARE;
  if ( description == "+12 Critical Strike Rating and Minor Run Speed Increase"           ) return META_SWIFT_SKYFIRE;
  if ( description == "+21 Critical Strike Rating and Minor Run Speed Increase"           ) return META_SWIFT_SKYFLARE;
  if ( description == "+17 Critical Strike Rating and Minor Run Speed Increase"           ) return META_SWIFT_STARFLARE;
  if ( description == "Chance to Increase Melee/Ranged Attack Speed"                      ) return META_THUNDERING_SKYFLARE; //FIXME: Assuming the best one for now, since there's no way to tell by just the description
  if ( description == "+17 Intellect and Minor Run Speed Increase"                        ) return META_TIRELESS_STARFLARE;
  if ( description == "+21 Intellect and Minor Run Speed Increase"                        ) return META_TIRELESS_SKYFLARE;
  if ( description == "+21 Intellect and Stun Duration Reduced by 10%"                    ) return META_POWERFUL_EARTHSIEGE;
  if ( description == "+17 Intellect and Stun Duration Reduced by 10%"                    ) return META_POWERFUL_EARTHSHATTER;

  return META_GEM_NONE;
}

// mmo_champion::parse_gem ==================================================

gem_e mmo_champion::parse_gem( item_t&            item,
                               const std::string& gem_id,
                               cache::behavior_e  caching )
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
      util::tokenize( color_str );
      gem_e type = util::parse_gem_type( color_str );

      std::string property_str;
      xml_node_t* property_node = get_tti_node( node, "tti-gem_properties" );
      if ( property_node ) property_node -> get_value( property_str, "a/." );

      if ( type == GEM_NONE || property_str.empty() )
        return GEM_NONE;

      std::string& s = item.armory_gems_str;

      if ( type == GEM_META )
      {
        meta_gem_e mtype = parse_meta_gem( property_str );

        if ( mtype != META_GEM_NONE )
        {
          s += '_';
          s += util::meta_gem_type_string( mtype );
        }
        else
        {
          util::fuzzy_stats( s, property_str );
        }
      }
      else
      {
        util::fuzzy_stats( s, property_str );
      }

      return type;
    }
  }

  return GEM_NONE;
}

// mmo_champion::download_glyph =============================================

bool mmo_champion::download_glyph( player_t*          player,
                                   std::string&       glyph_name,
                                   const std::string& glyph_id,
                                   cache::behavior_e  caching )
{
  xml_node_t* node = download_id( player -> sim, glyph_id, caching );
  if ( ! node || ! node -> get_value( glyph_name, "title/." ) )
  {
    if ( caching != cache::ONLY )
      player -> sim -> errorf( "Unable to download glyph id %s from mmo-champion\n", glyph_id.c_str() );
    return false;
  }

  return true;
}

// mmo_champion::download_item ==============================================

bool mmo_champion::download_item( item_t&            item,
                                  const std::string& item_id,
                                  cache::behavior_e  caching )
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

// mmo_champion::download_slot ==============================================

bool mmo_champion::download_slot( item_t&            item,
                                  const std::string& item_id,
                                  const std::string& enchant_id,
                                  const std::string& addon_id,
                                  const std::string& reforge_id,
                                  const std::string& rsuffix_id,
                                  const std::string  gem_ids[ 3 ],
                                  cache::behavior_e  caching )
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

  if ( ! item_database::parse_enchant( item, item.armory_enchant_str, enchant_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse enchant id %s for item \"%s\" at slot %s.\n",
                        item.player -> name(), enchant_id.c_str(), item.name(), item.slot_name() );
  }

  if ( ! item_database::parse_enchant( item, item.armory_addon_str, addon_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse addon id %s for item \"%s\" at slot %s.\n",
                        item.player -> name(), addon_id.c_str(), item.name(), item.slot_name() );
  }

  if ( ! enchant::download_reforge( item, reforge_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse reforge id %s for item \"%s\" at slot %s.\n", p -> name(), reforge_id.c_str(), item.name(), item.slot_name() );
    //return false;
  }

  if ( ! enchant::download_rsuffix( item, rsuffix_id ) )
  {
    item.sim -> errorf( "Player %s unable to determine random suffix '%s' for item '%s' at slot %s.\n", p -> name(), rsuffix_id.c_str(), item.name(), item.slot_name() );
    return false;
  }

  return true;
}

