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

}

// item_t::item_t ===========================================================

item_t::item_t( player_t* p, const std::string& o ) :
  sim( p -> sim ),
  player( p ),
  slot( SLOT_INVALID ),
  unique( false ),
  unique_addon( false ),
  is_ptr( p -> dbc.ptr ),
  parsed(),
  xml(),
  options_str( o )
{
  parsed.data.name = name_str.c_str();
}

// item_t::has_stats =======================================================

bool item_t::has_stats()
{
  for ( size_t i = 0; i < sizeof_array( parsed.data.stat_type_e ); i++ )
  {
    if ( parsed.data.stat_type_e[ i ] > 0 )
      return true;
  }

  return false;
}

// item_t::socket_color_match ==============================================

bool item_t::socket_color_match() const
{
  for ( size_t i = 0, end = sizeof_array( parsed.data.socket_color ); i < end; i++ )
  {
    if ( parsed.data.socket_color[ i ] == SOCKET_COLOR_NONE )
      continue;

    // Empty gem slot, no bonus
    if ( parsed.gem_color[ i ] == SOCKET_COLOR_NONE )
      return false;

    // Socket color of the gem does not match the socket color of the item
    if ( ! ( parsed.gem_color[ i ] & parsed.data.socket_color[ i ] ) )
      return false;
  }

  return true;
}

// item_t::has_special_effect ==============================================

bool item_t::has_special_effect( special_effect_source_e source, special_effect_e type )
{
  return special_effect( source, type ).source != SPECIAL_EFFECT_SOURCE_NONE;
}


// item_t::special_effect ===================================================

const special_effect_t& item_t::special_effect( special_effect_source_e source, special_effect_e type )
{
  static special_effect_t nonevalue(this);

  // Note note returns first available, but odds that there are several on-
  // equip enchants in an item is slim to none
  for ( size_t i = 0; i < parsed.special_effects.size(); i++ )
  {
    if ( ( source == SPECIAL_EFFECT_SOURCE_NONE || parsed.special_effects[ i ].source == source ) && 
         ( type == SPECIAL_EFFECT_NONE || type == parsed.special_effects[ i ].type ) )
    {
      return parsed.special_effects[ i ];
    }
  }

  return nonevalue;
}

// item_t::to_string ========================================================

std::string item_t::to_string()
{
  std::ostringstream s;

  s << "name=" << name_str;
  s << " id=" << parsed.data.id;
  s << " slot=" << slot_name();
  s << " quality=" << util::item_quality_string( parsed.data.quality );
  s << " upgrade_level=" << upgrade_level();
  s << " ilevel=" << item_level();
  if ( sim -> scale_to_itemlevel > 0 )
    s << " (" << ( parsed.data.level + item_database::upgrade_ilevel( parsed.data, upgrade_level() ) ) << ")";
  else if ( upgrade_level() > 0 )
    s << " (" << parsed.data.level << ")";
  if ( parsed.data.lfr() )
    s << " LFR";
  if ( parsed.data.heroic() )
    s << " Heroic";
  if ( parsed.data.mythic() )
    s << " Mythic";
  if ( parsed.data.warforged() )
    s << " Warforged";
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

  if ( has_stats() )
  {
    s << " stats={ ";

    if ( parsed.armor > 0 )
      s << parsed.armor << " Armor, ";
    else if ( item_database::armor_value( *this ) )
      s << item_database::armor_value( *this ) << " Armor, ";

    for ( size_t i = 0; i < sizeof_array( parsed.data.stat_val ); i++ )
    {
      if ( parsed.data.stat_type_e[ i ] <= 0 )
        continue;

      int v = stat_value( i );
      if ( v == 0 )
        continue;

      if ( v > 0 )
        s << "+";

      s << v << " " << util::stat_type_abbrev( util::translate_item_mod( parsed.data.stat_type_e[ i ] ) ) << ", ";
    }
    std::streampos x = s.tellp(); s.seekp( x - std::streamoff( 2 ) );
    s << " }";
  }

  if ( parsed.suffix_stats.size() > 0 )
  {
    s << " suffix_stats={ ";

    for ( size_t i = 0; i < parsed.suffix_stats.size(); i++ )
    {
      s << "+" << parsed.suffix_stats[ i ].value
        << " " << util::stat_type_abbrev( parsed.suffix_stats[ i ].stat ) << ", ";
    }

    std::streampos x = s.tellp(); s.seekp( x - std::streamoff( 2 ) );
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

    std::streampos x = s.tellp(); s.seekp( x - std::streamoff( 2 ) );
    s << " }";
  }

  if ( socket_color_match() && parsed.socket_bonus_stats.size() > 0 )
  {
    s << " socket_bonus={ ";

    for ( size_t i = 0; i < parsed.socket_bonus_stats.size(); i++ )
    {
      s << "+" << parsed.socket_bonus_stats[ i ].value
        << " " << util::stat_type_abbrev( parsed.socket_bonus_stats[ i ].stat ) << ", ";
    }

    std::streampos x = s.tellp(); s.seekp( x - std::streamoff( 2 ) );
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

  if ( parsed.enchant_stats.size() > 0 && parsed.encoded_enchant.empty() )
  {
    s << " enchant={ ";

    for ( size_t i = 0; i < parsed.enchant_stats.size(); i++ )
    {
      s << "+" << parsed.enchant_stats[ i ].value << " "
        << util::stat_type_abbrev( parsed.enchant_stats[ i ].stat ) << ", ";
    }

    std::streampos x = s.tellp(); s.seekp( x - std::streamoff( 2 ) );
    s << " }";
  }
  else if ( ! parsed.encoded_enchant.empty() )
    s << " enchant={ " << parsed.encoded_enchant << " }";

  for ( size_t i = 0; i < parsed.special_effects.size(); i++ )
  {
    s << " effect={ ";
    s << parsed.special_effects[ i ].to_string();
    s << " }";
  }

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

    std::streampos x = s.tellp(); s.seekp( x - std::streamoff( 2 ) );
    s << " }";
  }

  return s.str();
}

