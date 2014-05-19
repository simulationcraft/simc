// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// TODO: Check if Power Torrent, Jade Spirit share buffs, or allow separate

#include "simulationcraft.hpp"

using namespace enchant;

namespace /* ANONYMOUS NAMESPACE */
{
/**
 * Additional translation mappings for enchant names. We use shorthands for
 * certain enchants (listed here), that cannot be deduced from the item
 * enchantment name in DBC. We can add additional shortcuts here, where
 * applicable. The enchant ID (number) maps to the array of
 * item_enchantment_data_t structs in sc_item_data.inc.
 */
static const enchant_db_item_t __enchant_db[] = {
  /* Engineering tinkers */
  { "synapse_springs_2",       4898 }, /* Default is "synapse_springs_mark_ii" */

  /* Tailoring cloak enchants */
  { "lightweave_1",            3722 },
  { "lightweave_2",            4115 },
  { "lightweave_3",            4892 },
  { "darkglow_1",              3728 },
  { "darkglow_2",              4116 },
  { "darkglow_3",              4893 },
  { "swordguard_1",            3730 },
  { "swordguard_2",            4118 },
  { "swordguard_3",            4894 },
  { 0,                         0    }
};

} /* ANONYMOUS NAMESPACE */

unsigned enchant::find_enchant_id( const std::string& name )
{
  for ( size_t i = 0; i < sizeof_array( __enchant_db ) - 1; i++ )
  {
    if ( util::str_compare_ci( __enchant_db[ i ].enchant_name, name ) )
      return __enchant_db[ i ].enchant_id;
  }

  return 0;
}

std::string enchant::find_enchant_name( unsigned enchant_id )
{
  for ( size_t i = 0; i < sizeof_array( __enchant_db ) - 1; i++ )
  {
    if ( __enchant_db[ i ].enchant_id == enchant_id )
      return __enchant_db[ i ].enchant_name;
  }

  return std::string();
}

/**
 * Return a "simc-encoded" enchant name for a given DBC item enchantment.
 *
 * This function simply encodes the item enchantment name (given in
 * SpellItemEnchantment.dbc) that is exported to simc. Normal simc tokenization
 * rules are applied on the enchant name, and any possible rank string (without
 * the "rank) is applied as a suffix. Rank information is found in the spell
 * that the enchant uses.
 *
 * Simc needs this information to save actor profiles with meaningful enchant
 * names (if the enchant is not a stat enchant), instead of enchant ids. When a
 * profile is loaded, the enchant name translates to a item enchantment entry
 * in the DBC data through the enchant::find_item_enchant() function.
 *
 * TODO: In the off chance blizzard makes item enchants with 2 spell ids, that
 * both have ranks, and there also is a "lower rank" item enchant of the same
 * name, we need to somehow cover that case. Highly unlikely in practice.
 */
std::string enchant::encoded_enchant_name( const dbc_t& dbc, const item_enchantment_data_t& enchant )
{
  std::string enchant_name;
  std::string enchant_rank_str;

  const spell_data_t* enchant_source = dbc.spell( enchant.id_spell );
  if ( enchant_source -> id() > 0 )
  {
    enchant_name = enchant_source -> name_cstr();
    std::string::size_type enchant_pos = enchant_name.find( "Enchant " );
    std::string::size_type enchant_hyphen_pos = enchant_name.find( "-" );

    // Cut out "Enchant XXX -" from the string, if it exists, also remove any 
    // following whitespace. Consider that to be the enchant name. If "Enchant
    // XXX -" is not found, just consider the linked spell's full name to be
    // the enchant name.
    if ( enchant_pos != std::string::npos && enchant_hyphen_pos != std::string::npos &&
         enchant_hyphen_pos > enchant_pos )
    {
      enchant_name = enchant_name.substr( enchant_hyphen_pos + 1 );
      while ( enchant_name[ 0 ] == ' ' )
        enchant_name.erase( enchant_name.begin() );
    }

    util::tokenize( enchant_name );

    if ( enchant_source -> rank_str() )
      enchant_rank_str = enchant_source -> rank_str();
    util::tokenize( enchant_rank_str );
  }
  // Revert back to figuring out name based on pure item enchantment data. This
  // will probably be wrong in many cases, but what can we do.
  else
  {
    enchant_name = enchant.name;
    util::tokenize( enchant_name );

    for ( size_t i = 0; i < sizeof_array( enchant.ench_prop ); i++ )
    {
      if ( enchant.ench_prop[ i ] == 0 || enchant.ench_type[ i ] == 0 )
        continue;

      if ( dbc.spell( enchant.ench_prop[ i ] ) -> rank_str() )
        enchant_rank_str = dbc.spell( enchant.ench_prop[ i ] ) -> rank_str();
      util::tokenize( enchant_rank_str );
    }
  }

  // Erase "rank_"
  std::string::size_type rank_offset = enchant_rank_str.find( "rank_" );
  if ( rank_offset != std::string::npos )
    enchant_rank_str.erase( rank_offset, 5 );

  if ( ! enchant_rank_str.empty() )
    enchant_name += "_" + enchant_rank_str;

  return enchant_name;
}

