// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

// is_number ================================================================

static bool is_number( const std::string s )
{
  int size = ( int ) s.size();
  for ( int i=0; i < size; i++ )
    if ( ! isdigit( s[ i ] ) )
      return false;
  return true;
}

// stat_search ==============================================================

static void stat_search( std::string&              encoding_str,
                         std::vector<std::string>& description_tokens,
                         int                       stat_type,
                         const std::string&        stat_str )
{
  std::vector<std::string> stat_tokens;
  int num_stats = util_t::string_split( stat_tokens, stat_str, " " );
  int num_descriptions = ( int ) description_tokens.size();

  for ( int i=0; i < num_descriptions; i++ )
  {
    bool match = true;

    for ( int j=0; j < num_stats && match; j++ )
    {
      if ( ( i + j ) == num_descriptions )
      {
        match = false;
      }
      else
      {
        if ( stat_tokens[ j ][ 0 ] == '!' )
        {
          if ( stat_tokens[ j ].substr( 1 ) == description_tokens[ i + j ] )
          {
            match = false;
          }
        }
        else
        {
          if ( stat_tokens[ j ] != description_tokens[ i + j ] )
          {
            match = false;
          }
        }
      }
    }

    if ( match )
    {
      std::string value_str;

      if ( ( i > 0 ) &&
           ( is_number( description_tokens[ i-1 ] ) ) )
      {
        value_str = description_tokens[ i-1 ];
      }
      if ( ( ( i + num_stats + 1 ) < num_descriptions ) &&
           ( description_tokens[ i + num_stats ] == "by" ) &&
           ( is_number( description_tokens[ i + num_stats + 1 ] ) ) )
      {
        value_str = description_tokens[ i + num_stats + 1 ];
      }

      if ( ! value_str.empty() )
      {
        encoding_str += "_" + value_str + util_t::stat_type_abbrev( stat_type );
      }
    }
  }
}

// is_proc_description ======================================================

static bool is_proc_description( const std::string& description_str )
{
  if ( description_str.find( "chance" ) != std::string::npos ) return true;
  if ( description_str.find( "stack"  ) != std::string::npos ) return true;
  if ( description_str.find( "time"   ) != std::string::npos ) return true;
  if ( ( description_str.find( "_sec"   ) != std::string::npos ) &&
       ! ( ( description_str.find( "restores" ) != std::string::npos ) &&
           ( ( description_str.find( "_per_5_sec" ) != std::string::npos ) ||
             ( description_str.find( "_every_5_sec" ) != std::string::npos ) ) ) )
    return true;

  return false;
}

// download_character_sheet =================================================

static xml_node_t* download_character_sheet( sim_t* sim,
                                             const std::string& region,
                                             const std::string& server,
                                             const std::string& name,
                                             cache::behavior_t  caching )
{
  std::string url = "http://" + region + ".wowarmory.com/character-sheet.xml?locale=en_US&r=" + server + "&n=" + name;
  xml_node_t* node = xml_t::get( sim, url, caching, "</characterTab>", sim -> current_throttle );

  if ( ! node )
  {
    sim -> current_throttle = sim -> current_throttle > 20 ? sim -> current_throttle : 20 ;
  }
  else
  {
    sim -> current_throttle = sim -> armory_throttle;
  }

  if ( sim -> debug ) xml_t::print( node, sim -> output_file );

  return node;
}

// download_character_talents ===============================================

static xml_node_t* download_character_talents( sim_t* sim,
                                               const std::string& region,
                                               const std::string& server,
                                               const std::string& name,
                                               cache::behavior_t  caching )
{
  std::string url = "http://" + region + ".wowarmory.com/character-talents.xml?locale=en_US&r=" + server + "&n=" + name;
  xml_node_t* node = xml_t::get( sim, url, caching, "</talentGroup>", sim -> current_throttle );

  if ( ! node )
  {
    sim -> current_throttle = sim -> current_throttle > 20 ? sim -> current_throttle : 20 ;
  }
  else
  {
    sim -> current_throttle = sim -> armory_throttle;
  }

  if ( sim -> debug ) xml_t::print( node, sim -> output_file );

  return node;
}

// download_item_tooltip ====================================================

static xml_node_t* download_item_tooltip( player_t* p,
                                          const std::string& id_str,
                                          int slot,
                                          cache::behavior_t caching )
{
  sim_t* sim = p -> sim;

  std::string url = "http://" + p -> region_str + ".wowarmory.com/item-tooltip.xml";
  url += "?r=" + p -> server_str;
  url += "&n=" + p -> name_str;
  url += "&i=" + id_str;
  url += "&s=" + util_t::to_string( slot );

  xml_node_t* node;

  node = xml_t::get( sim, url, caching, "</itemTooltip>", sim -> current_throttle );
  if ( caching != cache::ONLY )
  {
    if ( ! node )
    {
      sim -> current_throttle = sim -> current_throttle > 20 ? sim -> current_throttle : 20 ;
    }
    else
    {
      sim -> current_throttle = sim -> armory_throttle;
    }
  }

  if ( sim -> debug ) xml_t::print( node );

  return node;
}