// item_t::has_item_stat ====================================================

bool item_t::has_item_stat( stat_e stat ) const
{
  for ( size_t i = 0; i < sizeof_array( parsed.data.stat_val ); i++ )
  {
    if ( util::translate_item_mod( parsed.data.stat_type_e[ i ] ) == stat )
      return true;
  }

  return false;
}

// item_t::upgrade_level ====================================================

unsigned item_t::upgrade_level() const
{
  return parsed.upgrade_level + sim -> global_item_upgrade_level;
}

// item_t::item_level =======================================================

unsigned item_t::item_level() const
{
  if ( sim -> scale_to_itemlevel > 0 )
    return sim -> scale_to_itemlevel;
  else
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

bool item_t::active() const
{
  if ( slot == SLOT_INVALID ) return false;
  if ( ! ( name_str.empty() || name_str == "empty" || name_str == "none" ) ) return true;
  return false;
}

// item_t::name =============================================================

const char* item_t::name() const
{
  if ( ! option_name_str.empty() ) return option_name_str.c_str();
  if ( ! name_str.empty() ) return name_str.c_str();
  return "inactive";
}

// item_t::slot_name ========================================================

const char* item_t::slot_name() const
{
  return util::slot_type_string( slot );
}

// item_t::slot_name ========================================================

// item_t::weapon ===========================================================

weapon_t* item_t::weapon() const
{
  if ( slot == SLOT_MAIN_HAND ) return &( player -> main_hand_weapon );
  if ( slot == SLOT_OFF_HAND  ) return &( player ->  off_hand_weapon );
  return 0;
}

// item_t::inv_type =========================================================

inventory_type item_t::inv_type() const
{
  return static_cast<inventory_type>( parsed.data.inventory_type );
}

// item_t::parse_options ====================================================

bool item_t::parse_options()
{
  if ( options_str.empty() ) return true;

  option_name_str = options_str;
  std::string remainder = "";
  std::string DUMMY_REFORGE; // TODO-WOD: Remove once profiles update

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
    opt_string( "warforged", option_warforged_str ),
    opt_string( "lfr", option_lfr_str ),
    opt_string( "heroic", option_heroic_str ),
    opt_string( "mythic", option_mythic_str ),
    opt_string( "type", option_armor_type_str ),
    opt_string( "reforge", DUMMY_REFORGE ),
    opt_int( "suffix", parsed.suffix_id ),
    opt_string( "ilevel", option_ilevel_str ),
    opt_string( "quality", option_quality_str ),
    opt_string( "source", option_data_source_str ),
    opt_string( "gem_id", option_gem_id_str ),
    opt_string( "enchant_id", option_enchant_id_str ),
    opt_string( "addon_id", option_addon_id_str ),
    opt_string( "bonus_id", option_bonus_id_str ),
    opt_null()
  };

  if ( ! option_t::parse( sim, option_name_str.c_str(), options, remainder ) )
    return false;

  util::tokenize( option_name_str );

  util::tolower( option_stats_str            );
  util::tolower( option_gems_str             );
  util::tolower( option_enchant_str          );
  util::tolower( option_addon_str            );
  util::tolower( option_equip_str            );
  util::tolower( option_use_str              );
  util::tolower( option_weapon_str           );
  util::tolower( option_warforged_str        );
  util::tolower( option_lfr_str              );
  util::tolower( option_heroic_str           );
  util::tolower( option_mythic_str           );
  util::tolower( option_armor_type_str       );
  util::tolower( option_ilevel_str           );
  util::tolower( option_quality_str          );

  if ( ! option_gem_id_str.empty() )
  {
    std::vector<std::string> spl = util::string_split( option_gem_id_str, "/" );
    for ( size_t i = 0, end = std::min( sizeof_array( parsed.gem_id ), spl.size() ); i < end; i++ )
    {
      unsigned gem_id = util::to_unsigned( spl[ i ] );
      if ( gem_id == 0 )
        continue;

      parsed.gem_id[ i ] = gem_id;
    }
  }

  if ( ! option_enchant_id_str.empty() )
    parsed.enchant_id = util::to_unsigned( option_enchant_id_str );

  if ( ! option_addon_id_str.empty() )
    parsed.addon_id = util::to_unsigned( option_addon_id_str );

  if ( ! option_bonus_id_str.empty() )
  {
    std::vector<std::string> split = util::string_split( option_bonus_id_str, "/" );
    for ( size_t i = 0, end = split.size(); i < end; i++ )
    {
      int bonus_id = util::to_int( split[ i ] );
      if ( bonus_id <= 0 )
        continue;

      parsed.bonus_id.push_back( bonus_id );
    }
  }

  return true;
}