/**
 * Deduce DBC enchant data from user given enchant option value.
 *
 * The function loops through all item enchantments in the exported game client
 * data and checks if the item enchant name (and possibly rank) match the user
 * input enchant name. The function matches un-ranked enchants (for example
 * Dancing Steel), and ranked enchants in two ways. First, the full name
 * including the full rank string is checked (i.e.,
 * lightweave_embroidery_rank_2). If this is not a match, the function
 * additionally checks against the full name, where "rank_" is removed. Returns
 * an invalid item enchantment (with id of 0) if nothing is found.
 *
 * TODO: We could additionally check against the spell name (and rank) that the
 * enchant procs for even more detection. This would remove the need for
 * Tailoring enchant mappings in the array above.
 */
const item_enchantment_data_t& enchant::find_item_enchant( const dbc_t& dbc,
                                                           const std::string& name )
{
  // Check additional mapping table first
  if ( find_enchant_id( name ) > 0 )
    return dbc.item_enchantment( find_enchant_id( name ) );

  for ( const item_enchantment_data_t* item_enchant = dbc.item_enchantments();
        item_enchant -> id != 0;
        item_enchant++ )
  {
    if ( ! item_enchant -> name && ! item_enchant -> id_spell )
      continue;

    if ( util::str_compare_ci( name, encoded_enchant_name( dbc, *item_enchant ) ) )
      return *item_enchant;
  }

  return dbc.item_enchantment( 0 );
}

/**
 * Initialize an item enchant from DBC data.
 *
 * This function initializes a Blizzard SpellItemEnchantment.dbc entry so that
 * simc understands it. Blizzard uses item enchantmnts to represent Enchants,
 * Gems, and Tinkers, and this function handles all of the initialization.
 *
 * The function supports three types of information per item enchantment entry
 * 1) Stat enchants (will be translated into simc-understandable stats) that
 *    will be inserted into the given vector reference. Typically called by
 *    various decode_xx() functions in item_t.
 * 2) Passive aura enchants. Certain passive aura enchants are supported by the
 *    system out of the box. Currently, any primary stat, or rating granting
 *    passive aura will be translated directly into simc-understandable stat.
 *    TODO-WOD: Once we have "passive buff" support, we can offload the stat
 *    deduction to stat_buff_t, reducing code replication.
 * 3) Procs. Item enchantments use three relevant proc types for simc
 *    - Spell procs when an item is worn (EQUIP_SPELL)
 *    - Spell procs when a weapon lands a hit (COMBAT_SPELL)
 *    - On-use abilities (USE_SPELL)
 *    This function translates the relevant proc type information so that
 *    special_effect_t is initialized correctly, and then passes the rest of
 *    the proc initialization to the generalized initialize_special_effect()
 *    function in sc_unique_gear.cpp.
 */