// download_item_tooltip ====================================================

static xml_node_t* download_item_tooltip( player_t* p,
                                          const std::string& id_str,
                                          cache::behavior_t caching )
{
  sim_t* sim = p -> sim;

  std::string url = "http://" + p -> region_str + ".wowarmory.com/item-tooltip.xml?i=" + id_str;
  xml_node_t* node;

  node = xml_t::get( sim, url, caching, "</itemTooltip>", sim -> current_throttle );
  if ( caching != cache::ONLY )
  {
    if ( ! node )
    {
      sim -> current_throttle = sim -> current_throttle > 20 ? sim -> current_throttle : 20 ;
    }
    else
    {
      sim -> current_throttle = sim -> armory_throttle;
    }
  }

  if ( sim -> debug ) xml_t::print( node );

  return node;
}

// parse_item_name ==========================================================

static bool parse_item_name( item_t& item,
                             xml_node_t* node )
{
  if ( ! xml_t::get_value( item.armory_id_str,   node, "id/."   ) ) return false;
  if ( ! xml_t::get_value( item.armory_name_str, node, "name/." ) ) return false;

  armory_t::format( item.armory_name_str );

  return true;
}

// parse_item_heroic ========================================================

static bool parse_item_heroic( item_t& item,
                               xml_node_t* node )
{
  if ( ! xml_t::get_value( item.armory_heroic_str, node, "heroic/." ) ) item.armory_heroic_str = "";

  armory_t::format( item.armory_heroic_str );

  return true;
}

// parse_item_lfr ===========================================================

static bool parse_item_lfr( item_t& item, xml_node_t* node )
{
  // FIXME: Armory currently doesn't have a flag for LFR
  return true;

  if ( ! xml_t::get_value( item.armory_lfr_str, node, "raid-finder/." ) ) item.armory_lfr_str = "";

  armory_t::format( item.armory_lfr_str );

  return true;
}

// parse_item_armor_type ====================================================

static bool parse_item_armor_type( item_t& item,
                                   xml_node_t* node )
{
  item.armory_armor_type_str.clear();

  xml_node_t* equip_data = xml_t::get_node( node, "equipData" );

  if ( equip_data )
  {
    if ( ! xml_t::get_value( item.armory_armor_type_str, equip_data, "subclassName/." ) ) return false;
  }
  else
  {
    return false;
  }

  armory_t::format( item.armory_armor_type_str );

  return true;
}

// parse_item_stats =========================================================

static bool parse_item_stats( item_t& item,
                              xml_node_t* xml )
{
  std::string& s = item.armory_stats_str;
  s.clear();

  std::string value;
  if ( xml_t::get_value( value, xml, "bonusStrength/."  ) ) s += "_" + value + "str";
  if ( xml_t::get_value( value, xml, "bonusAgility/."   ) ) s += "_" + value + "agi";
  if ( xml_t::get_value( value, xml, "bonusStamina/."   ) ) s += "_" + value + "sta";
  if ( xml_t::get_value( value, xml, "bonusIntellect/." ) ) s += "_" + value + "int";
  if ( xml_t::get_value( value, xml, "bonusSpirit/."    ) ) s += "_" + value + "spi";

  if ( xml_t::get_value( value, xml, "bonusSpellPower/."       ) ) s += "_" + value + "sp";
  if ( xml_t::get_value( value, xml, "bonusSpellPenetration/." ) ) s += "_" + value + "spen";
  if ( xml_t::get_value( value, xml, "bonusManaRegen/."        ) ) s += "_" + value + "mp5";

  if ( xml_t::get_value( value, xml, "bonusAttackPower/."      ) ) s += "_" + value + "ap";
  if ( xml_t::get_value( value, xml, "bonusExpertiseRating/."  ) ) s += "_" + value + "exp";

  if ( xml_t::get_value( value, xml, "bonusHitRating/."   ) ) s += "_" + value + "hit";
  if ( xml_t::get_value( value, xml, "bonusCritRating/."  ) ) s += "_" + value + "crit";
  if ( xml_t::get_value( value, xml, "bonusHasteRating/." ) ) s += "_" + value + "haste";

  if ( xml_t::get_value( value, xml, "armor/."                   ) ) s += "_" + value + "armor";
  if ( xml_t::get_value( value, xml, "bonusDefenseSkillRating/." ) ) s += "_" + value + "def";
  if ( xml_t::get_value( value, xml, "bonusDodgeRating/."        ) ) s += "_" + value + "dodge";
  if ( xml_t::get_value( value, xml, "bonusParryRating/."        ) ) s += "_" + value + "parry";
  if ( xml_t::get_value( value, xml, "bonusBlockRating/."        ) ) s += "_" + value + "block";
  if ( xml_t::get_value( value, xml, "blockValue/."              ) ) s += "_" + value + "blockv";

  xml_node_t* spell_data = xml_t::get_node( xml, "spellData" );
  if ( spell_data )
  {
    std::vector<xml_node_t*> spell_nodes;
    int num_spells = xml_t::get_nodes( spell_nodes, spell_data, "spell" );
    for ( int i=0; i < num_spells; i++ )
    {
      if ( xml_t::get_value( value, spell_nodes[ i ], "trigger/." ) && ( value == "1" ) )
      {
        if ( xml_t::get_value( value, spell_nodes[ i ], "desc/." ) )
        {
          armory_t::fuzzy_stats( s, value );
        }
      }
    }
  }

  if ( ! s.empty() )
  {
    s.erase( 0, 1 );
    armory_t::format( s );
  }

  if ( item.sim -> debug && ! s.empty() )
    log_t::output( item.sim, "%s %s %s armory_stats=%s", item.player -> name(), item.slot_name(), item.name(), s.c_str() );

  return true;
}

