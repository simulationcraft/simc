#include "simulationcraft.h"

sc_data_access_t::sc_data_access_t( sc_data_t* p, const bool ptr ) : sc_data_t( p, ptr )
{
  
}

sc_data_access_t::sc_data_access_t( const sc_data_t& copy ) : sc_data_t( copy )
{

}

/************* Spells ************/

bool sc_data_access_t::spell_exists( const uint32_t spell_id ) SC_CONST
{
  return m_spells_index && ( spell_id < m_spells_index_size ) && m_spells_index[ spell_id ];
}

const char* sc_data_access_t::spell_name_str( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return "";

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->name;
}

bool sc_data_access_t::spell_is_used( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return false;

  assert( spell_exists( spell_id ) );

  return ( ( m_spells_index[ spell_id ]->flags & 0x01 ) == 0x01 );
}


void sc_data_access_t::spell_set_used( const uint32_t spell_id, const bool value )
{
  assert( spell_exists( spell_id ) );

  m_spells_index[ spell_id ]->flags &= (uint32_t) ~( (uint32_t)0x01 ); 
  m_spells_index[ spell_id ]->flags |= value ? 0x01 : 0x00;
}

bool sc_data_access_t::spell_is_enabled( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return false;

  assert( spell_exists( spell_id ) );

  return ( ( m_spells_index[ spell_id ]->flags & 0x02 ) == 0x00 );
}


void sc_data_access_t::spell_set_enabled( const uint32_t spell_id, const bool value )
{
  assert( spell_exists( spell_id ) );

  m_spells_index[ spell_id ]->flags &= (uint32_t) ~( (uint32_t)0x02 ); 
  m_spells_index[ spell_id ]->flags |= value ? 0x00 : 0x02;
}

double sc_data_access_t::spell_missile_speed( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return 0.0;

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->prj_speed;
}

uint32_t sc_data_access_t::spell_school_mask( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return 0;

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->school;
}

resource_type sc_data_access_t::spell_power_type( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return RESOURCE_NONE;

  assert( spell_exists( spell_id ) );

  switch ( m_spells_index[ spell_id ]->power_type )
  {
  case POWER_MANA:        return RESOURCE_MANA;
  case POWER_RAGE:        return RESOURCE_RAGE;
  case POWER_FOCUS:       return RESOURCE_FOCUS;
  case POWER_ENERGY:      return RESOURCE_ENERGY;
  case POWER_HAPPINESS:   return RESOURCE_HAPPINESS;
  case POWER_RUNE:        return RESOURCE_RUNE;
  case POWER_RUNIC_POWER: return RESOURCE_RUNIC;
  case POWER_SOUL_SHARDS: return RESOURCE_SOUL_SHARDS;
  case POWER_ECLIPSE:     return RESOURCE_ECLIPSE;
  case POWER_HEALTH:      return RESOURCE_HEALTH;
  }
  return RESOURCE_NONE;
}

bool sc_data_access_t::spell_is_class( const uint32_t spell_id, const player_type c ) SC_CONST
{
  if ( !spell_id )
    return false;

  uint32_t mask,mask2;

  assert( spell_exists( spell_id ) );

  mask = get_class_mask( c );
  mask2 = m_spells_index[ spell_id ]->class_mask;

  if ( mask2 == 0x0 )
    return true;

  return ( ( mask2 & mask ) == mask );
}

bool sc_data_access_t::spell_is_race( const uint32_t spell_id, const race_type r ) SC_CONST
{
  if ( !spell_id )
    return false;

  uint32_t mask,mask2;

  assert( spell_exists( spell_id ) );

  mask = get_race_mask( r );
  mask2 = m_spells_index[ spell_id ]->race_mask;

  if ( mask2 == 0x0 )
    return true;

  return ( ( mask2 & mask ) == mask );
}

bool sc_data_access_t::spell_is_level( const uint32_t spell_id, const uint32_t level ) SC_CONST
{
  if ( !spell_id )
    return false;

  assert( spell_exists( spell_id ) && ( level > 0 ) );

  return ( level >= m_spells_index[ spell_id ]->spell_level );
}

uint32_t sc_data_access_t::spell_level( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return false;

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->spell_level;
}

player_type sc_data_access_t::spell_scaling_class( uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return PLAYER_NONE;

  assert( spell_exists( spell_id ) );

  int c = m_spells_index[ spell_id ] -> scaling_type;

  return get_class_type( c );
}


double sc_data_access_t::spell_min_range( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return 0.0;

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->min_range;
}

double sc_data_access_t::spell_max_range( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return 0.0;

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->max_range;
}

bool sc_data_access_t::spell_in_range( const uint32_t spell_id, const double range ) SC_CONST
{
  if ( !spell_id )
    return false;

  assert( spell_exists( spell_id ) );

  return ( ( range >= m_spells_index[ spell_id ]->min_range ) && ( range <= m_spells_index[ spell_id ]->max_range ) );
}

double sc_data_access_t::spell_cooldown( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return 0.0;

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->cooldown / 1000.0;
}

double sc_data_access_t::spell_gcd( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return 0.0;

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->gcd / 1000.0;
}

uint32_t sc_data_access_t::spell_category( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return 0;

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->category;
}

double sc_data_access_t::spell_duration( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return 0.0;

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->duration / 1000.0;
}

double sc_data_access_t::spell_cost( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return 0.0;

  assert( spell_exists( spell_id ) );

  if ( m_spells_index[ spell_id ]->power_type == POWER_MANA )
  {
    // Return as a fraction of base mana.
    return m_spells_index[ spell_id ]->cost / 100.0;
  }

  return ( double ) m_spells_index[ spell_id ]->cost;
}

uint32_t sc_data_access_t::spell_rune_cost( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return 0;

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->rune_cost;
}

double sc_data_access_t::spell_runic_power_gain( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return 0.0;

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->runic_power_gain / 10.0;
}

uint32_t sc_data_access_t::spell_max_stacks( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return 0;

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->max_stack;
}

uint32_t sc_data_access_t::spell_initial_stacks( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return 0;

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->proc_charges;
}

double sc_data_access_t::spell_proc_chance( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return 0.0;

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->proc_chance / 100.0;
}