bool enchant::initialize_item_enchant( item_t& item,
                                       std::vector< stat_pair_t >& stats,
                                       special_effect_source_e source,
                                       const item_enchantment_data_t& enchant )
{
  // Profession check
  profession_e profession = util::translate_profession_id( enchant.req_skill );
  if ( profession != PROFESSION_NONE )
  {
    if ( item.player -> profession[ profession ] < static_cast<int>( enchant.req_skill_value ) )
    {
      item.sim -> errorf( "Player %s attempting to use %s '%s' without %s skill level of %d (has %d), disabling enchant.",
          item.player -> name(), util::special_effect_source_string( source ), enchant.name, 
          util::profession_type_string( profession ),
          enchant.req_skill_value, item.player -> profession[ profession ] );
      // Don't initialize the special effects, but do "succeed" the
      // initialization process.
      return true;
    }
  }

  for ( size_t i = 0; i < sizeof_array( enchant.ench_prop ); i++ )
  {
    // No stat ID, or spell ID given, continue
    if ( enchant.ench_prop[ i ] == 0 )
      continue;

    special_effect_t effect( &item );
    effect.source = source;
    switch ( enchant.ench_type[ i ] )
    {
      // "Chance on Hit", we need to help simc a bit with proc flags
      case ITEM_ENCHANTMENT_COMBAT_SPELL:
        // Require that "chance on hit" enchant effects are hit with the 
        // correct weapon
        effect.weapon_proc = true;
        effect.type = SPECIAL_EFFECT_EQUIP;
        effect.proc_flags_ = PF_MELEE | PF_MELEE_ABILITY | PF_RANGED | PF_RANGED_ABILITY;
        break;
      case ITEM_ENCHANTMENT_EQUIP_SPELL:
        // Passive enchants get a special treatment. In essence, we only support
        // a couple, and they are handled without special effect initialization
        // for now. Unfortunately, they do need to be added to the special
        // effect list, so we can output them in saved profiles as the enchant
        // name, instead of a bunch of stats.
        if ( passive_enchant( item, enchant.ench_prop[ i ] ) )
        {
          if ( source == SPECIAL_EFFECT_SOURCE_ENCHANT )
            item.parsed.encoded_enchant = encoded_enchant_name( item.player -> dbc, enchant );
          else if ( source == SPECIAL_EFFECT_SOURCE_ADDON )
            item.parsed.encoded_addon = encoded_enchant_name( item.player -> dbc, enchant );
          continue;
        }
        else
          effect.type = SPECIAL_EFFECT_EQUIP;
        break;
      case ITEM_ENCHANTMENT_USE_SPELL:
        effect.type = SPECIAL_EFFECT_USE;
        break;
      case ITEM_ENCHANTMENT_STAT:
      {
        stat_pair_t stat = item_database::item_enchantment_effect_stats( item.player, enchant, i );
        if ( stat.stat != STAT_NONE && stat.value != 0 )
          stats.push_back( stat );
        break;
      }
      default:
        break;
    }

    // First phase initialize the spell effect
    if ( effect.type != SPECIAL_EFFECT_NONE && 
         ! unique_gear::initialize_special_effect( effect, item, enchant.ench_prop[ i ] ) )
      return false;

    // If this enchant has any kind of special effect, we need to encode it's
    // name in the saved profiles, so when the profile is loaded, it's loaded
    // through the enchant init system.
    if ( effect.type != SPECIAL_EFFECT_NONE )
    {
      if ( source == SPECIAL_EFFECT_SOURCE_ENCHANT )
        item.parsed.encoded_enchant = encoded_enchant_name( item.player -> dbc, enchant );
      else if ( source == SPECIAL_EFFECT_SOURCE_ADDON )
        item.parsed.encoded_addon = encoded_enchant_name( item.player -> dbc, enchant );
      item.parsed.special_effects.push_back( effect );
    }
  }

  return true;
}

/**
 * Figure out if the ITEM_ENCHANT_EQUIP_SPELL is in fact a passive enchant.
 * Such enchants include any passive spells that apply a stat aura, for
 * example.
 *
 * TODO-WOD: For now, this needs to sync with stat_buff_t stats.
 */
bool enchant::passive_enchant( item_t& item, unsigned spell_id )
{
  bool ret = false;
  const spell_data_t* spell = item.player -> find_spell( spell_id );
  if ( ! spell -> ok() )
    return ret;

  if ( ! spell -> flags( SPELL_ATTR_PASSIVE ) &&
       spell -> duration() >= timespan_t::zero() )
    return ret;

  for ( size_t i = 1, end = spell -> effect_count(); i <= end; i++ )
  {
    const spelleffect_data_t& effect = spell -> effectN( i );

    if ( effect.type() != E_APPLY_AURA )
      continue;

    double value = util::round( effect.average( item.player ) );
    stat_e stat = STAT_NONE;

    switch ( effect.subtype() )
    {
      case A_MOD_STAT:
        if ( effect.misc_value1() == -1 )
          stat = STAT_ALL;
        else
          stat = static_cast< stat_e >( effect.misc_value1() + 1 );
        break;
      case A_MOD_RATING:
        stat = util::translate_rating_mod( effect.misc_value1() );
        break;
      // Clump all movement speed enchants additively into "passive_movement_speed"
      case A_MOD_SPEED_ALWAYS:
        item.player -> passive_modifier += effect.percent();
        ret = true;
        break;
      default:
        break;
    }

    if ( stat != STAT_NONE && value != 0 )
    {
      item.parsed.enchant_stats.push_back( stat_pair_t( stat, value ) );
      ret = true;
    }
  }
  return ret;
}

