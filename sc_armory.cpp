// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

// is_number ================================================================

static bool is_number( const std::string s )
{
  int size = s.size();
  for( int i=0; i < size; i++ )
    if( ! isdigit( s[ i ] ) )
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
  int num_descriptions = description_tokens.size();

  for( int i=0; i < num_descriptions; i++ )
  {
    bool match = true;

    for( int j=0; j < num_stats && match; j++ )
    {
      if( ( i + j ) == num_descriptions )
      {
	match = false;
      }
      else if( stat_tokens[ j ] != description_tokens[ i + j ] )
      {
	match = false;
      }
    }

    if( match )
    {
      std::string value_str;

      if( ( i > 0 ) && 
	  ( is_number( description_tokens[ i-1 ] ) ) )
      {
	value_str = description_tokens[ i-1 ];
      }
      if( ( ( i + num_stats + 1 ) < num_descriptions ) && 
	  ( description_tokens[ i + num_stats ] == "by" ) &&
	  ( is_number( description_tokens[ i + num_stats + 1 ] ) ) )
      {
	value_str = description_tokens[ i + num_stats + 1 ];
      }

      if( ! value_str.empty() )
      {
	encoding_str += "_" + value_str + util_t::stat_type_abbrev( stat_type );
      }
    }
  }
}

// is_proc_description ======================================================

static bool is_proc_description( const std::string& description_str )
{
  if( description_str.find( "chance" ) != std::string::npos ) return true;
  if( description_str.find( "stack"  ) != std::string::npos ) return true;
  if( description_str.find( "time"   ) != std::string::npos ) return true;
  if( description_str.find( "_sec"   ) != std::string::npos ) return true;

  return false;
}

// fuzzy_search =============================================================

static void fuzzy_search( std::string&       encoding_str,
			  const std::string& description_str )
{
  if( description_str.empty() ) return;

  std::string buffer = description_str;
  armory_t::format( buffer );

  if( is_proc_description( buffer ) )
    return;

  std::vector<std::string> splits;
  util_t::string_split( splits, buffer, "_." );

  stat_search( encoding_str, splits, STAT_MAX,  "all stats" );

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
  stat_search( encoding_str, splits, STAT_ARMOR_PENETRATION_RATING, "armor penetration" );

  stat_search( encoding_str, splits, STAT_HASTE_RATING, "haste rating" );
  stat_search( encoding_str, splits, STAT_HIT_RATING,   "ranged hit rating" );
  stat_search( encoding_str, splits, STAT_HIT_RATING,   "hit rating" );
  stat_search( encoding_str, splits, STAT_CRIT_RATING,  "ranged critical strike" );
  stat_search( encoding_str, splits, STAT_CRIT_RATING,  "critical strike rating" );
  stat_search( encoding_str, splits, STAT_CRIT_RATING,  "crit rating" );
}

// download_character_sheet =================================================

static xml_node_t* download_character_sheet( sim_t* sim,
					     const std::string& region, 
					     const std::string& server, 
					     const std::string& name )
{
  std::string url = "http://" + region + ".wowarmory.com/character-sheet.xml?r=" + server + "&n=" + name;

  xml_node_t* node = xml_t::download( url, "</characterTab>" );

  if( sim -> debug || ! node )
  {
    fprintf( sim -> output_file, "\nxml url: %s\n", url.c_str() );
    xml_t::print( node, sim -> output_file );
  }

  return node;
}

// download_character_talents ===============================================

static xml_node_t* download_character_talents( sim_t* sim,
					       const std::string& region, 
					       const std::string& server, 
					       const std::string& name )
{
  std::string url = "http://" + region + ".wowarmory.com/character-talents.xml?r=" + server + "&n=" + name;

  xml_node_t* node = xml_t::download( url, "</talentGroup>" );

  if( sim -> debug || ! node )
  {
    fprintf( sim -> output_file, "\nxml url: %s\n", url.c_str() );
    xml_t::print( node, sim -> output_file );
  }

  return node;
}

// download_item_tooltip ====================================================