// parse_item_reforge =======================================================

static bool parse_item_reforge( item_t& item,
                                xml_node_t* /* xml */ )
{
  item.armory_reforge_str.clear();

  // TO-DO

  armory_t::format( item.armory_reforge_str );

  return true;
}

// parse_item_gems ==========================================================

static bool parse_item_gems( item_t& item,
                             xml_node_t* xml )
{
  item.armory_gems_str.clear();

  xml_node_t* socket_data = xml_t::get_node( xml, "socketData" );

  if ( socket_data )
  {
    std::string& s = item.armory_gems_str;

    std::vector<xml_node_t*> socket_nodes;
    int num_sockets = xml_t::get_nodes( socket_nodes, socket_data, "socket" );
    bool socket_bonus = true;

    for ( int i=0; i < num_sockets; i++ )
    {
      xml_node_t* socket = socket_nodes[ i ];
      std::string enchant, color, match;

      if ( ! xml_t::get_value( enchant, socket, "enchant" ) ||
           ! xml_t::get_value( color,   socket, "color"   ) )
        continue;

      if ( color == "Meta" )
      {
        int meta_gem_type = armory_t::parse_meta_gem( enchant );

        if ( meta_gem_type != META_GEM_NONE )
        {
          s += "_";
          s += util_t::meta_gem_type_string( meta_gem_type );
        }
        else
        {
          armory_t::fuzzy_stats( s, enchant );
        }
      }
      else
      {
        armory_t::fuzzy_stats( s, enchant );
      }

      if ( ! xml_t::get_value( match, socket, "match" ) || ( match != "1" ) ) socket_bonus = false;
    }

    if ( socket_bonus )
    {
      std::string enchant;
      if ( xml_t::get_value( enchant, socket_data, "socketMatchEnchant/." ) )
      {
        armory_t::fuzzy_stats( s, enchant );
      }
    }

    if ( ! s.empty() )
    {
      s.erase( 0, 1 );
      armory_t::format( s );
    }

    if ( item.sim -> debug && ! s.empty() )
    {
      log_t::output( item.sim, "%s %s %s armory_gems=%s", item.player -> name(), item.slot_name(), item.name(), s.c_str() );
    }
  }

  return true;
}

// parse_item_enchant =======================================================

static bool parse_item_enchant( item_t& item,
                                xml_node_t* xml )
{
  item.armory_enchant_str.clear();

  std::string enchant;

  if ( xml_t::get_value( enchant, xml, "enchant/." ) )
  {
    std::string& s = item.armory_enchant_str;

    if      ( enchant == "Lightweave Embroidery"    ) { s += "_lightweave_embroidery_2";   }
    else if ( enchant == "Darkglow Embroidery"      ) { s += "_darkglow_embroidery_2";     }
    else if ( enchant == "Swordguard Embroidery"    ) { s += "_swordguard_embroidery_2"; }
    else if ( enchant == "Avalanche"                ) { s += "_avalanche";   }
    else if ( enchant == "Elemental Slayer"         ) { s += "_elemental_slayer"; }
    else if ( enchant == "Hurricane"                ) { s += "_hurricane";   }
    else if ( enchant == "Landslide"                ) { s += "_landslide";   }
    else if ( enchant == "Power Torrent"            ) { s += "_power_torrent"; }
    else if ( enchant == "Windwalk"                 ) { s += "_windwalk";    }
    else if ( enchant == "Hand-Mounted Pyro Rocket" ) { s += "_hand_mounted_pyro_rocket";  }
    else if ( enchant == "Berserking"               ) { s += "_berserking";  }
    else if ( enchant == "Mongoose"                 ) { s += "_mongoose";    }
    else if ( enchant == "Executioner"              ) { s += "_executioner"; }
    else if ( enchant == "Spellsurge"               ) { s += "_spellsurge";  }
    else
    {
      armory_t::fuzzy_stats( s, enchant );
    }

    if ( ! s.empty() )
    {
      s.erase( 0, 1 );
      armory_t::format( s );
    }

    if ( item.sim -> debug && ! s.empty() )
      log_t::output( item.sim, "%s %s %s armory_enchant=%s", item.player -> name(), item.slot_name(), item.name(), s.c_str() );
  }

  return true;
}