double sc_data_access_t::spell_cast_time( const uint32_t spell_id, const uint32_t level ) SC_CONST
{
  if ( !spell_id )
    return 0.0;

  assert( spell_exists( spell_id ) && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  int min_cast = m_spells_index[ spell_id ]->cast_min;
  int max_cast = m_spells_index[ spell_id ]->cast_max;
  int div_cast = m_spells_index[ spell_id ]->cast_div;

  if ( div_cast < 0 )
  {
    if ( min_cast < 0 )
      return 0.0;

    return ( double ) min_cast;
  }

  if ( level >= ( uint32_t) div_cast )
    return max_cast/1000.0;
  
  return ( 1.0 * min_cast + ( 1.0 * max_cast - min_cast ) * ( level - 1 ) / ( 1.0 * div_cast - 1 )) / 1000.0;
}

uint32_t sc_data_access_t::spell_effect_id( const uint32_t spell_id, const uint32_t effect_num ) SC_CONST
{
  if ( !spell_id )
    return 0;

  assert( spell_exists( spell_id ) && ( effect_num >= 1 ) && ( effect_num  <= MAX_EFFECTS ) );

  return m_spells_index[ spell_id ]->effect[ effect_num - 1 ];
}

bool sc_data_access_t::spell_flags( const uint32_t spell_id, const spell_attribute_t f ) SC_CONST
{
  if ( !spell_id )
    return 0.0;

  assert( spell_exists( spell_id ) );

  uint32_t index, bit, mask;

  index = ( ( ( uint32_t ) f ) >> 8 ) & 0x000000FF;
  bit = ( ( uint32_t ) f ) & 0x0000001F;
  mask = ( 1 << bit );

  assert( index < NUM_SPELL_FLAGS );

  return ( ( m_spells_index[ spell_id ]->attributes[ index ] & mask ) == mask );
}

const char* sc_data_access_t::spell_desc( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return "";

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->desc;
}

const char* sc_data_access_t::spell_tooltip( const uint32_t spell_id ) SC_CONST
{
  if ( !spell_id )
    return "";

  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->tooltip;
}

double sc_data_access_t::spell_scaling_multiplier( uint32_t spell_id ) SC_CONST
{
 if ( ! spell_id ) 
   return 0.0;
   
 assert( spell_exists( spell_id ) );
 
 return m_spells_index[ spell_id ] -> c_scaling;
}

double sc_data_access_t::spell_extra_coeff( uint32_t spell_id ) SC_CONST
{
 if ( ! spell_id ) 
   return 0.0;
   
 assert( spell_exists( spell_id ) );
 
 return m_spells_index[ spell_id ] -> extra_coeff;
}

unsigned sc_data_access_t::spell_scaling_threshold( uint32_t spell_id ) SC_CONST
{
  if ( ! spell_id )
    return 0;
    
  assert( spell_exists( spell_id ) );
  
  return m_spells_index[ spell_id ] -> c_scaling_level;
}

/************ Effects ******************/

bool sc_data_access_t::effect_exists( const uint32_t effect_id ) SC_CONST
{
  return m_effects_index && ( effect_id < m_effects_index_size ) && m_effects_index[ effect_id ];
}

bool sc_data_access_t::effect_is_used( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return false;

  assert( effect_exists( effect_id ) );

  return ( ( m_effects_index[ effect_id ]->flags & 0x01 ) == 0x01 );
}


void sc_data_access_t::effect_set_used( const uint32_t effect_id, const bool value )
{
  assert( effect_exists( effect_id ) );

  m_effects_index[ effect_id ]->flags &= (uint32_t) ~( (uint32_t)0x01 ); 
  m_effects_index[ effect_id ]->flags |= value ? 0x01 : 0x00;
}

bool sc_data_access_t::effect_is_enabled( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return false;

  assert( effect_exists( effect_id ) );

  return ( ( m_effects_index[ effect_id ]->flags & 0x02 ) == 0x02 );
}

void sc_data_access_t::effect_set_enabled( const uint32_t effect_id, const bool value )
{
  assert( effect_exists( effect_id ) );

  m_effects_index[ effect_id ]->flags &= (uint32_t) ~( (uint32_t)0x02 ); 
  m_effects_index[ effect_id ]->flags |= value ? 0x02 : 0x00;
}

uint32_t sc_data_access_t::effect_spell_id( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->spell_id;
}

uint32_t sc_data_access_t::effect_spell_effect_num( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->index;
}

int32_t sc_data_access_t::effect_type( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->type;
}

int32_t sc_data_access_t::effect_subtype( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->subtype;
}

int32_t sc_data_access_t::effect_base_value( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->base_value;
}

int32_t sc_data_access_t::effect_misc_value1( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->misc_value;
}

int32_t sc_data_access_t::effect_misc_value2( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->misc_value_2;
}

uint32_t sc_data_access_t::effect_trigger_spell_id( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0;

  assert( effect_exists( effect_id ) );

  if ( m_effects_index[ effect_id ]->trigger_spell_id < 0 )
    return 0;

  return ( uint32_t ) m_effects_index[ effect_id ]->trigger_spell_id;
}

double sc_data_access_t::effect_chain_multiplier( uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0;

  assert( effect_exists( effect_id ) );

  return ( uint32_t ) m_effects_index[ effect_id ]->m_chain;
}

double sc_data_access_t::effect_m_average( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0.0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->m_avg;
}

double sc_data_access_t::effect_m_delta( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0.0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->m_delta;
}

double sc_data_access_t::effect_m_unk( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0.0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->m_unk;
}

double sc_data_access_t::effect_coeff( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0.0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->coeff;
}

double sc_data_access_t::effect_period( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0.0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->amplitude / 1000.0;
}

double sc_data_access_t::effect_radius( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0.0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->radius;
}

double sc_data_access_t::effect_radius_max( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0.0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->radius_max;
}

double sc_data_access_t::effect_pp_combo_points( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0.0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->pp_combo_points;
}

double sc_data_access_t::effect_real_ppl( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0.0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->real_ppl;
}

int sc_data_access_t::effect_die_sides( const uint32_t effect_id ) SC_CONST
{
  if ( !effect_id )
    return 0;

  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->die_sides;
}

double sc_data_access_t::effect_average( const uint32_t effect_id, const player_type c, const uint32_t level ) SC_CONST
{
  if ( !effect_id )
    return 0.0;

  assert( effect_exists( effect_id ) &&
         ( level > 0 ) && ( level <= MAX_LEVEL ) );

  uint32_t c_id = get_class_id( c );

  if ( effect_m_average( effect_id ) != 0 && c_id != 0 )
  {
    double* p_scale_ref = m_spell_scaling.ptr( level - 1, c_id );

    assert( p_scale_ref != NULL );

    return effect_m_average( effect_id ) * *p_scale_ref;
  }
  else if ( effect_real_ppl( effect_id ) != 0 )
  {
    assert( m_spells_index[ effect_spell_id( effect_id ) ] );
    const spell_data_t* spell = m_spells_index[ effect_spell_id( effect_id ) ];

    return effect_base_value( effect_id ) + 
      ( ( spell -> max_level > 0 ? std::min( level, spell -> max_level ) : level ) - spell -> spell_level ) * effect_real_ppl( effect_id );
  }
  else
    return effect_base_value( effect_id );
}

double sc_data_access_t::effect_delta( const uint32_t effect_id, const player_type c, const uint32_t level ) SC_CONST
{
  if ( !effect_id )
    return 0.0;

  assert( effect_exists( effect_id ) &&
         ( level > 0 ) && ( level <= MAX_LEVEL ) );

  uint32_t c_id = get_class_id( c );
  
  if ( effect_m_delta( effect_id ) != 0 && c_id != 0 )
  {
    double* p_scale_ref = m_spell_scaling.ptr( level - 1, c_id );

    assert( p_scale_ref != NULL );

    return effect_m_average( effect_id ) * effect_m_delta( effect_id ) * *p_scale_ref;
  }
  else if ( ( effect_m_average( effect_id ) == 0.0 ) && ( effect_m_delta( effect_id ) == 0.0 ) && ( effect_die_sides( effect_id ) != 0 ) )
    return effect_die_sides( effect_id );
  
  return 0;
}

double sc_data_access_t::effect_bonus( const uint32_t effect_id, const player_type c, const uint32_t level ) SC_CONST
{
  if ( !effect_id )
    return 0.0;

  assert( effect_exists( effect_id ) &&
         ( level > 0 ) && ( level <= MAX_LEVEL ) );

  uint32_t c_id = get_class_id( c );
  
  if ( effect_m_unk( effect_id ) != 0 && c_id != 0 )
  {
    double* p_scale_ref = m_spell_scaling.ptr( level - 1, c_id );

    assert( p_scale_ref != NULL );

    return effect_m_unk( effect_id ) * *p_scale_ref;
  }
  else
    return effect_pp_combo_points( effect_id );
  
  return 0;
}

double sc_data_access_t::effect_min( const uint32_t effect_id, const player_type c, const uint32_t level ) SC_CONST
{
  if ( !effect_id )
    return 0.0;

  assert( effect_exists( effect_id ) &&
         ( level > 0 ) && ( level <= MAX_LEVEL ) );

  double avg, delta, result;

  avg = effect_average( effect_id, c, level );

  if ( get_class_id( c ) != 0 && 
     ( effect_m_average( effect_id ) != 0 || effect_m_delta( effect_id ) != 0 ) )
  {
    delta = effect_delta( effect_id, c, level );
    result = avg - ( delta / 2 );
  }
  else
  {
    int die_sides = effect_die_sides( effect_id );
    if ( die_sides == 0 )
      result = avg;
    else if ( die_sides == 1 )
      result =  avg + die_sides;
    else
      result = avg + ( die_sides > 1  ? 1 : die_sides );

    switch ( effect_type( effect_id ) )
    {
      case E_WEAPON_PERCENT_DAMAGE :
        result *= 0.01;
        break;
      default:
        break;
    }
  }

  return result;
}

double sc_data_access_t::effect_max( const uint32_t effect_id, const player_type c, const uint32_t level ) SC_CONST
{
  if ( !effect_id )
    return 0.0;

  assert( effect_exists( effect_id ) &&
         ( level > 0 ) && ( level <= MAX_LEVEL ) );

  double avg, delta, result;

  avg = effect_average( effect_id, c, level );

  if ( get_class_id( c ) != 0 && 
     ( effect_m_average( effect_id ) != 0 || effect_m_delta( effect_id ) != 0 ) )
  {
    delta = effect_delta( effect_id, c, level );

    result = avg + ( delta / 2 );
  }
  else
  {
    int die_sides = effect_die_sides( effect_id );
    if ( die_sides == 0 )
      result = avg;
    else if ( die_sides == 1 )
      result = avg + die_sides;
    else
      result = avg + ( die_sides > 1  ? die_sides : -1 );

    switch ( effect_type( effect_id ) )
    {
      case E_WEAPON_PERCENT_DAMAGE :
        result *= 0.01;
        break;
      default:
        break;
    }
  }

  return result;
}

/*************** Effect type based searching of spell id *************************/

const spelleffect_data_t * sc_data_access_t::effect( uint32_t spell_id, effect_type_t type, effect_subtype_t sub_type, int misc_value, int misc_value2 ) SC_CONST
{
  const spelleffect_data_t * e;

  if ( !spell_id )
    return 0;

  assert( spell_exists( spell_id ) );
  
  for ( int i = 0; i < MAX_EFFECTS; i++ )
  {
    if ( ! m_spells_index[ spell_id ] -> effect[ i ] || ! m_effects_index[ m_spells_index[ spell_id ] -> effect[ i ] ] )
      continue;

    e = m_effects_index[ m_spells_index[ spell_id ] -> effect[ i ] ];

    if ( ( type == E_MAX || e -> type == type ) &&
         ( sub_type == A_MAX || e -> subtype == sub_type ) &&
         ( misc_value == DEFAULT_MISC_VALUE || e -> misc_value == misc_value ) &&
         ( misc_value2 == DEFAULT_MISC_VALUE || e -> misc_value_2 == misc_value2 ) )
      return e;
  }

  return 0;
}

double sc_data_access_t::effect_min( uint32_t spell_id, uint32_t level, effect_type_t type, effect_subtype_t sub_type, int misc_value ) SC_CONST
{
  const spelleffect_data_t * e = 0;

  if ( !spell_id )
    return 0;

  assert( spell_exists( spell_id ) );

  if ( ( e = effect( spell_id, type, sub_type, misc_value ) ) == 0 )
    return 0.0;

  return effect_min( e -> id, get_class_type( m_spells_index[ spell_id ] -> scaling_type ), level );
}

double sc_data_access_t::effect_max( uint32_t spell_id, uint32_t level, effect_type_t type, effect_subtype_t sub_type, int misc_value ) SC_CONST
{
  const spelleffect_data_t * e = 0;

  if ( !spell_id )
    return 0;

  assert( spell_exists( spell_id ) );

  if ( ( e = effect( spell_id, type, sub_type, misc_value ) ) == 0 )
    return 0.0;

  return effect_max( e -> id, get_class_type( m_spells_index[ spell_id ] -> scaling_type ), level );
}

double sc_data_access_t::effect_base_value( uint32_t spell_id, effect_type_t type, effect_subtype_t sub_type, int misc_value, int misc_value2 ) SC_CONST
{
  double                     bv = 0.0;
  const spelleffect_data_t * e  = 0;

  if ( !spell_id )
    return 0;

  assert( spell_exists( spell_id ) );

  if ( ( e = effect( spell_id, type, sub_type, misc_value, misc_value2 ) ) == 0 )
    return 0.0;

  bv = fmt_value( 1.0 * e -> base_value, type, sub_type );

  return bv;
}

double sc_data_access_t::effect_coeff( uint32_t spell_id, effect_type_t type, effect_subtype_t sub_type, int misc_value ) SC_CONST
{
  const spelleffect_data_t * e  = 0;

  if ( !spell_id )
    return 0;

  assert( spell_exists( spell_id ) );

  if ( ( e = effect( spell_id, type, sub_type, misc_value ) ) == 0 )
    return 0.0;

  return e -> coeff;
}

double sc_data_access_t::effect_period( uint32_t spell_id, effect_type_t type, effect_subtype_t sub_type, int misc_value ) SC_CONST
{
  const spelleffect_data_t * e  = 0;

  if ( !spell_id )
    return 0;

  assert( spell_exists( spell_id ) );

  if ( ( e = effect( spell_id, type, sub_type, misc_value ) ) == 0 )
    return 0.0;

  return e -> amplitude;
}

uint32_t sc_data_access_t::effect_trigger_spell_id( uint32_t spell_id, effect_type_t type, effect_subtype_t sub_type, int misc_value ) SC_CONST
{
  const spelleffect_data_t * e  = 0;

  if ( !spell_id )
    return 0;

  assert( spell_exists( spell_id ) );

  if ( ( e = effect( spell_id, type, sub_type, misc_value ) ) == 0 )
    return 0;

  return e -> trigger_spell_id;
}

/*************** Talent functions ***********************/

bool sc_data_access_t::talent_exists( const uint32_t talent_id ) SC_CONST
{
  return m_talents_index && ( talent_id < m_talents_index_size ) && m_talents_index[ talent_id ];
}

const char* sc_data_access_t::talent_name_str( const uint32_t talent_id ) SC_CONST
{
  assert( talent_exists( talent_id ) );

  return m_talents_index[ talent_id ]->name;
}

bool sc_data_access_t::talent_is_used( const uint32_t talent_id ) SC_CONST
{
  assert( talent_exists( talent_id ) );

  return ( ( m_talents_index[ talent_id ]->flags & 0x01 ) == 0x01 );
}

void sc_data_access_t::talent_set_used( const uint32_t talent_id, const bool value )
{
  assert( talent_exists( talent_id ) );

  m_talents_index[ talent_id ]->flags &= (uint32_t) ~( (uint32_t)0x01 ); 
  m_talents_index[ talent_id ]->flags |= value ? 0x01 : 0x00;
}

bool sc_data_access_t::talent_is_enabled( const uint32_t talent_id ) SC_CONST
{
  assert( talent_exists( talent_id ) );

  return ( ( m_talents_index[ talent_id ]->flags & 0x02 ) == 0x00 );
}

void sc_data_access_t::talent_set_enabled( const uint32_t talent_id, const bool value )
{
  assert( talent_exists( talent_id ) );

  m_talents_index[ talent_id ]->flags &= (uint32_t) ~( (uint32_t)0x02 ); 
  m_talents_index[ talent_id ]->flags |= value ? 0x00 : 0x02;
}

uint32_t sc_data_access_t::talent_tab_page( const uint32_t talent_id )
{
  assert( talent_exists( talent_id ) );

  return m_talents_index[ talent_id ]->tab_page;
}

bool sc_data_access_t::talent_is_class( const uint32_t talent_id, const player_type c ) SC_CONST
{
  assert( talent_exists( talent_id ) );

  uint32_t mask = get_class_mask( c );

  if ( mask == 0 )
    return false;

  return ( ( m_talents_index[ talent_id ]->m_class & mask ) == mask );
}

bool sc_data_access_t::talent_is_pet( const uint32_t talent_id, const pet_type_t p ) SC_CONST
{
  assert( talent_exists( talent_id ) );

  uint32_t mask = get_pet_mask( p );

  if ( mask == 0 )
    return false;

  return ( ( m_talents_index[ talent_id ]->m_pet & mask ) == mask );
}

uint32_t sc_data_access_t::talent_depends_id( const uint32_t talent_id ) SC_CONST
{
  assert( talent_exists( talent_id ) );

  return m_talents_index[ talent_id ]->dependance;
}

uint32_t sc_data_access_t::talent_depends_rank( const uint32_t talent_id ) SC_CONST
{
  assert( talent_exists( talent_id ) );

  return m_talents_index[ talent_id ]->depend_rank + 1;
}

uint32_t sc_data_access_t::talent_col( const uint32_t talent_id ) SC_CONST
{
  assert( talent_exists( talent_id ) );

  return m_talents_index[ talent_id ]->col;
}

uint32_t sc_data_access_t::talent_row( const uint32_t talent_id ) SC_CONST
{
  assert( talent_exists( talent_id ) );

  return m_talents_index[ talent_id ]->row;
}

uint32_t sc_data_access_t::talent_rank_spell_id( const uint32_t talent_id, const uint32_t rank ) SC_CONST
{
  assert( talent_exists( talent_id ) && ( rank >= 0 ) && ( rank <= MAX_RANK ) );

  if ( rank == 0 )
    return 0;

  return m_talents_index[ talent_id ]->rank_id[ rank - 1 ];
}

uint32_t sc_data_access_t::talent_max_rank( const uint32_t talent_id ) SC_CONST
{
  assert( talent_exists( talent_id ) );

  uint32_t i = 0;

  for ( i = 0; i < MAX_RANK; i++ )
  {
    if ( m_talents_index[ talent_id ]->rank_id[ i ] == 0 )
    {
      return i;
    }
  }

  return i;
}


uint32_t sc_data_access_t::talent_player_get_id_by_num( const player_type c, const uint32_t tab, const uint32_t num ) SC_CONST
{
  uint32_t cid = get_class_id( c );

  assert( ( cid != 0 ) && ( tab < MAX_TALENT_TABS ) && ( num < MAX_TALENTS ) );

  uint32_t* p = m_talent_trees.ptr( num, tab, cid );

  if ( !p )
    return 0;

  return *p;
}

uint32_t sc_data_access_t::talent_pet_get_id_by_num( const pet_type_t p, const uint32_t num ) SC_CONST
{
  uint32_t pid = get_pet_id( p );

  assert( ( pid != 0 ) && ( num < MAX_TALENTS ) );

  uint32_t* q = m_pet_talent_trees.ptr( num, pid - 1 );

  if ( !q )
    return 0;

  return *q;
}

uint32_t sc_data_access_t::talent_player_get_num_talents( const player_type c, const uint32_t tab ) SC_CONST
{
  uint32_t cid = get_class_id( c );

  assert( ( cid != 0 ) && ( tab < MAX_TALENT_TABS ) );

  uint32_t num = 0;
  uint32_t* q = NULL;

  while ( ( q = m_talent_trees.ptr( num, tab, cid ) ) != NULL )
  {
    if ( ! *q )
    {
      return num;
    }
    num++;
  }

  return num;
}

uint32_t sc_data_access_t::talent_pet_get_num_talents( const pet_type_t p ) SC_CONST
{
  uint32_t pid = get_pet_id( p );
  uint32_t num = 0;
  uint32_t* q = NULL;

  assert( ( pid != 0 ) );


  while ( ( q = m_pet_talent_trees.ptr( num, pid - 1 ) ) != NULL )
  {
    num++;
  }

  return num;
}


/**************************** Scale functions *****************/

double sc_data_access_t::melee_crit_base( const player_type c ) SC_CONST
{
  uint32_t cid = get_class_id( c );

  assert( cid != 0 );

  double* v = m_melee_crit_base.ptr( cid );

  return *v;
}

double sc_data_access_t::melee_crit_base( const pet_type_t c ) SC_CONST
{
  return melee_crit_base( get_pet_class_type( c ) );  
}

double sc_data_access_t::spell_crit_base( const player_type c ) SC_CONST
{
  uint32_t cid = get_class_id( c );

  assert( cid != 0 );

  double* v = m_spell_crit_base.ptr( cid );

  return *v;
}

double sc_data_access_t::spell_crit_base( const pet_type_t c ) SC_CONST
{
  return spell_crit_base( get_pet_class_type( c ) );
}

double sc_data_access_t::melee_crit_scale( const player_type c, const uint32_t level ) SC_CONST
{
  uint32_t cid = get_class_id( c );

  assert( ( cid != 0 ) && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  double* v = m_melee_crit_scale.ptr( level - 1, cid );

  return *v;
}

double sc_data_access_t::melee_crit_scale( const pet_type_t c, const uint32_t level ) SC_CONST
{
  return melee_crit_scale( get_pet_class_type( c ), level );
}

double sc_data_access_t::spell_crit_scale( const player_type c, const uint32_t level ) SC_CONST
{
  uint32_t cid = get_class_id( c );

  assert( ( cid != 0 ) && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  double* v = m_spell_crit_scale.ptr( level - 1, cid );

  return *v;
}

double sc_data_access_t::spell_crit_scale( const pet_type_t c, const uint32_t level ) SC_CONST
{
  return spell_crit_scale( get_pet_class_type( c ), level );
}

// Result is the constant in the k*sqrt(int)*spi regen in terms of mana/second.
double sc_data_access_t::spi_regen( const player_type c, const uint32_t level ) SC_CONST
{
  uint32_t cid = get_class_id( c );

  assert( ( cid != 0 ) && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  double* v = m_regen_spi.ptr( level - 1, cid );

  return *v;
}

double sc_data_access_t::spi_regen( const pet_type_t c, const uint32_t level ) SC_CONST
{
  return spi_regen( get_pet_class_type( c ), level );
}

double sc_data_access_t::oct_regen( const player_type c, const uint32_t level ) SC_CONST
{
  uint32_t cid = get_class_id( c );

  assert( ( cid != 0 ) && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  double* v = m_octregen.ptr( level - 1, cid );

  return *v;
}

double sc_data_access_t::oct_regen( const pet_type_t c, const uint32_t level ) SC_CONST
{
  return oct_regen( get_pet_class_type( c ), level );
}

double sc_data_access_t::combat_ratings( const player_type c, const rating_type r, const uint32_t level ) SC_CONST
{
  uint32_t cid = get_class_id( c );

  assert( ( cid != 0 ) && ( r >= 0 ) && ( r < RATING_MAX ) && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  double* v = m_combat_ratings.ptr( level - 1, r );

  // Doesn't seem to be actually used currently.
  // double* s = m_class_combat_rating_scalar.ptr( cid, r );
  // return *v * *s * 100;

  return *v * 100.0;
}

double sc_data_access_t::combat_ratings( const pet_type_t c, const rating_type r, const uint32_t level ) SC_CONST
{
  return combat_ratings( get_pet_class_type( c ), r, level );
}

double sc_data_access_t::dodge_base( const player_type c ) SC_CONST
{
  uint32_t cid = get_class_id( c );

  assert( ( cid != 0 ) );

  double* v = m_dodge_base.ptr( cid );

  return *v;
}

double sc_data_access_t::dodge_base( const pet_type_t c ) SC_CONST
{
  return dodge_base( get_pet_class_type( c ) );
}

double sc_data_access_t::dodge_scale( const player_type c, const uint32_t level ) SC_CONST
{
  uint32_t cid = get_class_id( c );
  double res = 0.0;
  uint32_t l = level;

  assert( ( cid != 0 ) && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  while ( l > 0 )
  {
    double* v = m_dodge_scale.ptr( level - 1, cid );
    res = *v;

    if ( res != 0.0 )
    {
      return res;
    }
    l--;
  }

  return res;
}

double sc_data_access_t::dodge_scale( const pet_type_t c, const uint32_t level ) SC_CONST
{
  return dodge_scale( get_pet_class_type( c ), level );
}

double sc_data_access_t::base_mp5( const player_type c, const uint32_t level ) SC_CONST
{
  uint32_t cid = get_class_id( c );
  double res = 0.0;
  uint32_t l = level;

  assert( ( cid != 0 ) && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  while ( l > 0 )
  {
    double* v = m_base_mp5.ptr( level - 1, cid );
    res = *v;

    if ( res != 0.0 )
    {
      return res;
    }
    l--;
  }

  return res;
}

double sc_data_access_t::base_mp5( const pet_type_t c, const uint32_t level ) SC_CONST
{
  return base_mp5( get_pet_class_type( c ), level );
}

double sc_data_access_t::class_stats( const player_type c, const uint32_t level, const stat_type s ) SC_CONST
{
  uint32_t cid = get_class_id( c );
  double res = 0.0;
  uint32_t l = level;

  assert( ( level > 0 ) && ( level <= MAX_LEVEL ) );

  while ( l > 0 )
  {
    stat_data_t* v = m_class_stats.ptr( l - 1, cid );

    switch ( s )
    {
    case STAT_STRENGTH: res = v->strength; break;
    case STAT_AGILITY: res = v->agility; break;
    case STAT_STAMINA: res = v->stamina; break;
    case STAT_INTELLECT: res = v->intellect; break;
    case STAT_SPIRIT: res = v->spirit; break;
    case STAT_HEALTH: res = v->base_health; break;
    case STAT_MANA:
    case STAT_RAGE:
    case STAT_ENERGY:
    case STAT_FOCUS:
    case STAT_RUNIC: res = v->base_resource;
      break;
    default: break;
    }

    if ( res != 0.0 )
    {
      return res;
    }
    l--;
  }

  return res;
}

double sc_data_access_t::class_stats( const pet_type_t c, const uint32_t level, const stat_type s ) SC_CONST
{
  return class_stats( get_pet_class_type( c ), level, s );
}

double sc_data_access_t::race_stats( const race_type r, const stat_type s ) SC_CONST
{
  uint32_t rid = get_race_id( r );

  stat_data_t* v = m_race_stats.ptr( rid );

  switch ( s )
  {
  case STAT_STRENGTH: return v->strength;
  case STAT_AGILITY: return v->agility;
  case STAT_STAMINA: return v->stamina;
  case STAT_INTELLECT: return v->intellect;
  case STAT_SPIRIT: return v->spirit;
  case STAT_HEALTH: return v->base_health;
  case STAT_MANA:
  case STAT_RAGE:
  case STAT_ENERGY:
  case STAT_FOCUS:
  case STAT_RUNIC: return v->base_resource;
  default: break;
  }

  return 0.0;
}

double sc_data_access_t::race_stats( const pet_type_t r, const stat_type s ) SC_CONST
{
  return race_stats( RACE_NONE, s );
}


bool sc_data_access_t::is_class_spell( uint32_t spell_id ) SC_CONST
{
  uint32_t s   = m_class_spells.rows;
  uint32_t k   = m_class_spells.depth;
  uint32_t i   = 0;
  uint32_t* p  = 0;

  for ( unsigned int cls = 0; cls < k; cls++ )
  {
    for ( uint32_t j = 0; j < s; j++ )
    {
      i = 0;
      while ( ( ( p = m_class_spells.ptr( i, j, cls ) ) != NULL ) && *p )
      {
        if ( *p == spell_id )
          return true;
        i++;
      }
    }
  }

  return false;
}

uint32_t sc_data_access_t::find_class_spell( player_type c, const char* spell_name, int tree ) SC_CONST
{
  uint32_t cid = get_class_id( c );

  uint32_t s = m_class_spells.rows;

  if ( ( c == PLAYER_PET ) || ( c == PLAYER_GUARDIAN ) ) tree = 3;

  assert( spell_name && spell_name[ 0 ] && ( tree < ( int32_t ) s ) && ( tree >= -1 ) );

  uint32_t i = 0;
  uint32_t* p = NULL;

  if ( tree < 0 )
  {
    for ( uint32_t j = 0; j < s; j++ )
    {
      i = 0;
      while ( ( ( p = m_class_spells.ptr( i, j, cid ) ) != NULL ) && *p )
      {
        if ( check_spell_name( *p, spell_name ) )
          return *p;
        i++;
      };
    }
  }
  else
  {
    while ( ( ( p = m_class_spells.ptr( i, tree, cid ) ) != NULL ) && *p )
    {
      if ( check_spell_name( *p, spell_name ) )
        return *p;
      i++;
    }
  }

  return 0;
}

int sc_data_access_t::find_class_spell_tree( player_type c, uint32_t spell_id ) SC_CONST
{
  uint32_t cid = get_class_id( c );
  uint32_t s = m_class_spells.rows;
  uint32_t i = 0;
  uint32_t* p = NULL;

  for ( uint32_t j = 0; j < s; j++ )
  {
    i = 0;
    while ( ( ( p = m_class_spells.ptr( i, j, cid ) ) != NULL ) && *p )
    {
      if ( *p == spell_id )
        return j;
      i++;
    }
  }

  return -1;
}

uint32_t sc_data_access_t::find_talent_spec_spell( player_type c, const char* spell_name, int tab_name ) SC_CONST
{
  uint32_t cid = get_class_id( c );
  uint32_t i = 0;
  uint32_t* p = NULL;
  uint32_t s = m_talent_spec_spells.rows;

  assert( spell_name && spell_name[ 0 ] && ( tab_name < 3 ) );

  if ( tab_name < 0 )
  {
    for ( uint32_t j = 0; j < s; j++ )
    {
      i = 0;
      while ( ( ( p = m_talent_spec_spells.ptr( i, j, cid ) ) != NULL ) && *p )
      {
        if ( check_spell_name( *p, spell_name ) )
          return *p;
        i++;
      };
    }
  }
  else
  {
    while ( ( ( p = m_talent_spec_spells.ptr( i, tab_name, cid ) ) != NULL ) && *p )
    {
      if ( check_spell_name( *p, spell_name ) )
        return *p;
      i++;
    }
  }
  
  return 0;
}

int sc_data_access_t::find_talent_spec_spell_tree( player_type c, uint32_t spell_id ) SC_CONST
{
  uint32_t cid = get_class_id( c );
  uint32_t s = m_talent_spec_spells.rows;
  uint32_t i = 0;
  uint32_t* p = NULL;

  for ( uint32_t j = 0; j < s; j++ )
  {
    i = 0;
    while ( ( ( p = m_talent_spec_spells.ptr( i, j, cid ) ) != NULL ) && *p )
    {
      if ( *p == spell_id )
        return j;
      i++;
    }
  }

  return -1;
}

bool sc_data_access_t::is_talent_spec_spell( uint32_t spell_id ) SC_CONST
{
  uint32_t s   = m_talent_spec_spells.rows;
  uint32_t k   = m_talent_spec_spells.depth;
  uint32_t i   = 0;
  uint32_t* p  = 0;

  for ( unsigned int cls = 0; cls < k; cls++ )
  {
    for ( uint32_t j = 0; j < s; j++ )
    {
      i = 0;
      while ( ( ( p = m_talent_spec_spells.ptr( i, j, cls ) ) != NULL ) && *p )
      {
        if ( *p == spell_id )
          return true;
        i++;
      }
    }
  }

  return false;
}

uint32_t sc_data_access_t::find_racial_spell( player_type c, race_type r, const char* spell_name ) SC_CONST
{
  uint32_t cid = get_class_id( c );

  assert( spell_name && spell_name[ 0 ] );

  uint32_t i = 0;
  uint32_t* p = NULL;
  while ( ( ( p = m_class_spells.ptr( i, cid ) ) != NULL ) && *p )
  {
    if ( check_spell_name( *p, spell_name ) )
      return *p;
    i++;
  }
  
  return 0;
}

bool sc_data_access_t::is_racial_spell( uint32_t spell_id ) SC_CONST
{
  return false; 
}

uint32_t sc_data_access_t::find_mastery_spell( player_type c, const char* spell_name ) SC_CONST
{
  uint32_t cid = get_class_id( c );
  
  assert( spell_name && spell_name[ 0 ] );

  uint32_t i = 0;
  uint32_t* p = NULL;
  while ( ( ( p = m_mastery_spells.ptr( i, cid ) ) != NULL ) && *p )
  {
    if ( check_spell_name( *p, spell_name ) )
      return *p;
    i++;
  };
  return 0;
}

bool sc_data_access_t::is_mastery_spell( uint32_t spell_id ) SC_CONST
{
  uint32_t s   = m_mastery_spells.rows;
  uint32_t i   = 0;
  uint32_t* p  = 0;

  for ( unsigned int cls = 0; cls < s; cls++ )
  {
    i = 0;
    while ( ( ( p = m_mastery_spells.ptr( i, cls ) ) != NULL ) && *p )
    {
      if ( *p == spell_id )
        return true;
      i++;
    }
  }

  return false;
}

uint32_t sc_data_access_t::find_glyph_spell( player_type c, const char* spell_name ) SC_CONST
{
  uint32_t cid = get_class_id( c );
  
  assert( spell_name && spell_name[ 0 ] );

  uint32_t i = 0;
  uint32_t* p = NULL;
  for ( uint32_t j = 0; j < m_glyph_spells.rows; j++ )
  {
    i = 0;
    while ( ( ( p = m_glyph_spells.ptr( i, j, cid ) ) != NULL ) && *p )
    {
      if ( check_spell_name( *p, spell_name ) )
        return *p;
      i++;
    };
  }
  return 0;
}

uint32_t sc_data_access_t::find_glyph_spell( player_type c, glyph_type type, uint32_t num ) SC_CONST
{
  uint32_t cid = get_class_id( c );
  
  uint32_t* p = NULL;

  p = m_glyph_spells.ptr( num, type, cid );

  if ( !p )
    return 0;

  return *p;
}

bool sc_data_access_t::is_glyph_spell( uint32_t spell_id ) SC_CONST
{
  uint32_t s   = m_glyph_spells.rows;
  uint32_t i   = 0;
  uint32_t* p  = 0;

  for ( unsigned int cls = 0; cls < s; cls++ )
  {
    i = 0;
    while ( ( ( p = m_glyph_spells.ptr( i, cls ) ) != NULL ) && *p )
    {
      if ( *p == spell_id )
        return true;
      i++;
    }
  }

  return false;
}

uint32_t sc_data_access_t::find_set_bonus_spell( player_type c, const char* name, int tier ) SC_CONST
{
  uint32_t cid = get_class_id( c );
  
  assert( name && name[ 0 ] && ( tier < ( int ) m_set_bonus_spells.rows ) );

  uint32_t i = 0;
  uint32_t* p = NULL;
  uint32_t j = 0;
  uint32_t max_row = m_set_bonus_spells.rows;
  if ( tier >= 0 )
  {
    j = tier;
    max_row = tier + 1;
  }
  while ( j < max_row )
  {
    i = 0;
    while ( ( ( p = m_set_bonus_spells.ptr( i, j, cid ) ) != NULL ) && *p )
    {
      if ( check_spell_name( *p, name ) )
        return *p;
      i++;
    };        
    j++;
  }
  return 0;
}

bool sc_data_access_t::is_set_bonus_spell( uint32_t spell_id ) SC_CONST
{
  uint32_t s   = m_set_bonus_spells.rows;
  uint32_t k   = m_set_bonus_spells.depth;
  uint32_t i   = 0;
  uint32_t* p  = 0;

  for ( unsigned int cls = 0; cls < k; cls++ )
  {
    for ( uint32_t j = 0; j < s; j++ )
    {
      i = 0;
      while ( ( ( p = m_set_bonus_spells.ptr( i, j, cls ) ) != NULL ) && *p )
      {
        if ( *p == spell_id )
          return true;
        i++;
      }
    }
  }

  return false;
}

bool sc_data_access_t::check_spell_name( const uint32_t spell_id, const char* name ) SC_CONST
{
  assert( name && name[ 0 ] );

  if ( !spell_exists( spell_id ) )
    return false;

  return util_t::str_compare_ci( spell_name_str( spell_id ), name );
}

bool sc_data_access_t::check_talent_name( const uint32_t talent_id, const char* name ) SC_CONST
{
  assert( name && name[ 0 ] );
  
  if ( !talent_exists( talent_id ) )
    return false;

  return util_t::str_compare_ci( talent_name_str( talent_id ), name );
}

const random_prop_data_t* sc_data_access_t::find_rand_property_data( unsigned ilevel ) SC_CONST
{
  const random_prop_data_t* p = 0;
  
  for ( unsigned i = 0; i < m_random_property_data.size(); i++ )
  {
    p = m_random_property_data.ptr( i );
    if ( ilevel == p -> ilevel )
      return p;
  }
  
  return 0;
}

const random_suffix_data_t* sc_data_access_t::find_random_suffix( unsigned suffix_id ) SC_CONST
{
  const random_suffix_data_t* p = 0;
  
  for ( unsigned i = 0; i < m_random_suffixes.size(); i++ )
  {
    p = m_random_suffixes.ptr( i );
    if ( suffix_id == p -> id )
      return p;
  }
  
  return 0;
}

const item_enchantment_data_t* sc_data_access_t::find_item_enchantment( unsigned enchant_id ) SC_CONST
{
  const item_enchantment_data_t* p = 0;
  
  for ( unsigned i = 0; i < m_item_enchantments.size(); i++ )
  {
    p = m_item_enchantments.ptr( i );
    if ( enchant_id == p -> id )
      return p;
  }
  
  return 0;
}

/***************************** Static functions *************************************/
double sc_data_access_t::fmt_value( double v, effect_type_t type, effect_subtype_t sub_type )
{
  // Automagically divide by 100.0 for percent based abilities
  switch ( type )
  {
    case E_ENERGIZE_PCT:
    case E_WEAPON_PERCENT_DAMAGE:
      v /= 100.0;
      break;
    case E_APPLY_AURA:
    case E_APPLY_AREA_AURA_PARTY:
    case E_APPLY_AREA_AURA_RAID:
      switch ( sub_type )
      {
        case A_HASTE_ALL:
        case A_MOD_HIT_CHANCE:
        case A_MOD_SPELL_HIT_CHANCE:
        case A_ADD_PCT_MODIFIER:
        case A_MOD_OFFHAND_DAMAGE_PCT:
        case A_MOD_ATTACK_POWER_PCT:
        case A_MOD_RANGED_ATTACK_POWER_PCT:
        case A_MOD_TOTAL_STAT_PERCENTAGE:
        case A_MOD_INCREASES_SPELL_PCT_TO_HIT:
        case A_MOD_RATING_FROM_STAT:
        case A_MOD_CASTING_SPEED_NOT_STACK: // Wrath of Air, note this can go > +-100, but only on NPC (and possibly item) abilities
        case A_MOD_SPELL_DAMAGE_OF_ATTACK_POWER:
        case A_MOD_SPELL_HEALING_OF_ATTACK_POWER:
        case A_MOD_SPELL_DAMAGE_OF_STAT_PERCENT:
        case A_MOD_SPELL_HEALING_OF_STAT_PERCENT:
        case A_MOD_DAMAGE_PERCENT_DONE:
        case A_MOD_DAMAGE_FROM_CASTER: // vendetta
        case A_MOD_ALL_CRIT_CHANCE:
        case A_MOD_EXPERTISE:
        case A_MOD_MANA_REGEN_INTERRUPT:  // Meditation
        case A_308: // Increase critical chance of something, Stormstrike, Mind Spike, Holy Word: Serenity
        case A_317: // Totemic Wrath, Flametongue Totem, Demonic Pact, etc ...
        case A_319: // Windfury Totem
          v /= 100.0;
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }

  return v;
}

/***************************** static functions *************************************/

player_type sc_data_access_t::get_class_type( const int c )
{
  switch ( c )
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

uint32_t sc_data_access_t::get_class_id( const player_type c )
{
  switch ( c )
  {
    case PLAYER_SPECIAL_SCALE: return 12;
    case DEATH_KNIGHT: return 6;
    case DRUID: return 11;
    case HUNTER: return 3;
    case MAGE: return 8;
    case PALADIN: return 2;
    case PRIEST: return 5;
    case ROGUE: return 4;
    case SHAMAN: return 7;
    case WARLOCK: return 9;
    case WARRIOR: return 1;
    case PLAYER_GUARDIAN: return 1; // Temporary assignment
    case PLAYER_PET: return 1; // Temporary assignment
    default: break;
  }
  return 0;
}

player_type sc_data_access_t::get_pet_class_type( const pet_type_t c )
{
  player_type p = WARRIOR;

  if ( c <= PET_HUNTER )
  {
    p = WARRIOR;
  }
  else if ( c == PET_GHOUL )
  {
    p = ROGUE;
  }
  else if ( c == PET_FELGUARD )
  {
    p = WARRIOR;
  }
  else if ( c <= PET_WARLOCK )
  {
    p = WARLOCK;
  }

  return p;
}

uint32_t sc_data_access_t::get_class_mask( const player_type c )
{
  uint32_t id = get_class_id( c );

  if ( id > 0 )
    return ( 1 << ( id - 1 ) );

  return 0x00;
}


uint32_t sc_data_access_t::get_race_id( const race_type r )
{
  switch ( r )
  {
  case RACE_NIGHT_ELF: return 4;
  case RACE_HUMAN: return 1;
  case RACE_GNOME: return 7;
  case RACE_DWARF: return 3;
  case RACE_DRAENEI: return 11;
  case RACE_WORGEN: return 22;
  case RACE_ORC: return 2;
  case RACE_TROLL: return 8;
  case RACE_UNDEAD: return 5;
  case RACE_BLOOD_ELF: return 10;
  case RACE_TAUREN: return 6;
  case RACE_GOBLIN: return 9;
  default: break;
  }
  return 0;
}

uint32_t sc_data_access_t::get_race_mask( const race_type r )
{
  uint32_t id = get_race_id( r );

  if ( id > 0 )
    return ( 1 << ( id - 1 ) );

  return 0x00;
}

uint32_t sc_data_access_t::get_pet_mask( const pet_type_t p )
{
  if ( p <= PET_FEROCITY )
    return 0x1;
  if ( p <= PET_TENACITY )
    return 0x2;
  if ( p <= PET_CUNNING )
    return 0x4;

  return 0x0;
}

uint32_t sc_data_access_t::get_pet_id( const pet_type_t p )
{
  uint32_t mask = get_pet_mask( p );

  switch ( mask )
  {
  case 0x1: return 1;
  case 0x2: return 2;
  case 0x4: return 3;
  }

  return 0;
}
