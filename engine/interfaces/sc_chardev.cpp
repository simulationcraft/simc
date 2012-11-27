// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// download_profile =========================================================

static js_node_t* download_profile( sim_t* sim,
                                    const std::string& id,
                                    cache::behavior_e caching,
                                    bool mop )
{
  std::string url = "";
  std::string profile_str;

  if ( mop )
    url = "http://mop.chardev.org/php/interface/profiles/get_profile.php?id=" + id;
  else
    url = "http://chardev.org/php/interface/profiles/get_profile.php?id=" + id;


  if ( ! http::get( profile_str, url, caching ) )
    return 0;

  return js::create( sim, profile_str );
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
  }

  return "unknown";
}

// chardev::download_player ===============================================

player_t* chardev::download_player( sim_t* sim,
                                    const std::string& id,
                                    cache::behavior_e caching,
                                    bool mop )
{
  sim -> current_slot = 0;
  sim -> current_name = id;

  js_node_t* profile_js = download_profile( sim, id, caching, mop );
  if ( ! profile_js || ! ( profile_js = js::get_node( profile_js, "character" ) ) )
  {
    sim -> errorf( "Unable to download character profile %s from chardev.\n", id.c_str() );
    return 0;
  }

  if ( sim -> debug ) js::print( profile_js, sim -> output_file );

  std::string name_str, race_str, type_str, level_str;
  uint32_t level;

  if ( ! js::get_value( name_str, profile_js, "0/0" ) )
    name_str = "chardev_" + id;

  if ( ! js::get_value( race_str,  profile_js, "0/2/1" ) ||
       ! js::get_value( type_str,  profile_js, "0/3/1" ) ||
       ! js::get_value( level_str, profile_js, "0/4" ) )
  {
    sim -> errorf( "Unable to extract player race/type/level from CharDev id %s.\nThis is often caused by not saving the profile.\n", id.c_str() );
    return 0;
  }

  util::tokenize( type_str );
  util::tokenize( race_str );

  // Hack to discover "death_knight"
  if ( type_str == "death_knight" )
    type_str = "deathknight";

  if ( 1 != util::string_split( level_str, "", "i", &level ) )
  {
    sim -> errorf( "Unable to parse player level from CharDev id %s.\nThis is often caused by not saving the profile.\n", id.c_str() );
    return 0;
  }

  race_e race = util::parse_race_type( race_str );
  module_t* module = module_t::get( type_str );

  if ( ! module || ! module -> valid() )
  {
    sim -> errorf( "\nModule for class %s is currently not available.\n", type_str.c_str() );
    return 0;
  }

  player_t* p = sim -> active_player = module -> create_player( sim, name_str, race );

  if ( ! p )
  {
    sim -> errorf( "Unable to build player with class '%s' and name '%s' from CharDev id %s.\n",
                   type_str.c_str(), name_str.c_str(), id.c_str() );
    return 0;
  }

  p -> level = level;

  p -> origin_str = ( mop ? "http://mop.chardev.org/profile/" : "http://chardev.org/profile/" ) + id + '-' + name_str + ".html";
  http::format( p -> origin_str );

  js_node_t*        gear_root = js::get_child( profile_js, "1" );

  js_node_t*     talents_root = js::get_child( profile_js, "2" );

  js_node_t*      glyphs_root = js::get_child( profile_js, "3" );

  js_node_t*        spec_root = js::get_child( profile_js, "4" );

  js_node_t* professions_root = js::get_child( profile_js, "5" );

  for ( int i=0; i < SLOT_MAX; i++ )
  {
    if ( sim -> canceled ) return 0;
    sim -> current_slot = i;
    item_t& item = p -> items[ i ];

    js_node_t* slot_node = js::get_child( gear_root, translate_slot( i ) );
    if ( ! slot_node ) continue;

    std::string item_id;
    js::get_value( item_id, slot_node, "0/0" );
    if ( item_id.empty() ) continue;

    std::string enchant_id, addon_id, gem_ids[ 3 ];
    js::get_value( gem_ids[ 0 ], slot_node, "1/0" );
    js::get_value( gem_ids[ 1 ], slot_node, "2/0" );
    js::get_value( gem_ids[ 2 ], slot_node, "3/0" );
    js::get_value( enchant_id,   slot_node, "4/0" );
    js::get_value(   addon_id,   slot_node, "7/0/0" );

    std::string reforge_id;
    int reforge_from, reforge_to;
    if ( js::get_value( reforge_from, slot_node, "5/0" ) &&
         js::get_value( reforge_to,   slot_node, "5/1" ) )
    {
      if ( ( reforge_from >= 0 ) && ( reforge_to >= 0 ) )
      {
        stat_e from = util::translate_item_mod( static_cast<item_mod_type>( reforge_from ) );
        stat_e to   = util::translate_item_mod( static_cast<item_mod_type>( reforge_to ) );
        reforge_id = util::to_string( enchant::get_reforge_id( from, to ) );
      }
    }

    std::string rsuffix_id;
    int random_suffix;
    if ( js::get_value( random_suffix, slot_node, "6" ) )
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

    if ( ! item_t::download_slot( item, item_id, enchant_id, addon_id, reforge_id, rsuffix_id, "0", gem_ids ) ) // FIXME: Proper upgrade level once chardev supports it
      return 0;
  }

  // TO-DO: can remove remaining ranged slot references once BCP updated for MoP
  if ( p -> type == HUNTER && ! p -> items[ SLOT_RANGED ].armory_name_str.empty() )
  {
    p -> items[ SLOT_RANGED ].slot = SLOT_MAIN_HAND;
    p -> items[ SLOT_MAIN_HAND ] = p -> items[ SLOT_RANGED ];
  }
  p -> items[ SLOT_RANGED ].armory_name_str.clear();



  std::string talent_encodings;

  if ( mop )
  {
    if ( ! js::get_value( talent_encodings, talents_root ) )
    {
      sim -> errorf( "\nTalent data is not available.\n" );
      return 0;
    }

    std::string spec_str;

    if ( ! js::get_value( spec_str, spec_root ) )
    {
      sim -> errorf( "\nSpecialization is not available.\n" );
      return 0;
    }

    uint32_t maxi = 0;

    if ( 1 != util::string_split( spec_str, " ", "i", &maxi ) )
    {
      sim -> errorf( "\nInvalid Specialization given.\n" );
      return 0;
    }

    p -> _spec = p -> dbc.spec_by_idx( p -> type, maxi );
  }
  else
  {
    // FIX-ME: Temporary override until chardev profile updated for MoP
    uint32_t maxv = 0;
    uint32_t maxi = 0;
    uint32_t v[ 3 ];
    v[ 0 ] = v[ 1 ] = v[ 2 ] = 0;

    std::vector<js_node_t*> talent_nodes;
    int num_talents = js::get_children( talent_nodes, talents_root );
    for ( int i=0; i < num_talents; i++ )
    {
      int ranks;
      if ( js::get_value( ranks, talent_nodes[ i ] ) )
      {
        v[ ( 3 * i ) / num_talents ] += ranks;
      }
    }
    maxv = v[ 0 ];
    for ( int i = 1; i < 3; i++ )
    {
      if ( v[ i ] >= maxv )
      {
        maxi = i;
        maxv = v[ i ];
      }
    }

    if ( maxv > 0 )
    {
      if ( p -> type == DRUID && maxi == 2 )
        maxi = 3;

      p -> _spec = p -> dbc.spec_by_idx( p -> type, maxi );
    }

    if ( p -> _spec == DRUID_FERAL )
    {
      int a;

      js::get_value( a, talent_nodes[ 30 ] );

      if ( a > 0 )
      {
        p -> _spec = DRUID_GUARDIAN;
      }
    }

    talent_encodings = p -> set_default_talents();
  }

  if ( ! p -> parse_talents_numbers( talent_encodings ) )
  {
    sim -> errorf( "Player %s unable to parse talent encoding '%s'.\n", p -> name(), talent_encodings.c_str() );
    return 0;
  }

  p -> create_talents_armory();

  if ( mop )
  {
    p -> glyphs_str = "";
    std::vector<js_node_t*> glyph_nodes;
    int num_glyphs = js::get_children( glyph_nodes, glyphs_root );
    for ( int i=0; i < num_glyphs; i++ )
    {
      std::string glyph_name;
      if ( js::get_value( glyph_name, glyph_nodes[ i ], "2/1" ) )
      {
        util::glyph_name( glyph_name );
        if ( ! p -> glyphs_str.empty() )
          p -> glyphs_str += '/';
        p -> glyphs_str += glyph_name;
      }
    }
  }
  else
  {
    p -> glyphs_str = p -> set_default_glyphs();
  }

  p -> professions_str = "";
  if ( professions_root )
  {
    std::vector<js_node_t*> skill_nodes;
    int num_skills = js::get_children( skill_nodes, professions_root );
    for ( int i=0; i < num_skills; i++ )
    {
      int skill_id;
      std::string skill_level;

      if ( js::get_value( skill_id, skill_nodes[ i ], "0" ) &&
           js::get_value( skill_level, skill_nodes[ i ], "1" ) )
      {
        std::string skill_name_str = util::profession_type_string( util::translate_profession_id( skill_id ) );
        if ( i ) p -> professions_str += "/";
        p -> professions_str += skill_name_str + "=" + skill_level;
      }
    }
  }

  return p;
}
