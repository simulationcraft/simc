
#include "spell_data.hpp"

#include <array>

#include "generated/sc_spell_data.inc"
#if SC_USE_PTR
#include "generated/sc_spell_data_ptr.inc"
#endif

#include "dbc/client_data.hpp"
#include "dbc/dbc.hpp"
#include "dbc/item_database.hpp"
#include "item/item.hpp"
#include "player/sc_player.hpp"
#include "util/generic.hpp"
#include "util/util.hpp"

#include "fmt/format.h"

// ==========================================================================
// Spell Label Data - SpellLabel.db2
// ==========================================================================

util::span<const spelllabel_data_t> spelllabel_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __spelllabel_data, __ptr_spelllabel_data, ptr );
}

// ==========================================================================
// Spell Power Data - SpellPower.dbc
// ==========================================================================

resource_e spellpower_data_t::resource() const
{
  return util::translate_power_type( type() );
}

const spellpower_data_t& spellpower_data_t::find( unsigned id, bool ptr )
{
  const auto index = SC_DBC_GET_DATA( __spellpower_id_index, __ptr_spellpower_id_index, ptr );
  return dbc::find_indexed( id, data( ptr ), index, &spellpower_data_t::_id );
}

util::span<const spellpower_data_t> spellpower_data_t::data( bool ptr )
{
  return SC_DBC_GET_DATA( __spellpower_data, __ptr_spellpower_data, ptr );
}

// ==========================================================================
// Spell Effect Data - SpellEffect.dbc
// ==========================================================================

resource_e spelleffect_data_t::resource_gain_type() const
{
  return util::translate_power_type( static_cast< power_e >( misc_value1() ) );
}

school_e spelleffect_data_t::school_type() const
{
  return dbc::get_school_type( as<uint32_t>( misc_value1() ) );
}

bool spelleffect_data_t::has_common_school( school_e school ) const
{
  return ( as<uint32_t>( misc_value1() ) & dbc::get_school_mask( school ) ) != 0;
}

double spelleffect_data_t::delta( const player_t* p, unsigned level ) const
{
  assert( level <= MAX_SCALING_LEVEL );

  double m_scale = 0;
  if ( _m_delta != 0 && _spell -> scaling_class() != 0 )
  {
    unsigned scaling_level = level ? level : p -> level();
    if ( _spell -> max_scaling_level() > 0 )
      scaling_level = std::min( scaling_level, _spell -> max_scaling_level() );
    m_scale = p -> dbc->spell_scaling( _spell -> scaling_class(), scaling_level );
  }

  return scaled_delta( m_scale );
}

double spelleffect_data_t::delta( const item_t* item ) const
{
  double m_scale = 0;

  if ( ! item )
    return 0;

  if ( _m_delta != 0 )
    m_scale = item_database::item_budget( item, _spell -> max_scaling_level() );

  if ( _spell -> scaling_class() == PLAYER_SPECIAL_SCALE7 )
  {
    m_scale = item_database::apply_combat_rating_multiplier( *item, m_scale );
  }
  else if ( _spell -> scaling_class() == PLAYER_SPECIAL_SCALE8 )
  {
    const auto& props = item -> player -> dbc->random_property( item -> item_level() );
    m_scale = props.damage_replace_stat;
  }

  return scaled_delta( m_scale );
}

double spelleffect_data_t::bonus( const player_t* p, unsigned level ) const
{
  assert( p );
  return p -> dbc->effect_bonus( id(), level ? level : p -> level() );
}

double spelleffect_data_t::min( const player_t* p, unsigned level ) const
{
  assert( level <= MAX_SCALING_LEVEL );

  return scaled_min( average( p, level ), delta( p, level ) );
}

double spelleffect_data_t::min( const item_t* item ) const
{
  return scaled_min( average( item ), delta( item ) );
}

double spelleffect_data_t::max( const player_t* p, unsigned level ) const
{
  assert( level <= MAX_SCALING_LEVEL );

  return scaled_max( average( p, level ), delta( p, level ) );
}