static xml_node_t* download_item_tooltip( sim_t* sim,
					  const std::string& region, 
					  const std::string& server, 
					  const std::string& name,
					  const std::string& id,
					  int slot )
{
  std::string url = "http://" + region + ".wowarmory.com/item-tooltip.xml?r=" + server + "&n=" + name + "&i=" + id;

  char buffer[ 80 ];
  sprintf( buffer, "&s=%d", slot );
  url += buffer;

  xml_node_t* node = xml_t::download( url, "</itemTooltip>", 1 );

  if( sim -> debug || ! node )
  {
    fprintf( sim -> output_file, "\nxml url: %s\n", url.c_str() );
    xml_t::print( node, sim -> output_file );
  }

  return node;
}

// download_item_tooltip ====================================================

static xml_node_t* download_item_tooltip( sim_t* sim,
					  const std::string& region, 
					  const std::string& id )
{
  std::string url = "http://" + region + ".wowarmory.com/item-tooltip.xml?i=" + id;

  xml_node_t* node = xml_t::download( url, "</itemTooltip>", 1 );

  if( sim -> debug || ! node )
  {
    fprintf( sim -> output_file, "\nxml url: %s\n", url.c_str() );
    xml_t::print( node, sim -> output_file );
  }

  return node;
}

// parse_item_name ==========================================================

static bool parse_item_name( item_t& item,
			     xml_node_t* node )
{
  if( ! xml_t::get_value( item.armory_id_str,   node, "id/."   ) ) return false;
  if( ! xml_t::get_value( item.armory_name_str, node, "name/." ) ) return false;

  armory_t::format( item.armory_name_str );

  return true;
}

// parse_item_stats =========================================================

static bool parse_item_stats( item_t& item,
			      xml_node_t* xml )
{
  std::string& s = item.armory_stats_str;
  s.clear();

  std::string value;
  if( xml_t::get_value( value, xml, "bonusStrength/."  ) ) s += "_" + value + "str";
  if( xml_t::get_value( value, xml, "bonusAgility/."   ) ) s += "_" + value + "agi";
  if( xml_t::get_value( value, xml, "bonusStamina/."   ) ) s += "_" + value + "sta";
  if( xml_t::get_value( value, xml, "bonusIntellect/." ) ) s += "_" + value + "int";
  if( xml_t::get_value( value, xml, "bonusSpirit/."    ) ) s += "_" + value + "spi";

  if( xml_t::get_value( value, xml, "bonusSpellPower/."       ) ) s += "_" + value + "sp";
  if( xml_t::get_value( value, xml, "bonusSpellPenetration/." ) ) s += "_" + value + "spen";
  if( xml_t::get_value( value, xml, "bonusManaRegen/."        ) ) s += "_" + value + "mp5";
  
  if( xml_t::get_value( value, xml, "bonusAttackPower/."      ) ) s += "_" + value + "ap";
  if( xml_t::get_value( value, xml, "bonusExpertiseRating/."  ) ) s += "_" + value + "exp";
  if( xml_t::get_value( value, xml, "bonusArmorPenetration/." ) ) s += "_" + value + "arpen";

  if( xml_t::get_value( value, xml, "bonusHitRating/."   ) ) s += "_" + value + "hit";
  if( xml_t::get_value( value, xml, "bonusCritRating/."  ) ) s += "_" + value + "crit";
  if( xml_t::get_value( value, xml, "bonusHasteRating/." ) ) s += "_" + value + "haste";

  if( xml_t::get_value( value, xml, "armor/." ) ) s += "_" + value + "armor";

  xml_node_t* spell_data = xml_t::get_node( xml, "spellData" );
  if( spell_data )
  {
    std::vector<xml_node_t*> spell_nodes;
    int num_spells = xml_t::get_nodes( spell_nodes, spell_data, "spell" );
    for( int i=0; i < num_spells; i++ )
    {
      if( xml_t::get_value( value, spell_nodes[ i ], "trigger/." ) && ( value == "1" ) )
      {
	if( xml_t::get_value( value, spell_nodes[ i ], "desc/." ) )
	{
	  fuzzy_search( s, value );
	}
      }
    }
  }
  
  if( ! s.empty() ) 
  {
    s.erase( 0, 1 );
    armory_t::format( s );
  }

  if( item.sim -> debug && ! s.empty() )
    log_t::output( item.sim, "%s %s %s armory_stats=%s", item.player -> name(), item.slot_name(), item.name(), s.c_str() );

  return true;
}

