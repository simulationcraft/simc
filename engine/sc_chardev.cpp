// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

// download_profile =========================================================

static js_node_t* download_profile( sim_t* sim,
                                    const std::string& id,
                                    cache::behavior_t caching )
{
  std::string url = "http://chardev.org/php/interface/profiles/get_profile.php?id=" + id;
  std::string profile_str;

  if ( ! http_t::get( profile_str, url, caching ) )
    return 0;

  return js_t::create( sim, profile_str );
}

// translate_slot ===========================================================

static const char* translate_slot( int slot )
{
  switch ( slot )
  {
  case SLOT_HEAD:      return "0";
  case SLOT_NECK:      return "1";
  case SLOT_SHOULDERS: return "2";
  case SLOT_BACK:      return "3";
  case SLOT_CHEST:     return "4";
  case SLOT_SHIRT:     return "5";
  case SLOT_TABARD:    return "6";
  case SLOT_WRISTS:    return "7";
  case SLOT_HANDS:     return "8";
  case SLOT_WAIST:     return "9";
  case SLOT_LEGS:      return "10";
  case SLOT_FEET:      return "11";
  case SLOT_FINGER_1:  return "12";
  case SLOT_FINGER_2:  return "13";
  case SLOT_TRINKET_1: return "14";
  case SLOT_TRINKET_2: return "15";
  case SLOT_MAIN_HAND: return "16";
  case SLOT_OFF_HAND:  return "17";
  case SLOT_RANGED:    return "18";
  }

  return "unknown";
}

} // ANONYMOUS NAMESPACE ====================================================

// chardev_t::download_player ===============================================

