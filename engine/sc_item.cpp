// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE ==========================================

struct token_t
{
  std::string full;
  std::string name;
  double value;
  std::string value_str;
};

// parse_tokens =============================================================

int parse_tokens( std::vector<token_t>& tokens,
                  const std::string&    encoded_str )
{
  std::vector<std::string> splits;
  int num_splits = util::string_split( splits, encoded_str, "_" );

  tokens.resize( num_splits );
  for ( int i=0; i < num_splits; i++ )
  {
    token_t& t = tokens[ i ];
    t.full = splits[ i ];
    int index=0;
    while ( t.full[ index ] != '\0' &&
            t.full[ index ] != '%'  &&
            ! isalpha( static_cast<unsigned char>( t.full[ index ] ) ) ) index++;
    if ( index == 0 )
    {
      t.name = t.full;
      t.value = 0;
    }
    else
    {
      t.name = t.full.substr( index );
      t.value_str = t.full.substr( 0, index );
      t.value = atof( t.value_str.c_str() );
    }
  }

  return num_splits;
}

// is_meta_prefix ===========================================================

bool is_meta_prefix( const std::string& option_name )
{
  for ( meta_gem_e i = META_GEM_NONE; i < META_GEM_MAX; i++ )
  {
    const char* meta_gem_name = util::meta_gem_type_string( i );

    for ( int j=0; tolower( meta_gem_name[ j ] ) == tolower( option_name[ j ] ); j++ )
      if ( option_name[ j+1 ] == '\0' )
        return true;
  }

  return false;
}

// is_meta_suffix ===========================================================

bool is_meta_suffix( const std::string& option_name )
{
  for ( meta_gem_e i = META_GEM_NONE; i < META_GEM_MAX; i++ )
  {
    const char* meta_gem_name = util::meta_gem_type_string( i );

    const char* s = strstr( meta_gem_name, "_" );
    if ( ! s ) continue;
    s++;

    for ( int j=0; tolower( s[ j ] ) == tolower( option_name[ j ] ); j++ )
      if ( option_name[ j ] == '\0' )
        return true;
  }

  return false;
}

// parse_meta_gem ===========================================================

meta_gem_e parse_meta_gem( const std::string& prefix,
                           const std::string& suffix )
{
  if ( prefix.empty() || suffix.empty() ) return META_GEM_NONE;
  return util::parse_meta_gem_type( prefix + '_' + suffix );
}

} // UNNAMED NAMESPACE ====================================================

// item_t::item_t ===========================================================

item_t::item_t( player_t* p, const std::string& o ) :
  sim( p -> sim ), player( p ), slot( SLOT_INVALID ), quality( 0 ), ilevel( 0 ), effective_ilevel( 0 ), unique( false ), unique_enchant( false ),
  unique_addon( false ), is_heroic( false ), is_lfr( false ), is_ptr( p -> dbc.ptr ),
  is_matching_type( false ), is_reforged( false ), reforged_from( STAT_NONE ), reforged_to( STAT_NONE ), upgrade_level( 0 ), options_str( o )
{
}

// item_t::active ===========================================================

bool item_t::active()
{
  if ( slot == SLOT_INVALID ) return false;
  if ( ! ( encoded_name_str.empty() || encoded_name_str == "empty" || encoded_name_str == "none" ) ) return true;
  return false;
}

// item_t::heroic ===========================================================

bool item_t::heroic()
{
  if ( slot == SLOT_INVALID ) return false;
  return is_heroic;
}

// item_t::lfr ==============================================================

bool item_t::lfr()
{
  if ( slot == SLOT_INVALID ) return false;
  return is_lfr;
}

// item_t::ptr ==============================================================

bool item_t::ptr()
{
  return is_ptr;
}

// item_t::matching_type ====================================================

bool item_t::matching_type()
{
  if ( slot == SLOT_INVALID ) return false;
  return is_matching_type;
}

// item_t::reforged =========================================================

bool item_t::reforged()
{
  if ( slot == SLOT_INVALID ) return false;
  return is_reforged;
}

// item_t::name =============================================================

const char* item_t::name()
{
  if ( ! encoded_name_str.empty() ) return encoded_name_str.c_str();
  if ( !  armory_name_str.empty() ) return  armory_name_str.c_str();
  return "inactive";
}

// item_t::name =============================================================

std::string& item_t::name_str()
{
  static std::string inactive = "inactive";
  if ( ! encoded_name_str.empty() ) return encoded_name_str;
  if ( !  armory_name_str.empty() ) return  armory_name_str;
  return inactive;
}

// item_t::slot_name ========================================================

const char* item_t::slot_name()
{
  return util::slot_type_string( slot );
}

// item_t::slot_name ========================================================

const char* item_t::armor_type()
{
  return util::armor_type_string( player -> type, slot );
}

// item_t::weapon ===========================================================

weapon_t* item_t::weapon()
{
  if ( slot == SLOT_MAIN_HAND ) return &( player -> main_hand_weapon );
  if ( slot == SLOT_OFF_HAND  ) return &( player ->  off_hand_weapon );
  return 0;
}

// item_t::parse_options ====================================================

bool item_t::parse_options()
{
  if ( options_str.empty() ) return true;

  option_name_str = options_str;
  std::string remainder = "";

  std::string::size_type cut_pt = options_str.find_first_of( "," );

  if ( cut_pt != options_str.npos )
  {
    remainder       = options_str.substr( cut_pt + 1 );
    option_name_str = options_str.substr( 0, cut_pt );
  }

  option_t options[] =
  {
    opt_string( "id", option_id_str ),
    opt_string( "stats", option_stats_str ),
    opt_string( "gems", option_gems_str ),
    opt_string( "enchant", option_enchant_str ),
    opt_string( "addon", option_addon_str ),
    opt_string( "equip", option_equip_str ),
    opt_string( "use", option_use_str ),
    opt_string( "weapon", option_weapon_str ),
    opt_string( "heroic", option_heroic_str ),
    opt_string( "lfr", option_lfr_str ),
    opt_string( "type", option_armor_type_str ),
    opt_string( "reforge", option_reforge_str ),
    opt_string( "suffix", option_random_suffix_str ),
    opt_string( "ilevel", option_ilevel_str ),
    opt_string( "eilevel", option_effective_ilevel_str ),
    opt_string( "quality", option_quality_str ),
    opt_string( "source", option_data_source_str ),
    opt_string( "upgrade", option_upgrade_level_str ),
    opt_null()
  };

  option_t::parse( sim, option_name_str.c_str(), options, remainder );

  util::tokenize( option_name_str );

  util::tolower( option_id_str               );
  util::tolower( option_stats_str            );
  util::tolower( option_gems_str             );
  util::tolower( option_enchant_str          );
  util::tolower( option_addon_str            );
  util::tolower( option_equip_str            );
  util::tolower( option_use_str              );
  util::tolower( option_weapon_str           );
  util::tolower( option_heroic_str           );
  util::tolower( option_lfr_str              );
  util::tolower( option_armor_type_str       );
  util::tolower( option_reforge_str          );
  util::tolower( option_random_suffix_str    );
  util::tolower( option_ilevel_str           );
  util::tolower( option_effective_ilevel_str );
  util::tolower( option_quality_str          );
  util::tolower( option_upgrade_level_str    );

  return true;
}

void item_t::encode_option( std::string prefix_str, std::string& option_str, std::string& encoded_str )
{
  std::string& o = options_str;
  std::string& c = comment_str;

  if ( ! option_str.empty() )
  {
    o += ",";
    o += prefix_str;
    o += encoded_str;
  }
  else if ( ! encoded_str.empty() )
  {
    if ( c.empty() )
    {
      c += "# ";
    }
    else
    {
      c += ",";
    }
    c += prefix_str;
    c += encoded_str;
  }
}

// item_t::encode_options ===================================================

