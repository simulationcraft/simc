// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Spell Data
// ==========================================================================

spell_data_nil_t spell_data_nil_t::singleton;

spell_data_nil_t::spell_data_nil_t() : spell_data_t()
{ _effect1 = _effect2 = _effect3 = spelleffect_data_t::nil(); }

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

// spell_data_t::power_type =================================================

resource_type spell_data_t::power_type() const
{
  switch ( _power_type )
  {
  case POWER_MANA:        return RESOURCE_MANA;
  case POWER_RAGE:        return RESOURCE_RAGE;
  case POWER_FOCUS:       return RESOURCE_FOCUS;
  case POWER_ENERGY:      return RESOURCE_ENERGY;
  case POWER_HAPPINESS:   return RESOURCE_HAPPINESS;
  // rune power types are not currently used
  // case POWER_RUNE:        return RESOURCE_RUNE;
  // case POWER_RUNE_BLOOD:  return RESOURCE_RUNE_BLOOD;
  // case POWER_RUNE_UNHOLY: return RESOURCE_RUNE_UNHOLY;
  // case POWER_RUNE_FROST:  return RESOURCE_RUNE_FROST;
  case POWER_RUNIC_POWER: return RESOURCE_RUNIC;
  case POWER_SOUL_SHARDS: return RESOURCE_SOUL_SHARDS;
  case POWER_ECLIPSE:     return RESOURCE_ECLIPSE;
  case POWER_HOLY_POWER:  return RESOURCE_HOLY_POWER;
  case POWER_HEALTH:      return RESOURCE_HEALTH;
  }
  return RESOURCE_NONE;
}

// spell_data_t::is_class ===================================================

bool spell_data_t::is_class( player_type c ) const
{
  if ( ! _class_mask )
    return true;

  unsigned mask = util_t::class_id_mask( c );
  return ( _class_mask & mask ) == mask;
}

// spell_data_t::is_race ====================================================

bool spell_data_t::is_race( race_type r ) const
{
  unsigned mask = util_t::race_mask( r );
  return ( _race_mask & mask ) == mask;
}

// spell_data_t::scaling_class ==============================================

player_type spell_data_t::scaling_class() const
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

double spell_data_t::cost() const
{
  double divisor;

  switch ( _power_type )
  {
  case POWER_MANA:
    divisor = 100;
    break;
  case POWER_RAGE:
  case POWER_RUNIC_POWER:
    divisor = 10;
    break;
  default:
    divisor = 1;
    break;
  }

  return _cost / divisor;
}

// spell_data_t::cast_time ==================================================

double spell_data_t::cast_time( uint32_t level ) const
{
  if ( _cast_div < 0 )
  {
    if ( _cast_min < 0 )
      return 0;
    return _cast_min / 1000.0;
  }

  if ( level >= static_cast<uint32_t>( _cast_div ) )
    return _cast_max / 1000.0;

  return ( _cast_min + ( _cast_max - _cast_min ) * ( level - 1 ) / ( double )( _cast_div - 1 ) ) / 1000.0;
}

// spell_data_t::flags ======================================================

bool spell_data_t::flags( spell_attribute_t f ) const
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

bool talent_data_t::is_class( player_type c ) const
{
  unsigned mask = util_t::class_id_mask( c );

  if ( mask == 0 )
    return false;

  return ( ( _m_class & mask ) == mask );
}

bool talent_data_t::is_pet( pet_type_t p ) const
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