player_t* chardev_t::download_player( sim_t* sim,
                                      const std::string& id,
                                      cache::behavior_t caching )
{
  sim -> current_slot = 0;
  sim -> current_name = id;

  js_node_t* profile_js = download_profile( sim, id, caching );
  if ( ! profile_js || ! ( profile_js = js_t::get_node( profile_js, "character" ) ) )
  {
    sim -> errorf( "Unable to download character profile %s from chardev.\n", id.c_str() );
    return 0;
  }

  if ( sim -> debug ) js_t::print( profile_js, sim -> output_file );

  std::string name_str, race_str, type_str;

  if ( ! js_t::get_value( name_str, profile_js, "0/0" ) )
  {
    name_str = "chardev_";
    name_str += id;
  }

  if ( ! js_t::get_value( race_str, profile_js, "0/2/1" ) ||
       ! js_t::get_value( type_str, profile_js, "0/3/1" ) )
  {
    sim -> errorf( "Unable to extract player race/type from CharDev id %s.\nThis is often caused by not saving the profile.\n", id.c_str() );
    return 0;
  }

  armory_t::format( name_str );
  armory_t::format( type_str );
  armory_t::format( race_str );

  player_t* p = player_t::create( sim, type_str, name_str, util_t::parse_race_type( race_str ) );
  sim -> active_player = p;
  if ( ! p )
  {
    sim -> errorf( "Unable to build player with class '%s' and name '%s' from CharDev id %s.\n",
                   type_str.c_str(), name_str.c_str(), id.c_str() );
    return 0;
  }

  p -> origin_str = "http://chardev.org/?profile=" + id;
  http_t::format( p -> origin_str );

  js_node_t*        gear_root = js_t::get_child( profile_js, "1" );
  js_node_t*     talents_root = js_t::get_child( profile_js, "2" );
  js_node_t*      glyphs_root = js_t::get_child( profile_js, "3" );
  js_node_t* professions_root = js_t::get_child( profile_js, "5" );

  for ( int i=0; i < SLOT_MAX; i++ )
  {
    if ( sim -> canceled ) return 0;
    sim -> current_slot = i;
    item_t& item = p -> items[ i ];

    js_node_t* slot_node = js_t::get_child( gear_root, translate_slot( i ) );
    if ( ! slot_node ) continue;

    std::string item_id;
    js_t::get_value( item_id, slot_node, "0/0" );
    if ( item_id.empty() ) continue;

    std::string enchant_id, addon_id, gem_ids[ 3 ];
    js_t::get_value( gem_ids[ 0 ], slot_node, "1/0" );
    js_t::get_value( gem_ids[ 1 ], slot_node, "2/0" );
    js_t::get_value( gem_ids[ 2 ], slot_node, "3/0" );
    js_t::get_value( enchant_id,   slot_node, "4/0" );
    js_t::get_value(   addon_id,   slot_node, "7/0/0" );

    std::string reforge_id;
    int reforge_from, reforge_to;
    if ( js_t::get_value( reforge_from, slot_node, "5/0" ) &&
         js_t::get_value( reforge_to,   slot_node, "5/1" ) )
    {
      if ( ( reforge_from >= 0 ) && ( reforge_to >= 0 ) )
      {
        std::stringstream ss;
        ss << enchant_t::get_reforge_id( util_t::translate_item_mod( reforge_from ),
                                         util_t::translate_item_mod( reforge_to   ) );
        reforge_id = ss.str();
      }
    }

    std::string rsuffix_id;
    int random_suffix;
    if ( js_t::get_value( random_suffix, slot_node, "6" ) )
    {
      if ( random_suffix > 0 ) // random "property" ???
      {
      }
      else if ( random_suffix < 0 ) // random "suffix"
      {
        std::stringstream ss;
        ss << -random_suffix;
        rsuffix_id = ss.str();
      }
    }

    if ( ! item_t::download_slot( item, item_id, enchant_id, addon_id, reforge_id, rsuffix_id, gem_ids ) )
      return 0;
  }

  std::string talents_encoding;
  std::vector<js_node_t*> talent_nodes;
  int num_talents = js_t::get_children( talent_nodes, talents_root );
  for ( int i=0; i < num_talents; i++ )
  {
    std::string ranks;
    if ( js_t::get_value( ranks, talent_nodes[ i ] ) )
    {
      talents_encoding += ranks;
    }
  }
  if ( ! p -> parse_talents_armory( talents_encoding ) )
  {
    sim -> errorf( "Player %s unable to parse talents '%s'.\n", p -> name(), talents_encoding.c_str() );
    return 0;
  }

  std::string player_type_string = util_t::player_type_string( p -> type );
  p -> talents_str = "http://www.wowhead.com/talent#" + player_type_string + "-" + talents_encoding;

  p -> glyphs_str = "";
  std::vector<js_node_t*> glyph_nodes;
  int num_glyphs = js_t::get_children( glyph_nodes, glyphs_root );
  for ( int i=0; i < num_glyphs; i++ )
  {
    std::string glyph_name;
    if ( js_t::get_value( glyph_name, glyph_nodes[ i ], "2/1" ) )
    {
      if      ( glyph_name.substr( 0, 9 ) == "Glyph of " ) glyph_name.erase( 0, 9 );
      else if ( glyph_name.substr( 0, 8 ) == "Glyph - "  ) glyph_name.erase( 0, 8 );
      armory_t::format( glyph_name );
      if ( p -> glyphs_str.size() > 0 ) p -> glyphs_str += "/";
      p -> glyphs_str += glyph_name;
    }
  }

  p -> professions_str = "";
  if ( professions_root )
  {
    std::vector<js_node_t*> skill_nodes;
    int num_skills = js_t::get_children( skill_nodes, professions_root );
    for ( int i=0; i < num_skills; i++ )
    {
      int skill_id;
      std::string skill_level;

      if ( js_t::get_value( skill_id, skill_nodes[ i ], "0" ) &&
           js_t::get_value( skill_level, skill_nodes[ i ], "1" ) )
      {
        std::string skill_name_str = util_t::profession_type_string( util_t::translate_profession_id( skill_id ) );
        if ( i ) p -> professions_str += "/";
        p -> professions_str += skill_name_str + "=" + skill_level;
      }
    }
  }

  return p;
}