void item_t::encode_options()
{
  // Re-build options_str for use in saved profiles

  std::string& o = options_str;

  o = encoded_name_str;

  if ( ! id_str.empty() && id_str[ 0 ] != 0 )   { o += ",id=";               o += id_str;                                                 }
  if ( ! encoded_random_suffix_str.empty() )    { o += ",suffix=";           o += encoded_random_suffix_str;                              }
  if ( heroic()                               ) { encode_option( "heroic=",  option_heroic_str,           encoded_heroic_str );           }
  if ( lfr()                                  ) { encode_option( "lfr=",     option_lfr_str,              encoded_lfr_str );              }
  if ( armor_type()                           ) { encode_option( "type=",    option_armor_type_str,       encoded_armor_type_str );       }
  if ( ! encoded_ilevel_str.empty()           ) { encode_option( "ilevel=",  option_ilevel_str,           encoded_ilevel_str );           }
  if ( ! encoded_effective_ilevel_str.empty() ) { encode_option( "eilevel=", option_effective_ilevel_str, encoded_effective_ilevel_str ); }
  if ( ! encoded_quality_str.empty()          ) { encode_option( "quality=", option_quality_str,          encoded_quality_str );          }
  if ( ! encoded_stats_str.empty()            ) { encode_option( "stats=",   option_stats_str,            encoded_stats_str );            }
  if ( ! encoded_equip_str.empty()            ) { encode_option( "equip=",   option_equip_str,            encoded_equip_str );            }
  if ( ! encoded_use_str.empty()              ) { encode_option( "use=",     option_use_str,              encoded_use_str );              }
  if ( ! encoded_weapon_str.empty()           ) { encode_option( "weapon=",  option_weapon_str,           encoded_weapon_str );           }
  if ( ! encoded_gems_str.empty()             ) { o += ",gems=";             o += encoded_gems_str;                                       }
  if ( ! encoded_enchant_str.empty()          ) { o += ",enchant=";          o += encoded_enchant_str;                                    }
  if ( ! encoded_addon_str.empty()            ) { o += ",addon=";            o += encoded_addon_str;                                      }
  if ( ! encoded_reforge_str.empty()          ) { o += ",reforge=";          o += encoded_reforge_str;                                    }
  if ( ! encoded_upgrade_level_str.empty()    ) { o += ",upgrade=";          o += encoded_upgrade_level_str;                              }
}

// item_t::init =============================================================

bool item_t::init()
{
  parse_options();

  encoded_name_str = armory_name_str;

  if ( ! option_name_str.empty() ) encoded_name_str = option_name_str;

  if ( ! option_id_str.empty() )
  {
    if ( ! item_t::download_item( *this, option_id_str, option_upgrade_level_str ) )
    {
      return false;
    }

    // Downloaded item, put in the item's base name.
    if ( ! armory_name_str.empty() )
      encoded_name_str = armory_name_str;
  }

  if ( encoded_name_str.empty() || encoded_name_str == "empty" || encoded_name_str == "none" )
  {
    if ( ! decode_armor_type() ) return false;
    encode_options();
    return true;
  }

  id_str                       = armory_id_str;

  encoded_stats_str            = armory_stats_str;
  encoded_reforge_str          = armory_reforge_str;
  encoded_gems_str             = armory_gems_str;
  encoded_enchant_str          = armory_enchant_str;
  encoded_addon_str            = armory_addon_str;
  encoded_weapon_str           = armory_weapon_str;
  encoded_heroic_str           = armory_heroic_str;
  encoded_lfr_str              = armory_lfr_str;
  encoded_armor_type_str       = armory_armor_type_str;
  encoded_ilevel_str           = armory_ilevel_str;
  encoded_effective_ilevel_str = armory_effective_ilevel_str;
  encoded_quality_str          = armory_quality_str;
  encoded_random_suffix_str    = armory_random_suffix_str;
  encoded_upgrade_level_str    = armory_upgrade_level_str;

  if ( ! option_id_str.empty() ) id_str = option_id_str;

  if ( ! option_heroic_str.empty()  ) encoded_heroic_str  = option_heroic_str;

  if ( ! decode_heroic()  ) return false;

  if ( ! option_lfr_str.empty() ) encoded_lfr_str = option_lfr_str;

  if ( ! decode_lfr() ) return false;

  if ( ! option_armor_type_str.empty() ) encoded_armor_type_str = option_armor_type_str;

  if ( ! decode_armor_type() ) return false;

  if ( ! option_ilevel_str.empty() ) encoded_ilevel_str = option_ilevel_str;

  if ( ! option_effective_ilevel_str.empty() ) encoded_effective_ilevel_str = option_effective_ilevel_str;

  if ( ! decode_ilevel() ) return false;

  if ( ! decode_effective_ilevel() ) return false;

  if ( ! option_quality_str.empty() ) encoded_quality_str = option_quality_str;

  if ( ! decode_quality() ) return false;

  unique_gear::get_equip_encoding( armory_equip_str, encoded_name_str, heroic(), lfr(), player -> dbc.ptr, id_str );
  unique_gear::get_use_encoding  ( armory_use_str,   encoded_name_str, heroic(), lfr(), player -> dbc.ptr, id_str );

  encoded_equip_str = armory_equip_str;
  encoded_use_str   = armory_use_str;

  if ( ! option_stats_str.empty()   ) encoded_stats_str   = option_stats_str;
  if ( ! option_reforge_str.empty() ) encoded_reforge_str = option_reforge_str;
  if ( ! option_gems_str.empty()    ) encoded_gems_str    = option_gems_str;
  if ( ! option_enchant_str.empty() )
  {
    if ( ( slot == SLOT_FINGER_1 || slot == SLOT_FINGER_2 ) && ! ( player -> profession[ PROF_ENCHANTING ] > 0 ) )
    {
      sim -> errorf( "Player %s's ring at slot %s has a ring enchant without the enchanting profession\n",
                     player -> name(), slot_name() );
    }
    else
    {
      encoded_enchant_str = option_enchant_str;
    }
  }
  if ( ! option_addon_str.empty()   ) encoded_addon_str   = option_addon_str;
  if ( ! option_weapon_str.empty()  ) encoded_weapon_str  = option_weapon_str;
  if ( ! option_random_suffix_str.empty() ) encoded_random_suffix_str = option_random_suffix_str;
  if ( ! option_upgrade_level_str.empty() ) encoded_upgrade_level_str = option_upgrade_level_str;


  if ( ! decode_stats()         ) return false;
  if ( ! decode_gems()          ) return false;
  if ( ! decode_enchant()       ) return false;
  decode_addon();
  if ( ! decode_weapon()        ) return false;
  if ( ! decode_random_suffix() ) return false;
  if ( ! decode_upgrade_level() ) return false;
  if ( ! decode_reforge()       ) return false;

  if ( ! option_equip_str.empty() ) encoded_equip_str = option_equip_str;
  if ( ! option_use_str.empty()   ) encoded_use_str   = option_use_str;

  if ( ! decode_special(   use, encoded_use_str   ) ) return false;
  if ( ! decode_special( equip, encoded_equip_str ) ) return false;

  encode_options();

  if ( ! option_name_str.empty() && ( option_name_str != encoded_name_str ) )
  {
    sim -> errorf( "Player %s at slot %s has inconsistency between name '%s' and '%s' for id '%s'\n",
                   player -> name(), slot_name(), option_name_str.c_str(), encoded_name_str.c_str(), option_id_str.c_str() );

    encoded_name_str = option_name_str;
  }

  return true;
}

// item_t::decode_heroic ====================================================

bool item_t::decode_heroic()
{
  is_heroic = ! ( encoded_heroic_str.empty() || ( encoded_heroic_str == "0" ) || ( encoded_heroic_str == "no" ) );

  return true;
}

// item_t::decode_lfr =======================================================

bool item_t::decode_lfr()
{
  is_lfr = ! ( encoded_lfr_str.empty() || ( encoded_lfr_str == "0" ) || ( encoded_lfr_str == "no" ) );

  return true;
}

// item_t::decode_armor_type ================================================

bool item_t::decode_armor_type()
{
  // Cloth classes don't actually check the gear. They just get their bonus even if naked.
  switch ( player -> type )
  {
  case MAGE:
  case PRIEST:
  case WARLOCK:
    is_matching_type = true;
    return true;
  default:
    break;
  }
  if ( encoded_name_str == "empty" ||
       encoded_name_str == "none"  ||
       encoded_name_str == "" )
  {
    is_matching_type = armor_type() == NULL;
  }
  else
  {
    is_matching_type = encoded_armor_type_str.empty() || ( armor_type() == NULL ) || ( armor_type() == encoded_armor_type_str );
  }

  return true;
}

// item_t::decode_ilevel ====================================================

bool item_t::decode_ilevel()
{
  if ( encoded_ilevel_str.empty() ) return true;

  long ilvl = strtol( encoded_ilevel_str.c_str(), 0, 10 );

  if ( ilvl < 1 || ilvl == std::numeric_limits<long>::max() ) return false;

  ilevel = ilvl;

  return true;
}

// item_t::decode_effective_ilevel ====================================================

bool item_t::decode_effective_ilevel()
{
  if ( encoded_effective_ilevel_str.empty() ) return true;

  long ilvl = strtol( encoded_effective_ilevel_str.c_str(), 0, 10 );

  if ( ilvl < 1 || ilvl == std::numeric_limits<long>::max() ) return false;

  effective_ilevel = ilvl;

  return true;
}