double spelleffect_data_t::max( const item_t* item ) const
{
  return scaled_max( average( item ), delta( item ) );
}

double spelleffect_data_t::average( const player_t* p, unsigned level ) const
{
  assert( level <= MAX_SCALING_LEVEL );

  double m_scale = 0;

  if ( _m_coeff != 0 && _spell -> scaling_class() != 0 )
  {
    unsigned scaling_level = level ? level : p -> level();
    if ( _spell -> max_scaling_level() > 0 )
      scaling_level = std::min( scaling_level, _spell -> max_scaling_level() );
    m_scale = p -> dbc->spell_scaling( _spell -> scaling_class(), scaling_level );
  }

  return scaled_average( m_scale, level );
}

double spelleffect_data_t::average( const item_t* item ) const
{
  if ( ! item )
    return 0;

  auto budget = item_database::item_budget( item, _spell -> max_scaling_level() );
  if ( _spell -> scaling_class() == PLAYER_SPECIAL_SCALE7 )
  {
    budget = item_database::apply_combat_rating_multiplier( *item, budget );
  }
  else if ( _spell -> scaling_class() == PLAYER_SPECIAL_SCALE8 )
  {
    const auto& props = item -> player -> dbc->random_property( item -> item_level() );
    budget = props.damage_replace_stat;
  }
  else if ( _spell->scaling_class() == PLAYER_NONE &&
            _spell->flags( spell_attribute::SX_SCALE_ILEVEL ) )
  {
    const auto& props = item -> player -> dbc->random_property( item -> item_level() );
    budget = props.damage_secondary;
  }

  return _m_coeff * budget;
}

double spelleffect_data_t::scaled_average( double budget, unsigned level ) const
{
  if ( _m_coeff != 0 && _spell -> scaling_class() != 0 )
    return _m_coeff * budget;
  else if ( _real_ppl != 0 )
  {
    if ( _spell -> max_level() > 0 )
      return _base_value + ( std::min( level, _spell -> max_level() ) - _spell -> level() ) * _real_ppl;
    else
      return _base_value + ( level - _spell -> level() ) * _real_ppl;
  }
  else
    return _base_value;
}

double spelleffect_data_t::scaled_delta( double budget ) const
{
  if ( _m_delta != 0 && budget > 0 )
    return _m_coeff * _m_delta * budget;
  else
    return 0;
}

double spelleffect_data_t::scaled_min( double avg, double delta ) const
{
  double result = 0;

  if ( _m_coeff != 0 || _m_delta != 0 )
    result = avg - ( delta / 2 );
  else
  {
    result = avg;
  }

  switch ( _type )
  {
    case E_WEAPON_PERCENT_DAMAGE:
      result *= 0.01;
      break;
    default:
      break;
  }

  return result;
}

double spelleffect_data_t::scaled_max( double avg, double delta ) const
{
  double result = 0;

  if ( _m_coeff != 0 || _m_delta != 0 )
    result = avg + ( delta / 2 );
  else
  {
    result = avg;
  }

  switch ( _type )
  {
    case E_WEAPON_PERCENT_DAMAGE:
      result *= 0.01;
      break;
    default:
      break;
  }

  return result;
}

double spelleffect_data_t::default_multiplier() const
{
  switch ( type() )
  {
    case E_APPLY_AURA:
      switch ( subtype() )
      {
        case A_PERIODIC_TRIGGER_SPELL:
        case A_PERIODIC_ENERGIZE:
        case A_MOD_INCREASE_ENERGY:
        case A_PROC_TRIGGER_SPELL:
        case A_MOD_MAX_RESOURCE_COST:
        case A_MOD_MAX_RESOURCE:
          return 1.0; // base_value

        case A_ADD_FLAT_MODIFIER:
        case A_ADD_FLAT_LABEL_MODIFIER:
          switch ( property_type() )
          {
            case P_DURATION:
            case P_CAST_TIME:
            case P_TICK_TIME:
            case P_GCD:
              return 0.001; // time_value

            default:
              return 1.0; // base_value
          }
          break;

        case A_MOD_MASTERY_PCT:
          return 1.0;

        case A_RESTORE_HEALTH:
        case A_RESTORE_POWER:
          return 0.2; // Resource per 5s

        default:
          return 0.01; // percent
      }

    default:
      break;
  }

  return 1.0; // base_value
}

