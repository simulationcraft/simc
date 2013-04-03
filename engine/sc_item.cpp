// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE ==========================================

static const stat_e reforge_stats[] =
{
  STAT_SPIRIT,
  STAT_DODGE_RATING,
  STAT_PARRY_RATING,
  STAT_HIT_RATING,
  STAT_CRIT_RATING,
  STAT_HASTE_RATING,
  STAT_EXPERTISE_RATING,
  STAT_MASTERY_RATING,
  STAT_NONE
};

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
  std::vector<std::string> splits = util::string_split( encoded_str, "_" );

  tokens.resize( splits.size() );
  for ( size_t i = 0; i < splits.size(); i++ )
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

  return splits.size();
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

std::string special_effect_t::to_string()
{
  std::ostringstream s;

  if ( ! trigger_str.empty() )
    s << "proc=" << trigger_str;
  else
    s << name_str;

  if ( stat != STAT_NONE )
  {
    s << " stat=" << util::stat_type_abbrev( stat );
    s << " amount=" << stat_amount;
    s << " duration=" << duration.total_seconds();
  }

  if ( school != SCHOOL_NONE )
  {
    s << " school=" << util::school_type_string( school );
    s << " amount=" << discharge_amount;
    if ( discharge_scaling > 0 )
      s << " coeff=" << discharge_scaling;
  }

  if ( max_stacks > 0 )
    s << " max_stack=" << max_stacks;

  if ( proc_chance > 0 )
    s << " proc_chance=" << proc_chance * 100.0 << "%";

  if ( ppm > 0 )
    s << " ppm=" << ppm;
  else if ( ppm < 0 )
    s << " rppm=" << std::fabs( ppm );

  if ( cooldown != timespan_t::zero() )
    s << " icd=" << cooldown.total_seconds();

  return s.str();
}

// item_t::item_t =============================================================

item_t::item_t( player_t* p, const std::string& o ) :
  sim( p -> sim ), player( p ), slot( SLOT_INVALID ), unique( false ),
  unique_addon( false ), is_ptr( p -> dbc.ptr ),
  fetched( false ), parsed(), xml( 0 ), options_str( o )
{
  parsed.data.name = name_str.c_str();
  parsed.data.icon = icon_str.c_str();
}

// item_t::to_string ==========================================================

std::string item_t::to_string()
{
  std::ostringstream s;

  s << "name=" << name_str;
  s << " id=" << parsed.data.id;
  s << " slot=" << slot_name();
  s << " quality=" << util::item_quality_string( parsed.data.quality );
  s << " upgrade_level=" << upgrade_level();
  s << " ilevel=" << item_level();
  if ( upgrade_level() > 0 )
    s << " (" << parsed.data.level << ")";
  if ( util::is_match_slot( slot ) )
    s << " match=" << is_matching_type();

  bool is_weapon = false;
  if ( parsed.data.item_class == ITEM_CLASS_ARMOR )
    s << " type=armor/" << util::armor_type_string( parsed.data.item_subclass );
  else if ( parsed.data.item_class == ITEM_CLASS_WEAPON )
  {
    std::string class_str;
    if ( util::weapon_class_string( parsed.data.inventory_type ) )
      class_str = util::weapon_class_string( parsed.data.inventory_type );
    else
      class_str = "Unknown";
    std::string str = util::weapon_subclass_string( parsed.data.item_subclass );
    util::tokenize( str );
    util::tokenize( class_str );
    s << " type=" << class_str << "/" << str;
    is_weapon = true;
  }

  if ( parsed.reforged_from != STAT_NONE && parsed.reforged_to != STAT_NONE )
  {
    int idx_from = 0;
    for ( size_t i = 0; i < sizeof_array( parsed.data.stat_type_e ); i++ )
    {
      if ( parsed.data.stat_type_e[ i ] == util::translate_stat( parsed.reforged_from ) )
        idx_from = i;
    }

    s << " reforge={ " << "-" << floor( 0.4 * stat_value( idx_from ) )
      << " " << util::stat_type_abbrev( parsed.reforged_from )
      << " -> " << "+" << floor( 0.4 * stat_value( idx_from ) )
      << " " << util::stat_type_abbrev( parsed.reforged_to ) << " }";
  }

  if ( parsed.data.stat_type_e[ 0 ] > 0 )
  {
    s << " stats={ ";

    if ( parsed.armor > 0 )
      s << parsed.armor << " Armor, ";
    else if ( item_database::armor_value( *this ) )
      s << item_database::armor_value( *this ) << " Armor, ";

    for ( size_t i = 0; i < sizeof_array( parsed.data.stat_val ); i++ )
    {
      if ( parsed.data.stat_type_e[ i ] == -1 )
        break;
      if ( parsed.data.stat_val[ i ] > 0 )
        s << "+";

      s << stat_value( i ) << " "
        << util::stat_type_abbrev( util::translate_item_mod( parsed.data.stat_type_e[ i ] ) )
        << ", ";
    }
    long x = s.tellp(); s.seekp( x - 2 );
    s << " }";
  }

  if ( parsed.gem_stats.size() > 0 )
  {
    s << " gems={ ";

    for ( size_t i = 0; i < parsed.gem_stats.size(); i++ )
    {
      s << "+" << parsed.gem_stats[ i ].value
        << " " << util::stat_type_abbrev( parsed.gem_stats[ i ].stat ) << ", ";
    }

    long x = s.tellp(); s.seekp( x - 2 );
    s << " }";
  }

  if ( is_weapon )
  {
    s << " damage={ ";
    weapon_t* w = weapon();
    s << w -> min_dmg;
    s << " - ";
    s << w -> max_dmg;
    s << " }";

    s << " speed=";
    s << w -> swing_time.total_seconds();
  }

  if ( parsed.enchant_stats.size() > 0 )
  {
    s << " enchant={ ";

    for ( size_t i = 0; i < parsed.enchant_stats.size(); i++ )
    {
      s << "+" << parsed.enchant_stats[ i ].value << " "
        << util::stat_type_abbrev( parsed.enchant_stats[ i ].stat ) << ", ";
    }

    long x = s.tellp(); s.seekp( x - 2 );
    s << " }";
  }
  else if ( ! parsed.enchant.name_str.empty() )
    s << " enchant={ " << parsed.enchant.to_string() << " }";

  if ( ! parsed.addon.name_str.empty() )
    s << " addon={ " << parsed.addon.to_string() << " }";

  if ( ! parsed.equip.name_str.empty() )
    s << " equip={ " << parsed.equip.to_string() << " }";

  if ( ! parsed.use.name_str.empty() )
    s << " use={ " << parsed.use.to_string() << " }";

  if ( ! source_str.empty() )
    s << " source=" << source_str;

  bool has_spells = false;
  for ( size_t i = 0; i < sizeof_array( parsed.data.id_spell ); i++ )
  {
    if ( parsed.data.id_spell[ i ] > 0 )
    {
      has_spells = true;
      break;
    }
  }

  if ( has_spells )
  {
    s << " proc_spells={ ";
    for ( size_t i = 0; i < sizeof_array( parsed.data.id_spell ); i++ )
    {
      if ( parsed.data.id_spell[ i ] <= 0 )
        continue;

      s << "proc=";
      switch ( parsed.data.trigger_spell[ i ] )
      {
        case ITEM_SPELLTRIGGER_ON_USE:
          s << "OnUse";
          break;
        case ITEM_SPELLTRIGGER_ON_EQUIP:
          s << "OnEquip";
          break;
        default:
          s << "Unknown";
          break;
      }
      s << "/" << parsed.data.id_spell[ i ] << ", ";
    }

    long x = s.tellp(); s.seekp( x - 2 );
    s << " }";
  }

  return s.str();
}