// item_t::decode_quality ===================================================

bool item_t::decode_quality()
{
  if ( ! encoded_quality_str.empty() )
    quality = util::parse_item_quality( encoded_quality_str );
  return true;
}

// item_t::decode_stats =====================================================

bool item_t::decode_stats()
{
  if ( encoded_stats_str == "none" ) return true;

  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoded_stats_str );

  for ( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];

    stat_e s = util::parse_stat_type( t.name );

    if ( s != STAT_NONE )
    {
      if ( s == STAT_ARMOR )
      {
        if ( slot != SLOT_HEAD      &&
             slot != SLOT_SHOULDERS &&
             slot != SLOT_CHEST     &&
             slot != SLOT_WAIST     &&
             slot != SLOT_LEGS      &&
             slot != SLOT_FEET      &&
             slot != SLOT_WRISTS    &&
             slot != SLOT_HANDS     &&
             slot != SLOT_BACK      &&
             slot != SLOT_OFF_HAND )
        {
          s = STAT_BONUS_ARMOR;
        }
      }
      base_stats.add_stat( s, t.value );
      stats.add_stat( s, t.value );
    }
    else
    {
      sim -> errorf( "Player %s has unknown 'stats=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
      return false;
    }
  }

  return true;
}

// item_t::decode_reforge ===================================================

bool item_t::decode_reforge()
{
  is_reforged = false;

  if ( encoded_reforge_str == "none" ) return true;

  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoded_reforge_str );

  if ( num_tokens <= 0 )
    return true;

  if ( num_tokens != 2 )
  {
    sim -> errorf( "Player %s has unknown 'reforge=' '%s' at slot %s\n", player -> name(), encoded_reforge_str.c_str(), slot_name() );
    return false;
  }

  stat_e s1 = util::parse_reforge_type( tokens[ 0 ].name );
  stat_e s2 = util::parse_reforge_type( tokens[ 1 ].name );
  if ( ( s1 == STAT_NONE ) || ( s2 == STAT_NONE ) )
  {
    sim -> errorf( "Player %s has unknown 'reforge=' '%s' at slot %s\n",
                   player -> name(), encoded_reforge_str.c_str(), slot_name() );
    return false;
  }
  if ( base_stats.get_stat( s1 ) <= 0.0 )
  {
    sim -> errorf( "Player %s with 'reforge=' '%s' at slot %s does not have source stat on item.\n",
                   player -> name(), encoded_reforge_str.c_str(), slot_name() );
    return false;
  }
  if ( base_stats.get_stat( s2 ) > 0.0 )
  {
    sim -> errorf( "Player %s with 'reforge=' '%s' at slot %s already has target stat on item.\n",
                   player -> name(), encoded_reforge_str.c_str(), slot_name() );
    return false;
  }

  reforged_from = s1;
  reforged_to   = s2;

  double amount = floor( base_stats.get_stat( reforged_from ) * 0.4 );
  stats.add_stat( reforged_from, -amount );
  stats.add_stat( reforged_to,    amount );

  is_reforged   = true;
  return true;
}

// item_t::decode_random_suffix =============================================

bool item_t::decode_random_suffix()
{
  int f = item_database::random_suffix_type( *this );

  if ( encoded_random_suffix_str.empty() ||
       encoded_random_suffix_str == "none" ||
       encoded_random_suffix_str == "0" )
    return true;

  // We need the ilevel/quality data, otherwise we cannot figure out
  // the random suffix point allocation.
  if ( ilevel == 0 || quality == 0 )
  {
    sim -> errorf( "Player %s with random suffix at slot %s requires both ilevel= and quality= information.\n", player -> name(), slot_name() );
    return true;
  }

  long rsid = abs( strtol( encoded_random_suffix_str.c_str(), 0, 10 ) );
  const random_prop_data_t& ilevel_data   = player -> dbc.random_property( ilevel );
  const random_suffix_data_t& suffix_data = player -> dbc.random_suffix( rsid );

  if ( ! suffix_data.id )
  {
    sim -> errorf( "Warning: Unknown random suffix identifier %ld at slot %s for item %s.\n",
                   rsid, slot_name(), name() );
    return true;
  }

  if ( sim -> debug )
  {
    sim -> output( "random_suffix: item=%s suffix_id=%ld ilevel=%d quality=%d random_point_pool=%d",
                   name(), rsid, ilevel, quality, f );
  }

  std::vector<std::string>          stat_list;
  for ( int i = 0; i < 5; i++ )
  {
    unsigned                         enchant_id;
    if ( ! ( enchant_id = suffix_data.enchant_id[ i ] ) )
      continue;

    const item_enchantment_data_t& enchant_data = player -> dbc.item_enchantment( enchant_id );

    if ( ! enchant_data.id )
      continue;

    // Calculate amount of points
    double                          stat_amount;
    if ( quality == 4 ) // Epic
      stat_amount = ilevel_data.p_epic[ f ] * suffix_data.enchant_alloc[ i ] / 10000.0;
    else if ( quality == 3 ) // Rare
      stat_amount = ilevel_data.p_rare[ f ] * suffix_data.enchant_alloc[ i ] / 10000.0;
    else if ( quality == 2 ) // Uncommon
      stat_amount = ilevel_data.p_uncommon[ f ] * suffix_data.enchant_alloc[ i ] / 10000.0;
    else // Impossible choices, so bogus data, skip
      continue;

    // Loop through enchantment stats, adding valid ones to the stats of the item.
    // Typically (and for cata random suffixes), there seems to be only one stat per enchantment
    for ( int j = 0; j < 3; j++ )
    {
      if ( enchant_data.ench_type[ j ] != ITEM_ENCHANTMENT_STAT ) continue;

      stat_e stat = util::translate_item_mod( static_cast<item_mod_type>( enchant_data.ench_prop[ j ] ) );

      if ( stat == STAT_NONE ) continue;

      // Make absolutely sure we do not add stats twice
      if ( base_stats.get_stat( stat ) == 0 )
      {
        base_stats.add_stat( stat, static_cast< int >( stat_amount ) );
        stats.add_stat( stat, static_cast< int >( stat_amount ) );

        std::string stat_str = util::stat_type_abbrev( stat );
        stat_str = util::tolower( stat_str );
        char statbuf[32];
        snprintf( statbuf, sizeof( statbuf ), "%d%s", static_cast< int >( stat_amount ), stat_str.c_str() );
        stat_list.push_back( statbuf );

        if ( sim -> debug )
          sim -> output( "random_suffix: stat=%d (%s) stat_amount=%f", stat, stat_str.c_str(), stat_amount );
      }
      else
      {
        if ( sim -> debug )
        {
          sim -> output( "random_suffix: Player %s item %s attempted to add base stat %d %d (%d) twice, due to random suffix.",
                         player -> name(), name(), j, stat, enchant_data.ench_type[ j ] );
        }
      }
    }
  }

  std::string name_str = suffix_data.suffix;
  util::tokenize( name_str );

  if ( encoded_name_str.find( name_str ) == std::string::npos )
  {
    encoded_name_str += '_' + name_str;
  }


  // Append stats to the existing encoded stats string, as
  // a simple suffix will not tell the user anything about
  // the item
  if ( ! encoded_stats_str.empty() && !stat_list.empty() )
    encoded_stats_str += "_";

  for ( unsigned i = 0; i < stat_list.size(); i++ )
  {
    encoded_stats_str += stat_list[ i ];

    if ( i < stat_list.size() - 1 )
      encoded_stats_str += "_";
  }

  return true;
}

// item_t::decode_random_suffix =============================================

bool item_t::decode_upgrade_level()
{
  if ( encoded_upgrade_level_str.empty() ||
       encoded_upgrade_level_str == "none" ||
       encoded_upgrade_level_str == "0" )
    return true;

  // We need the ilevel/quality data, otherwise we cannot figure out
  // the upgraded stat allocation.
  if ( ilevel == 0 || quality == 0 )
  {
    sim -> errorf( "Player %s with upgrade at slot %s requires both ilevel= and quality= information.\n", player -> name(), slot_name() );
    return true;
  }

  if ( encoded_upgrade_level_str == "1" )
  {
    upgrade_level = 1;
    return true;
  }
  else if ( encoded_upgrade_level_str == "2" )
  {
    upgrade_level = 2;
    return true;
  }
  else
  {
    sim -> errorf( "Player %s has unknown 'upgrade=' token '%s' at slot %s\n", player -> name(), encoded_upgrade_level_str.c_str(), slot_name() );
    return false;
  }
}