// item_t::encoded_item =====================================================

void item_t::encoded_item( xml_writer_t& writer )
{
  writer.begin_tag( "item" );
  writer.print_attribute( "name", name_str );

  if ( parsed.data.id )
    writer.print_attribute( "id", util::to_string( parsed.data.id ) );

  if ( parsed.bonus_id.size() > 0 )
  {
    std::string bonus_id_str;
    for ( size_t i = 0, end = parsed.bonus_id.size(); i < end; i++ )
    {
      bonus_id_str += util::to_string( parsed.bonus_id[ i ] );
      if ( i < parsed.bonus_id.size() - 1 )
        bonus_id_str += "/";
    }

    writer.print_attribute( "bonus_id", bonus_id_str );
  }

  if ( parsed.upgrade_level > 0 )
    writer.print_attribute( "upgrade_level", encoded_upgrade_level() );

  if ( parsed.suffix_id != 0 )
    writer.print_attribute( "suffix", encoded_random_suffix_id() );

  if ( parsed.gem_stats.size() > 0 || ( slot == SLOT_HEAD && player -> meta_gem != META_GEM_NONE ) )
    writer.print_attribute( "gems", encoded_gems() );

  if ( parsed.enchant_stats.size() > 0 || ! parsed.encoded_enchant.empty() )
    writer.print_attribute( "enchant", encoded_enchant() );

  if ( parsed.addon_stats.size() > 0 || ! parsed.encoded_addon.empty() )
    writer.print_attribute( "addon", encoded_addon() );

  writer.end_tag( "item" );
}

std::string item_t::encoded_item()
{
  std::ostringstream s;

  s << name_str;

  if ( parsed.data.id )
    s << ",id=" << parsed.data.id;

  if ( parsed.bonus_id.size() > 0 )
  {
    s << ",bonus_id=";
    for ( size_t i = 0, end = parsed.bonus_id.size(); i < end; i++ )
    {
      s << parsed.bonus_id[ i ];
      if ( i < parsed.bonus_id.size() - 1 )
        s << "/";
    }
  }

  if ( ! option_ilevel_str.empty() )
    s << ",ilevel=" << parsed.data.level;

  if ( ! option_armor_type_str.empty() )
    s << ",type=" << util::armor_type_string( parsed.data.item_subclass );

  if ( !option_warforged_str.empty() )
    s << ",warforged=" << ( parsed.data.warforged() ? 1 : 0 );

  if ( !option_lfr_str.empty() )
    s << ",lfr=" << ( parsed.data.lfr() ? 1 : 0 );

  if ( ! option_heroic_str.empty() )
    s << ",heroic=" << ( parsed.data.heroic() ? 1 : 0 );

  if ( ! option_mythic_str.empty() )
    s << ",mythic=" << ( parsed.data.mythic() ? 1 : 0 );

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

  // If gems= is given, always print it out
  if ( ! option_gems_str.empty() )
    s << ",gems=" << encoded_gems();
  // Print out gems= string to profiles, if there is no gem_id= given, and
  // there are gems to spit out.  Note that gem_id= option is also always
  // printed below, and if present, the gems= string will be found in "gear
  // comments" (enabled by save_gear_comments=1 option).
  else if ( option_gem_id_str.empty() && ( parsed.gem_stats.size() > 0 ||
      ( slot == SLOT_HEAD && player -> meta_gem != META_GEM_NONE ) ) )
    s << ",gems=" << encoded_gems();

  if ( ! option_gem_id_str.empty() )
    s << ",gem_id=" << option_gem_id_str;

  if ( ! option_enchant_str.empty() )
    s << ",enchant=" << encoded_enchant();
  else if ( option_enchant_id_str.empty() && 
            ( parsed.enchant_stats.size() > 0 || ! parsed.encoded_enchant.empty() ) )
    s << ",enchant=" << encoded_enchant();

  if ( ! option_enchant_id_str.empty() )
    s << ",enchant_id=" << option_enchant_id_str;

  if ( ! option_addon_str.empty() )
    s << ",addon=" << encoded_addon();
  else if (  option_addon_id_str.empty() &&
        ( parsed.addon_stats.size() > 0 || ! parsed.encoded_addon.empty() ) )
    s << ",addon=" << encoded_addon();

  if ( ! option_addon_id_str.empty() )
    s << ",addon_id=" << option_addon_id_str;

  if ( ! option_equip_str.empty() )
    s << ",equip=" << option_equip_str;

  if ( ! option_use_str.empty() )
    s << ",use=" << option_use_str;

  if ( ! option_data_source_str.empty() )
    s << ",source=" << option_data_source_str;

  return s.str();
}