/**
 * Find the spell item enchantment data entry for an user given gem encoding
 * string, that may, or may not contain a meta gem name. The encoding is
 * checked against all full meta gem names in a tokenized form
 * (capacitive_primal_diamond), and the "short" form of the tokenized name
 * (capacitive_primal).
 */
const item_enchantment_data_t& enchant::find_meta_gem( const dbc_t&       dbc, 
                                                       const std::string& encoding )
{
  for ( const gem_property_data_t* gem_property = dbc.gem_properties();
        gem_property -> id != 0;
        gem_property++ )
  {
    if ( gem_property -> color != SOCKET_COLOR_META )
      continue;

    const item_enchantment_data_t& data = dbc.item_enchantment( gem_property -> enchant_id );
    if ( data.id == 0 )
      continue;

    if ( data.id_gem == 0 )
      continue;

    const item_data_t* gem = dbc.item( data.id_gem );
    // A lot of the old meta gems no longer exist in game
    if ( ! gem )
      continue;

    if ( gem -> id != data.id_gem )
      continue;

    std::string tokenized_name = gem -> name;
    util::tokenize( tokenized_name );
    std::string shortname;
    std::string::size_type offset = tokenized_name.find( "_diamond" );
    if ( offset != std::string::npos )
      shortname = tokenized_name.substr( 0, offset );
    
    if ( util::str_in_str_ci( encoding, tokenized_name ) ||
         ( ! shortname.empty() && util::str_in_str_ci( encoding, shortname ) ) )
    {
      return data;
    }
  }

  return dbc.item_enchantment( 0 );
}

/**
 * Translate meta gem item enchantment data entry into a meta_gem_e type for
 * simc.
 */
meta_gem_e enchant::meta_gem_type( const dbc_t&                   dbc,
                                   const item_enchantment_data_t& data )
{
  if ( data.id == 0 )
    return META_GEM_NONE;

  const item_data_t* gem = dbc.item( data.id_gem );
  if ( ! gem )
    return META_GEM_NONE;

  std::string tokenized_name = gem -> name;
  util::tokenize( tokenized_name );
  std::string shortname;
  std::string::size_type offset = tokenized_name.find( "_diamond" );
  if ( offset != std::string::npos )
    shortname = tokenized_name.substr( 0, offset );

  return util::parse_meta_gem_type( shortname );
}

/**
 * Intialize gem, based on game client data.
 *
 * Gem stats will be set to item_t::parsed.gem_stats, and the gem color
 * returned. Actual initialization is performed through the unified
 * initialize_item_enchant() function. Additionally, for backwards
 * compatibility, the meta gem type is translated from the gem (item) name.
 *
 * Note that meta gem stats are separateed form normal gem stats, as the meta
 * gem initialization is now fully DBC aware, and will initialize correct stats
 * for the actors purely based on the meta gem name. Thus, hardcoding meta gem
 * stats in a separate place (such as player_t::init_meta_gem()) is no longer
 * necessary.
 */
item_socket_color enchant::initialize_gem( item_t& item, unsigned gem_id )
{
  if ( gem_id == 0 )
    return SOCKET_COLOR_NONE;

  const item_data_t* gem = item.player -> dbc.item( gem_id );
  if ( ! gem )
    return SOCKET_COLOR_NONE;

  const gem_property_data_t& gem_prop = item.player -> dbc.gem_property( gem -> gem_properties );
  if ( ! gem_prop.id )
    return SOCKET_COLOR_NONE;

  const item_enchantment_data_t& data = item.player -> dbc.item_enchantment( gem_prop.enchant_id );

  if ( ! enchant::initialize_item_enchant( item, 
                                           gem_prop.color != SOCKET_COLOR_META ? item.parsed.gem_stats : item.parsed.meta_gem_stats, 
                                           SPECIAL_EFFECT_SOURCE_GEM,
                                           data ) )
    return SOCKET_COLOR_NONE;

  // TODO: This should really be removed, as should player -> meta_gem
  if ( gem_prop.color == SOCKET_COLOR_META )
    item.player -> meta_gem = enchant::meta_gem_type( item.player -> dbc, data );

  assert( dbc::valid_gem_color( gem_prop.color ) );

  return static_cast< item_socket_color >( gem_prop.color );
}