// item_t::decode_gems ======================================================

bool item_t::decode_gems()
{
  if ( encoded_gems_str == "none" ) return true;

  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoded_gems_str );

  std::string meta_prefix, meta_suffix;

  for ( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];
    stat_e s;

    if ( ( s = util::parse_stat_type( t.name ) ) != STAT_NONE )
    {
      stats.add_stat( s, t.value );
    }
    else if ( is_meta_prefix( t.name ) )
    {
      meta_prefix = t.name;
    }
    else if ( is_meta_suffix( t.name ) )
    {
      meta_suffix = t.name;
    }
    else
    {
      sim -> errorf( "Player %s has unknown 'gems=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
      //return false;
    }
  }

  meta_gem_e meta_gem = parse_meta_gem( meta_prefix, meta_suffix );

  if ( meta_gem != META_GEM_NONE )
  {
    player -> meta_gem = meta_gem;
  }

  return true;
}

// item_t::decode_enchant ===================================================

bool item_t::decode_enchant()
{
  if ( encoded_enchant_str == "none" ) return true;

  if ( encoded_enchant_str == "berserking"        ||
       encoded_enchant_str == "executioner"       ||
       encoded_enchant_str == "mongoose"          ||
       encoded_enchant_str == "avalanche"         ||
       encoded_enchant_str == "elemental_slayer"  ||
       encoded_enchant_str == "elemental_force"   ||
       encoded_enchant_str == "hurricane"         ||
       encoded_enchant_str == "landslide"         ||
       encoded_enchant_str == "power_torrent"     ||
       encoded_enchant_str == "jade_spirit"       ||
       encoded_enchant_str == "windwalk"          ||
       encoded_enchant_str == "spellsurge"        ||
       encoded_enchant_str == "rivers_song"       ||
       encoded_enchant_str == "mirror_scope"      ||
       encoded_enchant_str == "gnomish_xray"      ||
       encoded_enchant_str == "windsong"          ||
       encoded_enchant_str == "dancing_steel"     ||
       encoded_enchant_str == "colossus"          ||
       encoded_enchant_str == "lord_blastingtons_scope_of_doom" )
  {
    unique_enchant = true;
    return true;
  }

  std::string use_str;
  if ( unique_gear::get_use_encoding( use_str, encoded_enchant_str, heroic(), lfr(), player -> dbc.ptr ) )
  {
    unique_enchant = true;
    use.name_str = encoded_enchant_str;
    return decode_special( use, use_str );
  }

  std::string equip_str;
  if ( unique_gear::get_equip_encoding( equip_str, encoded_enchant_str, heroic(), lfr(), player -> dbc.ptr ) )
  {
    unique_enchant = true;
    enchant.name_str = encoded_enchant_str;
    return decode_special( enchant, equip_str );
  }

  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoded_enchant_str );

  for ( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];
    stat_e s;

    if ( ( s = util::parse_stat_type( t.name ) ) != STAT_NONE )
    {
      stats.add_stat( s, t.value );
    }
    else
    {
      sim -> errorf( "Player %s has unknown 'enchant=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
      continue;
    }
  }

  return true;
}

// item_t::decode_addon =====================================================

bool item_t::decode_addon()
{
  if ( encoded_addon_str == "none" ) return true;

  if ( encoded_addon_str == "synapse_springs"         ||
       encoded_addon_str == "synapse_springs_2"       ||
       encoded_addon_str == "synapse_springs_mark_ii" ||
       encoded_addon_str == "phase_fingers"           ||
       encoded_addon_str == "nitro_boosts"            ||
       encoded_addon_str == "flexweave_underlay"      ||
       encoded_addon_str == "grounded_plasma_shield"  ||
       encoded_addon_str == "cardboard_assassin"      ||
       encoded_addon_str == "invisibility_field"      ||
       encoded_addon_str == "mind_amplification_dish" ||
       encoded_addon_str == "personal_electromagnetic_pulse_generator" ||
       encoded_addon_str == "frag_belt"               ||
       encoded_addon_str == "spinal_healing_injector" ||
       encoded_addon_str == "z50_mana_gulper"         ||
       encoded_addon_str == "reticulated_armor_webbing" ||
       encoded_addon_str == "goblin_glider"           ||
       encoded_addon_str == "watergliding_jets" )
  {
    unique_addon = true;
    return true;
  }

  std::string use_str;
  if ( unique_gear::get_use_encoding( use_str, encoded_addon_str, heroic(), lfr(), player -> dbc.ptr ) )
  {
    unique_addon = true;
    use.name_str = encoded_addon_str;
    return decode_special( use, use_str );
  }

  std::string equip_str;
  if ( unique_gear::get_equip_encoding( equip_str, encoded_addon_str, heroic(), lfr(), player -> dbc.ptr ) )
  {
    unique_addon = true;
    addon.name_str = encoded_addon_str;
    return decode_special( addon, equip_str );
  }

  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoded_addon_str );

  for ( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];
    stat_e s;

    if ( ( s = util::parse_stat_type( t.name ) ) != STAT_NONE )
    {
      stats.add_stat( s, t.value );
    }
    else
    {
      sim -> errorf( "Player %s has unknown 'addon=' token '%s' at slot %s\n", player -> name(), encoded_addon_str.c_str(), slot_name() );
      return false;
    }
  }

  return true;
}

// item_t::decode_special ===================================================