// item_t::has_item_stat ====================================================

bool item_t::has_item_stat( stat_e stat )
{
  for ( size_t i = 0; i < sizeof_array( parsed.data.stat_val ); i++ )
  {
    if ( util::translate_item_mod( parsed.data.stat_type_e[ i ] ) == stat )
      return true;
  }

  return false;
}

// item_t::upgrade_level ====================================================

unsigned item_t::upgrade_level()
{
  return parsed.upgrade_level + sim -> global_item_upgrade_level;
}

// item_t::item_level =======================================================

unsigned item_t::item_level()
{
  return parsed.data.level + item_database::upgrade_ilevel( parsed.data, upgrade_level() );
}

stat_e item_t::stat( size_t idx )
{
  if ( idx >= sizeof_array( parsed.data.stat_type_e ) )
    return STAT_NONE;

  return util::translate_item_mod( parsed.data.stat_type_e[ idx ] );
}

int item_t::stat_value( size_t idx )
{
  if ( idx >= sizeof_array( parsed.data.stat_val ) - 1 )
    return -1;

  return item_database::scaled_stat( parsed.data, player -> dbc, idx, item_level() );
}

// item_t::active ===========================================================

bool item_t::active()
{
  if ( slot == SLOT_INVALID ) return false;
  if ( ! ( name_str.empty() || name_str == "empty" || name_str == "none" ) ) return true;
  return false;
}

// item_t::name =============================================================

const char* item_t::name()
{
  if ( ! option_name_str.empty() ) return option_name_str.c_str();
  if ( ! name_str.empty() ) return name_str.c_str();
  return "inactive";
}

// item_t::slot_name ========================================================

const char* item_t::slot_name()
{
  return util::slot_type_string( slot );
}

// item_t::slot_name ========================================================

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
    opt_uint( "id", parsed.data.id ),
    opt_int( "upgrade", parsed.upgrade_level ),
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
    opt_int( "suffix", parsed.suffix_id ),
    opt_string( "ilevel", option_ilevel_str ),
    opt_string( "quality", option_quality_str ),
    opt_string( "source", option_data_source_str ),
    opt_null()
  };

  option_t::parse( sim, option_name_str.c_str(), options, remainder );

  util::tokenize( option_name_str );

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
  util::tolower( option_ilevel_str           );
  util::tolower( option_quality_str          );

  return true;
}

// item_t::encoded_item =======================================================

std::string item_t::encoded_item()
{
  std::ostringstream s;

  s << name_str;

  if ( parsed.data.id )
    s << ",id=" << parsed.data.id;

  if ( ! option_ilevel_str.empty() )
    s << ",ilevel=" << parsed.data.level;

  if ( ! option_armor_type_str.empty() )
    s << ",type=" << util::armor_type_string( parsed.data.item_subclass );

  if ( ! option_heroic_str.empty() )
    s << ",heroic=" << parsed.data.heroic;

  if ( ! option_lfr_str.empty() )
    s << ",lfr=" << parsed.data.lfr;

  if ( ! option_quality_str.empty() )
    s << ",quality=" << util::item_quality_string( parsed.data.quality );

  if ( parsed.upgrade_level > 0 )
    s << ",upgrade=" << encoded_upgrade_level();

  if ( parsed.suffix_id != 0 )
    s << ",suffix=" << encoded_random_suffix_id();

  if ( ! option_stats_str.empty() )
    s << ",stats=" << encoded_stats();

  if ( ! option_weapon_str.empty() )
    s << ",weapon=" << encoded_weapon();

  if ( ! option_gems_str.empty() || parsed.gem_stats.size() > 0 ||
       ( slot == SLOT_HEAD && player -> meta_gem != META_GEM_NONE ) )
    s << ",gems=" << encoded_gems();

  if ( ! option_enchant_str.empty() || parsed.enchant_stats.size() > 0 ||
       ! parsed.enchant.name_str.empty() )
    s << ",enchant=" << encoded_enchant();

  if ( ! option_addon_str.empty() || parsed.addon_stats.size() > 0 ||
       ! parsed.addon.name_str.empty() )
    s << ",addon=" << encoded_addon();

  if ( ! option_equip_str.empty() )
    s << ",equip=" << option_equip_str;

  if ( ! option_use_str.empty() )
    s << ",use=" << option_use_str;

  if ( ! option_reforge_str.empty() ||
       ( parsed.reforged_from != STAT_NONE && parsed.reforged_to != STAT_NONE ) )
    s << ",reforge=" << encoded_reforge();

  if ( ! option_data_source_str.empty() )
    s << ",source=" << option_data_source_str;

  return s.str();
}

// item_t::encoded_comment ====================================================