// item_t::encoded_comment ==================================================

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

  if ( option_heroic_str.empty() && parsed.data.heroic() )
    s << "heroic=1,";

  if ( option_lfr_str.empty() && parsed.data.lfr() )
    s << "lfr=1,";

  if ( option_mythic_str.empty() && parsed.data.mythic() )
    s << "mythic=1,";

  if ( option_warforged_str.empty() && parsed.data.warforged() )
    s << "warforged=1,";

  if ( option_stats_str.empty() && ! encoded_stats().empty() )
    s << "stats=" << encoded_stats() << ",";

  if ( option_weapon_str.empty() && ! encoded_weapon().empty() )
    s << "weapon=" << encoded_weapon() << ",";

  // Print out encoded comment string if there's no gems= option given, and we
  // have something relevant to spit out
  if ( option_gems_str.empty() && ( parsed.gem_stats.size() > 0 ||
      ( slot == SLOT_HEAD && player -> meta_gem != META_GEM_NONE ) ) )
    s << "gems=" << encoded_gems() << ",";

  if ( option_enchant_str.empty() &&
       ( parsed.enchant_stats.size() > 0 || ! parsed.encoded_enchant.empty() ) )
    s << "enchant=" << encoded_enchant() << ",";

  if ( option_addon_str.empty() &&
       ( parsed.addon_stats.size() > 0 || ! parsed.encoded_addon.empty() ) )
    s << "enchant=" << encoded_addon() << ",";

  if ( option_equip_str.empty() )
  {
    for ( size_t i = 0; i < parsed.special_effects.size(); i++ )
    {
      const special_effect_t& se = parsed.special_effects[ i ];
      if ( se.type == SPECIAL_EFFECT_EQUIP && se.source == SPECIAL_EFFECT_SOURCE_ITEM )
        s << "equip=" << se.encoding_str << ",";
    }
  }

  if ( option_use_str.empty() )
  {
    for ( size_t i = 0; i < parsed.special_effects.size(); i++ )
    {
      const special_effect_t& se = parsed.special_effects[ i ];
      if ( se.type == SPECIAL_EFFECT_USE && se.source == SPECIAL_EFFECT_SOURCE_ITEM )
        s << "use=" << se.encoding_str << ",";
    }
  }

  std::string str = s.str();
  if ( ! str.empty() )
    str.erase( str.end() - 1 );
  return str;
}


// item_t::encoded_gems =====================================================

