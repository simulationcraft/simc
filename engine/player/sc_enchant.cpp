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
    if ( item_enchant -> name == 0 )
      continue;

    std::string enchant_name = item_enchant -> name;
    util::tokenize( enchant_name );
    // Check for partial match in the name, enchant name must be fully
    // contained in the given parameter
    if ( ! util::str_in_str_ci( name, enchant_name ) )
      continue;

    // Partial match found, see if the spell has a rank string, and massage
    // name to account for it.
    for ( size_t i = 0; i < sizeof_array( item_enchant -> ench_prop ); i++ )
    {
      if ( item_enchant -> ench_prop[ i ] == 0 ||
           item_enchant -> ench_type[ i ] == 0 )
        continue;

      std::string rank_str;
      if ( dbc.spell( item_enchant -> ench_prop[ i ] ) -> rank_str() )
        rank_str = dbc.spell( item_enchant -> ench_prop[ i ] ) -> rank_str();
      util::tokenize( rank_str );

      // Compare to exact enchant name match, if rank string is missing
      if ( rank_str.empty() && util::str_compare_ci( name, enchant_name ) )
        return *item_enchant;

      // Compare directly to enchant name + rank
      if ( ! rank_str.empty() && util::str_compare_ci( name, enchant_name + "_" + rank_str ) )
        return *item_enchant;

      std::string::size_type offset = rank_str.find( "rank_" );
      if ( offset != std::string::npos )
      {
        // Erase "rank_", and try again
        rank_str.erase( offset, 5 );

        // Match <enchant name>_<enchant rank number>
        if ( util::str_compare_ci( name, enchant_name + "_" + rank_str ) )
          return *item_enchant;
      }
    }
  }

  return dbc.item_enchantment( 0 );
}

bool enchant::initialize_item_enchant( item_t& item,
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
        effect.trigger_type = PROC_ATTACK;
        effect.trigger_mask = RESULT_HIT_MASK;
        break;
      case ITEM_ENCHANTMENT_EQUIP_SPELL:
        // Passive (stat) giving enchants get a special treatment
        if ( passive_enchant( item, enchant.ench_prop[ i ] ) )
          continue;

        effect.type = SPECIAL_EFFECT_EQUIP;
        break;
      case ITEM_ENCHANTMENT_USE_SPELL:
        effect.type = SPECIAL_EFFECT_USE;
        break;
      case ITEM_ENCHANTMENT_STAT:
      {
        stat_pair_t stat = item_database::item_enchantment_effect_stats( item.player, enchant, i );
        if ( stat.stat != STAT_NONE && stat.value != 0 )
        {
          if ( source == SPECIAL_EFFECT_SOURCE_ENCHANT )
            item.parsed.enchant_stats.push_back( stat );
          else if ( source == SPECIAL_EFFECT_SOURCE_GEM )
            item.parsed.gem_stats.push_back( stat );
          else if ( source == SPECIAL_EFFECT_SOURCE_ADDON )
            item.parsed.addon_stats.push_back( stat );
        }
        break;
      }
      default:
        break;
    }

    // First phase initialize the spell effect
    if ( effect.type != SPECIAL_EFFECT_NONE && 
         unique_gear::initialize_special_effect( effect, item, enchant.ench_prop[ i ] ) )
    {
      if ( effect.type != SPECIAL_EFFECT_NONE )
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
 * TODO: Add socket enchant can be done here
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