std::string item_t::encoded_comment()
{
  std::ostringstream s;

  if ( option_ilevel_str.empty() )
    s << "ilevel=" << parsed.data.level << ",";

  if ( option_quality_str.empty() && parsed.data.quality )
    s << "quality=" << util::item_quality_string( parsed.data.quality ) << ",";

  if ( option_armor_type_str.empty() &&
       parsed.data.item_class == ITEM_CLASS_ARMOR &&
       parsed.data.item_subclass != ITEM_SUBCLASS_ARMOR_MISC )
    s << "type=" << util::armor_type_string( parsed.data.item_subclass ) << ",";

  if ( option_heroic_str.empty() && parsed.data.heroic )
    s << "heroic=" << parsed.data.heroic << ",";

  if ( option_lfr_str.empty() && parsed.data.lfr )
    s << "lfr=" << parsed.data.lfr << ",";

  if ( option_stats_str.empty() && ! encoded_stats().empty() )
    s << "stats=" << encoded_stats() << ",";

  if ( option_weapon_str.empty() && ! encoded_weapon().empty() )
    s << "weapon=" << encoded_weapon() << ",";

  if ( option_equip_str.empty() && ! parsed.equip.encoding_str.empty() )
    s << "equip=" << parsed.equip.encoding_str << ",";

  if ( option_use_str.empty() && ! parsed.use.encoding_str.empty() )
    s << "use=" << parsed.use.encoding_str << ",";

  std::string str = s.str();
  if ( ! str.empty() )
    str.erase( str.end() - 1 );
  return str;
}


// item_t::encoded_gems =======================================================

std::string item_t::encoded_gems()
{
  if ( ! option_gems_str.empty() )
    return option_gems_str;

  std::string stats_str = stat_pairs_to_str( parsed.gem_stats );
  if ( slot == SLOT_HEAD && player -> meta_gem != META_GEM_NONE )
  {
    std::string meta_str = util::meta_gem_type_string( player -> meta_gem );
    if ( ! stats_str.empty() )
      stats_str = meta_str + std::string( "_" ) + stats_str;
    else
      stats_str = meta_str;
  }

  return stats_str;
}

// item_t::encoded_enchant ====================================================

std::string item_t::encoded_enchant()
{
  if ( ! option_enchant_str.empty() )
    return option_enchant_str;

  std::string stats_str = stat_pairs_to_str( parsed.enchant_stats );
  if ( stats_str.empty() )
    stats_str = parsed.enchant.name_str;

  return stats_str;
}

// item_t::encoded_addon ======================================================

std::string item_t::encoded_addon()
{
  if ( ! option_addon_str.empty() )
    return option_addon_str;

  std::string stats_str = stat_pairs_to_str( parsed.addon_stats );
  if ( stats_str.empty() )
    stats_str = parsed.addon.name_str;

  return stats_str;
}

// item_t::encoded_upgrade_level ==============================================

std::string item_t::encoded_upgrade_level()
{
  std::string upgrade_level_str;
  if ( parsed.upgrade_level > 0 )
    upgrade_level_str = util::to_string( parsed.upgrade_level );

  return upgrade_level_str;
}

// item_t::encoded_random_suffix_id ===========================================

std::string item_t::encoded_random_suffix_id()
{
  std::string str;

  if ( parsed.suffix_id != 0 )
    str += util::to_string( parsed.suffix_id );

  return str;
}

// item_t::encoded_reforge ====================================================

std::string item_t::encoded_reforge()
{
  if ( ! option_reforge_str.empty() )
    return option_reforge_str;

  std::string str;
  if ( parsed.reforged_from != STAT_NONE && parsed.reforged_to != STAT_NONE )
  {
    str =  util::stat_type_abbrev( parsed.reforged_from );
    str += "_";
    str += util::stat_type_abbrev( parsed.reforged_to );
  }

  util::tokenize( str );

  return str;
}

// item_t::encoded_stats ======================================================

std::string item_t::encoded_stats()
{
  if ( ! option_stats_str.empty() )
    return option_stats_str;

  std::vector<std::string> stats;

  for ( size_t i = 0; i < sizeof_array( parsed.data.stat_type_e ); i++ )
  {
    if ( parsed.data.stat_type_e[ i ] < 0 )
      continue;

    std::string stat_str = item_database::stat_to_str( parsed.data.stat_type_e[ i ], parsed.data.stat_val[ i ] );
    if ( ! stat_str.empty() ) stats.push_back( stat_str );
  }

  std::string str;
  for ( size_t i = 0; i < stats.size(); i++ )
  {
    if ( i ) str += '_';
    str += stats[ i ];
  }

  return str;
}

// item_t::encoded_weapon ====================================================

std::string item_t::encoded_weapon()
{
  if ( ! option_weapon_str.empty() )
    return option_weapon_str;

  std::string str;

  if ( parsed.data.item_class != ITEM_CLASS_WEAPON )
    return str;

  double speed     = parsed.data.delay / 1000.0;
  unsigned min_dam = item_database::weapon_dmg_min( *this );
  unsigned max_dam = item_database::weapon_dmg_max( *this );

  if ( ! speed || ! min_dam || ! max_dam )
    return str;

  weapon_e w = util::translate_weapon_subclass( ( item_subclass_weapon ) parsed.data.item_subclass );
  if ( w == WEAPON_NONE )
    return str;

  str += util::weapon_type_string( w );

  if ( w != WEAPON_WAND )
  {
    str += "_" + util::to_string( speed, 2 ) + "speed_" +
           util::to_string( min_dam ) + "min_" + util::to_string( max_dam ) + "max";
  }

  return str;
}

// item_t::init =============================================================