bool item_t::decode_special( special_effect_t& effect,
                             const std::string& encoding )
{
  if ( encoding == "custom" || encoding == "none" ) return true;

  std::vector<token_t> tokens;

  int num_tokens = parse_tokens( tokens, encoding );

  for ( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];
    stat_e s;
    school_e sc;

    if ( ( s = util::parse_stat_type( t.name ) ) != STAT_NONE )
    {
      effect.stat = s;
      effect.stat_amount = t.value;

      // Support scaling procs in a hacky way.
      if ( ! id_str.empty() && upgrade_level > 0 )
      {
        int item_id = strtol( id_str.c_str(), 0, 10 );
        
        const item_data_t* item_data = player -> dbc.item( item_id );

        if ( item_data )
        {
          int orig_ilevel = ilevel - upgrade_ilevel( *item_data, upgrade_level );
          if ( orig_ilevel == ilevel )
            return true;

          const random_prop_data_t& orig_data = player -> dbc.random_property( orig_ilevel );
          const random_prop_data_t& new_data = player -> dbc.random_property( ilevel );
          int old_budget = 0;
          int new_budget = 0;

          if ( quality == 4 )
          {
            old_budget = ( int ) orig_data.p_epic[ 0 ];
            new_budget = ( int ) new_data.p_epic[ 0 ];
          }
          else if ( quality == 3 )
          {
            old_budget = ( int ) orig_data.p_rare[ 0 ];
            new_budget = ( int ) new_data.p_rare[ 0 ];
          }
          else
          {
            old_budget = ( int ) orig_data.p_uncommon[ 0 ];
            new_budget = ( int ) new_data.p_uncommon[ 0 ];
          }

          bool found_from_data = false;

          if ( item_data -> id_spell[ 0 ] != 0 )
          {
            const spell_data_t& proc_spell = *player -> find_spell( item_data -> id_spell[ 0 ] );
            if ( proc_spell.ok() )
            {
              const spell_data_t& buff_spell = *proc_spell.effectN( 1 ).trigger();
              if ( buff_spell.ok() )
              {
                for ( size_t i = 1; i <= buff_spell.effect_count(); i++ )
                {
                  if ( buff_spell.effectN( i ).type() == E_APPLY_AURA &&
                       ( buff_spell.effectN( i ).subtype() == A_MOD_STAT || buff_spell.effectN( i ).subtype() == A_MOD_RATING ) )
                  {
                    effect.stat_amount = util::round( new_budget * buff_spell.effectN( i ).m_average() );
                    found_from_data = true;
                    break;
                  }
                }
              }
            }
          }

          // t.value / old_budget gives us the average scaling coefficient for the proc spell
          if ( ! found_from_data )
            effect.stat_amount = util::round( new_budget * t.value / old_budget );

          if ( sim -> debug )
          {
            sim -> output( "decode_special: %s (%d) ilevel=%d quality=%d orig_ilevel=%d old_budget=%d new_budget=%d old_value=%.0f avg_coeff=%f new_value=%.0f",
                           name(), item_id, ilevel, quality, orig_ilevel, old_budget, new_budget, t.value, t.value / old_budget, effect.stat_amount );
          }
        }
      }

    }
    else if ( ( sc = util::parse_school_type( t.name ) ) != SCHOOL_NONE )
    {
      effect.school = sc;
      effect.discharge_amount = t.value;

      std::vector<std::string> splits;
      if ( 2 == util::string_split( splits, t.value_str, "+" ) )
      {
        effect.discharge_amount  = atof( splits[ 0 ].c_str() );
        effect.discharge_scaling = atof( splits[ 1 ].c_str() ) / 100.0;
      }
    }
    else if ( t.name == "stacks" || t.name == "stack" )
    {
      effect.max_stacks = ( int ) t.value;
    }
    else if ( t.name == "%" )
    {
      effect.proc_chance = t.value / 100.0;
    }
    else if ( t.name == "ppm" )
    {
      effect.proc_chance = -( t.value );
    }
    else if ( t.name == "duration" || t.name == "dur" )
    {
      effect.duration = timespan_t::from_seconds( t.value );
    }
    else if ( t.name == "cooldown" || t.name == "cd" )
    {
      effect.cooldown = timespan_t::from_seconds( t.value );
    }
    else if ( t.name == "tick" )
    {
      effect.tick = timespan_t::from_seconds( t.value );
    }
    else if ( t.full == "reverse" )
    {
      effect.reverse = true;
    }
    else if ( t.full == "chance" )
    {
      effect.chance_to_discharge = true;
    }
    else if ( t.name == "costrd" )
    {
      effect.cost_reduction = true;
      effect.no_refresh = true;
    }
    else if ( t.name == "nocrit" )
    {
      effect.override_result_es_mask |= RESULT_CRIT_MASK;
      effect.result_es_mask &= ~RESULT_CRIT_MASK;
    }
    else if ( t.name == "maycrit" )
    {
      effect.override_result_es_mask |= RESULT_CRIT_MASK;
      effect.result_es_mask |= RESULT_CRIT_MASK;
    }
    else if ( t.name == "nomiss" )
    {
      effect.override_result_es_mask |= RESULT_MISS_MASK;
      effect.result_es_mask &= ~RESULT_MISS_MASK;
    }
    else if ( t.name == "maymiss" )
    {
      effect.override_result_es_mask |= RESULT_MISS_MASK;
      effect.result_es_mask |= RESULT_MISS_MASK;
    }
    else if ( t.name == "nododge" )
    {
      effect.override_result_es_mask |= RESULT_DODGE_MASK;
      effect.result_es_mask &= ~RESULT_DODGE_MASK;
    }
    else if ( t.name == "maydodge" )
    {
      effect.override_result_es_mask |= RESULT_DODGE_MASK;
      effect.result_es_mask |= RESULT_DODGE_MASK;
    }
    else if ( t.name == "noparry" )
    {
      effect.override_result_es_mask |= RESULT_PARRY_MASK;
      effect.result_es_mask &= ~RESULT_PARRY_MASK;
    }
    else if ( t.name == "mayparry" )
    {
      effect.override_result_es_mask |= RESULT_PARRY_MASK;
      effect.result_es_mask |= RESULT_PARRY_MASK;
    }
    else if ( t.name == "noblock" )
    {
      effect.override_result_es_mask |= RESULT_BLOCK_MASK;
      effect.result_es_mask &= ~RESULT_BLOCK_MASK;
    }
    else if ( t.name == "mayblock" )
    {
      effect.override_result_es_mask |= RESULT_BLOCK_MASK;
      effect.result_es_mask |= RESULT_BLOCK_MASK;
    }
    else if ( t.name == "norefresh" )
    {
      effect.no_refresh = true;
    }
    else if ( t.full == "aoe" )
    {
      effect.aoe = -1;
    }
    else if ( t.name == "aoe" )
    {
      effect.aoe = ( int ) t.value;
      if ( effect.aoe < -1 )
        effect.aoe = -1;
    }
    else if ( t.full == "ondamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "onheal" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_HEAL;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "ontickdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_TICK_DAMAGE;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "onspelltickdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_SPELL_TICK_DAMAGE;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "ondirectdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DIRECT_DAMAGE;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "ondirectcrit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DIRECT_CRIT;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "onspelldirectdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_SPELL_DIRECT_DAMAGE;
      effect.trigger_mask = SCHOOL_ALL_MASK;
    }
    else if ( t.full == "ondirectharmfulspellhit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DIRECT_HARMFUL_SPELL;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "onspelldamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = SCHOOL_SPELL_MASK;
    }
    else if ( t.full == "onattackdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = SCHOOL_ATTACK_MASK;
    }
    else if ( t.full == "onattacktickdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_TICK_DAMAGE;
      effect.trigger_mask = SCHOOL_ATTACK_MASK;
    }
    else if ( t.full == "onattackdirectdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DIRECT_DAMAGE;
      effect.trigger_mask = SCHOOL_ATTACK_MASK;
    }
    else if ( t.full == "onarcanedamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_ARCANE );
    }
    else if ( t.full == "onbleeddamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_TICK_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_PHYSICAL );
    }
    else if ( t.full == "onchaosdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_CHAOS );
    }
    else if ( t.full == "onfiredamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_FIRE );
    }
    else if ( t.full == "onfrostdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_FROST );
    }
    else if ( t.full == "onfrostfiredamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_FROSTFIRE );
    }
    else if ( t.full == "onholydamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_HOLY );
    }
    else if ( t.full == "onnaturedamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_NATURE );
    }
    else if ( t.full == "onphysicaldamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_PHYSICAL );
    }
    else if ( t.full == "onshadowdamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_SHADOW );
    }
    else if ( t.full == "ondraindamage" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE;
      effect.trigger_mask = ( int64_t( 1 ) << SCHOOL_DRAIN );
    }
    else if ( t.full == "ontick" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_TICK;
      effect.trigger_mask = RESULT_ALL_MASK;
    }
    else if ( t.full == "ontickhit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_TICK;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "ontickcrit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_TICK;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onspellcast" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_SPELL;
      effect.trigger_mask = RESULT_NONE_MASK;
    }
    else if ( t.full == "onspellhit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_SPELL;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "onspellcrit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_SPELL_AND_TICK;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onspelltickcrit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_TICK;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onspelldirectcrit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_SPELL;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onspellmiss" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_SPELL;
      effect.trigger_mask = RESULT_MISS_MASK;
    }
    else if ( t.full == "onharmfulspellcast" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_HARMFUL_SPELL;
      effect.trigger_mask = RESULT_NONE_MASK;
    }
    else if ( t.full == "onharmfulspellhit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_HARMFUL_SPELL;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "onharmfulspellcrit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_HARMFUL_SPELL;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onharmfulspellmiss" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_HARMFUL_SPELL;
      effect.trigger_mask = RESULT_MISS_MASK;
    }
    else if ( t.full == "onhealcast" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_HEAL_SPELL;
      effect.trigger_mask = RESULT_NONE_MASK;
    }
    else if ( t.full == "onhealhit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_HEAL_SPELL;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "onhealdirectcrit" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_HEAL_SPELL;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onhealmiss" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_HEAL_SPELL;
      effect.trigger_mask = RESULT_MISS_MASK;
    }
    else if ( t.full == "onattack" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_ATTACK;
      effect.trigger_mask = RESULT_ALL_MASK;
    }
    else if ( t.full == "onattackhit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_ATTACK;
      effect.trigger_mask = RESULT_HIT_MASK;
    }
    else if ( t.full == "onattackcrit" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_ATTACK;
      effect.trigger_mask = RESULT_CRIT_MASK;
    }
    else if ( t.full == "onattackmiss" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_ATTACK;
      effect.trigger_mask = RESULT_MISS_MASK;
    }
    else if ( t.full == "onspelldamageheal" )
    {
      effect.trigger_str  = t.full;
      effect.trigger_type = PROC_DAMAGE_HEAL;
      effect.trigger_mask = SCHOOL_SPELL_MASK;
    }
    else if ( t.full == "ondamagehealspellcast" )
    {
      effect.trigger_str = t.full;
      effect.trigger_type = PROC_DAMAGE_HEAL_SPELL;
      effect.trigger_mask = RESULT_NONE_MASK;
    }
    else
    {
      sim -> errorf( "Player %s has unknown 'use/equip=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
      return false;
    }
  }

  return true;
}

// item_t::decode_weapon ====================================================