// parse_item_enchant =======================================================

static bool parse_item_addon( item_t& item,
                              xml_node_t* xml )
{
  item.armory_addon_str.clear();

  std::string enchant;

  // TO-DO: support when armory updated

  if ( xml_t::get_value( enchant, xml, "addon/." ) )
  {
    std::string& s = item.armory_addon_str;

    if      ( enchant == "Lightweave Embroidery"    ) { s += "_lightweave_embroidery_2";   }
    else if ( enchant == "Darkglow Embroidery"      ) { s += "_darkglow_embroidery_2";     }
    else if ( enchant == "Swordguard Embroidery"    ) { s += "_swordguard_embroidery_2"; }
    else if ( enchant == "Avalanche"                ) { s += "_avalanche";   }
    else if ( enchant == "Elemental Slayer"         ) { s += "_elemental_slayer"; }
    else if ( enchant == "Hurricane"                ) { s += "_hurricane";   }
    else if ( enchant == "Landslide"                ) { s += "_landslide";   }
    else if ( enchant == "Power Torrent"            ) { s += "_power_torrent"; }
    else if ( enchant == "Windwalk"                 ) { s += "_windwalk";    }
    else if ( enchant == "Hand-Mounted Pyro Rocket" ) { s += "_hand_mounted_pyro_rocket";  }
    else if ( enchant == "Berserking"               ) { s += "_berserking";  }
    else if ( enchant == "Mongoose"                 ) { s += "_mongoose";    }
    else if ( enchant == "Executioner"              ) { s += "_executioner"; }
    else if ( enchant == "Spellsurge"               ) { s += "_spellsurge";  }
    else
    {
      armory_t::fuzzy_stats( s, enchant );
    }

    if ( ! s.empty() )
    {
      s.erase( 0, 1 );
      armory_t::format( s );
    }

    if ( item.sim -> debug && ! s.empty() )
      log_t::output( item.sim, "%s %s %s armory_addon=%s", item.player -> name(), item.slot_name(), item.name(), s.c_str() );
  }

  return true;
}

// parse_item_weapon ========================================================

static bool parse_item_weapon( item_t& item,
                               xml_node_t* xml )
{
  item.armory_weapon_str.clear();

  std::string speed, dps, dmg_min, dmg_max;
  if (  xml_t::get_value( speed,   xml, "speed/." ) &&
        xml_t::get_value( dps,     xml, "dps/."   ) &&
        xml_t::get_value( dmg_min, xml, "min/."   ) &&
        xml_t::get_value( dmg_max, xml, "max/."   ) )
  {
    std::string sub_class;
    int inventory_type = -1;
    xml_t::get_value( sub_class,      xml, "subclassName/."  );
    xml_t::get_value( inventory_type, xml, "inventoryType/." );

    int weapon_type = WEAPON_NONE;
    if      ( sub_class == "Axe" && inventory_type == 17   ) weapon_type = WEAPON_AXE;
    else if ( sub_class == "Axe"                           ) weapon_type = WEAPON_AXE_2H;
    else if ( sub_class == "Dagger"                        ) weapon_type = WEAPON_DAGGER;
    else if ( sub_class == "Fist Weapon"                   ) weapon_type = WEAPON_FIST;
    else if ( sub_class == "Mace" && inventory_type == 17  ) weapon_type = WEAPON_MACE;
    else if ( sub_class == "Mace"                          ) weapon_type = WEAPON_MACE_2H;
    else if ( sub_class == "Polearm"                       ) weapon_type = WEAPON_POLEARM;
    else if ( sub_class == "Staff"                         ) weapon_type = WEAPON_STAFF;
    else if ( sub_class == "Sword" && inventory_type == 17 ) weapon_type = WEAPON_SWORD_2H;
    else if ( sub_class == "Sword"                         ) weapon_type = WEAPON_SWORD;
    else if ( sub_class == "Bow"                           ) weapon_type = WEAPON_BOW;
    else if ( sub_class == "Crossbow"                      ) weapon_type = WEAPON_CROSSBOW;
    else if ( sub_class == "Gun"                           ) weapon_type = WEAPON_GUN;
    else if ( sub_class == "Thrown"                        ) weapon_type = WEAPON_THROWN;
    else if ( sub_class == "Wand"                          ) weapon_type = WEAPON_WAND;
    else if ( sub_class == "Fishking Pole"                 ) weapon_type = WEAPON_POLEARM;
    else if ( sub_class.empty() && inventory_type == 21    ) weapon_type = WEAPON_POLEARM; // Fishing Pole

    if ( weapon_type == WEAPON_NONE ) return false;
    if ( weapon_type == WEAPON_WAND ) return true;

    item.armory_weapon_str = util_t::weapon_type_string( weapon_type );
    item.armory_weapon_str += "_" + speed + "speed" + "_" + dmg_min + "min" + "_" + dmg_max + "max";
  }

  return true;
}

} // ANONYMOUS NAMESPACE ===================================================