bool item_t::init()
{
  parse_options();

  // Item specific source list has to be decoded first so we can properly
  // download the item from the correct source
  if ( ! decode_data_source() )
    return false;

  if ( parsed.data.id > 0 )
  {
    if ( ! fetched && ! item_t::download_item( *this ) &&
         option_stats_str.empty() && option_weapon_str.empty() )
      return false;
  }
  else
    name_str = option_name_str;

  if ( name_str.empty() || name_str == "empty" || name_str == "none" )
    return true;

  std::string use_str = option_use_str;
  if ( use_str.empty() )
  {
    if ( unique_gear::get_use_encoding( use_str, name_str, parsed.data.heroic,
                                        parsed.data.lfr, player -> dbc.ptr, parsed.data.id ) )
      parsed.use.name_str = name_str;
  }
  else
    parsed.use.name_str = name_str;

  std::string equip_str = option_equip_str;
  if ( equip_str.empty() )
  {
    if ( unique_gear::get_equip_encoding( equip_str, name_str, parsed.data.heroic,
                                          parsed.data.lfr, player -> dbc.ptr, parsed.data.id ) )
      parsed.equip.name_str = name_str;
  }
  else
    parsed.equip.name_str = name_str;

  // Process basic stats
  if ( ! decode_heroic()                           ) return false;
  if ( ! decode_lfr()                              ) return false;
  if ( ! decode_quality()                          ) return false;
  if ( ! decode_ilevel()                           ) return false;
  if ( ! decode_armor_type()                       ) return false;

  if ( parsed.upgrade_level > 0 && ( ! parsed.data.quality || ! parsed.data.level ) )
    sim -> errorf( "Player %s upgrading item %s at slot %s without quality or ilevel, upgrading will not work\n",
                   player -> name(), name(), slot_name() );

  if ( ! option_enchant_str.empty() && ( slot == SLOT_FINGER_1 || slot == SLOT_FINGER_2 ) &&
       ! ( player -> profession[ PROF_ENCHANTING ] > 0 ) )
    sim -> errorf( "Player %s's ring at slot %s has a ring enchant without the enchanting profession\n",
                   player -> name(), slot_name() );

  // Process complex input, and initialize item in earnest
  if ( ! decode_stats()                            ) return false;
  if ( ! decode_gems()                             ) return false;
  if ( ! decode_enchant()                          ) return false;
  if ( ! decode_addon()                            ) return false;
  if ( ! decode_weapon()                           ) return false;
  if ( ! decode_random_suffix()                    ) return false;
  if ( ! decode_reforge()                          ) return false;
  if ( ! decode_special( parsed.use, use_str     ) ) return false;
  if ( ! decode_special( parsed.equip, equip_str ) ) return false;

  if ( ! option_name_str.empty() && ( option_name_str != name_str ) )
  {
    sim -> errorf( "Player %s at slot %s has inconsistency between name '%s' and '%s' for id %u\n",
                   player -> name(), slot_name(), option_name_str.c_str(), name_str.c_str(), parsed.data.id );

    name_str = option_name_str;
  }

  if ( source_str.empty() )
    source_str = "Manual";

  if ( sim -> debug )
    sim -> output( "%s", to_string().c_str() );

  return true;
}

// item_t::decode_heroic ====================================================

bool item_t::decode_heroic()
{
  if ( ! option_heroic_str.empty() )
    parsed.data.heroic = option_heroic_str == "1";

  return true;
}

// item_t::decode_lfr =======================================================

bool item_t::decode_lfr()
{
  if ( ! option_lfr_str.empty() )
    parsed.data.lfr = option_heroic_str == "1";

  return true;
}

// item_t::is_matching_type =================================================

bool item_t::is_matching_type()
{
  if ( player -> type == MAGE || player -> type == PRIEST || player -> type == WARLOCK )
    return true;

  if ( ! util::is_match_slot( slot ) )
    return true;

  return util::matching_armor_type( player -> type ) == parsed.data.item_subclass;
}

// item_t::decode_armor_type ================================================

bool item_t::decode_armor_type()
{
  if ( ! option_armor_type_str.empty() )
  {
    parsed.data.item_subclass = util::parse_armor_type( option_armor_type_str );
    if ( parsed.data.item_subclass == ITEM_SUBCLASS_ARMOR_MISC )
      return false;
    parsed.data.item_class = ITEM_CLASS_ARMOR;
  }

  return true;
}

// item_t::decode_ilevel ====================================================

bool item_t::decode_ilevel()
{
  if ( ! option_ilevel_str.empty() )
  {
    parsed.data.level = util::to_unsigned( option_ilevel_str );
    if ( parsed.data.level == 0 )
      return false;
  }

  return true;
}

// item_t::decode_quality ===================================================

bool item_t::decode_quality()
{
  if ( ! option_quality_str.empty() )
    parsed.data.quality = util::parse_item_quality( option_quality_str );

  return true;
}

// item_t::decode_stats =====================================================

bool item_t::decode_stats()
{
  if ( ! option_stats_str.empty() || option_stats_str == "none" )
  {
    // First, clear any stats in current data
    parsed.armor = 0;
    range::fill( parsed.data.stat_type_e, -1 );
    range::fill( parsed.data.stat_val, 0 );
    range::fill( parsed.data.stat_alloc, 0 );

    std::vector<token_t> tokens;
    int num_tokens = parse_tokens( tokens, option_stats_str );
    int stat = 0;

    for ( int i = 0; i < num_tokens && stat < ( int ) sizeof_array( parsed.data.stat_val ); i++ )
    {
      stat_e s = util::parse_stat_type( tokens[ i ].name );
      if ( s == STAT_NONE )
      {
        sim -> errorf( "Player %s has unknown 'stats=' token '%s' at slot %s\n",
                       player -> name(), tokens[ i ].full.c_str(), slot_name() );
        return false;
      }

      if ( s == STAT_ARMOR && ! util::is_match_slot( slot ) && slot != SLOT_BACK && slot != SLOT_OFF_HAND )
        s = STAT_BONUS_ARMOR;

      if ( s != STAT_ARMOR )
      {
        parsed.data.stat_type_e[ stat ] = util::translate_stat( s );
        parsed.data.stat_val[ stat ] = static_cast<int>( tokens[ i ].value );
        stat++;
      }
      else
        parsed.armor = tokens[ i ].value;
    }
  }

  for ( size_t i = 0; i < sizeof_array( parsed.data.stat_val ); i++ )
  {
    stat_e s = stat( i );
    if ( s == STAT_NONE ) continue;

    base_stats.add_stat( s, stat_value( i ) );
    stats.add_stat( s, stat_value( i ) );
  }

  // Hardcoded armor value in stats, use the approximation coefficient to do
  // item upgrades
  if ( parsed.armor > 0 )
  {
    base_stats.add_stat( STAT_ARMOR, parsed.armor * item_database::approx_scale_coefficient( parsed.data.level, item_level() ) );
    stats.add_stat( STAT_ARMOR, parsed.armor * item_database::approx_scale_coefficient( parsed.data.level, item_level() ) );
  }
  // DBC based armor value, item upgrades are handled by the increased itemlevel
  else
  {
    base_stats.add_stat( STAT_ARMOR, item_database::armor_value( *this ) );
    stats.add_stat( STAT_ARMOR, item_database::armor_value( *this ) );
  }

  return true;
}

// item_t::decode_reforge ===================================================