// parse_item_gems ==========================================================

static bool parse_item_gems( item_t& item,
			     xml_node_t* xml )
{
  item.armory_gems_str.clear();

  xml_node_t* socket_data = xml_t::get_node( xml, "socketData" );

  if( socket_data )
  {
    std::string& s = item.armory_gems_str;

    std::vector<xml_node_t*> socket_nodes;
    int num_sockets = xml_t::get_nodes( socket_nodes, socket_data, "socket" );
    bool socket_bonus = true;

    for( int i=0; i < num_sockets; i++ )
    {
      xml_node_t* socket = socket_nodes[ i ];
      std::string enchant, color, match;

      if( ! xml_t::get_value( enchant, socket, "enchant" ) ||
	        ! xml_t::get_value( color,   socket, "color"   ) )
	          continue;

      if( color == "Meta" )
      {
	      if     ( enchant == "+32 Stamina and 2% Increased Armor Value from Items"         ) { s += "_austere_earthsiege";    }
	      else if( enchant == "+25 Spell Power and +2% Intellect"                           ) { s += "_ember_skyflare";        }
	      else if( enchant == "+21 Critical Strike Rating and 3% Increased Critical Damage" ) { s += "_chaotic_skyflare";      }
	      else if( enchant == "+12 Critical Strike Rating and 3% Increased Critical Damage" ) { s += "_chaotic_skyfire";       }
	      else if( enchant == "+21 Agility and 3% Increased Critical Damage"                ) { s += "_relentless_earthsiege"; }
	      else if( enchant == "+12 Agility and 3% Increased Critical Damage"                ) { s += "_relentless_earthstorm"; }
	      else
	      {
	        fuzzy_search( s, enchant );
	      }
      }
      else
      {
	       fuzzy_search( s, enchant );
      }

      if( ! xml_t::get_value( match, socket, "match" ) || ( match != "1" ) ) socket_bonus = false;
    }

    if( socket_bonus )
    {
      std::string enchant;
      if( xml_t::get_value( enchant, socket_data, "socketMatchEnchant/." ) )
      {
	      fuzzy_search( s, enchant );
      }
    }

    if( ! s.empty() ) 
    {
      s.erase( 0, 1 );
      armory_t::format( s );
    }

    if( item.sim -> debug && ! s.empty() )
      log_t::output( item.sim, "%s %s %s armory_gems=%s", item.player -> name(), item.slot_name(), item.name(), s.c_str() );

  }
  
  return true;
}

// parse_item_enchant =======================================================

static bool parse_item_enchant( item_t& item,
				xml_node_t* xml )
{
  item.armory_enchant_str.clear();

  std::string enchant;

  if( xml_t::get_value( enchant, xml, "enchant/." ) )
  {
    std::string& s = item.armory_enchant_str;

    if     ( enchant == "Lightweave Embroidery"    ) { s += "_lightweave";  }
    else if( enchant == "Hand-Mounted Pyro Rocket" ) { s += "_pyrorocket";  }
    else if( enchant == "Berserking"               ) { s += "_berserking";  }
    else if( enchant == "Mongoose"                 ) { s += "_mongoose";    }
    else if( enchant == "Executioner"              ) { s += "_executioner"; }
    else if( enchant == "Spellsurge"               ) { s += "_spellsurge";  }
    else
    {
      fuzzy_search( s, enchant );
    }

    if( ! s.empty() ) 
    {
      s.erase( 0, 1 );
      armory_t::format( s );
    }

    if( item.sim -> debug && ! s.empty() )
      log_t::output( item.sim, "%s %s %s armory_enchant=%s", item.player -> name(), item.slot_name(), item.name(), s.c_str() );
  }

  return true;
}

// parse_item_weapon ========================================================