// armory_t::fuzzy_stats ====================================================

void armory_t::fuzzy_stats( std::string&       encoding_str,
                            const std::string& description_str )
{
  if ( description_str.empty() ) return;

  std::string buffer = description_str;
  armory_t::format( buffer );

  if ( is_proc_description( buffer ) )
    return;

  std::vector<std::string> splits;
  util_t::string_split( splits, buffer, "_." );

  stat_search( encoding_str, splits, STAT_MAX,  "all stats" );
  stat_search( encoding_str, splits, STAT_MAX,  "to all stats" );

  stat_search( encoding_str, splits, STAT_STRENGTH,  "strength" );
  stat_search( encoding_str, splits, STAT_AGILITY,   "agility" );
  stat_search( encoding_str, splits, STAT_STAMINA,   "stamina" );
  stat_search( encoding_str, splits, STAT_INTELLECT, "intellect" );
  stat_search( encoding_str, splits, STAT_SPIRIT,    "spirit" );

  stat_search( encoding_str, splits, STAT_SPELL_POWER, "spell power" );
  stat_search( encoding_str, splits, STAT_MP5,         "mana regen" );
  stat_search( encoding_str, splits, STAT_MP5,         "mana every 5" );
  stat_search( encoding_str, splits, STAT_MP5,         "mana per 5" );
  stat_search( encoding_str, splits, STAT_MP5,         "mana restored per 5" );
  stat_search( encoding_str, splits, STAT_MP5,         "mana 5" );

  stat_search( encoding_str, splits, STAT_ATTACK_POWER,             "attack power" );
  stat_search( encoding_str, splits, STAT_EXPERTISE_RATING,         "expertise rating" );

  stat_search( encoding_str, splits, STAT_HASTE_RATING,         "haste rating" );
  stat_search( encoding_str, splits, STAT_HIT_RATING,           "ranged hit rating" );
  stat_search( encoding_str, splits, STAT_HIT_RATING,           "hit rating" );
  stat_search( encoding_str, splits, STAT_CRIT_RATING,          "ranged critical strike" );
  stat_search( encoding_str, splits, STAT_CRIT_RATING,          "critical strike rating" );
  stat_search( encoding_str, splits, STAT_CRIT_RATING,          "crit rating" );
  stat_search( encoding_str, splits, STAT_MASTERY_RATING,       "mastery rating" );

  stat_search( encoding_str, splits, STAT_BONUS_ARMOR,    "armor !penetration" );
  stat_search( encoding_str, splits, STAT_DODGE_RATING,   "dodge rating" );
  stat_search( encoding_str, splits, STAT_PARRY_RATING,   "parry rating" );
  stat_search( encoding_str, splits, STAT_BLOCK_RATING,   "block_rating" );
}

// armory_t::parse_meta_gem =================================================

int armory_t::parse_meta_gem( const std::string& description )
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

// armory_t::download_guild =================================================

