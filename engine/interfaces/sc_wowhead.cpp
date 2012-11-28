// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// source_str ===============================================================

static std::string source_str( wowhead::wowhead_e source )
{
  switch ( source )
  {
  case wowhead::PTR:  return "ptr";
  default:   return "www";
  }
}

static std::string source_desc_str( wowhead::wowhead_e source )
{
  switch ( source )
  {
  case wowhead::PTR:  return "Public Test Realm";
  default:   return "Live";
  }
}

// format_server ============================================================

static std::string& format_server( std::string& name )
{
  if ( name.empty() ) return name;

  std::string buffer;

  int size = ( int ) name.size();
  for ( int i=0; i < size; i++ )
  {
    unsigned char c = name[ i ];

    if ( c >= 0x80 )
    {
      continue;
    }
    else if ( ( c == '\'' ) || ( c == '.' ) )
    {
      continue;
    }
    else if ( isalpha( ( int ) c ) )
    {
      c = std::tolower( ( unsigned ) c );
    }
    buffer += c;
  }
  name = buffer;

  return name;
}

// download_profile =========================================================

static js_node_t* download_profile( sim_t* sim,
                                    const std::string& id,
                                    cache::behavior_e caching )
{
  std::string url_www = "http://www.wowhead.com/profile=load&id=" + id;
  std::string url_ptr = "http://ptr.wowhead.com/profile=load&id=" + id;
  std::string result;

  if ( http::get( result, url_www, caching, "WowheadProfiler.registerProfile(" ) ||
       http::get( result, url_ptr, caching, "WowheadProfiler.registerProfile(" ) )
  {
    std::string::size_type start = result.find( "WowheadProfiler.registerProfile(" );
    if ( start != std::string::npos ) start = result.find( '{', start );
    if ( start != std::string::npos )
    {
      std::string::size_type finish = result.find( ';', start );
      if ( finish != std::string::npos ) finish = result.rfind( '}', finish );
      if ( finish != std::string::npos )
      {
        result.resize( finish + 1 );
        result.erase( 0, start );
        if ( sim -> debug ) util::fprintf( sim -> output_file, "Profile JS:\n%s\n", result.c_str() );
        return js::create( sim, result );
      }
    }
  }

  return 0;
}

// download_id ==============================================================

static xml_node_t* download_id( sim_t*             sim,
                                const std::string& id_str,
                                cache::behavior_e  caching,
                                wowhead::wowhead_e source )
{
  if ( id_str.empty() || id_str == "0" ) return 0;

  std::string url_www = "http://" + source_str( source ) + ".wowhead.com/item="
                        + id_str + "&xml";

  xml_node_t *node = xml_node_t::get( sim, url_www, caching, "</json>" );
  if ( sim -> debug && node ) node -> print();
  return node;
}

// parse_stats ==============================================================

static void parse_stats( std::string& encoding,
                         const std::string& stats_str )
{
  std::vector<std::string> splits;
  std::string temp_stats_str = stats_str;

  util::string_strip_quotes( temp_stats_str );

  int num_splits = util::string_split( splits, temp_stats_str, ",", true );

  for ( int i=0; i < num_splits; i++ )
  {
    std::string type_str, value_str;

    if ( 2 == util::string_split( splits[ i ], ":", "S S", &type_str, &value_str ) )
    {
      stat_e type = util::parse_stat_type( type_str );
      if ( type != STAT_NONE )
      {
        if ( ! encoding.empty() )
          encoding += '_';
        encoding += value_str;
        encoding += util::stat_type_abbrev( type );
      }
    }
  }
}

// parse_gems ===============================================================