bool item_t::decode_weapon()
{
  weapon_t* w = weapon();
  if ( ! w ) return true;

  std::vector<token_t> tokens;
  int num_tokens = parse_tokens( tokens, encoded_weapon_str );

  bool dps_set=false, dmg_set=false, min_set=false, max_set=false;

  for ( int i=0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];
    weapon_e type;
    school_e school;

    if ( ( type = util::parse_weapon_type( t.name ) ) != WEAPON_NONE )
    {
      w -> type = type;
    }
    else if ( ( school = util::parse_school_type( t.name ) ) != SCHOOL_NONE )
    {
      w -> school = school;
    }
    else if ( t.name == "dps" )
    {
      if ( ! dmg_set )
      {
        w -> dps = t.value;
        dps_set = true;
      }
    }
    else if ( t.name == "damage" || t.name == "dmg" )
    {
      w -> damage  = t.value;
      w -> min_dmg = t.value;
      w -> max_dmg = t.value;
      dmg_set = true;
    }
    else if ( t.name == "speed" || t.name == "spd" )
    {
      w -> swing_time = timespan_t::from_seconds( t.value );
    }
    else if ( t.name == "min" )
    {
      w -> min_dmg = t.value;
      min_set = true;

      if ( max_set )
      {
        dmg_set = true;
        dps_set = false;
        w -> damage = ( w -> min_dmg + w -> max_dmg ) / 2;
      }
      else
      {
        w -> max_dmg = w -> min_dmg;
      }
    }
    else if ( t.name == "max" )
    {
      w -> max_dmg = t.value;
      max_set = true;

      if ( min_set )
      {
        dmg_set = true;
        dps_set = false;
        w -> damage = ( w -> min_dmg + w -> max_dmg ) / 2;
      }
      else
      {
        w -> min_dmg = w -> max_dmg;
      }
    }
    else
    {
      sim -> errorf( "Player %s has unknown 'weapon=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
      return false;
    }
  }

  if ( dps_set ) w -> damage = w -> dps    * w -> swing_time.total_seconds();
  if ( dmg_set ) w -> dps    = w -> damage / w -> swing_time.total_seconds();

  if ( ! max_set || ! min_set )
  {
    w -> max_dmg = w -> damage;
    w -> min_dmg = w -> damage;
  }

  return true;
}

// item_t::download_slot ====================================================

bool item_t::download_slot( item_t& item,
                            const std::string& item_id,
                            const std::string& enchant_id,
                            const std::string& addon_id,
                            const std::string& reforge_id,
                            const std::string& rsuffix_id,
                            const std::string& upgrade_level,
                            const std::string gem_ids[ 3 ] )
{
  const cache::behavior_e cb = cache::items();
  bool success = false;

  // FIXME: For now we want to try the local item db first if we are upgrading - none of the other sources support it yet.
  if ( ! upgrade_level.empty() && upgrade_level != "0" )
  {
    success = item_database::download_slot( item, item_id, enchant_id, addon_id, reforge_id, rsuffix_id, upgrade_level, gem_ids );
    if ( success ) return true;
  }

  if ( cb != cache::CURRENT )
  {
    for ( unsigned i = 0; ! success && i < item.sim -> item_db_sources.size(); i++ )
    {
      const std::string& src = item.sim -> item_db_sources[ i ];
      if ( src == "local" )
        success = item_database::download_slot( item, item_id, enchant_id, addon_id, reforge_id,
                                                rsuffix_id, upgrade_level, gem_ids );
      else if ( src == "wowhead" )
        success = wowhead::download_slot( item, item_id, enchant_id, addon_id, reforge_id,
                                          rsuffix_id, gem_ids, wowhead::LIVE, cache::ONLY );
      else if ( src == "ptrhead" )
        success = wowhead::download_slot( item, item_id, enchant_id, addon_id, reforge_id,
                                          rsuffix_id, gem_ids, wowhead::PTR, cache::ONLY );
      else if ( src == "bcpapi" )
        success = bcp_api::download_slot( item, item_id, enchant_id, addon_id, reforge_id,
                                          rsuffix_id, gem_ids, cache::ONLY );
    }
  }

  if ( cb != cache::ONLY )
  {
    // Download in earnest from a data source
    for ( unsigned i = 0; ! success && i < item.sim -> item_db_sources.size(); i++ )
    {
      const std::string& src = item.sim -> item_db_sources[ i ];
      if ( src == "wowhead" )
        success = wowhead::download_slot( item, item_id, enchant_id, addon_id, reforge_id,
                                          rsuffix_id, gem_ids, wowhead::LIVE, cb );
      else if ( src == "ptrhead" )
        success = wowhead::download_slot( item, item_id, enchant_id, addon_id, reforge_id,
                                          rsuffix_id, gem_ids, wowhead::PTR, cb );
      else if ( src == "bcpapi" )
        success = bcp_api::download_slot( item, item_id, enchant_id, addon_id, reforge_id,
                                          rsuffix_id, gem_ids, cb );
    }
  }

  return success;
}

// item_t::download_item ====================================================

bool item_t::download_item( item_t& item, const std::string& item_id, const std::string& upgrade_level )
{
  std::vector<std::string> source_list;
  if ( ! item_database::initialize_item_sources( item, source_list ) )
  {
    item.sim -> errorf( "Your item-specific data source string \"%s\" contained no valid sources to download item id %s.\n",
                        item.option_data_source_str.c_str(), item_id.c_str() );
    return false;
  }

  bool success = false;

  // FIXME: For now we want to try the local item db first if we are upgrading - none of the other sources support it yet.
  if ( ! upgrade_level.empty() && upgrade_level != "0" )
  {
    success = item_database::download_item( item, item_id, upgrade_level );
    if ( success ) return true;
  }

  if ( cache::items() != cache::CURRENT )
  {
    for ( unsigned i = 0; ! success && i < source_list.size(); i++ )
    {
      if ( source_list[ i ] == "local" )
        success = item_database::download_item( item, item_id, upgrade_level );
      else if ( source_list[ i ] == "wowhead" )
        success = wowhead::download_item( item, item_id, wowhead::LIVE, cache::ONLY );
      else if ( source_list[ i ] == "ptrhead" )
        success = wowhead::download_item( item, item_id, wowhead::PTR, cache::ONLY );
      else if ( source_list[ i ] == "bcpapi" )
        success = bcp_api::download_item( item, item_id, cache::ONLY );
    }
  }

  if ( cache::items() != cache::ONLY )
  {
    // Download in earnest from a data source
    for ( unsigned i = 0; ! success && i < source_list.size(); i++ )
    {
      if ( source_list[ i ] == "wowhead" )
        success = wowhead::download_item( item, item_id, wowhead::LIVE );
      else if ( source_list[ i ] == "ptrhead" )
        success = wowhead::download_item( item, item_id, wowhead::PTR );
      else if ( source_list[ i ] == "bcpapi" )
        success = bcp_api::download_item( item, item_id );
    }
  }

  return success;
}

// item_t::download_glyph ===================================================

bool item_t::download_glyph( player_t* player, std::string& glyph_name, const std::string& glyph_id )
{
  bool success = false;

  if ( cache::items() != cache::CURRENT )
  {
    for ( unsigned i = 0; ! success && i < player -> sim -> item_db_sources.size(); i++ )
    {
      const std::string& src = player -> sim -> item_db_sources[ i ];
      if ( src == "local" )
        success = item_database::download_glyph( player, glyph_name, glyph_id );
      else if ( src == "wowhead" )
        success = wowhead::download_glyph( player, glyph_name, glyph_id, wowhead::LIVE, cache::ONLY );
      else if ( src == "ptrhead" )
        success = wowhead::download_glyph( player, glyph_name, glyph_id, wowhead::PTR, cache::ONLY );
      else if ( src == "bcpapi" )
        success = bcp_api::download_glyph( player, glyph_name, glyph_id, cache::ONLY );
    }
  }

  if ( cache::items() != cache::ONLY )
  {
    // Download in earnest from a data source
    for ( unsigned i = 0; ! success && i < player -> sim -> item_db_sources.size(); i++ )
    {
      const std::string& src = player -> sim -> item_db_sources[ i ];
      if ( src == "wowhead" )
        success = wowhead::download_glyph( player, glyph_name, glyph_id, wowhead::LIVE );
      else if ( src == "ptrhead" )
        success = wowhead::download_glyph( player, glyph_name, glyph_id, wowhead::PTR );
      else if ( src == "bcpapi" )
        success = bcp_api::download_glyph( player, glyph_name, glyph_id );
    }
  }

  util::glyph_name( glyph_name );

  return success;
}

// item_t::parse_gem ========================================================

