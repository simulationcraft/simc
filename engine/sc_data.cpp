// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Spell Data
// ==========================================================================

spell_data_nil_t spell_data_nil_t::singleton;

spell_data_nil_t::spell_data_nil_t() : spell_data_t()
{
  _effects = new std::vector< const spelleffect_data_t* >();
}

// spell_data_t::set_used ===================================================

void spell_data_t::set_used( bool value )
{
  if ( value )
    _flags |= FLAG_USED;
  else
    _flags &= ~FLAG_USED;
}

// spell_data_t::set_enabled ================================================

void spell_data_t::set_enabled( bool value )
{
  if ( value )
    _flags &= ~FLAG_DISABLED;
  else
    _flags |= FLAG_DISABLED;
}

// spell_data_t::is_class ===================================================

bool spell_data_t::is_class( player_type_e c ) const
{
  if ( ! _class_mask )
    return true;

  unsigned mask = util_t::class_id_mask( c );
  return ( _class_mask & mask ) == mask;
}

// spell_data_t::is_race ====================================================

bool spell_data_t::is_race( race_type_e r ) const
{
  unsigned mask = util_t::race_mask( r );
  return ( _race_mask & mask ) == mask;
}

// spell_data_t::scaling_class ==============================================

player_type_e spell_data_t::scaling_class() const
{
  switch ( _scaling_type )
  {
  case -1: return PLAYER_SPECIAL_SCALE;
  case 1:  return WARRIOR;
  case 2:  return PALADIN;
  case 3:  return HUNTER;
  case 4:  return ROGUE;
  case 5:  return PRIEST;
  case 6:  return DEATH_KNIGHT;
  case 7:  return SHAMAN;
  case 8:  return MAGE;
  case 9:  return WARLOCK;
  case 11: return DRUID;
  default: break;
  }

  return PLAYER_NONE;
}

// spell_data_t::cost =======================================================

double spell_data_t::cost( power_type_e pt ) const
{
  if ( _power == 0 )
    return 0;

  for ( size_t i = 0; i < _power -> size(); i++ )
  {
    if ( _power -> at( i ) -> _power_type_e == pt )
      return _power -> at( i ) -> cost();
  }

  return 0.0;
}

// spell_data_t::cast_time ==================================================

timespan_t spell_data_t::cast_time( uint32_t level ) const
{
  if ( _cast_div < 0 )
  {
    if ( _cast_min < 0 )
      return timespan_t::zero();
    return timespan_t::from_millis( _cast_min );
  }

  if ( level >= static_cast<uint32_t>( _cast_div ) )
    return timespan_t::from_millis( _cast_max );

  return timespan_t::from_millis( _cast_min + ( _cast_max - _cast_min ) * ( level - 1 ) / ( double )( _cast_div - 1 ) );
}

// spell_data_t::flags ======================================================

bool spell_data_t::flags( spell_attribute_e f ) const
{
  unsigned bit = static_cast<unsigned>( f ) & 0x1Fu;
  unsigned index = ( static_cast<unsigned>( f ) >> 8 ) & 0xFFu;
  uint32_t mask = 1u << bit;

  assert( index < NUM_SPELL_FLAGS );

  return ( _attributes[ index ] & mask ) != 0;
}

// ==========================================================================
// Spell Effect Data
// ==========================================================================

spelleffect_data_nil_t spelleffect_data_nil_t::singleton;

spelleffect_data_nil_t::spelleffect_data_nil_t() :
  spelleffect_data_t()
{ _spell = _trigger_spell = spell_data_t::nil(); }

void spelleffect_data_t::set_used( bool value )
{
  if ( value )
    _flags |= FLAG_USED;
  else
    _flags &= ~FLAG_USED;
}

void spelleffect_data_t::set_enabled( bool value )
{
  if ( value )
    _flags |= FLAG_ENABLED;
  else
    _flags &= ~FLAG_ENABLED;
}

resource_type_e spelleffect_data_t::resource_gain_type() const
{
  return util_t::translate_power_type( static_cast< power_type_e >( _misc_value ) );
}

// ==========================================================================
// Spell Power Data
// ==========================================================================

spellpower_data_nil_t::spellpower_data_nil_t() :
  spellpower_data_t()
{ }

spellpower_data_nil_t spellpower_data_nil_t::singleton;

// spell_data_t::power_type_e =================================================

resource_type_e spellpower_data_t::resource() const
{
  return util_t::translate_power_type( static_cast< power_type_e >( _power_type_e ) );
}

// ==========================================================================
// Talent Data
// ==========================================================================

talent_data_nil_t talent_data_nil_t::singleton;

talent_data_nil_t::talent_data_nil_t() :
  talent_data_t()
{ spell1 = spell2 = spell3 = spell_data_t::nil(); }

void talent_data_t::set_used( bool value )
{
  if ( value )
    _flags |= FLAG_USED;
  else
    _flags &= ~FLAG_USED;
}

void talent_data_t::set_enabled( bool value )
{
  if ( value )
    _flags &= ~FLAG_DISABLED;
  else
    _flags |= FLAG_DISABLED;
}

bool talent_data_t::is_class( player_type_e c ) const
{
  unsigned mask = util_t::class_id_mask( c );

  if ( mask == 0 )
    return false;

  return ( ( _m_class & mask ) == mask );
}

bool talent_data_t::is_pet( pet_type_e p ) const
{
  unsigned mask = util_t::pet_mask( p );

  if ( mask == 0 )
    return false;

  return ( ( _m_pet & mask ) == mask );
}

unsigned talent_data_t::rank_spell_id( unsigned rank ) const
{
  assert( rank <= MAX_RANK );

  if ( rank == 0 )
    return 0;

  return _rank_id[ rank - 1 ];
}

unsigned talent_data_t::max_rank() const
{
  unsigned i;

  for ( i = 0; i < MAX_RANK; i++ )
  {
    if ( _rank_id[ i ] == 0 )
      break;
  }

  return i;
}