static bool parse_gems( item_t&           item,
                        xml_node_t*       node,
                        const std::string gem_ids[ 3 ] )
{
  item.armory_gems_str.clear();

  std::string stats_str;
  if ( ! node || ! node -> get_value( stats_str, "jsonEquip/cdata" ) )
    return true;

  std::string temp_stats_str = stats_str;

  util::string_strip_quotes( temp_stats_str );
  util::tolower( temp_stats_str );

  int sockets[ 3 ] = { 0, 0, 0 };
  std::vector<std::string> splits;
  int num_splits = util::string_split( splits, temp_stats_str, "," );
  int num_sockets = 0;

  for ( int i=0; i < num_splits; i++ )
  {
    std::string type_str;
    int value;

    if ( 2 == util::string_split( splits[ i ], ":", "S i", &type_str, &value ) )
    {
      if ( type_str == "socket1" ) sockets[ 0 ] = value;
      if ( type_str == "socket2" ) sockets[ 1 ] = value;
      if ( type_str == "socket3" ) sockets[ 2 ] = value;
      if ( type_str == "nsockets" ) num_sockets = value;
    }
  }

  if ( num_sockets < 3 )
  {
    if ( item.slot == SLOT_WRISTS ||
         item.slot == SLOT_HANDS  ||
         item.slot == SLOT_WAIST  )
    {
      num_sockets++;
    }
  }

  bool match = true;
  for ( int i=0; i < num_sockets; i++ )
  {
    unsigned gem = item_t::parse_gem( item, gem_ids[ i ] );

    gem_e socket;
    switch ( sockets[ i ] )
    {
    case  1: socket = GEM_META;      break;
    case  2: socket = GEM_RED;       break;
    case  4: socket = GEM_YELLOW;    break;
    case  8: socket = GEM_BLUE;      break;
    case 14: socket = GEM_PRISMATIC; break;
    case 32: socket = GEM_COGWHEEL;  break;
    default: socket = GEM_NONE;      break;
    }

    if ( ! util::socket_gem_match( socket, gem ) )
      match = false;
  }

  if ( match )
  {
    std::string tooltip_str;
    if ( node -> get_value( tooltip_str, "htmlTooltip/cdata" ) )
    {
      const char* search = "Socket Bonus: ";
      std::string::size_type pos = tooltip_str.find( search );
      if ( pos != std::string::npos )
      {
        std::string::size_type start = pos + strlen( search );
        std::string::size_type new_pos = tooltip_str.find( "<", start );
        if ( new_pos != std::string::npos )
        {
          util::fuzzy_stats( item.armory_gems_str, tooltip_str.substr( start, new_pos-start ) );
        }
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
  std::string stats_str;
  if ( ! node || ! node -> get_value( stats_str, "jsonEquip/cdata" ) )
    return true;

  std::string temp_stats_str = stats_str;

  util::string_strip_quotes( temp_stats_str );

  std::string speed, dps, dmgmin, dmgmax;
  std::vector<std::string> splits;
  int num_splits = util::string_split( splits, temp_stats_str, "," );

  for ( int i=0; i < num_splits; i++ )
  {
    std::string type_str, value_str;

    if ( 2 == util::string_split( splits[ i ], ":", "S S", &type_str, &value_str ) )
    {
      if ( type_str == "speed"  ) speed  = value_str;
      if ( type_str == "dps"    ) dps    = value_str;
      if ( type_str == "dmgMin" ) dmgmin = value_str;
      if ( type_str == "dmgMax" ) dmgmax = value_str;
      // Wowhead may change their mind again
      if ( type_str == "dmgmin1" ) dmgmin = value_str;
      if ( type_str == "dmgmax1" ) dmgmax = value_str;
    }
  }

  if ( speed.empty() || dps.empty() || dmgmin.empty() || dmgmax.empty() ) return true;

  std::string subclass_str;
  if ( ! node -> get_value( subclass_str, "subclass/cdata" ) )
    return true;

  weapon_e type = WEAPON_NONE;
  if      ( subclass_str == "One-Handed Axes"         ) type = WEAPON_AXE;
  else if ( subclass_str == "Two-Handed Axes"         ) type = WEAPON_AXE_2H;
  else if ( subclass_str == "Daggers"                 ) type = WEAPON_DAGGER;
  else if ( subclass_str == "Fist Weapons"            ) type = WEAPON_FIST;
  else if ( subclass_str == "One-Handed Maces"        ) type = WEAPON_MACE;
  else if ( subclass_str == "Two-Handed Maces"        ) type = WEAPON_MACE_2H;
  else if ( subclass_str == "Polearms"                ) type = WEAPON_POLEARM;
  else if ( subclass_str == "Staves"                  ) type = WEAPON_STAFF;
  else if ( subclass_str == "One-Handed Swords"       ) type = WEAPON_SWORD;
  else if ( subclass_str == "Two-Handed Swords"       ) type = WEAPON_SWORD_2H;
  else if ( subclass_str == "Bows"                    ) type = WEAPON_BOW;
  else if ( subclass_str == "Crossbows"               ) type = WEAPON_CROSSBOW;
  else if ( subclass_str == "Guns"                    ) type = WEAPON_GUN;
  else if ( subclass_str == "Thrown"                  ) type = WEAPON_THROWN;
  else if ( subclass_str == "Wands"                   ) type = WEAPON_WAND;
  else if ( subclass_str == "Fishing Poles"           ) type = WEAPON_POLEARM;
  else if ( subclass_str == "Miscellaneous (Weapons)" ) type = WEAPON_POLEARM;

  if ( type == WEAPON_NONE ) return false;
  if ( type == WEAPON_WAND ) return true;

  item.armory_weapon_str = util::weapon_type_string( type );
  item.armory_weapon_str += "_" + speed + "speed" + "_" + dmgmin + "min" + "_" + dmgmax + "max";

  return true;
}

// parse_item_stats =========================================================

static bool parse_item_stats( item_t&     item,
                              xml_node_t* node )
{
  item.armory_stats_str.clear();

  std::string stats_str;
  if ( ! node || ! node -> get_value( stats_str, "jsonEquip/cdata" ) )
    return true;

  parse_stats( item.armory_stats_str, stats_str );
  util::tokenize( item.armory_stats_str );

  return true;
}

// parse_item_reforge =======================================================

static bool parse_item_reforge( item_t&     item,
                                xml_node_t* /* node */ )
{
  item.armory_reforge_str.clear();

// TO-DO: Find out how wowhead handles them.
#if 0
  std::string stats_str;
  if ( ! node || ! node -> get_value( stats_str, "jsonEquip/cdata" ) )
    return true;

  parse_stats( item.armory_reforge_str, stats_str );
  util::tokenize( item.armory_reforge_str );
#endif
  return true;
}

// parse_item_name ==========================================================

static bool parse_item_name( item_t&     item,
                             xml_node_t* node )
{
  if ( ! node || ! node -> get_value( item.armory_name_str, "name/cdata" ) )
    return false;

  if ( ! node || ! node -> get_value( item.armory_id_str, "item/id" ) )
    return false;

  util::tokenize( item.armory_name_str );

  return true;
}

// parse_item_heroic ========================================================

static bool parse_item_heroic_lfr( item_t&     item,
                                   xml_node_t* node )
{
  item.armory_heroic_str.clear();
  item.armory_lfr_str.clear();

  std::string info_str;
  if ( ! node || ! node -> get_value( info_str, "json/cdata" ) )
    return false;

  util::string_strip_quotes( info_str );

  std::vector<std::string> splits;
  int num_splits = util::string_split( splits, info_str, "," );

  for ( int i=0; i < num_splits; i++ )
  {
    std::string type_str, value_str;

    if ( 2 == util::string_split( splits[ i ], ":", "S S", &type_str, &value_str ) )
    {
      if ( type_str == "heroic" )
      {
        item.armory_heroic_str = value_str;
        break;
      }
      else if ( type_str == "raidfinder" )
      {
        item.armory_lfr_str = value_str;
        break;
      }
    }
  }

  util::tokenize( item.armory_heroic_str );
  util::tokenize( item.armory_lfr_str );

  return true;
}

// parse_item_quality =======================================================

static bool parse_item_quality( item_t&     item,
                                xml_node_t* node )
{
  int quality;
  if ( ! node || ! node -> get_value( quality, "quality/id" ) )
    return false;

  item.armory_quality_str.clear();
  // Let's just convert the quality id to text, and then
  // in decode() parse it into an integer
  if ( quality > 1 )
    item.armory_quality_str = util::item_quality_string( quality );

  return true;
}

// parse_item_level =========================================================

static bool parse_item_level( item_t&     item,
                              xml_node_t* node )
{
  std::string info_str;

  item.armory_ilevel_str.clear();

  if ( ! node || ! node -> get_value( info_str, "level/." ) )
    return false;

  item.armory_ilevel_str = info_str;

  return true;
}

// parse_item_armor_type ====================================================

static bool parse_item_armor_type( item_t&     item,
                                   xml_node_t* node )
{
  std::string info_str;
  item.armory_armor_type_str = "";
  bool is_armor = false;
  std::string temp_str = "";

  if ( ! node || ! node -> get_value( info_str, "json/cdata" ) )
    return false;

  std::string temp_info_str = info_str;

  util::string_strip_quotes( temp_info_str );

  std::vector<std::string> splits;
  int num_splits = util::string_split( splits, temp_info_str, "," );

  for ( int i=0; i < num_splits; i++ )
  {
    std::string type_str, value_str;

    if ( 2 == util::string_split( splits[ i ], ":", "S S", &type_str, &value_str ) )
    {
      if ( type_str == "classs"   )
      {
        if ( value_str == "4" )
        {
          is_armor = true;
        }
      }
      else if ( type_str == "subclass"   )
      {
        temp_str = value_str;
        break;
      }
    }
  }

  if ( is_armor && ( temp_str != "" ) )
  {
    if      ( temp_str == "1" ) { item.armory_armor_type_str = "cloth"; }
    else if ( temp_str == "2" ) { item.armory_armor_type_str = "leather"; }
    else if ( temp_str == "3" ) { item.armory_armor_type_str = "mail"; }
    else if ( temp_str == "4" ) { item.armory_armor_type_str = "plate"; }

    util::tokenize( item.armory_heroic_str );
  }

  return true;
}

// translate_profession_id ==================================================

static const char* translate_profession_id( int type )
{
  switch ( type )
  {
  case PROF_ALCHEMY:        return "skills/171";
  case PROF_BLACKSMITHING:  return "skills/164";
  case PROF_ENCHANTING:     return "skills/333";
  case PROF_ENGINEERING:    return "skills/202";
  case PROF_HERBALISM:      return "skills/182";
  case PROF_INSCRIPTION:    return "skills/773";
  case PROF_JEWELCRAFTING:  return "skills/755";
  case PROF_LEATHERWORKING: return "skills/165";
  case PROF_MINING:         return "skills/186";
  case PROF_SKINNING:       return "skills/393";
  case PROF_TAILORING:      return "skills/197";
  }
  return "unknown";
}

// translate_inventory_id ===================================================

static const char* translate_inventory_id( int slot )
{
  switch ( slot )
  {
  case SLOT_HEAD:      return "inventory/1";
  case SLOT_NECK:      return "inventory/2";
  case SLOT_SHOULDERS: return "inventory/3";
  case SLOT_SHIRT:     return "inventory/4";
  case SLOT_CHEST:     return "inventory/5";
  case SLOT_WAIST:     return "inventory/6";
  case SLOT_LEGS:      return "inventory/7";
  case SLOT_FEET:      return "inventory/8";
  case SLOT_WRISTS:    return "inventory/9";
  case SLOT_HANDS:     return "inventory/10";
  case SLOT_FINGER_1:  return "inventory/11";
  case SLOT_FINGER_2:  return "inventory/12";
  case SLOT_TRINKET_1: return "inventory/13";
  case SLOT_TRINKET_2: return "inventory/14";
  case SLOT_BACK:      return "inventory/15";
  case SLOT_MAIN_HAND: return "inventory/16";
  case SLOT_OFF_HAND:  return "inventory/17";
  case SLOT_RANGED:    return "inventory/18";
  }

  return "unknown";
}

static bool is_valid_profile_id( const std::string& id )
{
  // IIRC, Can't test right now.
  for ( size_t i = 0, s = id.size(); i < s; ++i )
    if ( ! std::isdigit( id[ i ] ) )
      return false;
  return true;
}

// download_player_profile ==================================================

static player_t* download_player_profile( sim_t* sim,
                                          const std::string& id,
                                          const std::string& spec,
                                          cache::behavior_e caching )
{
  sim -> current_slot = 0;
  sim -> current_name = id;

  js_node_t* profile_js = download_profile( sim, id, caching );

  if ( ! profile_js )
  {
    sim -> errorf( "Unable to download character profile %s from wowhead.\n", id.c_str() );
    return 0;
  }

  if ( sim -> debug ) js::print( profile_js, sim -> output_file );

  std::string name_str;
  if ( ! js::get_value( name_str, profile_js, "name"  ) )
  {
    sim -> errorf( "Unable to extract player name from wowhead id '%s'.\n", id.c_str() );
    name_str = "wowhead" + id;
  }

  sim -> current_name = name_str;

  util::format_text ( name_str, sim -> input_is_utf8 );

  std::string level_str;
  if ( ! js::get_value( level_str, profile_js, "level"  ) )
  {
    sim -> errorf( "Unable to extract player level from wowhead id '%s'.\n", id.c_str() );
    return 0;
  }
  int level = atoi( level_str.c_str() );

  std::string cid_str;
  if ( ! js::get_value( cid_str, profile_js, "classs" ) )
  {
    sim -> errorf( "Unable to extract player class from wowhead id '%s'.\n", id.c_str() );
    return 0;
  }
  player_e player_type = util::translate_class_id( atoi( cid_str.c_str() ) );
  std::string type_str = util::player_type_string( player_type );

  std::string rid_str;
  if ( ! js::get_value( rid_str, profile_js, "race" ) )
  {
    sim -> errorf( "Unable to extract player race from wowhead id '%s'.\n", id.c_str() );
    return 0;
  }
  race_e r = util::translate_race_id( atoi( rid_str.c_str() ) );

  module_t* module = module_t::get( type_str );

  if ( ! module || ! module -> valid() )
  {
    sim -> errorf( "\nModule for class %s is currently not available.\n", type_str.c_str() );
    return 0;
  }

  player_t* p = sim -> active_player = module -> create_player( sim, name_str, r );

  if ( ! p )
  {
    sim -> errorf( "Unable to build player with class '%s' and name '%s' from wowhead id '%s'.\n",
                   type_str.c_str(), name_str.c_str(), id.c_str() );
    return 0;
  }

  p -> level = level;

  std::vector<std::string> region_data;
  int num_region = js::get_value( region_data, profile_js, "region" );
  if ( num_region > 0 ) p -> region_str = region_data[ 0 ];

  std::vector<std::string> realm_data;
  int num_realm = js::get_value( realm_data, profile_js, "realm" );
  if ( num_realm > 0 ) p -> server_str = realm_data[ 0 ];

  int user_id=0;
  if ( js::get_value( user_id, profile_js, "id" ) && ( user_id != 0 ) )
  {
    p -> origin_str = "http://www.wowhead.com/profile=" + id;
  }
  else
  {
    std::string server_name = p -> server_str;
    std::string character_name = name_str;
    format_server( server_name );
    util::tokenize( character_name, FORMAT_CHAR_NAME );
    p -> origin_str = "http://www.wowhead.com/profile=" + p -> region_str + "." + server_name + "." + character_name;
  }

  for ( profession_e i = PROFESSION_NONE; i < PROFESSION_MAX; i++ )
  {
    std::vector<std::string> skill_levels;
    if ( 2 == js::get_value( skill_levels, profile_js, translate_profession_id( i ) ) )
    {
      if ( ! p -> professions_str.empty() ) p -> professions_str += '/';
      p -> professions_str += util::profession_type_string( i );
      p -> professions_str += "=";
      p -> professions_str += skill_levels[ 0 ];
    }
  }

  int active_talents = 0;
  js::get_value( active_talents, profile_js, "talents/active" );

  int use_talents;
  if ( util::str_compare_ci( spec, "active" ) )
    use_talents = active_talents;
  else if ( util::str_compare_ci( spec, "inactive" ) )
    use_talents = 1 - active_talents;
  else if ( util::str_compare_ci( spec, "primary" ) )
    use_talents = 0;
  else if ( util::str_compare_ci( spec, "secondary" ) )
    use_talents = 1;
  else
  {
    // FIXME.
    sim -> errorf( "Warning: no one implemented spec matching for Wowhead. Using active talents\n" );
    use_talents = active_talents;
  }

  js_node_t* builds = js::get_node( profile_js, "talents/builds" );

  if ( builds ) // !!! NEW FORMAT !!!
  {
    js_node_t* build = js::get_node( builds, ( use_talents ? "1" : "0" ) );
    if ( ! build )
    {
      sim -> errorf( "Player %s unable to access talent/glyph build from profile.\n", p -> name() );
      return 0;
    }

    // FIX-ME: Temporary override until wowhead profile updated for MoP

    // Determine spec from number of talent points spent.
    js_node_t* spents = js::get_node( build, "spent" );
    if ( spents )
    {
      js_node_t* spent[ 3 ];
      int maxv = 0;
      uint32_t maxi = 0;

      for ( uint32_t i = 0; i < 3; i++ )
      {
        int v;
        spent[ i ] = js::get_node( spents, util::to_string( i ) );
        if ( spent[ i ] && ( js::get_value( v, spent[ i ] ) ) )
        {
          if ( v >= maxv )
          {
            maxv = v;
            maxi = i;
          }
        }
      }

      if ( maxv > 0 )
      {
        if ( p -> type == DRUID && maxi == 2 )
          maxi = 3;

        p -> _spec = p -> dbc.spec_by_idx( p -> type, maxi );
      }
    }
    std::string talent_encoding;

    if ( ! js::get_value( talent_encoding, build, "talents" ) )
    {
      sim -> errorf( "Player %s unable to access talent encoding from profile.\n", p -> name() );
      return 0;
    }

    if ( p -> _spec == DRUID_FERAL && talent_encoding[ 30 ] > '0' )
    {
      p -> _spec = DRUID_GUARDIAN;
    }

    talent_encoding = p -> set_default_talents();


    if ( ! p -> parse_talents_numbers( talent_encoding ) )
    {
      sim -> errorf( "Player %s unable to parse talent encoding '%s'.\n", p -> name(), talent_encoding.c_str() );
      return 0;
    }

    p -> create_talents_armory();

    p -> glyphs_str = p -> set_default_glyphs();

/*
    std::string glyph_encoding;
    if ( ! js::get_value( glyph_encoding, build, "glyphs" ) )
    {
      sim -> errorf( "Player %s unable to access glyph encoding from profile.\n", p -> name() );
      return 0;
    }

    std::vector<std::string> glyph_ids;
    int num_glyphs = util::string_split( glyph_ids, glyph_encoding, ":" );
    for ( int i=0; i < num_glyphs; i++ )
    {
      std::string& glyph_id = glyph_ids[ i ];
      if ( glyph_id == "0" ) continue;
      std::string glyph_name;

      if ( ! item_t::download_glyph( p, glyph_name, glyph_id ) )
      {
        return 0;
      }
      if ( i ) p -> glyphs_str += "/";
      p -> glyphs_str += glyph_name;
    }
*/
  }
  else // !!! OLD FORMAT !!!
  {
    // FIX-ME: Temporary override until wowhead profile updated for MoP
    std::string talent_encodings;
    talent_encodings = p -> set_default_talents();
    if ( ! p -> parse_talents_numbers( talent_encodings ) )
    {
      sim -> errorf( "Player %s unable to parse talent encoding '%s'.\n", p -> name(), talent_encodings.c_str() );
      return 0;
    }

    p -> create_talents_wowhead();
    p -> glyphs_str = p -> set_default_glyphs();


/*
    std::vector<std::string> talent_encodings;

    int num_builds = js::get_value( talent_encodings, profile_js, "talents/build" );
    if ( num_builds == 2 )
    {
      std::string& encoding = talent_encodings[ use_talents ];
      if ( ! p -> parse_talents_armory( encoding ) )
      {
        sim -> errorf( "Player %s unable to parse talent encoding '%s'.\n", p -> name(), encoding.c_str() );
        return 0;
      }
      p -> talents_str = "http://www.wowhead.com/talent#" + type_str + "-" + encoding;
    }

    std::vector<std::string> glyph_encodings;
    num_builds = js::get_value( glyph_encodings, profile_js, "glyphs" );
    if ( num_builds == 2 )
    {
      std::vector<std::string> glyph_ids;
      int num_glyphs = util::string_split( glyph_ids, glyph_encodings[ use_talents ], ":" );
      for ( int i=0; i < num_glyphs; i++ )
      {
        std::string& glyph_id = glyph_ids[ i ];
        if ( glyph_id == "0" ) continue;
        std::string glyph_name;

        if ( ! item_t::download_glyph( p, glyph_name, glyph_id ) )
          return 0;

        if ( i ) p -> glyphs_str += '/';
        p -> glyphs_str += glyph_name;
      }
    }
*/
  }

  for ( int i=0; i < SLOT_MAX; i++ )
  {
    sim -> current_slot = i;
    if ( sim -> canceled ) return 0;

    std::vector<std::string> inventory_data;
    if ( js::get_value( inventory_data, profile_js, translate_inventory_id( i ) ) )
    {
      std::string    item_id = inventory_data[ 0 ];
      std::string rsuffix_id = ( inventory_data[ 1 ] == "0" ) ? "" : inventory_data[ 1 ];
      std::string enchant_id = inventory_data[ 2 ];

      std::string gem_ids[ 3 ];
      gem_ids[ 0 ] = inventory_data[ 4 ];
      gem_ids[ 1 ] = inventory_data[ 5 ];
      gem_ids[ 2 ] = inventory_data[ 6 ];

      std::string addon_id; // WoWHead only supports an enchant OR tinker, both are in the enchant spot
      std::string reforge_id = inventory_data[ 8 ];

      if ( item_id == "0" ) continue; // ignore empty slots

      if ( ! item_t::download_slot( p -> items[ i ], item_id, enchant_id, addon_id, reforge_id, rsuffix_id, "", gem_ids ) ) // FIXME: Proper upgrade level once wowhead supports it
        return 0;

    }
  }
  // TO-DO: can remove remaining ranged slot references once BCP updated for MoP
  if ( p -> type == HUNTER && ! p -> items[ SLOT_RANGED ].armory_name_str.empty() )
  {
    p -> items[ SLOT_RANGED ].slot = SLOT_MAIN_HAND;
    p -> items[ SLOT_MAIN_HAND ] = p -> items[ SLOT_RANGED ];
  }
  p -> items[ SLOT_RANGED ].armory_name_str.clear();

  return p;
}

// wowhead::parse_gem =====================================================

gem_e wowhead::parse_gem( item_t&            item,
                          const std::string& gem_id,
                          wowhead_e          source,
                          cache::behavior_e  caching )
{
  if ( gem_id.empty() || gem_id == "0" )
    return GEM_NONE;

  xml_node_t* node = download_id( item.sim, gem_id, caching, source );
  if ( ! node )
  {
    if ( caching != cache::ONLY )
      item.sim -> errorf( "Player %s unable to download gem id %s from wowhead\n", item.player -> name(), gem_id.c_str() );
    return GEM_NONE;
  }

  gem_e type = GEM_NONE;

  std::string color_str;
  if ( node -> get_value( color_str, "subclass/cdata" ) )
  {
    std::string::size_type pos = color_str.find( ' ' );
    if ( pos != std::string::npos ) color_str.erase( pos );
    util::tokenize( color_str );
    type = util::parse_gem_type( color_str );

    if ( type == GEM_META )
    {
      std::string name_str;
      if ( node -> get_value( name_str, "name/cdata" ) )
      {
        std::string::size_type new_pos = name_str.find( " Diamond" );
        if ( new_pos != std::string::npos ) name_str.erase( new_pos );
        util::tokenize( name_str );
        if ( ! item.armory_gems_str.empty() )
          item.armory_gems_str += '_';
        item.armory_gems_str += name_str;
      }
    }
    else
    {
      std::string stats_str;
      if ( node -> get_value( stats_str, "jsonEquip/cdata" ) )
      {
        parse_stats( item.armory_gems_str, stats_str );
      }
    }
  }

  return type;
}

// wowhead::download_glyph ================================================

bool wowhead::download_glyph( player_t*          player,
                              std::string&       glyph_name,
                              const std::string& glyph_id,
                              wowhead_e          source,
                              cache::behavior_e  caching )
{
  xml_node_t* node = download_id( player -> sim, glyph_id, caching, source );
  if ( ! node || ! node -> get_value( glyph_name, "name/cdata" ) )
  {
    if ( caching != cache::ONLY )
      player -> sim -> errorf( "Unable to download glyph id %s from wowhead\n", glyph_id.c_str() );
    return false;
  }

  return true;
}

// wowhead::download_item =================================================

bool wowhead::download_item( item_t&            item,
                             const std::string& item_id,
                             wowhead_e          source,
                             cache::behavior_e  caching )
{
  player_t* p = item.player;
  std::string error_str;

  xml_node_t* node = download_id( item.sim, item_id, caching, source );
  if ( ! node )
  {
    if ( caching != cache::ONLY )
      item.sim -> errorf( "Player %s unable to download item id '%s' from wowhead at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( node && node -> get_value( error_str, "error/." ) )
  {
    if ( item.sim -> debug )
    {
      item.sim -> errorf( "Wowhead (%s): Player %s item id '%s' in slot '%s' error: %s",
                          source_desc_str( source ).c_str(), p -> name(),
                          item_id.c_str(), item.slot_name(), error_str.c_str() );
    }

    return false;
  }

  if ( ! parse_item_name( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine item name for id '%s'' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_quality( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine item quality for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_level( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine item level for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_heroic_lfr( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine heroic/LFR flag for id %s at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
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

// wowhead::download_slot =================================================

bool wowhead::download_slot( item_t&            item,
                             const std::string& item_id,
                             const std::string& enchant_id,
                             const std::string& addon_id,
                             const std::string& reforge_id,
                             const std::string& rsuffix_id,
                             const std::string  gem_ids[ 3 ],
                             wowhead_e          source,
                             cache::behavior_e  caching )
{
  player_t* p = item.player;
  std::string error_str;

  xml_node_t* node = download_id( item.sim, item_id, caching, source );
  if ( ! node )
  {
    if ( caching != cache::ONLY )
      item.sim -> errorf( "Player %s unable to download item id '%s' from wowhead at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( node && node -> get_value( error_str, "error/." ) )
  {
    if ( item.sim -> debug )
    {
      item.sim -> errorf( "Wowhead (%s): Player %s item id '%s' in slot '%s' error: %s",
                          source_desc_str( source ).c_str(), p -> name(),
                          item_id.c_str(), item.slot_name(), error_str.c_str() );
    }

    return false;
  }

  if ( ! parse_item_name( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine item name for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_quality( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine item quality for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_level( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine item level for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
    return false;
  }

  if ( ! parse_item_heroic_lfr( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine heroic flag for id '%s' at slot %s.\n", p -> name(), item_id.c_str(), item.slot_name() );
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

  if ( ! parse_item_reforge( item, node ) )
  {
    item.sim -> errorf( "Player %s unable to determine reforge for item '%s' at slot %s.\n", p -> name(), item.name(), item.slot_name() );
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

// wowhead::download_player ===============================================

player_t* wowhead::download_player( sim_t* sim,
                                    const std::string& region,
                                    const std::string& server,
                                    const std::string& name,
                                    const std::string& spec,
                                    wowhead_e source,
                                    cache::behavior_e  caching )
{
  std::string id = name;

  if ( ! is_valid_profile_id( id ) )
  {
    std::string character_name = name;
    util::tokenize( character_name, FORMAT_CHAR_NAME );

    std::string server_name = server;
    format_server( server_name );

    std::string url = "http://" + source_str( source ) + ".wowhead.com/profile=" + region + "." + server_name + "." + character_name;
    std::string result;
    if ( http::get( result, url, caching, "profilah.initialize(" ) )
    {
      if ( sim -> debug ) util::fprintf( sim -> output_file, "%s\n%s\n", url.c_str(), result.c_str() );

      std::string::size_type start = result.find( "profilah.initialize(" );
      if ( start != std::string::npos )
      {
        std::string::size_type finish = result.find( ';', start );
        if ( finish != std::string::npos )
        {
          std::vector<std::string> splits;
          int num_splits = util::string_split( splits, result.substr( start, finish-start ), "(){}.;,: \t\n" );
          for ( int i=0; i < num_splits-1; i++ )
          {
            if ( splits[ i ] == "id" )
            {
              id = splits[ i + 1 ];
              break;
            }
          }
        }
      }
    }
  }

  return download_player_profile( sim, id, spec, caching );
}
