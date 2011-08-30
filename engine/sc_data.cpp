#include "simulationcraft.h"

const char* spell_data_t::name_cstr() SC_CONST
{
  return _name;
}

bool spell_data_t::is_used() SC_CONST
{
  return _flags & 0x01;
}

void spell_data_t::set_used( bool value )
{
  _flags &= ( uint32_t ) ~( ( uint32_t ) 0x01 );
  _flags |= value ? 0x01 : 0x00;
}

bool spell_data_t::is_enabled() SC_CONST
{
  return ( ( _flags & 0x02 ) == 0x00 );
}


void spell_data_t::set_enabled( bool value )
{
  _flags &= ( uint32_t ) ~( ( uint32_t ) 0x02 );
  _flags |= value ? 0x02 : 0x00;
}

double spell_data_t::missile_speed() SC_CONST
{
  return _prj_speed;
}

uint32_t spell_data_t::school_mask() SC_CONST
{
  return _school;
}

resource_type spell_data_t::power_type() SC_CONST
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

bool spell_data_t::is_class( player_type c ) SC_CONST
{
  uint32_t mask, mask2;

  mask = util_t::class_id_mask( c );
  mask2 = _class_mask;

  if ( mask2 == 0x0 )
    return true;

  return ( ( mask2 & mask ) == mask );
}

bool spell_data_t::is_race( race_type r ) SC_CONST
{
  uint32_t mask, mask2;

  mask = util_t::race_mask( r );
  mask2 = _race_mask;

  return ( ( mask2 & mask ) == mask );
}

bool spell_data_t::is_level( uint32_t level ) SC_CONST
{
  return ( level >= _spell_level );
}

uint32_t spell_data_t::level() SC_CONST
{
  return _spell_level;
}

uint32_t spell_data_t::max_level() SC_CONST
{
  return _max_level;
}

uint32_t spell_data_t::class_mask() SC_CONST
{
  return _class_mask;
}

uint32_t spell_data_t::race_mask() SC_CONST
{
  return _race_mask;
}

player_type spell_data_t::scaling_class() SC_CONST
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

double spell_data_t::min_range() SC_CONST
{
  return _min_range;
}

double spell_data_t::max_range() SC_CONST
{
  return _max_range;
}

bool spell_data_t::in_range( double range ) SC_CONST
{
  return ( ( range >= _min_range ) && ( range <= _max_range ) );
}

double spell_data_t::cooldown() SC_CONST
{
  return _cooldown / 1000.0;
}

double spell_data_t::gcd() SC_CONST
{
  return _gcd / 1000.0;
}

uint32_t spell_data_t::category() SC_CONST
{
  return _category;
}

double spell_data_t::duration() SC_CONST
{
  return _duration / 1000.0;
}

double spell_data_t::cost() SC_CONST
{
  double divisor = 1.0;

  switch ( _power_type )
  {
  case POWER_MANA:        divisor = 100.0; break;
  case POWER_RAGE:        divisor =  10.0; break;
  case POWER_RUNIC_POWER: divisor =  10.0; break;
  }

  return ( double ) _cost / divisor;
}

uint32_t spell_data_t::rune_cost() SC_CONST
{
  return _rune_cost;
}

double spell_data_t::runic_power_gain() SC_CONST
{
  return _runic_power_gain / 10.0;
}

uint32_t spell_data_t::max_stacks() SC_CONST
{
  return _max_stack;
}

uint32_t spell_data_t::initial_stacks() SC_CONST
{
  return _proc_charges;
}

double spell_data_t::proc_chance() SC_CONST
{
  return _proc_chance / 100.0;
}

double spell_data_t::cast_time( uint32_t level ) SC_CONST
{
  int min_cast = _cast_min;
  int max_cast = _cast_max;
  int div_cast = _cast_div;

  if ( div_cast < 0 )
  {
    if ( min_cast < 0 )
      return 0.0;

    return ( double ) min_cast;
  }

  if ( level >= ( uint32_t ) div_cast )
    return max_cast / 1000.0;

  return ( 1.0 * min_cast + ( 1.0 * max_cast - min_cast ) * ( level - 1 ) / ( 1.0 * div_cast - 1 ) ) / 1000.0;
}

uint32_t spell_data_t::effect_id( uint32_t effect_num ) SC_CONST
{
  assert( ( effect_num >= 1 ) && ( effect_num <= MAX_EFFECTS ) );

  return _effect[ effect_num - 1 ];
}

bool spell_data_t::flags( spell_attribute_t f ) SC_CONST
{
  uint32_t index, bit, mask;

  index = ( ( ( uint32_t ) f ) >> 8 ) & 0x000000FF;
  bit = ( ( uint32_t ) f ) & 0x0000001F;
  mask = ( 1 << bit );

  assert( index < NUM_SPELL_FLAGS );

  return ( ( _attributes[ index ] & mask ) == mask );
}

const char* spell_data_t::desc() SC_CONST
{
  return _desc;
}

const char* spell_data_t::tooltip() SC_CONST
{
  return _tooltip;
}