unsigned item_t::parse_gem( item_t&            item,
                            const std::string& gem_id )
{
  if ( gem_id.empty() || gem_id == "0" )
    return GEM_NONE;

  std::vector<std::string> source_list;
  if ( ! item_database::initialize_item_sources( item, source_list ) )
  {
    item.sim -> errorf( "Your item-specific data source string \"%s\" contained no valid sources to download gem id %s.\n",
                        item.option_data_source_str.c_str(), gem_id.c_str() );
    return GEM_NONE;
  }

  unsigned type = GEM_NONE;

  if ( cache::items() != cache::CURRENT )
  {
    for ( unsigned i = 0; type == GEM_NONE && i < source_list.size(); i++ )
    {
      if ( source_list[ i ] == "local" )
        type = item_database::parse_gem( item, gem_id );
      else if ( source_list[ i ] == "wowhead" )
        type = wowhead::parse_gem( item, gem_id, wowhead::LIVE, cache::ONLY );
      else if ( source_list[ i ] == "ptrhead" )
        type = wowhead::parse_gem( item, gem_id, wowhead::PTR, cache::ONLY );
      else if ( source_list[ i ] == "bcpapi" )
        type = bcp_api::parse_gem( item, gem_id, cache::ONLY );
    }
  }

  if ( cache::items() != cache::ONLY )
  {
    // Nothing found from a cache, nor local item db. Let's fetch, again honoring our source list
    for ( unsigned i = 0; type == GEM_NONE && i < source_list.size(); i++ )
    {
      if ( source_list[ i ] == "wowhead" )
        type = wowhead::parse_gem( item, gem_id, wowhead::LIVE );
      else if ( source_list[ i ] == "ptrhead" )
        type = wowhead::parse_gem( item, gem_id, wowhead::PTR );
      else if ( source_list[ i ] == "bcpapi" )
        type = bcp_api::parse_gem( item, gem_id );
    }
  }

  return type;
}

unsigned item_t::upgrade_ilevel( const item_data_t& item_data, unsigned level ) {
  if ( level == 0 ) return 0;
  if ( item_data.quality == 3 ) return 8;
  if ( item_data.quality == 4 ) return level == 1 ? 4 : 8;
  return 0;
}