bool item_t::decode_reforge()
{
  if ( option_reforge_str == "none" || ! option_reforge_str.empty() )
  {
    parsed.reforged_from = parsed.reforged_to = STAT_NONE;

    std::vector<token_t> tokens;
    int num_tokens = parse_tokens( tokens, option_reforge_str );

    if ( num_tokens <= 0 )
      return true;

    if ( num_tokens != 2 )
    {
      sim -> errorf( "Player %s has unknown 'reforge=' '%s' at slot %s\n", player -> name(), option_reforge_str.c_str(), slot_name() );
      return false;
    }

    stat_e s1 = util::parse_reforge_type( tokens[ 0 ].name );
    stat_e s2 = util::parse_reforge_type( tokens[ 1 ].name );
    if ( ( s1 == STAT_NONE ) || ( s2 == STAT_NONE ) )
    {
      sim -> errorf( "Player %s has unknown 'reforge=' '%s' at slot %s\n",
                     player -> name(), option_reforge_str.c_str(), slot_name() );
      return false;
    }
    if ( base_stats.get_stat( s1 ) <= 0.0 )
    {
      sim -> errorf( "Player %s with 'reforge=' '%s' at slot %s does not have source stat on item.\n",
                     player -> name(), option_reforge_str.c_str(), slot_name() );
      return false;
    }
    if ( base_stats.get_stat( s2 ) > 0.0 )
    {
      sim -> errorf( "Player %s with 'reforge=' '%s' at slot %s already has target stat on item.\n",
                     player -> name(), option_reforge_str.c_str(), slot_name() );
      return false;
    }

    parsed.reforged_from = s1;
    parsed.reforged_to   = s2;
  }

  if ( parsed.reforge_id > 0 && ! parse_reforge_id() )
    return false;

  if ( parsed.reforged_from != STAT_NONE && parsed.reforged_to != STAT_NONE )
  {
    double amount = floor( base_stats.get_stat( parsed.reforged_from ) * 0.4 );
    stats.add_stat( parsed.reforged_from, -amount );
    stats.add_stat( parsed.reforged_to,    amount );
  }

  return true;
}

// item_t::decode_random_suffix =============================================

bool item_t::decode_random_suffix()
{
  if ( parsed.suffix_id == 0 )
    return true;

  // We need the ilevel/quality data, otherwise we cannot figure out
  // the random suffix point allocation.
  if ( item_level() == 0 || parsed.data.quality == 0 )
  {
    sim -> errorf( "Player %s with random suffix at slot %s requires both ilevel= and quality= information.\n", player -> name(), slot_name() );
    return true;
  }

  // These stats will be automatically at the correct upgrade level, as the
  // item budget is selected by the total ilevel of the item.
  const random_prop_data_t& ilevel_data   = player -> dbc.random_property( item_level() );
  const random_suffix_data_t& suffix_data = player -> dbc.random_suffix( abs( parsed.suffix_id ) );

  if ( ! suffix_data.id )
  {
    sim -> errorf( "Warning: Unknown random suffix identifier %d at slot %s for item %s.\n",
                   parsed.suffix_id, slot_name(), name() );
    return true;
  }

  int f = item_database::random_suffix_type( *this );

  if ( sim -> debug )
  {
    sim -> output( "random_suffix: item=%s parsed.suffix_id=%d ilevel=%d quality=%d random_point_pool=%d",
                   name(), parsed.suffix_id, item_level(), parsed.data.quality, f );
  }

  std::vector<std::string> stat_list;
  for ( size_t i = 0; i < sizeof_array( suffix_data.enchant_id ); i++ )
  {
    unsigned enchant_id;
    if ( ! ( enchant_id = suffix_data.enchant_id[ i ] ) )
      continue;

    const item_enchantment_data_t& enchant_data = player -> dbc.item_enchantment( enchant_id );

    if ( ! enchant_data.id )
      continue;

    // Calculate amount of points
    double stat_amount;
    if ( parsed.data.quality == 4 ) // Epic
      stat_amount = ilevel_data.p_epic[ f ] * suffix_data.enchant_alloc[ i ] / 10000.0;
    else if ( parsed.data.quality == 3 ) // Rare
      stat_amount = ilevel_data.p_rare[ f ] * suffix_data.enchant_alloc[ i ] / 10000.0;
    else if ( parsed.data.quality == 2 ) // Uncommon
      stat_amount = ilevel_data.p_uncommon[ f ] * suffix_data.enchant_alloc[ i ] / 10000.0;
    else // Impossible choices, so bogus data, skip
      continue;

    // Loop through enchantment stats, adding valid ones to the stats of the item.
    // Typically (and for cata random suffixes), there seems to be only one stat per enchantment
    for ( size_t j = 0; j < sizeof_array( enchant_data.ench_type ); j++ )
    {
      if ( enchant_data.ench_type[ j ] != ITEM_ENCHANTMENT_STAT ) continue;

      stat_pair_t stat = item_database::item_enchantment_effect_stats( enchant_data, j );
      if ( stat.stat == STAT_NONE )
        continue;

      bool has_stat = false;
      int free_idx = 0;

      for ( size_t k = 0; k < sizeof_array( parsed.data.stat_val ); k++ )
      {
        if ( parsed.data.stat_type_e[ k ] == ( int ) enchant_data.ench_prop[ j ] )
          has_stat = true;

        if ( parsed.data.stat_type_e[ k ] <= 0 )
        {
          free_idx = k;
          break;
        }
      }

      if ( has_stat == true )
      {
        if ( sim -> debug )
          sim -> output( "random_suffix: Player %s item %s attempted to add base stat %lu %s (%d) twice, due to random suffix.",
                         player -> name(), name(), j, util::stat_type_abbrev( stat.stat ), enchant_data.ench_type[ j ] );
        continue;
      }

      if ( sim -> debug )
        sim -> output( "random_suffix: stat=%d (%s) stat_amount=%f", stat.stat, util::stat_type_abbrev( stat.stat ), stat_amount );

      parsed.data.stat_type_e[ free_idx ] = stat.stat;
      parsed.data.stat_val[ free_idx ] = static_cast< int >( stat_amount );
      base_stats.add_stat( stat.stat, static_cast< int >( stat_amount ) );
      stats.add_stat( stat.stat, static_cast< int >( stat_amount ) );
      free_idx++;
    }
  }

  if ( suffix_data.suffix )
  {
    std::string str = suffix_data.suffix;
    util::tokenize( str );

    if ( name_str.find( str ) == std::string::npos )
    {
      name_str += '_' + str;
    }
  }

  return true;
}

// item_t::decode_gems ======================================================