std::string item_t::encoded_gems()
{
  if ( ! option_gems_str.empty() )
    return option_gems_str;

  std::string stats_str = stat_pairs_to_str( parsed.gem_stats );
  if ( socket_color_match() && parsed.socket_bonus_stats.size() > 0 )
  {
    if ( ! stats_str.empty() )
      stats_str += "_";

    stats_str += stat_pairs_to_str( parsed.socket_bonus_stats );
  }

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

// item_t::encoded_enchant ==================================================

std::string item_t::encoded_enchant()
{
  if ( ! option_enchant_str.empty() )
    return option_enchant_str;

  if ( ! parsed.encoded_enchant.empty() )
    return parsed.encoded_enchant;

  return stat_pairs_to_str( parsed.enchant_stats );
}

// item_t::encoded_addon ====================================================

std::string item_t::encoded_addon()
{
  if ( ! option_addon_str.empty() )
    return option_addon_str;

  if ( ! parsed.encoded_addon.empty() )
    return parsed.encoded_addon;

  return stat_pairs_to_str( parsed.addon_stats );
}

// item_t::encoded_upgrade_level ============================================

std::string item_t::encoded_upgrade_level()
{
  std::string upgrade_level_str;
  if ( parsed.upgrade_level > 0 )
    upgrade_level_str = util::to_string( parsed.upgrade_level );

  return upgrade_level_str;
}

// item_t::encoded_random_suffix_id =========================================

std::string item_t::encoded_random_suffix_id()
{
  std::string str;

  if ( parsed.suffix_id != 0 )
    str += util::to_string( parsed.suffix_id );

  return str;
}

// item_t::encoded_stats ====================================================

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

// item_t::encoded_weapon ===================================================

std::string item_t::encoded_weapon()
{
  if ( ! option_weapon_str.empty() )
    return option_weapon_str;

  std::string str;

  if ( parsed.data.item_class != ITEM_CLASS_WEAPON )
    return str;

  double speed     = parsed.data.delay / 1000.0;
  // Note the use of item_database::weapon_dmg_min( item_t& ) here, it's used
  // insted of the weapon_dmg_min( item_data_t*, dbc_t&, unsigned ) variant,
  // since we want the normal weapon stats of the item in the encoded option
  // string.
  unsigned min_dam = item_database::weapon_dmg_min( &parsed.data, player -> dbc );
  unsigned max_dam = item_database::weapon_dmg_max( &parsed.data, player -> dbc );

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
  if ( ! parse_options() )
    return false;

  // Item specific source list has to be decoded first so we can properly
  // download the item from the correct source
  if ( ! decode_data_source() )
    return false;

  if ( parsed.data.id > 0 )
  {
    if ( ! item_t::download_item( *this ) &&
         option_stats_str.empty() && option_weapon_str.empty() )
      return false;
  }
  else
    name_str = option_name_str;

  if ( name_str.empty() || name_str == "empty" || name_str == "none" )
    return true;

  // Process basic stats
  if ( ! decode_warforged()                        ) return false;
  if ( ! decode_lfr()                              ) return false;
  if ( ! decode_heroic()                           ) return false;
  if ( ! decode_mythic()                           ) return false;
  if ( ! decode_quality()                          ) return false;
  if ( ! decode_ilevel()                           ) return false;
  if ( ! decode_armor_type()                       ) return false;

  if ( parsed.upgrade_level > 0 && ( ! parsed.data.quality || ! parsed.data.level ) )
    sim -> errorf( "Player %s upgrading item %s at slot %s without quality or ilevel, upgrading will not work\n",
                   player -> name(), name(), slot_name() );

  // Process complex input, and initialize item in earnest
  if ( ! decode_stats()                            ) return false;
  if ( ! decode_gems()                             ) return false;
  if ( ! decode_weapon()                           ) return false;
  if ( ! decode_random_suffix()                    ) return false;
  if ( ! decode_equip_effect()                     ) return false;
  if ( ! decode_use_effect()                       ) return false;
  if ( ! decode_enchant()                          ) return false;
  if ( ! decode_addon()                            ) return false;

  if ( ! option_name_str.empty() && ( option_name_str != name_str ) )
  {
    sim -> errorf( "Player %s at slot %s has inconsistency between name '%s' and '%s' for id %u\n",
                   player -> name(), slot_name(), option_name_str.c_str(), name_str.c_str(), parsed.data.id );

    name_str = option_name_str;
  }

  if ( source_str.empty() )
    source_str = "Manual";

  if ( sim -> debug )
    sim -> out_debug.printf( "%s", to_string().c_str() );

  return true;
}

// item_t::decode_heroic ====================================================

bool item_t::decode_heroic()
{
  if ( ! option_heroic_str.empty() )
    parsed.data.type_flags |= RAID_TYPE_HEROIC;

  return true;
}

// item_t::decode_lfr =======================================================

bool item_t::decode_lfr()
{
  if ( ! option_lfr_str.empty() )
    parsed.data.type_flags |= RAID_TYPE_LFR;

  return true;
}

// item_t::decode_mythic =====================================================

bool item_t::decode_mythic()
{
  if ( ! option_mythic_str.empty() )
    parsed.data.type_flags |= RAID_TYPE_MYTHIC;

  return true;
}

// item_t::decode_warforged ==================================================

bool item_t::decode_warforged()
{
  if ( ! option_warforged_str.empty() )
    parsed.data.type_flags |= RAID_TYPE_WARFORGED;

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

// item_t::is_valid_type ====================================================

bool item_t::is_valid_type()
{
  if ( ! util::is_match_slot( slot ) )
    return true;

  return util::matching_armor_type( player -> type ) >= parsed.data.item_subclass;
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

    std::vector<item_database::token_t> tokens;
    size_t num_tokens = item_database::parse_tokens( tokens, option_stats_str );
    size_t stat = 0;

    for ( size_t i = 0; i < num_tokens && stat < sizeof_array( parsed.data.stat_val ); i++ )
    {
      stat_e s = util::parse_stat_type( tokens[ i ].name );
      if ( s == STAT_NONE )
      {
        sim -> errorf( "Player %s has unknown 'stats=' token '%s' at slot %s\n",
                       player -> name(), tokens[ i ].full.c_str(), slot_name() );
        return false;
      }

      if ( s != STAT_ARMOR )
      {
        parsed.data.stat_type_e[ stat ] = util::translate_stat( s );
        parsed.data.stat_val[ stat ] = static_cast<int>( tokens[ i ].value );
        stat++;
      }
      else
        parsed.armor = static_cast<int>( tokens[ i ].value );
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
    sim -> out_debug.printf( "random_suffix: item=%s parsed.suffix_id=%d ilevel=%d quality=%d random_point_pool=%d",
                   name(), parsed.suffix_id, item_level(), parsed.data.quality, f );
  }

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

      stat_pair_t stat = item_database::item_enchantment_effect_stats( enchant_data, as<int>( j ) );
      if ( stat.stat == STAT_NONE )
        continue;

      bool has_stat = false;
      size_t free_idx = 0;

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
          sim -> out_debug.printf( "random_suffix: Player %s item %s attempted to add base stat %u %s (%d) twice, due to random suffix.",
                         player -> name(), name(), as<unsigned>( j ), util::stat_type_abbrev( stat.stat ), enchant_data.ench_type[ j ] );
        continue;
      }

      if ( sim -> debug )
        sim -> out_debug.printf( "random_suffix: stat=%d (%s) stat_amount=%f", stat.stat, util::stat_type_abbrev( stat.stat ), stat_amount );

      stat.value = static_cast<int>( stat_amount );
      parsed.suffix_stats.push_back( stat );
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
  // Parse user given gems= string. Stats are parsed as is, meta gem through
  // DBC data
  if ( ! option_gems_str.empty() && option_gems_str != "none" )
  {
    parsed.gem_stats.clear();

    // Detect meta gem through DBC data, instead of clunky prefix matching
    const item_enchantment_data_t& meta_gem_enchant = enchant::find_meta_gem( player -> dbc, option_gems_str );
    meta_gem_e meta_gem = enchant::meta_gem_type( player -> dbc, meta_gem_enchant );

    if ( meta_gem != META_GEM_NONE )
    {
      player -> meta_gem = meta_gem;
      // Init meta gem based on spell data
      if ( ! enchant::initialize_item_enchant( *this, parsed.meta_gem_stats, SPECIAL_EFFECT_SOURCE_GEM, meta_gem_enchant ) )
        return false;
    }

    std::vector<item_database::token_t> tokens;
    size_t num_tokens = item_database::parse_tokens( tokens, option_gems_str );

    for ( size_t i = 0; i < num_tokens; i++ )
    {
      item_database::token_t& t = tokens[ i ];
      stat_e s;

      if ( ( s = util::parse_stat_type( t.name ) ) != STAT_NONE )
        parsed.gem_stats.push_back( stat_pair_t( s, static_cast<int>( t.value ) ) );
    }
  }
  // Parse gem_ids through DBC data
  else
  {
    for ( size_t i = 0, end = sizeof_array( parsed.gem_id ); i < end; i++ )
    {
      if ( parsed.gem_id[ i ] == 0 )
        continue;

      parsed.gem_color[ i ] = enchant::initialize_gem( *this, parsed.gem_id[ i ] );
    }
  }

  for ( size_t i = 0; i < parsed.gem_stats.size(); i++ )
    stats.add_stat( parsed.gem_stats[ i ].stat, parsed.gem_stats[ i ].value );

  // Meta gem stats need to be separated to avoid double dipping the static stats portion of the gem.
  for ( size_t i = 0; i < parsed.meta_gem_stats.size(); i++ )
    stats.add_stat( parsed.meta_gem_stats[ i ].stat, parsed.meta_gem_stats[ i ].value );

  if ( socket_color_match() )
  {
    for ( size_t i = 0; i < parsed.socket_bonus_stats.size(); i++ )
      stats.add_stat( parsed.socket_bonus_stats[ i ].stat, parsed.socket_bonus_stats[ i ].value );
  }

  return true;
}

// item_t::decode_equip_effect ==============================================

bool item_t::decode_equip_effect()
{
  special_effect_t effect( this );
  bool ret = false;
  int effects = 0;

  // If the user has defined an equip= string, use it
  if ( ! option_equip_str.empty() )
  {
    if ( ( ret = proc::parse_special_effect_encoding( effect, *this, option_equip_str ) ) )
    {
      effect.type = SPECIAL_EFFECT_EQUIP;
      effect.source = SPECIAL_EFFECT_SOURCE_ITEM;
      parsed.special_effects.push_back( effect );
    }
  }
  // Otherwise, use fancy schmancy database
  else
  {
    for ( size_t i = 0, end = sizeof_array( parsed.data.id_spell ); i < end; i++ )
    {
      if ( parsed.data.trigger_spell[ i ] != ITEM_SPELLTRIGGER_ON_EQUIP )
        continue;

      if ( parsed.data.id_spell[ i ] == 0 )
        continue;

      effect.reset();
      effect.type = SPECIAL_EFFECT_EQUIP;
      effect.source = SPECIAL_EFFECT_SOURCE_ITEM;

      // First phase initialization of the special effect. Sets up the relevant
      // fields in special_effect_t to create the proc. In the future, most of 
      // the manual field setup can also be removed ...
      if ( ( ret = unique_gear::initialize_special_effect( effect, *this, parsed.data.id_spell[ i ] ) ) && 
           effect.type != SPECIAL_EFFECT_NONE )
      {
        parsed.special_effects.push_back( effect );
        effects++;
      }
    }
  }

  return ret || effects == 0;
}

// item_t::decode_use_effect ================================================

bool item_t::decode_use_effect()
{
  special_effect_t effect( this );
  bool ret = false;
  int effects = 0;

  // If the user has defined an use= string, use it
  if ( ! option_use_str.empty() )
  {
    if ( ( ret = proc::parse_special_effect_encoding( effect, *this, option_use_str ) ) )
    {
      effect.type = SPECIAL_EFFECT_USE;
      effect.source = SPECIAL_EFFECT_SOURCE_ITEM;
      parsed.special_effects.push_back( effect );
    }
  }
  // Otherwise, use fancy schmancy database
  else
  {
    for ( size_t i = 0, end = sizeof_array( parsed.data.id_spell ); i < end; i++ )
    {
      if ( parsed.data.trigger_spell[ i ] != ITEM_SPELLTRIGGER_ON_USE )
        continue;

      if ( parsed.data.id_spell[ i ] == 0 )
        continue;

      effect.reset();
      effect.type = SPECIAL_EFFECT_USE;
      effect.source = SPECIAL_EFFECT_SOURCE_ITEM;
      // First phase initialization of the special effect. Sets up the relevant
      // fields in special_effect_t to create the use effect. In the future,
      // most of the manual field setup can also be removed ...
      if ( ( ret = unique_gear::initialize_special_effect( effect, *this, parsed.data.id_spell[ i ] ) ) && 
           effect.type != SPECIAL_EFFECT_NONE )
      {
        parsed.special_effects.push_back( effect );
        effects++;
      }
      else
        break;
    }
  }

  return ret || effects == 0;
}

// item_t::decode_enchant ===================================================

bool item_t::decode_enchant()
{
  // If the user gives an enchant, override everything fetched from the
  // data sources
  if ( ! option_enchant_str.empty() && option_enchant_str != "none" )
  {
    parsed.enchant_stats = str_to_stat_pair( option_enchant_str );

    // No stats, this is a special enchant that we need to parse out
    if ( parsed.enchant_stats.size() == 0 )
    {
      const item_enchantment_data_t& enchant_data = enchant::find_item_enchant( *this, option_enchant_str );
      if ( ! enchant::initialize_item_enchant( *this, parsed.enchant_stats, SPECIAL_EFFECT_SOURCE_ENCHANT, enchant_data ) )
        return false;
    }
  }
  // .. Otherwise, parse enchant based on DBC information and enchant_id
  else if ( parsed.enchant_id > 0 )
  {
    const item_enchantment_data_t& enchant_data = player -> dbc.item_enchantment( parsed.enchant_id );
    if ( ! enchant::initialize_item_enchant( *this, parsed.enchant_stats, SPECIAL_EFFECT_SOURCE_ENCHANT, enchant_data ) )
      return false;
  }

  for ( size_t i = 0; i < parsed.enchant_stats.size(); i++ )
    stats.add_stat( parsed.enchant_stats[ i ].stat, parsed.enchant_stats[ i ].value );

  return true;
}

// item_t::decode_addon =====================================================

bool item_t::decode_addon()
{
  // If the user gives an addon, override everything fetched from the
  // data sources
  if ( ! option_addon_str.empty() )
  {
    parsed.addon_stats = str_to_stat_pair( option_addon_str );

    // No stats, this is a special enchant that we need to parse out
    if ( parsed.addon_stats.size() == 0 )
    {
      const item_enchantment_data_t& enchant_data = enchant::find_item_enchant( *this, option_addon_str );
      if ( ! enchant::initialize_item_enchant( *this, parsed.addon_stats, SPECIAL_EFFECT_SOURCE_ADDON, enchant_data ) )
        return false;
    }
  }
  // .. Otherwise, parse addon based on DBC information and enchant_id
  else if ( parsed.addon_id > 0 )
  {
    special_effect_t effect( this );
    if ( ! item_database::parse_item_spell_enchant( *this, parsed.addon_stats, effect, parsed.addon_id ) )
      sim -> errorf( "Player %s unable to parse addon id %u for item \"%s\" at slot %s.\n",
          player -> name(), parsed.addon_id, name(), slot_name() );

    if ( parsed.addon_stats.size() == 0 )
    {
      const item_enchantment_data_t& enchant_data = player -> dbc.item_enchantment( parsed.addon_id );
      if ( ! enchant::initialize_item_enchant( *this, parsed.addon_stats, SPECIAL_EFFECT_SOURCE_ADDON, enchant_data ) )
        return false;
    }
  }

  for ( size_t i = 0; i < parsed.addon_stats.size(); i++ )
    stats.add_stat( parsed.addon_stats[ i ].stat, parsed.addon_stats[ i ].value );

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
    std::vector<item_database::token_t> tokens;
    size_t num_tokens = item_database::parse_tokens( tokens, option_weapon_str );

    bool dps_set = false, dmg_set = false, min_set = false, max_set = false;

    for ( size_t i = 0; i < num_tokens; i++ )
    {
      item_database::token_t& t = tokens[ i ];
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
    parsed.data.delay = static_cast<double>( w -> swing_time.total_millis() );
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

// item_t::decode_data_source ===============================================

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

// item_t::str_to_stat_pair =================================================

std::vector<stat_pair_t> item_t::str_to_stat_pair( const std::string& stat_str )
{
  std::vector<item_database::token_t> tokens;
  std::vector<stat_pair_t> stats;

  item_database::parse_tokens( tokens, stat_str );
  for ( size_t i = 0; i < tokens.size(); i++ )
  {
    stat_e s = STAT_NONE;
    if ( ( s = util::parse_stat_type( tokens[ i ].name ) ) != STAT_NONE && tokens[ i ].value != 0 )
      stats.push_back( stat_pair_t( s, static_cast<int>( tokens[ i ].value ) ) );
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
#if SC_BETA
      else if ( sources[ i ] == SC_BETA_STR "head" )
        success = wowhead::download_item( item, wowhead::BETA, cache::ONLY );
#endif
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
#if SC_BETA
      else if ( sources[ i ] == SC_BETA_STR "head" )
        success = wowhead::download_item( item, wowhead::BETA );
#endif
      else if ( sources[ i ] == "bcpapi" )
        success = bcp_api::download_item( item );
    }
  }

  // Post process data by figuring out socket bonus of the item, if the
  // identifier is set. BCP API does not provide us with the ID, so
  // bcp_api::download_item has already filled parsed.socket_bonus_stats. Both
  // local and wowhead provide a id, that needs to be parsed into
  // parsed.socket_bonus_stats.
  if ( success && item.parsed.socket_bonus_stats.size() == 0 &&
       item.parsed.data.id_socket_bonus > 0 )
  {
    const item_enchantment_data_t& bonus = item.player -> dbc.item_enchantment( item.parsed.data.id_socket_bonus );
    success = enchant::initialize_item_enchant( item, item.parsed.socket_bonus_stats, SPECIAL_EFFECT_SOURCE_SOCKET_BONUS, bonus );
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
#if SC_BETA
      else if ( src == SC_BETA_STR "head" )
        success = wowhead::download_glyph( player, glyph_name, glyph_id, wowhead::BETA, cache::ONLY );
#endif
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
#if SC_BETA
      else if ( src == SC_BETA_STR "head" )
        success = wowhead::download_glyph( player, glyph_name, glyph_id, wowhead::BETA );
#endif
      else if ( src == "bcpapi" )
        success = bcp_api::download_glyph( player, glyph_name, glyph_id );
    }
  }

  util::glyph_name( glyph_name );

  return success;
}