static bool parse_item_weapon( item_t& item,
			       xml_node_t* xml )
{
  item.armory_weapon_str.clear();

  std::string speed, dps;
  if(  xml_t::get_value( speed, xml, "speed/." ) &&
       xml_t::get_value( dps,   xml, "dps/."   ) )
  {
    std::string sub_class;
    int inventory_type;
    if( ! xml_t::get_value( sub_class,      xml, "subclassName/."  ) ) return false;
    if( ! xml_t::get_value( inventory_type, xml, "inventoryType/." ) ) return false;

    int weapon_type = WEAPON_NONE;
    if     ( sub_class == "Axe" && inventory_type == 17   ) weapon_type = WEAPON_AXE;
    else if( sub_class == "Axe"                           ) weapon_type = WEAPON_AXE_2H;
    else if( sub_class == "Dagger"                        ) weapon_type = WEAPON_DAGGER;
    else if( sub_class == "Fist Weapon"                   ) weapon_type = WEAPON_FIST;
    else if( sub_class == "Mace" && inventory_type == 17  ) weapon_type = WEAPON_MACE;
    else if( sub_class == "Mace"                          ) weapon_type = WEAPON_MACE_2H;
    else if( sub_class == "Polearm"                       ) weapon_type = WEAPON_POLEARM;
    else if( sub_class == "Staff"                         ) weapon_type = WEAPON_STAFF;
    else if( sub_class == "Sword" && inventory_type == 17 ) weapon_type = WEAPON_SWORD_2H;
    else if( sub_class == "Sword"                         ) weapon_type = WEAPON_SWORD;
    else if( sub_class == "Bow"                           ) weapon_type = WEAPON_BOW;
    else if( sub_class == "Crossbow"                      ) weapon_type = WEAPON_CROSSBOW;
    else if( sub_class == "Gun"                           ) weapon_type = WEAPON_GUN;
    else if( sub_class == "Thrown"                        ) weapon_type = WEAPON_THROWN;
    else if( sub_class == "Wand"                          ) weapon_type = WEAPON_WAND;

    if( weapon_type == WEAPON_NONE ) return false;
    if( weapon_type == WEAPON_WAND ) return true;
    
    item.armory_weapon_str = util_t::weapon_type_string( weapon_type );
    item.armory_weapon_str += "_" + speed + "speed" + "_" + dps + "dps";
  }

  return true;
}

} // ANONYMOUS NAMESPACE ===================================================

// armory_t::download_player ================================================

player_t* armory_t::download_player( sim_t* sim,
				     const std::string& region, 
				     const std::string& server, 
				     const std::string& name,
				     bool use_active_talents )
{
  xml_node_t*   sheet_xml = download_character_sheet  ( sim, region, server, name );
  xml_node_t* talents_xml = download_character_talents( sim, region, server, name );

  if( ! sheet_xml || ! talents_xml )
  {
    printf( "simcraft: Unable to download character %s|%s|%s from the Armory.\n", region.c_str(), server.c_str(), name.c_str() );
    return 0;
  }

  std::string type_str, name_str;

  if( ! xml_t::get_value( type_str, sheet_xml, "character/class" ) ) return 0;
  if( ! xml_t::get_value( name_str, sheet_xml, "character/name"  ) ) return 0;

  armory_t::format( type_str );
  armory_t::format( name_str );

  player_t* p = player_t::create( sim, type_str, name_str );
  if( ! p ) return 0;

  p -> region_str  = region;
  p -> server_str  = server;

  if( ! xml_t::get_value( p -> level, sheet_xml, "character/level" ) ) return 0;

  std::vector<xml_node_t*> talent_nodes;
  if( 2 != xml_t::get_nodes( talent_nodes, talents_xml, "talentGroup" ) ) return 0;

  xml_node_t*   active_talents = talent_nodes[ 0 ];
  xml_node_t* inactive_talents = talent_nodes[ 1 ];

  int active_value=0;
  if( ! xml_t::get_value( active_value, active_talents, "active" ) || ! active_value ) std::swap( active_talents, inactive_talents );
  if( ! use_active_talents ) std::swap( active_talents, inactive_talents );

  std::string talents_encoding;
  if( ! xml_t::get_value( talents_encoding, active_talents, "talentSpec/value" ) ) return 0;
  if( ! p -> parse_talents( talents_encoding ) ) return 0;
  p -> talents_str = "http://www.wowarmory.com/talent-calc.xml?c=" + type_str + "&tal=" + talents_encoding;

  p -> glyphs_str = "";
  std::vector<xml_node_t*> glyph_nodes;
  int num_glyphs = xml_t::get_nodes( glyph_nodes, active_talents, "glyph" );
  for( int i=0; i < num_glyphs; i++ )
  {
    std::string name;
    if( ! xml_t::get_value( name, glyph_nodes[ i ], "name" ) ) return 0;
    name.erase( 0, 9 ); // remove "Glyph of "
    armory_t::format( name );
    if( i ) p -> glyphs_str += ",";
    p -> glyphs_str += name;
  }

  std::vector<xml_node_t*> item_nodes;
  int num_items = xml_t::get_nodes( item_nodes, sheet_xml, "item" );
  for( int i=0; i < num_items; i++ )
  {
    std::string id_str;
    int slot;

    if( xml_t::get_value( id_str, item_nodes[ i ], "id"   ) &&
	xml_t::get_value( slot,   item_nodes[ i ], "slot" ) )
    {
      if( slot == -1 ) continue;

      if( ! armory_t::download_slot( p -> items[ slot ], id_str ) ) 
	return 0;
    }
    else return 0;
  }

  return p;
}