bool item_t::decode_gems()
{
  if ( ! option_gems_str.empty() && option_gems_str != "none" )
  {
    parsed.gem_stats.clear();

    std::vector<token_t> tokens;
    int num_tokens = parse_tokens( tokens, option_gems_str );

    std::string meta_prefix, meta_suffix;

    for ( int i = 0; i < num_tokens; i++ )
    {
      token_t& t = tokens[ i ];
      stat_e s;

      if ( ( s = util::parse_stat_type( t.name ) ) != STAT_NONE )
        parsed.gem_stats.push_back( stat_pair_t( s, t.value ) );
      else if ( is_meta_prefix( t.name ) )
        meta_prefix = t.name;
      else if ( is_meta_suffix( t.name ) )
        meta_suffix = t.name;
      else
        sim -> errorf( "Player %s has unknown 'gems=' token '%s' at slot %s\n", player -> name(), t.full.c_str(), slot_name() );
    }

    meta_gem_e meta_gem = parse_meta_gem( meta_prefix, meta_suffix );

    if ( meta_gem != META_GEM_NONE )
      player -> meta_gem = meta_gem;
  }

  for ( size_t i = 0; i < parsed.gem_stats.size(); i++ )
    stats.add_stat( parsed.gem_stats[ i ].stat, parsed.gem_stats[ i ].value );

  return true;
}

// item_t::decode_enchant ===================================================

bool item_t::decode_enchant()
{
  // If the user gives an enchant, override everything fetched from the
  // data sources
  if ( ! option_enchant_str.empty() && option_enchant_str != "none" )
  {
    parsed.enchant.clear();
    parsed.enchant_stats = str_to_stat_pair( option_enchant_str );
    if ( parsed.enchant_stats.size() == 0 )
      parsed.enchant.name_str = option_enchant_str;
  }

  // Special enchants are enchants that cannot be handled by a generic on-equip
  // or on-use string. They will be handled later on in the initialization
  // process by enchant::init() method call.
  if ( is_special_enchant( parsed.enchant.name_str ) )
  {
    parsed.enchant.unique = true;
    return true;
  }

  std::string equip_str;
  if ( unique_gear::get_equip_encoding( equip_str, parsed.enchant.name_str, parsed.data.heroic, parsed.data.lfr, player -> dbc.ptr ) )
  {
    parsed.enchant.unique = true;
    return decode_special( parsed.enchant, equip_str );
  }

  for ( size_t i = 0; i < parsed.enchant_stats.size(); i++ )
    stats.add_stat( parsed.enchant_stats[ i ].stat, parsed.enchant_stats[ i ].value );

  return true;
}

// item_t::decode_addon =====================================================

bool item_t::decode_addon()
{
  if ( ! option_addon_str.empty() && option_addon_str != "none" )
  {
    parsed.addon.clear();
    parsed.addon_stats = str_to_stat_pair( option_addon_str );
    if ( parsed.addon_stats.size() == 0 )
      parsed.addon.name_str = option_addon_str;
  }

  // Special addons are addons that cannot be handled by a generic on-equip
  // or on-use string. They will be handled later on in the initialization
  // process by enchant::init() method call.
  if ( is_special_addon( parsed.addon.name_str ) )
  {
    parsed.addon.unique = true;
    return true;
  }

  std::string use_str;
  if ( unique_gear::get_use_encoding( use_str, parsed.addon.name_str, parsed.data.heroic, parsed.data.lfr, player -> dbc.ptr ) )
  {
    parsed.use.unique = true;
    return decode_special( parsed.use, use_str );
  }

  std::string equip_str;
  if ( unique_gear::get_equip_encoding( equip_str, parsed.addon.name_str, parsed.data.heroic, parsed.data.lfr, player -> dbc.ptr ) )
  {
    parsed.addon.unique = true;
    return decode_special( parsed.addon, equip_str );
  }

  for ( size_t i = 0; i < parsed.addon_stats.size(); i++ )
    stats.add_stat( parsed.addon_stats[ i ].stat, parsed.addon_stats[ i ].value );

  return parsed.addon.name_str.empty() || parsed.addon_stats.size() > 0;
}

// item_t::decode_special ===================================================