bool armory_t::download_guild( sim_t* sim,
                               const std::string& region,
                               const std::string& server,
                               const std::string& name,
                               const std::vector<int>& ranks,
                               int player_filter,
                               int max_rank,
                               cache::behavior_t caching )
{
  std::string url = "http://" + region + ".wowarmory.com/guild-info.xml?r=" + server + "&gn=" + name;

  xml_node_t* guild_info = xml_t::get( sim, url, caching, "</members>", sim -> current_throttle );
  if ( ! guild_info )
  {
    sim -> current_throttle = sim -> current_throttle > 20 ? sim -> current_throttle : 20 ;

    sim -> errorf( "Unable to download guild %s|%s|%s from the Armory.\n", region.c_str(), server.c_str(), name.c_str() );
    return false;
  }
  sim -> current_throttle = sim -> armory_throttle;

  if ( sim -> debug ) xml_t::print( guild_info, sim -> output_file );

  xml_node_t* members = xml_t::get_node( guild_info, "members" );
  if ( ! members )
  {
    sim -> errorf( "Unable to determine members for guild %s|%s|%s from the Armory.\n", region.c_str(), server.c_str(), name.c_str() );
    return false;
  }

  std::vector<std::string> character_names;
  std::vector<xml_node_t*> characters;
  int num_characters = xml_t::get_nodes( characters, members, "character" );
  for ( int i=0; i < num_characters; i++ )
  {
    std::string character_name;
    int character_cid;
    int character_level;
    int character_rank;

    if ( xml_t::get_value( character_name,  characters[ i ], "name"    ) &&
         xml_t::get_value( character_cid,   characters[ i ], "classId" ) &&
         xml_t::get_value( character_level, characters[ i ], "level"   ) &&
         xml_t::get_value( character_rank,  characters[ i ], "rank"    ) )
    {
      if ( character_level < 80 )
        continue;

      if ( ( max_rank > 0 ) && ( character_rank > max_rank ) )
        continue;

      if ( ( ! ranks.empty() ) && ( range::find( ranks, character_rank ) == ranks.end() ) )
        continue;

      int player_type = util_t::translate_class_id( character_cid );

      if ( player_filter != PLAYER_NONE )
        if ( player_filter != player_type )
          continue;

      character_names.push_back( character_name );
    }
  }

  num_characters = ( int ) character_names.size();
  if ( num_characters > 0 )
  {
    range::sort( character_names );

    for ( int i=0; i < num_characters; i++ )
    {
      std::string& character_name = character_names[ i ];
      std::string  formatted_name = character_name;

      util_t::format_text( formatted_name, sim -> input_is_utf8 );

      sim -> errorf( "Downloading character: %s\n", formatted_name.c_str() );
      player_t* p = armory_t::download_player( sim, region, server, character_name, "active", caching );

      if ( ! p )
      {
        sim -> errorf( "simulationcraft: Armory failed...  Attempting to download character from wowhead.\n" );
        p = wowhead_t::download_player( sim, region, server, character_name, true, caching );
      }
      if ( ! p ) return false;

      int tree = p -> primary_tree();
      if ( tree == TREE_RESTORATION || tree == TREE_HOLY || tree == TREE_DISCIPLINE )
      {
        sim -> errorf( "Setting quiet=1 on healer %s\n", formatted_name.c_str() );
        p -> quiet = true;
      }
      if ( tree == TREE_PROTECTION )
      {
        sim -> errorf( "Setting quiet=1 on tank %s\n", formatted_name.c_str() );
        p -> quiet = true;
      }
    }
  }

  return true;
}

// armory_t::download_player ================================================