namespace new_item_stuff {


base_item_t::base_item_t() :
  m_item_data( item_data_t() ),
  m_enchant_data( item_enchantment_data_t() ),
  m_addon_data( item_enchantment_data_t() ),
  m_random_prop_data( random_prop_data_t() ),
  m_random_suffix_data( random_suffix_data_t() ),
  m_gems( std::array<item_data_t,3>() )
{ }

namespace base_item { // namespace init_item

bool init_item_data( const player_t& p, item_data_t& out, int item_id )
{
  if (  item_id )
  {
    const item_data_t* item_data = p.dbc.item( item_id );
    if ( item_data )
    {
      out = *item_data;
      return true;
    }
  }

  return false;
}

bool init_enchant_data( const player_t& p, item_enchantment_data_t& out, int enchant_id )
{
  if ( enchant_id )
  {
    out = p.dbc.item_enchantment( enchant_id );

    if ( out.id )
      return true;
  }
  return false;
}

bool init_random_suffix_data( const player_t& p, random_suffix_data_t& out, int rsuffix_id )
{
  if ( rsuffix_id )
  {
    out = p.dbc.random_suffix( rsuffix_id );

    if ( out.id )
      return true;
  }
  return false;
}

bool init_gem_data( const player_t& p, item_data_t& out, int gem_id )
{
  if ( gem_id )
  {
    const item_data_t* gem = p.dbc.item( gem_id );
    if ( gem )
    {
      out = *gem;
      return true;
    }
  }
  return false;
}

/* Obtain Item base stats contained in its item_data
 *
 */
bool parse_item_stats( const item_t& item, const player_t& /* p */, std::array<int,STAT_MAX>& out )
{
  const item_data_t& item_data = item.get_item().get_item_data();

  for ( unsigned i = 0; i < sizeof_array( item_data.stat_type_e ); ++i )
  {
    // Translate stat type to stat_e enum
    stat_e stat = util::translate_item_mod( static_cast<item_mod_type>( item_data.stat_type_e[ i ] ) );

    if ( stat != STAT_NONE )
    {
      out[ stat ] += item_data.stat_val[ i ];
    }
  }
  return true;
}

/* Obtain Item enchant/addon stats
 *
 */
bool parse_enchantement_stats( const player_t& /*p*/, const item_enchantment_data_t& enchant_data, std::array<int,STAT_MAX>& out )
{
  if ( ! enchant_data.id )
  {
    return false;
  }

  for ( unsigned i = 0; i < sizeof_array( enchant_data.ench_type ); ++i )
  {
    if ( enchant_data.ench_type[ i ] == ITEM_ENCHANTMENT_NONE )
      continue;

    // Only parse stat enchantments here
    if ( enchant_data.ench_type[ i ] == ITEM_ENCHANTMENT_STAT )
    {
      stat_e stat = util::translate_item_mod( static_cast<item_mod_type>( enchant_data.ench_prop[ i ] ) );
      if ( stat != STAT_NONE )
      {
        out[ stat ] += enchant_data.ench_amount[ i ];
      }
    }
  }
  return true;
}

/* Obtain random suffix data
 *
 */
bool parse_random_suffix_stats( const item_t& item, const player_t& p, std::array<int,STAT_MAX>& out )
{
  const item_data_t& item_data = item.get_item().get_item_data();

  int f = item_database::random_suffix_type( &item_data );

  const random_suffix_data_t& suffix_data = item.get_item().get_random_suffix_data();
  if ( ! suffix_data.id )
    return false; // Go quietly here, as noone might have specified a random suffix

  try
  {
    // We need the ilevel/quality data, otherwise we cannot figure out
    // the random suffix point allocation.
    if ( item_data.level == 0 || item_data.quality == 0 )
    {
      throw( "ilvl or quality is zero." );
    }

    const random_prop_data_t& ilevel_data   = item.get_item().get_random_prop_data();

    if ( p.sim -> debug )
    {
      p.sim -> output( "random_suffix: item=%s suffix_id=%d ilevel=%d quality=%d random_point_pool=%d",
                       item.name(), suffix_data.id, item_data.level, item_data.quality, f );
    }

    std::vector<stat_e> stats_added; // To make sure we do not add stats twice
    for ( unsigned i = 0; i < sizeof_array( suffix_data.enchant_id ); ++i )
    {
      unsigned enchant_id;
      if ( ! ( enchant_id = suffix_data.enchant_id[ i ] ) )
        continue;

      const item_enchantment_data_t& enchant_data = p.dbc.item_enchantment( enchant_id );

      if ( ! enchant_data.id )
        continue;

      // Calculate amount of points
      double stat_amount;
      if ( item_data.quality == 4 ) // Epic
        stat_amount = ilevel_data.p_epic[ f ] * suffix_data.enchant_alloc[ i ] / 10000.0;
      else if ( item_data.quality == 3 ) // Rare
        stat_amount = ilevel_data.p_rare[ f ] * suffix_data.enchant_alloc[ i ] / 10000.0;
      else if ( item_data.quality == 2 ) // Uncommon
        stat_amount = ilevel_data.p_uncommon[ f ] * suffix_data.enchant_alloc[ i ] / 10000.0;
      else // Impossible choices, so bogus data, skip
        continue;

      // Loop through enchantment stats, adding valid ones to the stats of the item.
      // Typically (and for cata random suffixes), there seems to be only one stat per enchantment
      for ( unsigned j = 0; j < sizeof_array( enchant_data.ench_type ); ++j )
      {
        if ( enchant_data.ench_type[ j ] != ITEM_ENCHANTMENT_STAT ) continue;

        stat_e stat = util::translate_item_mod( static_cast<item_mod_type>( enchant_data.ench_prop[ j ] ) );

        if ( stat == STAT_NONE ) continue;

        // Make absolutely sure we do not add stats twice
        if ( range::find( stats_added, stat ) != stats_added.end() )
          continue;

        stats_added.push_back( stat );
        out[ stat ] += static_cast<int>( stat_amount );

        if ( p.sim -> debug )
          p.sim -> output( "random_suffix: stat=%d (%s) stat_amount=%f", stat, util::stat_type_string( stat ), stat_amount );
      }
    }
  }
  catch ( const char* fieldname )
  {
    p.sim -> errorf( "Random Suffix Parsing: Player '%s' unable to parse item %s (%d) at slot '%s': %s\n",
                     p.name(), item.name(), item.id(), util::slot_type_string( item.get_item_slot() ), fieldname );
    return false;
  }

  return true;
}

unsigned parse_gem( const player_t& p, const item_data_t& gem, std::array<int,STAT_MAX>& out )
{
  const gem_property_data_t& gem_prop = p.dbc.gem_property( gem.gem_properties );
  if ( ! gem_prop.id )
    return GEM_NONE;

  if ( gem_prop.color == SOCKET_COLOR_META )
  {
    /*
    // For now, make meta gems simply parse the name out
    std::string gem_name = gem -> name;
    std::size_t cut_pt = gem_name.rfind( " Diamond" );
    if ( cut_pt != gem_name.npos )
    {
      gem_name.erase( cut_pt );
      util::tokenize( gem_name );
      item.armory_gems_str += gem_name;
    }*/
  }
  else
  {
    // Fetch gem stats
    const item_enchantment_data_t& item_ench = p.dbc.item_enchantment( gem_prop.enchant_id );
    if ( item_ench.id )
    {
      parse_enchantement_stats( p, item_ench, out );
    }
  }

  return gem_prop.color;
}

/* Obtain gem stats data
 *
 */
bool parse_gems_stats( const item_t& item, const player_t& p, std::array<int,STAT_MAX>& out )
{
  const item_data_t& item_data = item.get_item().get_item_data();

  try
  {
    bool match = true;


    for ( unsigned i = 0; i < sizeof_array( item.get_item().get_gem_data() ); i++ )
    {
      const item_data_t& gem = item.get_item().get_gem_data()[ i ];
      if ( ! gem.id )
      {
        // Check if there's a gem slot, if so, this is ungemmed item.
        if ( item_data.socket_color[ i ] )
          match = false;
        continue;
      }

      if ( item_data.socket_color[ i ] )
      {
        if ( ! ( parse_gem( p, gem, out ) & item_data.socket_color[ i ] ) )
          match = false;
      }
      else
      {
        // Naively accept gems to wrist/hands/waist past the "official" sockets, but only a
        // single extra one. Wrist/hands should be checked against player professions at
        // least ..
        if ( item.get_item_slot() == SLOT_WRISTS || item.get_item_slot() == SLOT_HANDS || item.get_item_slot() == SLOT_WAIST )
        {
          parse_gem( p, gem, out );
          break;
        }
      }
    }

    // Socket bonus
    const item_enchantment_data_t& socket_bonus = p.dbc.item_enchantment( item_data.id_socket_bonus );
    if ( match && socket_bonus.id )
    {
      parse_enchantement_stats( p, socket_bonus, out );
    }

  }
  catch ( const char* fieldname )
  {
    p.sim -> errorf( "Gem Parsing: Player '%s' unable to parse item %s (%d) at slot '%s': %s\n",
                     p.name(), item.name(), item.id(), util::slot_type_string( item.get_item_slot() ), fieldname );
    return false;
  }

  return true;
}

} // end unnamed namespace

bool item_t::parse_stats( const player_t& p )
{
  base_item::parse_item_stats( *this, p, m_stats );

  base_item::parse_enchantement_stats( p, get_item().get_enchant_data(), m_stats );

  base_item::parse_enchantement_stats( p, get_item().get_addon_data(), m_stats );

  base_item::parse_random_suffix_stats( *this, p, m_stats );

  base_item::parse_gems_stats( *this, p, m_stats );

  return true;
}

item_t::item_t( const player_t& p, const std::string& options_str ) :
  player( p ),
  m_item(),
  options_str( options_str )
{
  range::fill( m_stats, 0 );

  init();
  parse_data( p );;
}

void item_t::parse_data( const player_t& p )
{
  // First, clear data
  clear_data( p );

  m_slot = util::translate_invtype( static_cast<inventory_type>( get_item().get_item_data().inventory_type ) );

  parse_stats( p );
}

void item_t::clear_data( const player_t& /* p */ )
{
  m_slot = SLOT_MIN;
}

/* Parse the option string into opt
 *
 */
bool item_t::parse_options( options_t& opt, const std::string& options_str )
{

  if ( options_str.empty() ) return true;


  opt.name_str = options_str;
  std::string remainder = "";

  std::string::size_type cut_pt = options_str.find_first_of( "," );

  if ( cut_pt != options_str.npos )
  {
    remainder       = options_str.substr( cut_pt + 1 );
    opt.name_str = options_str.substr( 0, cut_pt );
  }

  option_t options[] =
  {
    opt_int( "id",         opt.item_id ),
    opt_string( "stats",   opt.stats_str ),
    opt_string( "gems",    opt.gems_str ),
    opt_int( "gem_id1",    opt.gem_ids[ 0 ] ),
    opt_int( "gem_id2",    opt.gem_ids[ 1 ] ),
    opt_int( "gem_id3",    opt.gem_ids[ 2 ] ),
    opt_string( "enchant", opt.enchant_str ),
    opt_int( "enchant_id", opt.enchant_id ),
    opt_string( "addon",   opt.addon_str ),
    opt_int( "addon_id",   opt.addon_id ),
    opt_string( "equip",   opt.equip_str ),
    opt_string( "use",     opt.use_str ),
    opt_string( "weapon",  opt.weapon_str ),
    opt_string( "heroic",  opt.heroic_str ),
    opt_string( "lfr",     opt.lfr_str ),
    opt_string( "type",    opt.armor_type_str ),
    opt_string( "reforge", opt.reforge_str ),
    opt_string( "suffix",  opt.random_suffix_str ),
    opt_int( "suffix_id",  opt.random_suffix_id ),
    opt_string( "ilevel",  opt.ilevel_str ),
    opt_string( "eilevel", opt.effective_ilevel_str ),
    opt_string( "quality", opt.quality_str ),
    opt_string( "source",  opt.data_source_str ),
    opt_string( "upgrade", opt.upgrade_level_str ),
    opt_null()
  };

  option_t::parse( player.sim, opt.name_str.c_str(), options, remainder );

  util::tolower( opt.stats_str            );
  util::tolower( opt.gems_str             );
  util::tolower( opt.enchant_str          );
  util::tolower( opt.addon_str            );
  util::tolower( opt.equip_str            );
  util::tolower( opt.use_str              );
  util::tolower( opt.weapon_str           );
  util::tolower( opt.heroic_str           );
  util::tolower( opt.lfr_str              );
  util::tolower( opt.armor_type_str       );
  util::tolower( opt.reforge_str          );
  util::tolower( opt.random_suffix_str    );
  util::tolower( opt.ilevel_str           );
  util::tolower( opt.effective_ilevel_str );
  util::tolower( opt.quality_str          );
  util::tolower( opt.upgrade_level_str    );

  return true;
}

void item_t::init()
{
  options_t options;
  parse_options( options, options_str );

  try
  {
    /* If we get a item id, try to import item_data into m_item.m_item_data
     */
    if ( options.item_id )
    {
      if ( base_item::init_item_data( player, m_item.m_item_data, options.item_id ) )
        name_str = std::string( m_item.m_item_data.name );
      else
        throw( "item data" );
    }

    /* If we get a enchant id, try to import enchant data into m_item.m_enchant_data
     */
    if ( options.enchant_id )
      if ( ! base_item::init_enchant_data( player, m_item.m_enchant_data, options.enchant_id ) )
        throw( "enchant data" );

    /* If we get a addon id, try to import addon data into m_item.m_addon_data
     */
    if ( options.addon_id )
      if ( ! base_item::init_enchant_data( player, m_item.m_addon_data, options.addon_id ) )
        throw( "addon data" );

    /* If we get a random suffix id, try to import random suffix data into m_item.m_random_suffix_data
     */
    if ( options.random_suffix_id )
      if ( ! base_item::init_random_suffix_data( player, m_item.m_random_suffix_data, options.random_suffix_id ) )
        throw( "random suffix data" );

    /* If we get a gem id, try to import gem data into m_item.m_random_suffix_data
     */
    for ( int i = 0; i < 3; ++i )
    {
      if (  options.gem_ids[ i ] )
        if ( ! base_item::init_gem_data( player, m_item.m_gems[ i ], options.gem_ids[ i ] ) )
          throw( "gem data (#" + util::to_string( i ) + ")" );
    }

    /* If we get a item name, compare it to item_data and if we don't have anything, write it to it
     */
    if ( !options.name_str.empty() )
    {
      if (options.name_str != name_str )
      {
        player.sim -> errorf( "Player %s item %s has inconsistency between name '%s' and '%s' for id '%i'\n",
                       player.name(), name(), name(), options.name_str.c_str(), m_item.m_item_data.id );

        m_item.m_item_data.name = options.name_str.c_str();
      }
    }

    // TODO: Continue initializing stuff with the remaining options

  }
  catch ( const char* fieldname )
  {
    player.sim -> errorf( "Processed Item: Player '%s' initializing item: Unable to initialize %s\n",
                     player.name(), fieldname );
  }
}


} // end namespace new_item_stuff