bool item_t::decode_special( special_effect_t& effect,
                             const std::string& encoding )
{
  if ( encoding.empty() || encoding == "custom" || encoding == "none" ) return true;

  std::vector<token_t> tokens;

  int num_tokens = parse_tokens( tokens, encoding );

  effect.encoding_str = encoding;

  for ( int i = 0; i < num_tokens; i++ )
  {
    token_t& t = tokens[ i ];
    stat_e s;
    school_e sc;

    if ( ( s = util::parse_stat_type( t.name ) ) != STAT_NONE )
    {
      effect.stat = s;
      effect.stat_amount = t.value;

      // Support scaling procs in a hacky way.
      if ( parsed.data.id && ( upgrade_level() > 0 || sim -> scale_to_itemlevel != -1 ) )
      {
        int ilevel = upgrade_level() ? item_level() : sim -> scale_to_itemlevel;

        if ( ilevel == parsed.data.level )
          return true;

        const random_prop_data_t& orig_data = player -> dbc.random_property( parsed.data.level );
        const random_prop_data_t& new_data = player -> dbc.random_property( ilevel );
        int old_budget = 0;
        int new_budget = 0;

        if ( parsed.data.quality == 4 )
        {
          old_budget = ( int ) orig_data.p_epic[ 0 ];
          new_budget = ( int ) new_data.p_epic[ 0 ];
        }
        else if ( parsed.data.quality == 3 )
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

        if ( parsed.data.id_spell[ 0 ] != 0 )
        {
          const spell_data_t& proc_spell = *player -> find_spell( parsed.data.id_spell[ 0 ] );
          if ( proc_spell.ok() )
          {
            const spell_data_t& buff_spell = *proc_spell.effectN( 1 ).trigger();
            if ( buff_spell.ok() )
            {
              for ( size_t j = 1; j <= buff_spell.effect_count(); j++ )
              {
                if ( buff_spell.effectN( j ).type() == E_APPLY_AURA &&
                     ( buff_spell.effectN( j ).subtype() == A_MOD_STAT || buff_spell.effectN( j ).subtype() == A_MOD_RATING ) )
                {
                  effect.stat_amount = util::round( new_budget * buff_spell.effectN( j ).m_average() );
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
                         name(), parsed.data.id, ilevel, parsed.data.quality, parsed.data.level, old_budget, new_budget, t.value, t.value / old_budget, effect.stat_amount );
        }
      }
    }
    else if ( ( sc = util::parse_school_type( t.name ) ) != SCHOOL_NONE )
    {
      effect.school = sc;
      effect.discharge_amount = t.value;

      std::vector<std::string> splits = util::string_split( t.value_str, "+" );
      if ( splits.size() == 2 )
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
      effect.ppm = t.value;
    }
    else if ( util::str_prefix_ci( t.name, "rppm" ) )
    {
      effect.ppm = -t.value;

      if ( util::str_in_str_ci( t.name, "spellcrit" ) )
        effect.rppm_scale = RPPM_SPELL_CRIT;
      else if ( util::str_in_str_ci( t.name, "attackcrit" ) )
        effect.rppm_scale = RPPM_ATTACK_CRIT;
      else
        effect.rppm_scale = RPPM_HASTE;
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
      effect.trigger_type = PROC_HARMFUL_SPELL_LANDING;
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
      effect.trigger_type = PROC_HARMFUL_SPELL_LANDING;
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

  // Custom weapon stats cant be unloaded to the "proxy" item data at all,
  // so edit the weapon in question right away based on either the
  // parsed data or the option data iven by the user.
  if ( option_weapon_str.empty() || option_weapon_str == "none" )
  {
    if ( parsed.data.item_class != ITEM_CLASS_WEAPON )
      return true;

    weapon_e wc = util::translate_weapon_subclass( parsed.data.item_subclass );
    if ( wc == WEAPON_NONE )
      return true;

    w -> type = wc;
    w -> swing_time = timespan_t::from_millis( parsed.data.delay );
    w -> dps = player -> dbc.weapon_dps( &parsed.data, item_level() );
    w -> damage = player -> dbc.weapon_dps( &parsed.data, item_level() ) * parsed.data.delay / 1000.0;
    w -> min_dmg = item_database::weapon_dmg_min( *this );
    w -> max_dmg = item_database::weapon_dmg_max( *this );
  }
  else
  {
    std::vector<token_t> tokens;
    int num_tokens = parse_tokens( tokens, option_weapon_str );

    bool dps_set = false, dmg_set = false, min_set = false, max_set = false;

    for ( int i = 0; i < num_tokens; i++ )
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

    parsed.data.item_class = ITEM_CLASS_WEAPON;
    parsed.data.item_subclass = util::translate_weapon( w -> type );
    parsed.data.delay = w -> swing_time.total_millis();
    if ( dps_set && min_set )
      parsed.data.dmg_range = 2 - 2 * w -> min_dmg / ( w -> dps * parsed.data.delay );

    if ( dps_set ) w -> damage = w -> dps    * w -> swing_time.total_seconds();
    if ( dmg_set ) w -> dps    = w -> damage / w -> swing_time.total_seconds();

    if ( ! max_set || ! min_set )
    {
      w -> max_dmg = w -> damage;
      w -> min_dmg = w -> damage;
    }

    // Approximate gear upgrades for user given strings too. Data source based
    // weapon stats will automatically be handled by the upgraded ilevel for
    // the item.
    w -> max_dmg *= item_database::approx_scale_coefficient( parsed.data.level, item_level() );
    w -> min_dmg *= item_database::approx_scale_coefficient( parsed.data.level, item_level() );
  }

  return true;
}

// item_t::decode_data_source =================================================

bool item_t::decode_data_source()
{
  if ( ! item_database::initialize_item_sources( *this, parsed.source_list ) )
  {
    sim -> errorf( "Your item-specific data source string \"%s\" contained no valid sources to download item id %u.\n",
                   option_data_source_str.c_str(), parsed.data.id );
    return false;
  }

  return true;
}

// item_t::is_special_enchant =================================================

bool item_t::is_special_enchant( const std::string& enchant_str )
{
  if ( enchant_str == "berserking"        ||
       enchant_str == "executioner"       ||
       enchant_str == "mongoose"          ||
       enchant_str == "avalanche"         ||
       enchant_str == "elemental_slayer"  ||
       enchant_str == "elemental_force"   ||
       enchant_str == "hurricane"         ||
       enchant_str == "landslide"         ||
       enchant_str == "power_torrent"     ||
       enchant_str == "jade_spirit"       ||
       enchant_str == "windwalk"          ||
       enchant_str == "spellsurge"        ||
       enchant_str == "rivers_song"       ||
       enchant_str == "mirror_scope"      ||
       enchant_str == "gnomish_xray"      ||
       enchant_str == "windsong"          ||
       enchant_str == "dancing_steel"     ||
       enchant_str == "colossus"          ||
       enchant_str == "lord_blastingtons_scope_of_doom" )
  {
    return true;
  }

  return false;
}

// item_t::is_special_addon ===================================================

bool item_t::is_special_addon( const std::string& addon_str )
{
  if ( addon_str == "synapse_springs"         ||
       addon_str == "synapse_springs_2"       ||
       addon_str == "synapse_springs_mark_ii" ||
       addon_str == "phase_fingers"           ||
       addon_str == "nitro_boosts"            ||
       addon_str == "flexweave_underlay"      ||
       addon_str == "grounded_plasma_shield"  ||
       addon_str == "cardboard_assassin"      ||
       addon_str == "invisibility_field"      ||
       addon_str == "mind_amplification_dish" ||
       addon_str == "personal_electromagnetic_pulse_generator" ||
       addon_str == "frag_belt"               ||
       addon_str == "spinal_healing_injector" ||
       addon_str == "z50_mana_gulper"         ||
       addon_str == "reticulated_armor_webbing" ||
       addon_str == "goblin_glider"           ||
       addon_str == "watergliding_jets" )
  {
    return true;
  }

  return false;
}

// item_t::str_to_stat_pair =================================================

std::vector<stat_pair_t> item_t::str_to_stat_pair( const std::string& stat_str )
{
  std::vector<token_t> tokens;
  std::vector<stat_pair_t> stats;

  parse_tokens( tokens, stat_str );
  for ( size_t i = 0; i < tokens.size(); i++ )
  {
    stat_e s = STAT_NONE;
    if ( ( s = util::parse_stat_type( tokens[ i ].name ) ) != STAT_NONE && tokens[ i ].value != 0 )
      stats.push_back( stat_pair_t( s, tokens[ i ].value ) );
  }

  return stats;
}

// item_t::stat_pairs_to_str ================================================

std::string item_t::stat_pairs_to_str( const std::vector<stat_pair_t>& stat_pairs )
{
  std::vector<std::string> stats;

  for ( size_t i = 0; i < stat_pairs.size(); i++ )
  {
    std::string stat_str = util::to_string( stat_pairs[ i ].value ) + util::stat_type_abbrev( stat_pairs[ i ].stat );
    if ( ! stat_str.empty() ) stats.push_back( stat_str );
  }

  std::string str;
  for ( size_t i = 0; i < stats.size(); i++ )
  {
    if ( i ) str += '_';
    str += stats[ i ];
  }

  util::tokenize( str );
  return str;
}

// item_t::download_slot ====================================================

bool item_t::download_slot( item_t& item )
{
  const cache::behavior_e cb = cache::items();
  bool success = false;

  if ( cb != cache::CURRENT )
  {
    for ( unsigned i = 0; ! success && i < item.sim -> item_db_sources.size(); i++ )
    {
      const std::string& src = item.sim -> item_db_sources[ i ];
      if ( src == "local" )
        success = item_database::download_slot( item );
      else if ( src == "wowhead" )
        success = wowhead::download_slot( item, wowhead::LIVE, cache::ONLY );
      else if ( src == "ptrhead" )
        success = wowhead::download_slot( item, wowhead::PTR, cache::ONLY );
      else if ( src == "bcpapi" )
        success = bcp_api::download_slot( item, cache::ONLY );
    }
  }

  if ( cb != cache::ONLY )
  {
    // Download in earnest from a data source
    for ( unsigned i = 0; ! success && i < item.sim -> item_db_sources.size(); i++ )
    {
      const std::string& src = item.sim -> item_db_sources[ i ];
      if ( src == "wowhead" )
        success = wowhead::download_slot( item, wowhead::LIVE, cb );
      else if ( src == "ptrhead" )
        success = wowhead::download_slot( item, wowhead::PTR, cb );
      else if ( src == "bcpapi" )
        success = bcp_api::download_slot( item, cb );
    }
  }

  if ( ! item_database::parse_item_spell_enchant( item, item.parsed.enchant_stats, item.parsed.enchant, item.parsed.enchant_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse enchant id %u for item \"%s\" at slot %s.\n",
                        item.player -> name(), item.parsed.enchant_id, item.name(), item.slot_name() );
  }

  if ( ! item_database::parse_item_spell_enchant( item, item.parsed.addon_stats, item.parsed.addon, item.parsed.addon_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse addon id %u for item \"%s\" at slot %s.\n",
                        item.player -> name(), item.parsed.addon_id, item.name(), item.slot_name() );
  }

  item.fetched = success;

  return success;
}

// item_t::download_item ====================================================

bool item_t::download_item( item_t& item )
{
  bool success = false;

  const std::vector<std::string>& sources = item.parsed.source_list;
  if ( cache::items() != cache::CURRENT )
  {
    for ( unsigned i = 0; ! success && i < sources.size(); i++ )
    {
      if ( sources[ i ] == "local" )
        success = item_database::download_item( item );
      else if ( sources[ i ] == "wowhead" )
        success = wowhead::download_item( item, wowhead::LIVE, cache::ONLY );
      else if ( sources[ i ] == "ptrhead" )
        success = wowhead::download_item( item, wowhead::PTR, cache::ONLY );
      else if ( sources[ i ] == "bcpapi" )
        success = bcp_api::download_item( item, cache::ONLY );
    }
  }

  if ( cache::items() != cache::ONLY )
  {
    // Download in earnest from a data source
    for ( unsigned i = 0; ! success && i < sources.size(); i++ )
    {
      if ( sources[ i ] == "wowhead" )
        success = wowhead::download_item( item, wowhead::LIVE );
      else if ( sources[ i ] == "ptrhead" )
        success = wowhead::download_item( item, wowhead::PTR );
      else if ( sources[ i ] == "bcpapi" )
        success = bcp_api::download_item( item );
    }
  }

  item.fetched = success;

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

unsigned item_t::parse_gem( item_t& item, unsigned gem_id )
{
  if ( gem_id == 0 )
    return GEM_NONE;

  unsigned type = GEM_NONE;
  const std::vector<std::string>& sources = item.parsed.source_list.size() > 0
                                            ? item.parsed.source_list
                                            : item.player -> sim -> item_db_sources;

  if ( cache::items() != cache::CURRENT )
  {
    for ( unsigned i = 0; type == GEM_NONE && i < sources.size(); i++ )
    {
      if ( sources[ i ] == "local" )
        type = item_database::parse_gem( item, gem_id );
      else if ( sources[ i ] == "wowhead" )
        type = wowhead::parse_gem( item, gem_id, wowhead::LIVE, cache::ONLY );
      else if ( sources[ i ] == "ptrhead" )
        type = wowhead::parse_gem( item, gem_id, wowhead::PTR, cache::ONLY );
      else if ( sources[ i ] == "bcpapi" )
        type = bcp_api::parse_gem( item, gem_id, cache::ONLY );
    }
  }

  if ( cache::items() != cache::ONLY )
  {
    // Nothing found from a cache, nor local item db. Let's fetch, again honoring our source list
    for ( unsigned i = 0; type == GEM_NONE && i < sources.size(); i++ )
    {
      if ( sources[ i ] == "wowhead" )
        type = wowhead::parse_gem( item, gem_id, wowhead::LIVE );
      else if ( sources[ i ] == "ptrhead" )
        type = wowhead::parse_gem( item, gem_id, wowhead::PTR );
      else if ( sources[ i ] == "bcpapi" )
        type = bcp_api::parse_gem( item, gem_id );
    }
  }

  return type;
}

// item_t::parse_reforge_id ==================================================

bool item_t::parse_reforge_id()
{
  if ( parsed.reforge_id == 0 )
    return true;

  int start = 0;
  int target = parsed.reforge_id;
  target %= 56;
  if ( target == 0 ) target = 56;
  else if ( target <= start ) return false;

  for ( int i=0; reforge_stats[ i ] != STAT_NONE; i++ )
  {
    for ( int j=0; reforge_stats[ j ] != STAT_NONE; j++ )
    {
      if ( i == j ) continue;
      if ( ++start == target )
      {
        parsed.reforged_from = reforge_stats[ i ];
        parsed.reforged_to = reforge_stats[ j ];

        return true;
      }
    }
  }

  return false;
}

// NEW ITEM STUFF REMOVED IN r<xxxxx>