const spelleffect_data_t* spelleffect_data_t::find( unsigned id, bool ptr )
{
  const auto index = SC_DBC_GET_DATA( __spelleffect_id_index, __ptr_spelleffect_id_index, ptr );
  return &dbc::find_indexed( id, data( ptr ), index, &spelleffect_data_t::id );
}

util::span<const spelleffect_data_t> spelleffect_data_t::data( bool ptr )
{
  return _data( ptr );
}

void spelleffect_data_t::link( bool ptr )
{
  for ( spelleffect_data_t& ed : _data( ptr ) )
  {
    if ( ed.id() == 0 )
    {
      ed._spell = ed._trigger_spell = spell_data_t::not_found();
    }
    else
    {
      ed._spell         = spell_data_t::find( ed.spell_id(), ptr );
      ed._trigger_spell = spell_data_t::find( ed.trigger_spell_id(), ptr );
    }
  }
}

util::span<spelleffect_data_t> spelleffect_data_t::_data( bool ptr )
{
  return SC_DBC_GET_DATA( __spelleffect_data, __ptr_spelleffect_data, ptr );
}

// ==========================================================================
// Spell Data
// ==========================================================================

school_e spell_data_t::get_school_type() const
{
  return dbc::get_school_type( as<uint32_t>( _school ) );
}

bool spell_data_t::is_race( race_e r ) const
{
  uint64_t mask = util::race_mask( r );
  return ( _race_mask & mask ) == mask;
}

bool spell_data_t::is_class( player_e c ) const
{
  if ( ! _class_mask )
    return true;

  unsigned mask = util::class_id_mask( c );
  return ( _class_mask & mask ) == mask;
}

std::string spell_data_t::to_str() const
{
  return fmt::format( " (ok={}) id={} name={} school={}",
                      ok(), id(), name_cstr(), util::school_type_string( get_school_type() ) );
}

// check if spell affected by effect through either class flag, label or category
bool spell_data_t::affected_by_all( const dbc_t& dbc, const spelleffect_data_t& effect ) const
{
  return affected_by( effect ) ||
         affected_by_label( effect ) ||
         affected_by_category( dbc, effect );
}

bool spell_data_t::affected_by_category( const dbc_t& dbc, const spelleffect_data_t& effect ) const
{
  return affected_by_category(dbc, effect.misc_value1());
}

bool spell_data_t::affected_by_category( const dbc_t& dbc, int category ) const
{
  const auto affected_spells = dbc.spells_by_category(category);
  return range::find(affected_spells, id(), &spell_data_t::id) != affected_spells.end();
}

bool spell_data_t::affected_by_label( const spelleffect_data_t& effect ) const
{
  return affected_by_label(effect.misc_value2());
}

bool spell_data_t::affected_by_label( int label ) const
{
  return range::contains(labels(), label, &spelllabel_data_t::label);
}

bool spell_data_t::affected_by( const spell_data_t* spell ) const
{
  if ( class_family() != spell -> class_family() )
    return false;

  for ( size_t flag_idx = 0; flag_idx < NUM_CLASS_FAMILY_FLAGS; flag_idx++ )
  {
    if ( ! class_flags( (int)flag_idx ) )
      continue;

    for ( size_t effect_idx = 1, end = spell -> effect_count(); effect_idx <= end; effect_idx++ )
    {
      const spelleffect_data_t& effect = spell -> effectN( effect_idx );
      if ( ! effect.id() )
        continue;

      if ( class_flags( (int)flag_idx ) & effect.class_flags( (int)flag_idx ) )
        return true;
    }
  }

  return false;
}