double spell_data_t::scaling_multiplier() SC_CONST
{
  return _c_scaling;
}

double spell_data_t::extra_coeff() SC_CONST
{
  return _extra_coeff;
}

unsigned spell_data_t::scaling_threshold() SC_CONST
{
  return _c_scaling_level;
}

bool spelleffect_data_t::is_used() SC_CONST
{
  return ( ( _flags & 0x01 ) == 0x01 );
}

void spelleffect_data_t::set_used( bool value )
{
  _flags &= ( uint32_t ) ~( ( uint32_t ) 0x01 );
  _flags |= value ? 0x01 : 0x00;
}

bool spelleffect_data_t::is_enabled() SC_CONST
{
  return ( ( _flags & 0x02 ) == 0x02 );
}

void spelleffect_data_t::set_enabled( bool value )
{
  _flags &= ( uint32_t ) ~( ( uint32_t ) 0x02 );
  _flags |= value ? 0x02 : 0x00;
}

uint32_t spelleffect_data_t::spell_id() SC_CONST
{
  return _spell_id;
}

unsigned spelleffect_data_t::index() SC_CONST
{
  return _index;
}

uint32_t spelleffect_data_t::spell_effect_num() SC_CONST
{
  return _index;
}

effect_type_t spelleffect_data_t::type() SC_CONST
{
  return _type;
}

effect_subtype_t spelleffect_data_t::subtype() SC_CONST
{
  return _subtype;
}

uint32_t spelleffect_data_t::trigger_spell_id() SC_CONST
{
  if ( _trigger_spell_id < 0 )
    return 0;

  return ( uint32_t ) _trigger_spell_id;
}

double spelleffect_data_t::chain_multiplier() SC_CONST
{
  return _m_chain;
}

double spelleffect_data_t::m_average() SC_CONST
{
  return _m_avg;
}

double spelleffect_data_t::m_delta() SC_CONST
{
  return _m_delta;
}

double spelleffect_data_t::m_unk() SC_CONST
{
  return _m_unk;
}

double spelleffect_data_t::coeff() SC_CONST
{
  return _coeff;
}

double spelleffect_data_t::period() SC_CONST
{
  return _amplitude / 1000.0;
}

double spelleffect_data_t::radius() SC_CONST
{
  return _radius;
}

double spelleffect_data_t::radius_max() SC_CONST
{
  return _radius_max;
}

double spelleffect_data_t::pp_combo_points() SC_CONST
{
  return _pp_combo_points;
}

double spelleffect_data_t::real_ppl() SC_CONST
{
  return _real_ppl;
}

int spelleffect_data_t::die_sides() SC_CONST
{
  return _die_sides;
}

const char* talent_data_t::name_cstr() SC_CONST
{
  return _name;
}

bool talent_data_t::is_used() SC_CONST
{
  return ( ( _flags & 0x01 ) == 0x01 );
}

void talent_data_t::set_used( bool value )
{
  _flags &= ( uint32_t ) ~( ( uint32_t ) 0x01 );
  _flags |= value ? 0x01 : 0x00;
}

bool talent_data_t::is_enabled() SC_CONST
{
  return ( ( _flags & 0x02 ) == 0x00 );
}

void talent_data_t::set_enabled( bool value )
{
  _flags &= ( uint32_t ) ~( ( uint32_t ) 0x02 );
  _flags |= value ? 0x00 : 0x02;
}

uint32_t talent_data_t::tab_page() SC_CONST
{
  return _tab_page;
}

bool talent_data_t::is_class( player_type c ) SC_CONST
{
  unsigned mask = util_t::class_id_mask( c );

  if ( mask == 0 )
    return false;

  return ( ( _m_class & mask ) == mask );
}

bool talent_data_t::is_pet( pet_type_t p ) SC_CONST
{
  unsigned mask = util_t::pet_mask( p );

  if ( mask == 0 )
    return false;

  return ( ( _m_pet & mask ) == mask );
}

uint32_t talent_data_t::depends_id() SC_CONST
{
  return _dependance;
}

uint32_t talent_data_t::depends_rank() SC_CONST
{
  return _depend_rank + 1;
}

uint32_t talent_data_t::col() SC_CONST
{
  return _col;
}

uint32_t talent_data_t::row() SC_CONST
{
  return _row;
}

unsigned talent_data_t::mask_class() SC_CONST
{
  return _m_class;
}

unsigned talent_data_t::mask_pet() SC_CONST
{
  return _m_pet;
}

uint32_t talent_data_t::rank_spell_id( uint32_t rank ) SC_CONST
{
  assert( rank <= MAX_RANK );

  if ( rank == 0 )
    return 0;

  return _rank_id[ rank - 1 ];
}

uint32_t talent_data_t::max_rank() SC_CONST
{
  uint32_t i = 0;

  for ( i = 0; i < MAX_RANK; i++ )
  {
    if ( _rank_id[ i ] == 0 )
    {
      return i;
    }
  }

  return i;
}