player_t* armory_t::download_player( sim_t* sim,
                                     const std::string& region,
                                     const std::string& server,
                                     const std::string& name,
                                     const std::string& talents_description,
                                     cache::behavior_t  caching )
{
  sim -> current_slot = 0;
  sim -> current_name = name;

  xml_node_t*   sheet_xml = download_character_sheet  ( sim, region, server, name, caching );
  xml_node_t* talents_xml = download_character_talents( sim, region, server, name, caching );

  if ( ! sheet_xml || ! talents_xml )
  {
    sim -> errorf( "Unable to download character %s|%s|%s from the Armory.\n", region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }

  std::string type_str, cid_str, name_str, race_str;
  int level;

  if ( ! xml_t::get_value( type_str, sheet_xml, "character/class"   ) ||
       ! xml_t::get_value(  cid_str, sheet_xml, "character/classId" ) ||
       ! xml_t::get_value( name_str, sheet_xml, "character/name"    ) ||
       ! xml_t::get_value(    level, sheet_xml, "character/level"   ) ||
       ! xml_t::get_value( race_str, sheet_xml, "character/race"    )
     )
  {
    sim -> errorf( "Unable to determine class/name/level/race from armory xml for %s|%s|%s.\n",
                   region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }
  armory_t::format( type_str );
  armory_t::format( race_str );

  race_type r = util_t::parse_race_type( race_str );

  if ( ! talents_description.empty() && ( talents_description != "active" ) )
  {
    name_str += '_' + talents_description;
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

  p -> origin_str = "http://" + region + ".wowarmory.com/character-sheet.xml?r=" + server + "&n=" + name;
  http_t::format( p -> origin_str );

  p -> professions_str = "";
  std::vector<xml_node_t*> skill_nodes;
  int num_skills = xml_t::get_nodes( skill_nodes, sheet_xml, "professions/skill" );
  for ( int i=0; i < num_skills; i++ )
  {
    std::string key_str, value_str;
    if ( xml_t::get_value(   key_str, skill_nodes[ i ], "key"   ) &&
         xml_t::get_value( value_str, skill_nodes[ i ], "value" ) )
    {
      if ( i ) p -> professions_str += '/';
      p -> professions_str += key_str + '=' + value_str;
    }
  }

  xml_node_t* talents_node = xml_t::get_node( talents_xml, "talents" );

  // US/EU Armory pages using different notations!
  if ( ! talents_node ) talents_node = xml_t::get_node( talents_xml, "talentGroups" );

  if ( talents_node )
  {
    xml_node_t*   active_talents = 0;
    xml_node_t* inactive_talents = 0;

    std::vector<xml_node_t*> talent_groups;
    int num_groups = xml_t::get_children( talent_groups, talents_node, "talentGroup" );

    if ( num_groups > 0 )   active_talents = talent_groups[ 0 ];
    if ( num_groups > 1 ) inactive_talents = talent_groups[ 1 ];

    if ( active_talents )
    {
      if ( inactive_talents )
      {
        int active_value=0;
        if ( xml_t::get_value( active_value, inactive_talents, "active" ) && active_value )
        {
          std::swap( active_talents, inactive_talents );
        }

        if ( talents_description == "active" )
        {
          // Leave it alone.
        }
        else if ( talents_description == "inactive" )
        {
          std::swap( active_talents, inactive_talents );
        }
        else
        {
          int primary_tree = util_t::parse_talent_tree( talents_description );
          if ( primary_tree == TREE_NONE )
          {
            sim -> errorf( "Valid values for 'talents' are 'active', 'inactive', or the name of a tree such as 'fury' or 'shadow'.\n" );
            return 0;
          }

          std::string primary_tree_str;
          if ( xml_t::get_value( primary_tree_str, inactive_talents, "prim" ) )
          {
            armory_t::format( primary_tree_str );
            if ( primary_tree == util_t::parse_talent_tree( primary_tree_str ) )
            {
              std::swap( active_talents, inactive_talents );
            }
          }
        }
      }

      std::string talents_encoding;
      if ( ! xml_t::get_value( talents_encoding, active_talents, "talentSpec/value" ) )
      {
        sim -> errorf( "Player %s unable to determine talents from armory xml.\n", p -> name() );
        return 0;
      }
      if ( ! p -> parse_talents_armory( talents_encoding ) )
      {
        sim -> errorf( "Player %s unable to parse talents '%s'.\n", p -> name(), talents_encoding.c_str() );
        return 0;
      }
      p -> talents_str = "http://www.wowhead.com/talent#";
      p -> talents_str += util_t::player_type_string( p -> type );
      p -> talents_str += "-" + talents_encoding;

      p -> glyphs_str = "";
      std::vector<xml_node_t*> glyph_nodes;
      int num_glyphs = xml_t::get_nodes( glyph_nodes, active_talents, "glyph" );
      for ( int i=0; i < num_glyphs; i++ )
      {
        std::string glyph_name;
        if ( ! xml_t::get_value( glyph_name, glyph_nodes[ i ], "name" ) )
        {
          sim -> errorf( "Player %s unable to determine glyph name from armory xml.\n", p -> name() );
          return 0;
        }
        if ( !glyph_name.compare( 0, 9, "Glyph of " ) )
          glyph_name.erase( 0, 9 ); // remove "Glyph of "
        else if ( !glyph_name.compare( 0, 8, "Glyph - " ) )
          glyph_name.erase( 0, 8 ); // remove "Glyph - "
        armory_t::format( glyph_name );
        if ( i ) p -> glyphs_str += "/";
        p -> glyphs_str += glyph_name;
      }
    }
  }

  std::vector<xml_node_t*> item_nodes;
  int num_items = xml_t::get_nodes( item_nodes, sheet_xml, "item" );
  for ( int i=0; i < num_items; i++ )
  {
    sim -> current_slot = i;
    if ( sim -> canceled ) return 0;

    std::string id_str;
    int slot;

    if ( xml_t::get_value( id_str, item_nodes[ i ], "id"   ) &&
         xml_t::get_value( slot,   item_nodes[ i ], "slot" ) )
    {
      if ( slot == -1 ) continue;

      item_t& item = p -> items[ slot ];

      bool success = false;

      std::string enchant_id, addon_id, reforge_id, rsuffix_id, gem_ids[ 3 ];
      if ( xml_t::get_value( enchant_id,   item_nodes[ i ], "permanentenchant" ) &&
           xml_t::get_value( gem_ids[ 0 ], item_nodes[ i ], "gem0Id"           ) &&
           xml_t::get_value( gem_ids[ 1 ], item_nodes[ i ], "gem1Id"           ) &&
           xml_t::get_value( gem_ids[ 2 ], item_nodes[ i ], "gem2Id"           ) )
      {
        success = item_t::download_slot( item, id_str, enchant_id, addon_id, reforge_id, rsuffix_id, gem_ids );
      }

      if ( ! success )
      {
        success = item_t::download_item( item, id_str );
      }

      if ( ! success ) return 0;
    }
    else return 0;
  }

  p -> armory_extensions( region, server, name, caching );

  return p;
}

// armory_t::download_slot ==================================================

bool armory_t::download_slot( item_t& item,
                              const std::string& id_str,
                              cache::behavior_t  caching )
{
  player_t* p = item.player;

  xml_node_t* slot_xml = download_item_tooltip( p, id_str, item.slot, caching );
  if ( ! slot_xml )
  {
    if ( caching != cache::ONLY )
      item.sim -> errorf( "Unable to download item %s from armory at slot %d for player %s\n", id_str.c_str(), item.slot, p -> name() );
    return false;
  }

  if ( ! parse_item_name( item, slot_xml ) )
  {
    item.sim -> errorf( "Player %s unable to parse name for item %s at slot %s.\n", p -> name(), id_str.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_heroic( item, slot_xml ) )
  {
    item.sim -> errorf( "Player %s unable to parse heroic flag for item %s at slot %s.\n", p -> name(), id_str.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_lfr( item, slot_xml ) )
  {
    item.sim -> errorf( "Player %s unable to parse LFR flag for item %s at slot %s.\n", p -> name(), id_str.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_armor_type( item, slot_xml ) )
  {
    item.sim -> errorf( "Player %s unable to parse armor type for item %s at slot %s.\n", p -> name(), id_str.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_stats( item, slot_xml ) )
  {
    item.sim -> errorf( "Player %s unable to parse stats for item \"%s\" at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_reforge( item, slot_xml ) )
  {
    item.sim -> errorf( "Player %s unable to parse reforge for item \"%s\" at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_gems( item, slot_xml ) )
  {
    item.sim -> errorf( "Player %s unable to parse gems for item \"%s\" at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_enchant( item, slot_xml ) )
  {
    item.sim -> errorf( "Player %s unable to parse enchant for item \"%s\" at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_addon( item, slot_xml ) )
  {
    item.sim -> errorf( "Player %s unable to parse addon for item \"%s\" at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_weapon( item, slot_xml ) )
  {
    item.sim -> errorf( "Player %s unable to parse weapon info for item \"%s\" at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  return true;
}

// armory_t::download_item ==================================================

bool armory_t::download_item( item_t& item,
                              const std::string& id_str,
                              cache::behavior_t  caching )
{
  player_t* p = item.player;

  xml_node_t* item_xml = download_item_tooltip( p, id_str, caching );
  if ( ! item_xml )
  {
    if ( caching != cache::ONLY )
      item.sim -> errorf( "Player %s unable to download item %s from armory at slot %s.\n", p -> name(), id_str.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_name( item, item_xml ) )
  {
    item.sim -> errorf( "Player %s unable to parse name for item %s at slot %s.\n", p -> name(), id_str.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_heroic( item, item_xml ) )
  {
    item.sim -> errorf( "Player %s unable to parse heroic flag for item %s at slot %s.\n", p -> name(), id_str.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_lfr( item, item_xml ) )
  {
    item.sim -> errorf( "Player %s unable to parse LFR flag for item %s at slot %s.\n", p -> name(), id_str.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_armor_type( item, item_xml ) )
  {
    item.sim -> errorf( "Player %s unable to parse armor type for item %s at slot %s.\n", p -> name(), id_str.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_stats( item, item_xml ) )
  {
    item.sim -> errorf( "Player %s unable to parse stats for item \"%s\" at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_weapon( item, item_xml ) )
  {
    item.sim -> errorf( "Player %s unable to parse weapon info for item \"%s\" at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  return true;
}

// armory_t::format =========================================================

std::string& armory_t::format( std::string& name, int format_type )
{
  if ( name.empty() ) return name;

  std::string buffer;

  switch ( format_type & FORMAT_CONVERT_MASK )
  {
  case FORMAT_UTF8_MASK:
    util_t::urlencode( name );
    break;
  case FORMAT_ASCII_MASK:
    util_t::str_to_utf8( name );
    break;
  }

  int size = ( int ) name.size();
  for ( int i=0; i < size; i++ )
  {
    unsigned char c = name[ i ];

    if ( c >= 0x80 )
    {
      continue;
    }
    else if ( isalpha( c ) )
    {
      switch ( format_type & FORMAT_ALL_NAME_MASK )
      {
      case FORMAT_GUILD_NAME_MASK:
        break;
      case FORMAT_CHAR_NAME_MASK:
        if ( i != 0 )
        {
          c = tolower( ( unsigned ) c );
        }
        break;
      default:
        c = tolower( ( unsigned ) c );
        break;
      }
    }
    else if ( c == ' ' )
    {
      c = '_';
    }
    else if ( ( c == '_' || c == '+' ) && i == 0 )
    {
      continue;
    }
    else if ( c != '_' &&
              c != '+' &&
              c != '.' &&
              c != '%' &&
              ! isdigit( c ) )
    {
      continue;
    }
    buffer += c;
  }
  name.swap( buffer );

  return name;
}