bool spell_data_t::affected_by( const spelleffect_data_t* effect ) const
{
  if ( class_family() != effect -> spell() -> class_family() )
    return false;

  for ( size_t flag_idx = 0; flag_idx < NUM_CLASS_FAMILY_FLAGS; flag_idx++ )
  {
    if ( ! class_flags( (int)flag_idx ) )
      continue;

    if ( class_flags( (int)flag_idx ) & effect -> class_flags( (int)flag_idx ) )
      return true;
  }

  return false;
}

bool spell_data_t::affected_by( const spelleffect_data_t& effect ) const
{
  return affected_by( &effect );
}

const spell_data_t* spell_data_t::find( unsigned spell_id, bool ptr )
{
  const auto __data = data( ptr );
  auto it = range::lower_bound( __data, spell_id, {}, &spell_data_t::id );
  if ( it != __data.end() && it->id() == spell_id )
    return &*it;
  return spell_data_t::nil();
}

const spell_data_t* spell_data_t::find( unsigned spell_id, util::string_view confirmation, bool ptr )
{
  ( void )confirmation;

  const spell_data_t* p = find( spell_id, ptr );
  assert( p && confirmation == p -> name_cstr() );
  return p;
}

const spell_data_t* spell_data_t::find( util::string_view name, bool ptr )
{
  const auto __data = data( ptr );
  auto it = range::find( __data, name, &spell_data_t::name_cstr );
  if ( it != __data.end() )
    return &*it;
  return nullptr;
}

util::span<const spell_data_t> spell_data_t::data( bool ptr )
{
  return _data( ptr );
}

template <typename T, size_t N>
static auto spell_data_linker(util::span<T, N> data) {
  return [index = size_t(0), data] (T*& ptr, size_t count) mutable {
    if ( count > 0 )
    {
      ptr = &data[index];
      index += count;
    }
  };
}

void spell_data_t::link( bool ptr )
{
  auto link_effects = spell_data_linker( spelleffect_data_t::data( ptr ) );
  auto link_power = spell_data_linker( spellpower_data_t::data( ptr ) );
  auto link_driver = spell_data_linker( SC_DBC_GET_DATA( __spelldriver_index_data, __ptr_spelldriver_index_data, ptr ) );
  auto link_labels = spell_data_linker( spelllabel_data_t::data( ptr ) );

  for ( spell_data_t& sd : _data( ptr ) )
  {
    link_effects( sd._effects, sd._effects_count );
    link_power( sd._power, sd._power_count );
    link_driver( sd._driver, sd._driver_count );
    link_labels( sd._labels, sd._labels_count );
  }
}

util::span<spell_data_t> spell_data_t::_data( bool ptr )
{
  return SC_DBC_GET_DATA( __spell_data, __ptr_spell_data, ptr );
}

// Hotfix data handling

util::span<const hotfix::client_hotfix_entry_t> spell_data_t::hotfixes( const spell_data_t& sd, bool ptr )
{
  auto data = SC_DBC_GET_DATA( __spell_hotfix_data, __ptr_spell_hotfix_data, ptr );
  return find_hotfixes( data, sd.id() );
}

util::span<const hotfix::client_hotfix_entry_t> spelleffect_data_t::hotfixes( const spelleffect_data_t& ed, bool ptr )
{
  auto data = SC_DBC_GET_DATA( __effect_hotfix_data, __ptr_effect_hotfix_data, ptr );
  return find_hotfixes( data, ed.id() );
}

util::span<const hotfix::client_hotfix_entry_t> spellpower_data_t::hotfixes( const spellpower_data_t& pd, bool ptr )
{
  auto data = SC_DBC_GET_DATA( __power_hotfix_data, __ptr_power_hotfix_data, ptr );
  return find_hotfixes( data, pd.id() );
}