// armory_t::download_slot ==================================================

bool armory_t::download_slot( item_t& item,
			      const std::string& id_str )
{
  player_t* p = item.player;

  xml_node_t* slot_xml = download_item_tooltip( p -> sim, p -> region_str, p -> server_str, p -> name_str, id_str, item.slot );
  if( ! slot_xml ) 
  {
    printf( "simcraft: Unable to download item %s at slot %d for player %s\n", id_str.c_str(), item.slot, p -> name() );
    return false;
  }

  if( ! parse_item_name( item, slot_xml ) )
  {
    printf( "simcraft: Player %s unable to parse name for item %s at slot %s.\n", p -> name(), id_str.c_str(), item.slot_name() );
    return false;
  }

  if( ! parse_item_stats( item, slot_xml ) )
  {
    printf( "simcraft: Player %s unable to parse stats for item \"%s\" at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if( ! parse_item_gems( item, slot_xml ) )
  {
    printf( "simcraft: Player %s unable to parse gems for item \"%s\" at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if( ! parse_item_enchant( item, slot_xml ) )
  {
    printf( "simcraft: Player %s unable to parse enchant for item \"%s\" at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if( ! parse_item_weapon( item, slot_xml ) )
  {
    printf( "simcraft: Player %s unable to parse weapon info for item \"%s\" at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  return true;
}

// armory_t::download_item ==================================================

bool armory_t::download_item( item_t& item,
			      const std::string& id_str )
{
  player_t* p = item.player;

  xml_node_t* item_xml = download_item_tooltip( p -> sim, p -> region_str, id_str );
  if( ! item_xml ) 
  {
    printf( "simcraft: Player %s unable to download item %s for slot %s.\n", p -> name(), id_str.c_str(), item.slot_name() );
    return false;
  }

  if( ! parse_item_name( item, item_xml ) )
  {
    printf( "simcraft: Player %s unable to parse name for item %s for slot %s.\n", p -> name(), id_str.c_str(), item.slot_name() );
    return false;
  }

  if( ! parse_item_stats( item, item_xml ) )
  {
    printf( "simcraft: Player %s unable to parse stats for item \"%s\" for slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  if( ! parse_item_weapon( item, item_xml ) )
  {
    printf( "simcraft: Player %s unable to parse weapon info for item \"%s\" at slot %s.\n", p -> name(), item.name(), item.slot_name() );
    return false;
  }

  return true;
}

// armory_t::format =========================================================

std::string& armory_t::format( std::string& name )
{
  if( name.empty() ) return name;

  std::string buffer="";

  int size = name.size();
  for( int i=0; i < size; i++ )
  {
    char c = name[ i ];
    if( isalpha( c ) )
    {
      c = tolower( c );
    }
    else if( c == ' ' )
    {
      c = '_';
    }
    else if( c != '_' && 
	     c != '.' && 
	     c != '%' && 
	     ! isdigit( c ) )
    {
      continue;
    }
    buffer += c;
  }
  name = buffer;

  return name;
}

// armory_t::clear_cache ====================================================

bool armory_t::clear_cache( sim_t* sim, 
			    const std::string& name, 
			    const std::string& value )
{
  assert( name == "armory_clear_cache" );
  if( value != "0" && ! sim -> parent ) http_t::cache_clear();
  return true;
}
