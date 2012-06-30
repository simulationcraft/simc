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

spell_data_nil_t::~spell_data_nil_t()
{
  _effects -> clear();
  delete _effects;
}

spell_data_not_found_t spell_data_not_found_t::singleton;

spell_data_not_found_t::spell_data_not_found_t() : spell_data_t()
{
  _effects = new std::vector< const spelleffect_data_t* >();
}

spell_data_not_found_t::~spell_data_not_found_t()
{
  _effects -> clear();
  delete _effects;
}

// spell_data_t::is_class ===================================================

bool spell_data_t::is_class( player_e c ) const
{
  if ( ! _class_mask )
    return true;

  unsigned mask = util::class_id_mask( c );
  return ( _class_mask & mask ) == mask;
}

// spell_data_t::is_race ====================================================

bool spell_data_t::is_race( race_e r ) const
{
  unsigned mask = util::race_mask( r );
  return ( _race_mask & mask ) == mask;
}

// spell_data_t::scaling_class ==============================================

player_e spell_data_t::scaling_class() const
{
  switch ( _scaling_type )
  {
  case -2: return PLAYER_SPECIAL_SCALE2;
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
  case 10: return MONK;
  case 11: return DRUID;
  default: break;
  }

  return PLAYER_NONE;
}

// spell_data_t::cost =======================================================

double spell_data_t::cost( power_e pt ) const
{
  if ( _power == 0 )
    return 0;

  for ( size_t i = 0; i < _power -> size(); i++ )
  {
    if ( _power -> at( i ) -> _power_e == pt )
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

std::string spell_data_t::to_str() const
{
  std::ostringstream s;

  s << " (ok=" << ( ok() ? "true" : "false" ) << ")";
  s << " id=" << id();
  s << " name=" << name_cstr();
  s << " school=" << util::school_type_string( get_school_type() );
  return s.str();
}

// ==========================================================================
// Spell Effect Data
// ==========================================================================

spelleffect_data_nil_t spelleffect_data_nil_t::singleton;

spelleffect_data_nil_t::spelleffect_data_nil_t() :
  spelleffect_data_t()
{ _spell = _trigger_spell = spell_data_t::nil(); }

resource_e spelleffect_data_t::resource_gain_type() const
{
  return util::translate_power_type( static_cast< power_e >( misc_value1() ) );
}

double spelleffect_data_t::resource( resource_e resource_type ) const
{
  switch ( resource_type )
  {
  case RESOURCE_RUNIC_POWER:
  case RESOURCE_RAGE:
    return base_value() * ( 1 / 10.0 );
  case RESOURCE_MANA:
    return base_value() * ( 1 / 100.0 );
  default:
    return base_value();
  }
}

// ==========================================================================
// Spell Power Data
// ==========================================================================

spellpower_data_nil_t::spellpower_data_nil_t() :
  spellpower_data_t()
{ }

spellpower_data_nil_t spellpower_data_nil_t::singleton;

// spell_data_t::power_e =================================================

resource_e spellpower_data_t::resource() const
{
  return util::translate_power_type( type() );
}

double spellpower_data_t::cost_divisor() const
{  
  switch ( type() )
  {
  case POWER_MANA:
  case POWER_SOUL_SHARDS:
    return 100.0;
  case POWER_RAGE:
  case POWER_RUNIC_POWER:
  case POWER_BURNING_EMBER:
    return 10.0;
  default:
    return 1.0;
  }
}

double spellpower_data_t::cost() const
{
  double cost = 0.0;

  if ( _cost > 0 )
    cost = _cost;
  else
    cost = _cost_2;

  return cost / cost_divisor();;
}

double spellpower_data_t::cost_per_second() const
{
  double cost = 0.0;

  if ( _cost_per_second > 0 )
    cost = _cost_per_second;
  else
    cost = _cost_per_second_2;

  return cost / cost_divisor();
}

// ==========================================================================
// Talent Data
// ==========================================================================

talent_data_nil_t talent_data_nil_t::singleton;

talent_data_nil_t::talent_data_nil_t() :
  talent_data_t()
{ spell1 = spell_data_t::nil(); }


bool talent_data_t::is_class( player_e c ) const
{
  unsigned mask = util::class_id_mask( c );

  if ( mask == 0 )
    return false;

  return ( ( _m_class & mask ) == mask );
}

bool talent_data_t::is_pet( pet_e p ) const
{
  unsigned mask = util::pet_mask( p );

  if ( mask == 0 )
    return false;

  return ( ( _m_pet & mask ) == mask );
}
