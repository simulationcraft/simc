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
  parent_slot( SLOT_INVALID ),
  unique( false ),
  unique_addon( false ),
  is_ptr( p -> dbc.ptr ),
  parsed(),
  xml(),
  options_str( o ), option_initial_cd(0),
  cached_upgrade_item_level( -1 )
{
  parsed.data.name = name_str.c_str();
}

// item_t::has_stats =======================================================

bool item_t::has_stats() const
{
  for ( size_t i = 0; i < sizeof_array( parsed.data.stat_type_e ); i++ )
  {
    if ( parsed.data.stat_type_e[ i ] > 0 )
      return true;
  }

  return false;
}

// item_t::has_scaling_stat_bonus_id =======================================

bool item_t::has_scaling_stat_bonus_id() const
{
  for ( auto bonus_id : parsed.bonus_id )
  {
    std::vector<const item_bonus_entry_t*> bonuses = player -> dbc.item_bonus( bonus_id );
    if ( std::find_if( bonuses.begin(), bonuses.end(), []( const item_bonus_entry_t* e )
          { return e -> type == ITEM_BONUS_SCALING; } ) != bonuses.end() )
    {
      return true;
    }
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

bool item_t::has_special_effect( special_effect_source_e source, special_effect_e type ) const
{
  return special_effect( source, type ) != nullptr;
}


// item_t::special_effect ===================================================

const special_effect_t* item_t::special_effect( special_effect_source_e source, special_effect_e type ) const
{
  // Note note returns first available, but odds that there are several on-
  // equip enchants in an item is slim to none
  for ( size_t i = 0; i < parsed.special_effects.size(); i++ )
  {
    if ( ( source == SPECIAL_EFFECT_SOURCE_NONE || parsed.special_effects[ i ] -> source == source ) &&
         ( type == SPECIAL_EFFECT_NONE || type == parsed.special_effects[ i ] -> type ) )
    {
      return parsed.special_effects[ i ];
    }
  }

  return nullptr;
}

gear_stats_t item_t::total_stats() const
{
  gear_stats_t total_stats;

  for ( stat_e stat = STAT_NONE; stat < STAT_MAX; ++stat )
  {
    // Ensure when returning gear stats that they are all converted to correct stat types in case of
    // hybrid stats. The simulator core will not understand, nor treat hybrid stats in any
    // meaningful way.
    stat_e to = player -> convert_hybrid_stat( stat );

    total_stats.add_stat( to, stats.get_stat( stat ) );

    range::for_each( parsed.enchant_stats, [ stat, to, &total_stats ]( const stat_pair_t& s ) {
      if ( stat == s.stat ) total_stats.add_stat( to, s.value );
    });

    range::for_each( parsed.gem_stats, [ stat, to, &total_stats ]( const stat_pair_t& s ) {
      if ( stat == s.stat ) total_stats.add_stat( to, s.value );
    });

    range::for_each( parsed.meta_gem_stats, [ stat, to, &total_stats ]( const stat_pair_t& s ) {
      if ( stat == s.stat ) total_stats.add_stat( to, s.value );
    });

    range::for_each( parsed.socket_bonus_stats, [ stat, to, &total_stats ]( const stat_pair_t& s ) {
      if ( stat == s.stat ) total_stats.add_stat( to, s.value );
    });

    range::for_each( parsed.addon_stats, [ stat, to, &total_stats ]( const stat_pair_t& s ) {
      if ( stat == s.stat ) total_stats.add_stat( to, s.value );
    });
  }

  return total_stats;
}

// item_t::to_string ========================================================

std::string item_t::item_stats_str() const
{
  std::ostringstream s;
  if ( parsed.armor > 0 )
    s << parsed.armor << " Armor, ";
  else if ( item_database::armor_value( *this ) )
    s << item_database::armor_value( *this ) << " Armor, ";

  for ( size_t i = 0; i < sizeof_array( parsed.data.stat_type_e ); i++ )
  {
    if ( parsed.data.stat_type_e[ i ] <= 0 )
      continue;

    int v = stat_value( i );
    stat_e stat = util::translate_item_mod( parsed.data.stat_type_e[ i ] );

    if ( v == 0 )
      continue;

    if ( v > 0 )
      s << "+";

    s << v << " " << util::stat_type_abbrev( stat ) << ", ";
  }

  std::string str = s.str();
  if ( str.size() > 2 )
  {
    str.erase( str.end() - 2, str.end() );
  }

  return str;
}

std::string item_t::weapon_stats_str() const
{
  if ( ! weapon() )
  {
    return std::string();
  }

  std::ostringstream s;

  weapon_t* w = weapon();
  s << w -> min_dmg;
  s << " - ";
  s << w -> max_dmg;

  s << ", ";
  s << w -> swing_time.total_seconds();

  return s.str();
}

std::string item_t::gem_stats_str() const
{
  if ( parsed.gem_stats.size() == 0 )
  {
    return std::string();
  }

  std::ostringstream s;

  for ( size_t i = 0; i < parsed.gem_stats.size(); i++ )
  {
    s << "+" << parsed.gem_stats[ i ].value
      << " " << util::stat_type_abbrev( parsed.gem_stats[ i ].stat ) << ", ";
  }

  std::string str = s.str();
  str.erase( str.end() - 2, str.end() );

  return str;
}

std::string item_t::enchant_stats_str() const
{
  if ( parsed.enchant_stats.size() == 0 )
  {
    return std::string();
  }

  std::ostringstream s;

  for ( size_t i = 0; i < parsed.enchant_stats.size(); i++ )
  {
    s << "+" << parsed.enchant_stats[ i ].value
      << " " << util::stat_type_abbrev( parsed.enchant_stats[ i ].stat ) << ", ";
  }

  std::string str = s.str();
  str.erase( str.end() - 2, str.end() );

  return str;
}

std::string item_t::socket_bonus_stats_str() const
{
  if ( parsed.socket_bonus_stats.size() == 0 )
  {
    return std::string();
  }

  std::ostringstream s;

  for ( size_t i = 0; i < parsed.socket_bonus_stats.size(); i++ )
  {
    s << "+" << parsed.socket_bonus_stats[ i ].value
      << " " << util::stat_type_abbrev( parsed.socket_bonus_stats[ i ].stat ) << ", ";
  }

  std::string str = s.str();
  str.erase( str.end() - 2, str.end() );

  return str;
}

std::ostream& operator<<(std::ostream& s, const item_t& item )
{

  s << "name=" << item.name_str;
  s << " id=" << item.parsed.data.id;
  if ( item.slot != SLOT_INVALID )
  {
    s << " slot=" << item.slot_name();
  }
  s << " quality=" << util::item_quality_string( item.parsed.data.quality );
  if ( item.upgrade_level() > 0 )
  {
    s << " upgrade_level=" << item.upgrade_level();
  }
  s << " ilevel=" << item.item_level();
  if ( item.parent_slot == SLOT_INVALID )
  {
    if ( item.sim -> scale_to_itemlevel > 0 )
      s << " (" << ( item.parsed.data.level + item_database::upgrade_ilevel( item, item.upgrade_level() ) ) << ")";
    else if ( item.upgrade_level() > 0 )
      s << " (" << item.parsed.data.level << ")";
    if ( item.parsed.drop_level > 0 )
      s << " drop_level=" << item.parsed.drop_level;
  }
  else
  {
    s << " (" << item.player -> items[ item.parent_slot ].slot_name() << ")";
  }

  if ( item.parsed.azerite_level > 0 )
  {
    s << " azerite_level=" << item.parsed.azerite_level;
  }

  if ( item.parsed.data.lfr() )
    s << " LFR";
  if ( item.parsed.data.heroic() )
    s << " Heroic";
  if ( item.parsed.data.mythic() )
    s << " Mythic";
  if ( item.parsed.data.warforged() )
    s << " Warforged";
  if ( util::is_match_slot( item.slot ) )
    s << " match=" << item.is_matching_type();

  bool is_weapon = false;
  if ( item.parsed.data.item_class == ITEM_CLASS_ARMOR )
    s << " type=armor/" << util::armor_type_string( item.parsed.data.item_subclass );
  else if ( item.parsed.data.item_class == ITEM_CLASS_WEAPON )
  {
    std::string class_str;
    if ( util::weapon_class_string( item.parsed.data.inventory_type ) )
      class_str = util::weapon_class_string( item.parsed.data.inventory_type );
    else
      class_str = "Unknown";
    std::string str = util::weapon_subclass_string( item.parsed.data.item_subclass );
    util::tokenize( str );
    util::tokenize( class_str );
    s << " type=" << class_str << "/" << str;
    is_weapon = true;
  }

  if ( item.has_stats() )
  {
    s << " stats={ ";
    s << item.item_stats_str();
    s << " }";
  }

  if ( item.parsed.gem_stats.size() > 0 )
  {
    s << " gems={ ";
    s << item.gem_stats_str();
    s << " }";
  }

  if ( item.socket_color_match() && item.parsed.socket_bonus_stats.size() > 0 )
  {
    s << " socket_bonus={ ";
    s << item.socket_bonus_stats_str();
    s << " }";
  }

  if ( is_weapon )
  {
    s << " damage={ ";
    weapon_t* w = item.weapon();
    s << w -> min_dmg;
    s << " - ";
    s << w -> max_dmg;
    s << " }";

    s << " speed=";
    s << w -> swing_time.total_seconds();
  }

  if ( item.parsed.enchant_stats.size() > 0 && item.parsed.encoded_enchant.empty() )
  {
    s << " enchant={ ";
    s << item.enchant_stats_str();
    s << " }";
  }
  else if ( ! item.parsed.encoded_enchant.empty() )
    s << " enchant={ " << item.parsed.encoded_enchant << " }";

  for ( size_t i = 0; i < item.parsed.special_effects.size(); i++ )
  {
    s << " effect={ ";
    s << item.parsed.special_effects[ i ] -> to_string();
    s << " }";
  }

  if ( ! item.source_str.empty() )
    s << " source=" << item.source_str;

  bool has_spells = false;
  for ( auto& spell_id : item.parsed.data.id_spell )
  {
    if ( spell_id > 0 )
    {
      has_spells = true;
      break;
    }
  }

  if ( has_spells )
  {
    s << " proc_spells={ ";
    for ( size_t i = 0; i < sizeof_array( item.parsed.data.id_spell ); i++ )
    {
      if ( item.parsed.data.id_spell[ i ] <= 0 )
        continue;

      s << "proc=";
      switch ( item.parsed.data.trigger_spell[ i ] )
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
      s << "/" << item.parsed.data.id_spell[ i ] << ", ";
    }

    std::streampos x = s.tellp(); s.seekp( x - std::streamoff( 2 ) );
    s << " }";
  }

  return s;
}

std::string item_t::to_string() const
{

  std::ostringstream s;
  s << *this;
  return s.str();
}

// item_t::has_item_stat ====================================================

bool item_t::has_item_stat( stat_e stat ) const
{
  for ( size_t i = 0; i < sizeof_array( parsed.data.stat_type_e ); i++ )
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

unsigned item_t::upgrade_item_level() const
{
  // upgrade_ilevel call is expensive, thus cache it.
  if ( cached_upgrade_item_level < 0 )
  {
    cached_upgrade_item_level = item_database::upgrade_ilevel( *this, upgrade_level() );
  }
  assert( as<unsigned>(cached_upgrade_item_level) == item_database::upgrade_ilevel( *this, upgrade_level() ) );
  return as<unsigned>(cached_upgrade_item_level);
}

// item_t::item_level =======================================================

unsigned item_t::item_level() const
{
  // If the item is a child, always override with whatever the parent is doing
  if ( parent_slot != SLOT_INVALID )
    return player -> items[ parent_slot ].item_level();

  if ( sim -> scale_to_itemlevel > 0 && ! sim -> scale_itemlevel_down_only )
    return sim -> scale_to_itemlevel;

  unsigned ilvl;

  // Overridden Option-based item level
  if ( parsed.item_level > 0 )
  {
    ilvl = parsed.item_level;
  }
  // Otherwise, normal ilevel processing (base ilevel + upgrade ilevel + artifact ilevel increase)
  else
  {
    ilvl = parsed.data.level + upgrade_item_level();
    if ( slot == player -> artifact -> slot() )
    {
      ilvl += player -> artifact -> ilevel_increase();
    }
  }

  if ( sim -> scale_to_itemlevel > 0 && sim -> scale_itemlevel_down_only )
    return std::min( (unsigned) sim -> scale_to_itemlevel, ilvl );

  return parsed.item_level == 0
         //? static_cast<unsigned>(dbc::item_level_squish( ilvl, player -> dbc.ptr ))
         ? ilvl
         : parsed.item_level;
}

unsigned item_t::base_item_level() const
{
  if ( sim -> scale_to_itemlevel > 0 )
    return sim -> scale_to_itemlevel;
  else if ( parsed.item_level > 0 )
  {
    return parsed.item_level;
  }
  else if ( parsed.azerite_level > 0 )
  {
    return player -> dbc.azerite_item_level( parsed.azerite_level );
  }
  else
    return parsed.data.level;
}

stat_e item_t::stat( size_t idx ) const
{
  if ( idx >= sizeof_array( parsed.data.stat_type_e ) )
    return STAT_NONE;

  return util::translate_item_mod( parsed.data.stat_type_e[ idx ] );
}

int item_t::stat_value( size_t idx ) const
{
  if ( idx >= sizeof_array( parsed.data.stat_type_e ) - 1 )
    return -1;

  return item_database::scaled_stat( *this, player -> dbc, idx, item_level() );
}

// item_t::active ===========================================================

bool item_t::active() const
{
  if ( slot == SLOT_INVALID ) return false;
  if ( ! ( name_str.empty() || name_str == "empty" || name_str == "none" || name_str == "nothing" ) ) return true;
  return false;
}

// item_t::name =============================================================

const char* item_t::name() const
{
  if ( ! option_name_str.empty() ) return option_name_str.c_str();
  if ( ! name_str.empty() ) return name_str.c_str();
  return "inactive";
}

// item_t::full_name ========================================================

std::string item_t::full_name() const
{
  std::string n;

  if ( const item_data_t* item = sim -> dbc.item( parsed.data.id ) )
  {
    n = item -> name;
  }
  else
  {
    n = name_str;
  }

  for ( auto bonus_id : parsed.bonus_id )
  {
    std::vector<const item_bonus_entry_t*> bonuses = sim -> dbc.item_bonus( bonus_id );
    for ( const auto bonus : bonuses )
    {
      if ( bonus -> type != ITEM_BONUS_SUFFIX )
      {
        continue;
      }

      const char* suffix_name = dbc::item_name_description( bonus -> value_1, sim -> dbc.ptr );
      if ( suffix_name )
      {
        n += " ";
        n += suffix_name;
      }
    }
  }

  if ( parsed.data.id == 0 )
  {
    util::tokenize( n );
  }

  return n;
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
  return nullptr;
}

// item_t::inv_type =========================================================

inventory_type item_t::inv_type() const
{
  return static_cast<inventory_type>( parsed.data.inventory_type );
}

// item_t::parse_options ====================================================

void item_t::parse_options()
{
  if ( options_str.empty() )
    return;

  option_name_str = options_str;
  std::string remainder = "";
  std::string DUMMY_REFORGE; // TODO-WOD: Remove once profiles update
  std::string DUMMY_CONTEXT; // not used by simc but used by 3rd parties (raidbots)

  std::string::size_type cut_pt = options_str.find_first_of( "," );

  if ( cut_pt != options_str.npos )
  {
    remainder       = options_str.substr( cut_pt + 1 );
    option_name_str = options_str.substr( 0, cut_pt );
  }

  std::vector<std::unique_ptr<option_t>> options;
  options.push_back(opt_uint("id", parsed.data.id));
  options.push_back(opt_int("upgrade", parsed.upgrade_level));
  options.push_back(opt_string("stats", option_stats_str));
  options.push_back(opt_string("gems", option_gems_str));
  options.push_back(opt_string("enchant", option_enchant_str));
  options.push_back(opt_string("addon", option_addon_str));
  options.push_back(opt_string("equip", option_equip_str));
  options.push_back(opt_string("use", option_use_str));
  options.push_back(opt_string("weapon", option_weapon_str));
  options.push_back(opt_string("warforged", option_warforged_str));
  options.push_back(opt_string("lfr", option_lfr_str));
  options.push_back(opt_string("heroic", option_heroic_str));
  options.push_back(opt_string("mythic", option_mythic_str));
  options.push_back(opt_string("type", option_armor_type_str));
  options.push_back(opt_string("reforge", DUMMY_REFORGE));
  options.push_back(opt_deprecated("suffix", "bonus_id"));
  options.push_back(opt_string("ilevel", option_ilevel_str));
  options.push_back(opt_string("quality", option_quality_str));
  options.push_back(opt_string("source", option_data_source_str));
  options.push_back(opt_string("gem_id", option_gem_id_str));
  options.push_back(opt_string("enchant_id", option_enchant_id_str));
  options.push_back(opt_string("addon_id", option_addon_id_str));
  options.push_back(opt_string("bonus_id", option_bonus_id_str));
  options.push_back(opt_string("initial_cd", option_initial_cd_str));
  options.push_back(opt_string("drop_level", option_drop_level_str));
  options.push_back(opt_string("relic_id", option_relic_id_str));
  options.push_back(opt_string("relic_ilevel", option_relic_ilevel_str));
  options.push_back(opt_string("azerite_powers", option_azerite_powers_str));
  options.push_back(opt_string("azerite_level", option_azerite_level_str));
  options.push_back(opt_string("context", DUMMY_CONTEXT));

  try
  {
    opts::parse( sim, option_name_str, options, remainder );
  }
  catch ( const std::exception& e )
  {
    std::throw_with_nested(std::invalid_argument(fmt::format("Cannot parse option from '{}'", options_str)));
  }

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
    std::vector<std::string> spl = util::string_split( option_gem_id_str, ":/" );
    for ( size_t i = 0, end = std::min( sizeof_array( parsed.gem_id ), spl.size() ); i < end; i++ )
    {
      int gem_id = std::stoi( spl[ i ] );

      parsed.gem_id[ i ] = gem_id;
    }
  }

  if ( ! option_relic_id_str.empty() )
  {
    std::vector<std::string> relic_split = util::string_split( option_relic_id_str, "/" );
    size_t relic_slot = 0;
    for ( const auto& relic_str : relic_split )
    {
      if ( relic_str == "0" )
      {
        relic_slot++;
        continue;
      }

      std::vector<std::string> bonus_id_split = util::string_split( relic_str, ":" );
      for ( const auto& bonus_id_str : bonus_id_split )
      {
        parsed.relic_data[ relic_slot ].push_back( util::to_unsigned( bonus_id_str ) );
      }

      relic_slot++;
    }

  }

  if ( ! option_relic_ilevel_str.empty() )
  {
    auto split = util::string_split( option_relic_ilevel_str, "/:" );
    auto relic_idx = 0U;
    for ( const auto& ilevel_str : split )
    {
      auto ilevel = std::stoi( ilevel_str );
      if ( ilevel >= 0 && ilevel < MAX_ILEVEL )
      {
        parsed.relic_ilevel[ relic_idx ] = ilevel;
      }

      ++relic_idx;
    }
  }

  if ( ! option_azerite_powers_str.empty() )
  {
    auto split = util::string_split( option_azerite_powers_str, "/:" );
    for ( const auto& power_str : split )
    {
      auto power_id = util::to_unsigned( power_str );
      if ( power_id > 0 )
      {
        parsed.azerite_ids.push_back( power_id );
      }
      // Try to convert the name to a power (id)
      else
      {
        const auto& power = player->dbc.azerite_power( power_str, true );
        if ( power.id > 0 )
        {
          parsed.azerite_ids.push_back( power.id );
        }
      }
    }
  }

  if ( ! option_enchant_id_str.empty() )
    parsed.enchant_id = util::to_unsigned( option_enchant_id_str );

  if ( ! option_addon_id_str.empty() )
    parsed.addon_id = util::to_unsigned( option_addon_id_str );

  if ( ! option_bonus_id_str.empty() )
  {
    try
    {
      std::vector<std::string> split = util::string_split( option_bonus_id_str, "/:" );
      for (auto & elem : split)
      {
        int bonus_id = std::stoi( elem );
        if ( bonus_id <= 0 )
        {
          throw std::invalid_argument("Negative or 0 bonus id.");
        }

        parsed.bonus_id.push_back( bonus_id );
      }
    }
    catch (const std::exception& e)
    {
      std::throw_with_nested(std::runtime_error("Bonus ID"));
    }
  }

  if ( ! option_initial_cd_str.empty() )
    parsed.initial_cd = timespan_t::from_seconds( std::stod( option_initial_cd_str ) );

  if ( ! option_drop_level_str.empty() )
    parsed.drop_level = util::to_unsigned( option_drop_level_str );

  if ( ! option_azerite_level_str.empty() )
    parsed.azerite_level = util::to_unsigned( option_azerite_level_str );
}

// item_t::initialize_data ==================================================

// Initializes the base data of the item, so we can perform full initialization later. Must be
// called after item_t::parse_options (so that the item has an id and potential data source
// overrides)
bool item_t::initialize_data()
{
  // Item specific source list has to be decoded first so we can properly
  // download the item from the correct source
  decode_data_source();

  if ( parsed.data.id > 0 )
  {
    if ( ! item_t::download_item( *this ) &&
         option_stats_str.empty() && option_weapon_str.empty() )
      return false;
  }
  else
    name_str = option_name_str;

  return true;
}

std::string item_t::encoded_item() const
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
    s << ",ilevel=" << option_ilevel_str;

  if ( ! option_azerite_level_str.empty() )
    s << ",azerite_level=" << option_azerite_level_str;
  else if ( parsed.azerite_level > 0 )
    s << ",azerite_level=" << parsed.azerite_level;

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

  // Figure out if any relics have "relic data" (relic bonus ids)
  auto relic_data_it = range::find_if( parsed.relic_data, []( const std::vector<unsigned> v ) {
    return v.size() > 0;
  } );

  if ( ! option_relic_id_str.empty() )
    s << ",relic_id=" << option_relic_id_str;
  else if ( relic_data_it != parsed.relic_data.end() )
  {
    s << ",relic_id=";
    for ( size_t relic_idx = 0; relic_idx < parsed.relic_data.size(); ++relic_idx )
    {
      const auto& relic_data = parsed.relic_data[ relic_idx ];
      if ( relic_data.size() == 0 )
      {
        s << "0";
      }
      else
      {
        for ( size_t relic_bonus_idx = 0; relic_bonus_idx < relic_data.size(); ++relic_bonus_idx )
        {
          s << relic_data[ relic_bonus_idx ];

          if ( relic_bonus_idx < relic_data.size() - 1 )
          {
            s << ":";
          }
        }
      }

      if ( relic_idx < parsed.relic_data.size() - 1 )
      {
        s << "/";
      }
    }
  }

  if ( ! option_relic_ilevel_str.empty() )
  {
    s << ",relic_ilevel=" << option_relic_ilevel_str;
  }

  if ( ! option_azerite_powers_str.empty() )
  {
    s << ",azerite_powers=" << option_azerite_powers_str;
  }
  else if ( parsed.azerite_ids.size() > 0 )
  {
    s << ",azerite_powers=" << util::string_join( parsed.azerite_ids, "/" );
  }

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

  if ( ! option_initial_cd_str.empty() )
    s << ",initial_cd=" << option_initial_cd_str;

  if ( ! option_drop_level_str.empty() )
    s << ",drop_level=" << option_drop_level_str;

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
      const special_effect_t& se = *parsed.special_effects[ i ];
      if ( se.type == SPECIAL_EFFECT_EQUIP && se.source == SPECIAL_EFFECT_SOURCE_ITEM )
        s << "equip=" << se.encoding_str << ",";
    }
  }

  if ( option_use_str.empty() )
  {
    for ( size_t i = 0; i < parsed.special_effects.size(); i++ )
    {
      const special_effect_t& se = *parsed.special_effects[ i ];
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

std::string item_t::encoded_gems() const
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

std::string item_t::encoded_enchant() const
{
  if ( ! option_enchant_str.empty() )
    return option_enchant_str;

  if ( ! parsed.encoded_enchant.empty() )
    return parsed.encoded_enchant;

  return stat_pairs_to_str( parsed.enchant_stats );
}

// item_t::encoded_addon ====================================================

std::string item_t::encoded_addon() const
{
  if ( ! option_addon_str.empty() )
    return option_addon_str;

  if ( ! parsed.encoded_addon.empty() )
    return parsed.encoded_addon;

  return stat_pairs_to_str( parsed.addon_stats );
}

// item_t::encoded_upgrade_level ============================================

std::string item_t::encoded_upgrade_level() const
{
  std::string upgrade_level_str;
  if ( parsed.upgrade_level > 0 )
    upgrade_level_str = util::to_string( parsed.upgrade_level );

  return upgrade_level_str;
}

// item_t::encoded_stats ====================================================

std::string item_t::encoded_stats() const
{
  if ( ! option_stats_str.empty() )
    return option_stats_str;

  std::vector<std::string> stats;

  for ( size_t i = 0; i < sizeof_array( parsed.data.stat_type_e ); i++ )
  {
    if ( parsed.data.stat_type_e[ i ] < 0 )
      continue;

    std::string stat_str = item_database::stat_to_str( parsed.data.stat_type_e[ i ], stat_value( i ) );
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

std::string item_t::encoded_weapon() const
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

bool item_t::verify_slot()
{
  inventory_type it = static_cast<inventory_type>( parsed.data.inventory_type );
  if ( it == INVTYPE_NON_EQUIP || slot == SLOT_INVALID )
  {
    return true;
  }

  switch ( slot )
  {
    case SLOT_MAIN_HAND:
    {
      switch ( it )
      {
        case INVTYPE_2HWEAPON:
        case INVTYPE_WEAPON:
        case INVTYPE_WEAPONMAINHAND:
        case INVTYPE_RANGED:
        case INVTYPE_RANGEDRIGHT:
          return true;
        default:
          return false;
      }
    }
    case SLOT_OFF_HAND:
    {
      switch ( it )
      {
        case INVTYPE_2HWEAPON: // Verify offhand 2handers due to Fury
        case INVTYPE_WEAPONOFFHAND:
        case INVTYPE_SHIELD:
        case INVTYPE_HOLDABLE:
        case INVTYPE_WEAPON:
          return true;
        default:
          return false;
      }
    }
    case SLOT_FINGER_1:
    case SLOT_FINGER_2:
      return it == INVTYPE_FINGER;
    case SLOT_TRINKET_1:
    case SLOT_TRINKET_2:
      return it == INVTYPE_TRINKET;
    // Rest of the slots can be checked with translator
    default:
      return slot == util::translate_invtype( it );
  }
}

// item_t::init =============================================================

void item_t::init()
{
  if ( name_str.empty() || name_str == "empty" || name_str == "none" )
    return;

  // Process basic stats
  decode_warforged();
  decode_lfr();
  decode_heroic();
  decode_mythic();
  decode_quality();
  decode_ilevel();
  decode_armor_type();

  if ( parsed.upgrade_level > 0 && ( ! parsed.data.quality || ( ! parsed.data.level && ! parsed.item_level ) ) )
  {
    sim -> errorf( "Player %s upgrading item %s at slot %s without quality or ilevel, upgrading will not work\n",
                   player -> name(), name(), slot_name() );
  }

  // Process complex input, and initialize item in earnest

  // Gems need to be processed first, because in Legion, they may affect the item level of the item
  decode_gems();
  decode_stats();
  decode_weapon();
  decode_equip_effect();
  decode_use_effect();
  decode_enchant();
  decode_addon();

  if ( ! option_name_str.empty() && ( option_name_str != name_str ) )
  {
    sim -> errorf( "Player %s at slot %s has inconsistency between name '%s' and '%s' for id %u\n",
                   player -> name(), slot_name(), option_name_str.c_str(), name_str.c_str(), parsed.data.id );

    name_str = option_name_str;
  }

  // Verify that slot makes sense, if given (through DBC data)
  if ( ! verify_slot() )
  {
    sim -> errorf( "Player %s at slot %s has an item '%s' meant for slot %s",
        player -> name(), slot_name(), name_str.c_str(),
        util::slot_type_string( util::translate_invtype( static_cast<inventory_type>( parsed.data.inventory_type ) ) ) );
  }

  if ( source_str.empty() )
    source_str = "Manual";
}

// item_t::decode_heroic ====================================================

void item_t::decode_heroic()
{
  if ( ! option_heroic_str.empty() )
    parsed.data.type_flags |= RAID_TYPE_HEROIC;
}

// item_t::decode_lfr =======================================================

void item_t::decode_lfr()
{
  if ( ! option_lfr_str.empty() )
    parsed.data.type_flags |= RAID_TYPE_LFR;
}

// item_t::decode_mythic =====================================================

void item_t::decode_mythic()
{
  if ( ! option_mythic_str.empty() )
    parsed.data.type_flags |= RAID_TYPE_MYTHIC;
}

// item_t::decode_warforged ==================================================

void item_t::decode_warforged()
{
  if ( ! option_warforged_str.empty() )
    parsed.data.type_flags |= RAID_TYPE_WARFORGED;
}

// item_t::is_matching_type =================================================

bool item_t::is_matching_type() const
{
  if ( ! util::is_match_slot( slot ) )
    return true;

  return util::matching_armor_type( player -> type ) == parsed.data.item_subclass;
}

// item_t::is_valid_type ====================================================

bool item_t::is_valid_type() const
{
  if ( ! util::is_match_slot( slot ) )
    return true;

  return parsed.data.item_subclass == ITEM_SUBCLASS_ARMOR_COSMETIC ||
         util::matching_armor_type( player -> type ) >= parsed.data.item_subclass;
}

// item_t::decode_armor_type ================================================

void item_t::decode_armor_type()
{
  if ( ! option_armor_type_str.empty() )
  {
    parsed.data.item_subclass = util::parse_armor_type( option_armor_type_str );
    if ( parsed.data.item_subclass == ITEM_SUBCLASS_ARMOR_MISC )
    {
      throw std::invalid_argument(fmt::format("Invalid item armor type '{}'.", option_armor_type_str));
    }
    parsed.data.item_class = ITEM_CLASS_ARMOR;
  }
}

// item_t::decode_ilevel ====================================================

void item_t::decode_ilevel()
{
  if ( ! option_ilevel_str.empty() )
  {
    parsed.item_level = std::stoi( option_ilevel_str );
    if ( parsed.item_level == 0 )
    {
      throw std::invalid_argument("Parsed item level is zero.");
    }

    if ( parsed.item_level > MAX_ILEVEL )
    {
      throw std::invalid_argument(fmt::format("Too high item level {}, maximum level supported is {}.",
          parsed.item_level, MAX_ILEVEL));
    }
  }
}

// item_t::decode_quality ===================================================

void item_t::decode_quality()
{
  if ( ! option_quality_str.empty() )
    parsed.data.quality = util::parse_item_quality( option_quality_str );
}
// item_t::decode_stats =====================================================

void item_t::decode_stats()
{
  if ( ! option_stats_str.empty() && option_stats_str != "none" )
  {
    // First, clear any stats in current data
    parsed.armor = 0;
    range::fill( parsed.data.stat_type_e, -1 );
    range::fill( parsed.data.stat_alloc, 0 );
    range::fill( parsed.stat_val, 0 );

    auto tokens = item_database::parse_tokens( option_stats_str );
    size_t stat = 0;

    for ( size_t i = 0; i < tokens.size() && stat < sizeof_array( parsed.stat_val ); i++ )
    {
      stat_e s = util::parse_stat_type( tokens[ i ].name );
      if ( s == STAT_NONE )
      {

        throw std::invalid_argument(fmt::format("Unknown 'stats=' token '{}' at slot {}.",
            tokens[ i ].full, slot_name()));
      }

      if ( s != STAT_ARMOR )
      {
        parsed.data.stat_type_e[ stat ] = util::translate_stat( s );
        parsed.stat_val[ stat ] = static_cast<int>( tokens[ i ].value );
        stat++;
      }
      else
        parsed.armor = static_cast<int>( tokens[ i ].value );
    }
  }

  // If scaling fails, then there's some issue with the scaling data, and the item stats will be
  // wrong.
  if ( ! has_scaling_stat_bonus_id() )
  {
    item_database::apply_item_scaling( *this, parsed.data.id_scaling_distribution, player -> level() );
  }

  for ( size_t i = 0; i < sizeof_array( parsed.data.stat_type_e ); i++ )
  {
    stat_e s = stat( i );
    if ( s == STAT_NONE )
      continue;

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
}

// item_t::decode_gems ======================================================

void item_t::decode_gems()
{
  try
  {
    // Disable gems in challenge modes.
    if ( sim -> challenge_mode )
      return;

    // First, attempt to parse gems by trying to find them through item data (split the strings by /
    // and search for each)
    auto split = util::string_split( option_gems_str, "/" );
    unsigned gem_index = 0;
    for ( const auto& gem_str : split )
    {
      auto item = dbc::find_gem( gem_str, player->dbc.ptr );
      if ( item->id > 0 )
      {
        parsed.gem_id[ gem_index++ ] = item->id;
      }

      if ( gem_index == parsed.gem_id.size() - 1 )
      {
        break;
      }
    }

    // Note, use the parsing results above only if all of the / delimited names can be parsed into a
    // gem id (gem item id).
    if ( gem_index == split.size() || option_gems_str.empty() || option_gems_str == "none" )
    {
      // Gems
      for ( size_t i = 0, end = parsed.gem_id.size(); i < end; i++ )
        parsed.gem_color[ i ] = enchant::initialize_gem( *this, i );

      // Socket bonus
      if ( socket_color_match() && parsed.socket_bonus_stats.size() == 0 )
      {
        const item_enchantment_data_t& socket_bonus = player -> dbc.item_enchantment( parsed.data.id_socket_bonus );
        enchant::initialize_item_enchant( *this, parsed.socket_bonus_stats, SPECIAL_EFFECT_SOURCE_SOCKET_BONUS, socket_bonus );
      }

      return;
    }

    // Parse user given gems= string. Stats are parsed as is, meta gem through
    // DBC data
    //
    // Detect meta gem through DBC data, instead of clunky prefix matching
    const item_enchantment_data_t& meta_gem_enchant = enchant::find_meta_gem( player -> dbc, option_gems_str );
    meta_gem_e meta_gem = enchant::meta_gem_type( player -> dbc, meta_gem_enchant );

    if ( meta_gem != META_GEM_NONE )
    {
      player -> meta_gem = meta_gem;
    }

    auto tokens = item_database::parse_tokens( option_gems_str );

    for ( auto& t : tokens )
    {
      stat_e s = util::parse_stat_type( t.name );
      if (s == STAT_NONE )
      {
        throw std::invalid_argument(fmt::format("Invalid stat '{}'.", t.name));
      }
      parsed.gem_stats.push_back( stat_pair_t( s, static_cast<int>( t.value ) ) );
    }
  }
  catch (const std::exception& e)
  {
    std::throw_with_nested( std::invalid_argument(
          fmt::format( "Error decoding gems from '{}'", option_gems_str ) ) ) ;
  }
}

// item_t::decode_equip_effect ==============================================

void item_t::decode_equip_effect()
{
  try
  {
    if ( option_equip_str.empty() || option_equip_str == "none" )
    {
      return;
    }

    special_effect_t effect( this );

    special_effect::parse_special_effect_encoding( effect, option_equip_str );

    effect.name_str = name_str;
    effect.type = SPECIAL_EFFECT_EQUIP;
    effect.source = SPECIAL_EFFECT_SOURCE_ITEM;

    if (!special_effect::usable_proc( effect ))
    {
      throw std::invalid_argument(fmt::format("No proc trigger flags found for effect '{}'.",
        option_equip_str ));
    }

    if ( effect.buff_type() == SPECIAL_EFFECT_BUFF_NONE &&
         effect.action_type() == SPECIAL_EFFECT_ACTION_NONE )
    {
      throw std::invalid_argument(fmt::format("No buff or action found for effect '{}'.", option_equip_str));
    }

    parsed.special_effects.push_back( new special_effect_t( effect ) );
  }
  catch (const std::exception& e)
  {
    std::throw_with_nested(std::invalid_argument(fmt::format("Error decoding equip='{}'", option_equip_str )));
  }
}

// item_t::decode_use_effect ================================================

void item_t::decode_use_effect()
{
  if ( option_use_str.empty() || option_use_str == "none" )
  {
    return;
  }

  special_effect_t effect( this );

  special_effect::parse_special_effect_encoding( effect, option_use_str );

  effect.name_str = name_str;
  effect.type = SPECIAL_EFFECT_USE;
  effect.source = SPECIAL_EFFECT_SOURCE_ITEM;

  if ( effect.buff_type() == SPECIAL_EFFECT_BUFF_NONE &&
       effect.action_type() == SPECIAL_EFFECT_ACTION_NONE )
  {
    throw std::invalid_argument(fmt::format("No buff or action found for use effect '{}'.", option_use_str));
  }

  parsed.special_effects.push_back( new special_effect_t( effect ) );
}

// item_t::decode_enchant ===================================================

void item_t::decode_enchant()
{
  if ( option_enchant_str.empty() || option_enchant_str == "none" )
  {
    return;
  }

  parsed.enchant_stats = str_to_stat_pair( option_enchant_str );
  if ( parsed.enchant_stats.size() > 0 )
  {
    return;
  }

  const item_enchantment_data_t& enchant_data = enchant::find_item_enchant( *this,
    option_enchant_str );

  if ( enchant_data.id > 0 )
  {
    parsed.enchant_id = enchant_data.id;
  }
  else
  {
    throw std::invalid_argument(fmt::format("Cannot find item enchant '{}'.", option_enchant_str));
  }
}

// item_t::decode_addon =====================================================

void item_t::decode_addon()
{
  if ( option_addon_str.empty() || option_addon_str == "none" )
  {
    return;
  }

  parsed.addon_stats = str_to_stat_pair( option_addon_str );
  if ( parsed.addon_stats.size() > 0 )
  {
    return;
  }

  const item_enchantment_data_t& enchant_data = enchant::find_item_enchant( *this,
    option_addon_str );

  if ( enchant_data.id > 0 )
  {
    parsed.addon_id = enchant_data.id;
  }
}

// item_t::decode_weapon ====================================================

void item_t::decode_weapon()
{
  weapon_t* w = weapon();
  if ( ! w )
    return;

  // Custom weapon stats cant be unloaded to the "proxy" item data at all,
  // so edit the weapon in question right away based on either the
  // parsed data or the option data iven by the user.
  if ( option_weapon_str.empty() || option_weapon_str == "none" )
  {
    if ( parsed.data.item_class != ITEM_CLASS_WEAPON )
      return;

    weapon_e wc = util::translate_weapon_subclass( parsed.data.item_subclass );
    if ( wc == WEAPON_NONE )
      return;

    w -> type = wc;
    w -> swing_time = timespan_t::from_millis( parsed.data.delay );
    w -> dps = player -> dbc.weapon_dps( &parsed.data, item_level() );
    w -> damage = player -> dbc.weapon_dps( &parsed.data, item_level() ) * parsed.data.delay / 1000.0;
    w -> min_dmg = item_database::weapon_dmg_min( *this );
    w -> max_dmg = item_database::weapon_dmg_max( *this );
  }
  else
  {
    auto tokens = item_database::parse_tokens( option_weapon_str );

    bool dps_set = false, dmg_set = false, min_set = false, max_set = false;

    for ( auto& t : tokens )
    {
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
        throw std::invalid_argument(fmt::format("unknown 'weapon=' token '{}'.", t.full));
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
}

// item_t::decode_data_source ===============================================

void item_t::decode_data_source()
{
  if ( ! item_database::initialize_item_sources( *this, parsed.source_list ) )
  {
    throw std::invalid_argument(
        fmt::format("Your item-specific data source string '{}' contained no valid sources to download item id {}.\n",
        option_data_source_str, parsed.data.id));
  }
}

// item_t::str_to_stat_pair =================================================

std::vector<stat_pair_t> item_t::str_to_stat_pair( const std::string& stat_str )
{
  std::vector<stat_pair_t> stats;

  auto tokens = item_database::parse_tokens( stat_str );
  for ( auto& t : tokens )
  {
    stat_e s = STAT_NONE;
    if ( ( s = util::parse_stat_type( t.name ) ) != STAT_NONE && t.value != 0 )
      stats.push_back( stat_pair_t( s, static_cast<int>( t.value ) ) );
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
    enchant::initialize_item_enchant( item, item.parsed.socket_bonus_stats, SPECIAL_EFFECT_SOURCE_SOCKET_BONUS, bonus );
  }

  return success;
}

// item_t::init_special_effects =============================================

void item_t::init_special_effects()
{
  special_effect_t proxy_effect( this );

  // Enchant
  const item_enchantment_data_t& enchant_data = player -> dbc.item_enchantment( parsed.enchant_id );
  enchant::initialize_item_enchant( *this, parsed.enchant_stats,
        SPECIAL_EFFECT_SOURCE_ENCHANT, enchant_data );

  // Addon (tinker)
  const item_enchantment_data_t& addon_data = player -> dbc.item_enchantment( parsed.addon_id );
  enchant::initialize_item_enchant( *this, parsed.addon_stats,
        SPECIAL_EFFECT_SOURCE_ADDON, addon_data );

  // On-use effects
  for ( size_t i = 0, end = sizeof_array( parsed.data.id_spell ); i < end; ++i )
  {
    if ( parsed.data.id_spell[ i ] == 0 ||
         parsed.data.trigger_spell[ i ] != ITEM_SPELLTRIGGER_ON_USE )
    {
      continue;
    }

    // We have something to work with
    proxy_effect.reset();
    proxy_effect.type = SPECIAL_EFFECT_USE;
    proxy_effect.source = SPECIAL_EFFECT_SOURCE_ITEM;
    unique_gear::initialize_special_effect( proxy_effect, parsed.data.id_spell[ i ] );

    // First-phase special effect initialize decided it's a usable special effect, so add it
    if ( proxy_effect.type != SPECIAL_EFFECT_NONE )
    {
      parsed.special_effects.push_back( new special_effect_t( proxy_effect ) );
    }
  }

  // On-equip effects
  for ( size_t i = 0, end = sizeof_array( parsed.data.id_spell ); i < end; ++i )
  {
    if ( parsed.data.id_spell[ i ] == 0 ||
         parsed.data.trigger_spell[ i ] != ITEM_SPELLTRIGGER_ON_EQUIP )
    {
      continue;
    }

    // We have something to work with
    proxy_effect.reset();
    proxy_effect.type = SPECIAL_EFFECT_EQUIP;
    proxy_effect.source = SPECIAL_EFFECT_SOURCE_ITEM;
    unique_gear::initialize_special_effect( proxy_effect, parsed.data.id_spell[ i ] );

    // First-phase special effect initialize decided it's a usable special effect, so add it
    if ( proxy_effect.type != SPECIAL_EFFECT_NONE )
    {
      parsed.special_effects.push_back( new special_effect_t( proxy_effect ) );
    }
  }
}
