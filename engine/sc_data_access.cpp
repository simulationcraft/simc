#include "simulationcraft.h"

static uint32_t get_class_id( const player_type c );

static uint32_t get_class_mask( const player_type c );

static uint32_t get_race_id( const race_type r );

static uint32_t get_race_mask( const race_type r );

static uint32_t get_pet_mask( const pet_type_t p );

static uint32_t get_pet_id( const pet_type_t p );

static player_type get_pet_class_type( const pet_type_t c );

sc_data_access_t::sc_data_access_t( sc_data_t* p ) : sc_data_t( p )
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
  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->name;
}

bool sc_data_access_t::spell_is_used( const uint32_t spell_id ) SC_CONST
{
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
  assert( spell_exists( spell_id ) );

  return ( ( m_spells_index[ spell_id ]->flags & 0x02 ) == 0x02 );
}


void sc_data_access_t::spell_set_enabled( const uint32_t spell_id, const bool value )
{
  assert( spell_exists( spell_id ) );

  m_spells_index[ spell_id ]->flags &= (uint32_t) ~( (uint32_t)0x02 ); 
  m_spells_index[ spell_id ]->flags |= value ? 0x02 : 0x00;
}

double sc_data_access_t::spell_missile_speed( const uint32_t spell_id ) SC_CONST
{
  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->prj_speed;
}

uint32_t sc_data_access_t::spell_school_mask( const uint32_t spell_id ) SC_CONST
{
  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->school;
}

resource_type sc_data_access_t::spell_power_type( const uint32_t spell_id ) SC_CONST
{
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
  assert( spell_exists( spell_id ) && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  return ( level >= m_spells_index[ spell_id ]->spell_level );
}

double sc_data_access_t::spell_min_range( const uint32_t spell_id ) SC_CONST
{
  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->min_range;
}

double sc_data_access_t::spell_max_range( const uint32_t spell_id ) SC_CONST
{
  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->max_range;
}

bool sc_data_access_t::spell_in_range( const uint32_t spell_id, const double range ) SC_CONST
{
  assert( spell_exists( spell_id ) );

  return ( ( range >= m_spells_index[ spell_id ]->min_range ) && ( range <= m_spells_index[ spell_id ]->max_range ) );
}

double sc_data_access_t::spell_cooldown( const uint32_t spell_id ) SC_CONST
{
  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->cooldown / 1000.0;
}

double sc_data_access_t::spell_gcd( const uint32_t spell_id ) SC_CONST
{
  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->gcd / 1000.0;
}

uint32_t sc_data_access_t::spell_category( const uint32_t spell_id ) SC_CONST
{
  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->category;
}

double sc_data_access_t::spell_duration( const uint32_t spell_id ) SC_CONST
{
  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->duration / 1000.0;
}

double sc_data_access_t::spell_cost( const uint32_t spell_id ) SC_CONST
{
  assert( spell_exists( spell_id ) );

  if ( m_spells_index[ spell_id ]->power_type == POWER_MANA )
  {
    // Return as a fraction of base mana.
    return m_spells_index[ spell_id ]->cost / 100.0;
  }

  return ( double ) m_spells_index[ spell_id ]->cost;
}

double sc_data_access_t::spell_rune_cost( const uint32_t spell_id ) SC_CONST
{
  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->rune_cost;
}

double sc_data_access_t::spell_runic_power_gain( const uint32_t spell_id ) SC_CONST
{
  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->runic_power_gain / 10.0;
}

uint32_t sc_data_access_t::spell_max_stacks( const uint32_t spell_id ) SC_CONST
{
  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->max_stack;
}

uint32_t sc_data_access_t::spell_initial_stacks( const uint32_t spell_id ) SC_CONST
{
  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->proc_charges;
}

double sc_data_access_t::spell_proc_chance( const uint32_t spell_id ) SC_CONST
{
  assert( spell_exists( spell_id ) );

  return m_spells_index[ spell_id ]->proc_chance / 100.0;
}

double sc_data_access_t::spell_cast_time( const uint32_t spell_id, const uint32_t level ) SC_CONST
{
  assert( spell_exists( spell_id ) && ( level > 0 ) && ( level <= MAX_LEVEL ) );

  uint32_t min_cast, max_cast, div_cast;

  min_cast = m_spells_index[ spell_id ]->cast_min;
  max_cast = m_spells_index[ spell_id ]->cast_max;
  div_cast = m_spells_index[ spell_id ]->cast_div;

  if ( level >= div_cast )
    return max_cast/1000;
  
  return (min_cast + ( max_cast - min_cast ) * ( level - 1 ) / ( div_cast - 1 )) / 1000;
}

uint32_t sc_data_access_t::spell_effect_id( const uint32_t spell_id, const uint32_t effect_num ) SC_CONST
{
  assert( spell_exists( spell_id ) && ( effect_num >= 1 ) && ( effect_num  <= MAX_EFFECTS ) );

  return m_spells_index[ spell_id ]->effect[ effect_num - 1 ];
}

bool sc_data_access_t::spell_flags( const uint32_t spell_id, const spell_attribute_t f ) SC_CONST
{
  assert( spell_exists( spell_id ) );

  uint32_t index, bit, mask;

  index = ( ( ( uint32_t ) f ) >> 8 ) & 0x000000FF;
  bit = ( ( uint32_t ) f ) & 0x0000001F;
  mask = ( 1 << bit );

  assert( index < NUM_SPELL_FLAGS );

  return ( ( m_spells_index[ spell_id ]->attributes[ index ] & mask ) == mask );
}

/************ Effects ******************/

bool sc_data_access_t::effect_exists( const uint32_t effect_id ) SC_CONST
{
  return m_effects_index && ( effect_id < m_effects_index_size ) && m_effects_index[ effect_id ];
}

bool sc_data_access_t::effect_is_used( const uint32_t effect_id ) SC_CONST
{
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
  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->spell_id;
}

uint32_t sc_data_access_t::effect_spell_effect_num( const uint32_t effect_id ) SC_CONST
{
  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->index;
}

uint32_t sc_data_access_t::effect_type( const uint32_t effect_id ) SC_CONST
{
  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->type;
}

uint32_t sc_data_access_t::effect_subtype( const uint32_t effect_id ) SC_CONST
{
  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->subtype;
}

int32_t sc_data_access_t::effect_base_value( const uint32_t effect_id ) SC_CONST
{
  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->base_value;
}

int32_t sc_data_access_t::effect_misc_value1( const uint32_t effect_id ) SC_CONST
{
  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->misc_value;
}

int32_t sc_data_access_t::effect_misc_value2( const uint32_t effect_id ) SC_CONST
{
  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->misc_value_2;
}

uint32_t sc_data_access_t::effect_trigger_spell_id( const uint32_t effect_id ) SC_CONST
{
  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->trigger_spell;
}

double sc_data_access_t::effect_m_average( const uint32_t effect_id ) SC_CONST
{
  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->m_avg;
}

double sc_data_access_t::effect_m_delta( const uint32_t effect_id ) SC_CONST
{
  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->m_delta;
}

double sc_data_access_t::effect_m_unk( const uint32_t effect_id ) SC_CONST
{
  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->m_unk;
}

double sc_data_access_t::effect_coeff( const uint32_t effect_id ) SC_CONST
{
  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->coeff;
}

double sc_data_access_t::effect_period( const uint32_t effect_id ) SC_CONST
{
  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->amplitude/1000;
}

double sc_data_access_t::effect_radius( const uint32_t effect_id ) SC_CONST
{
  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->radius;
}

double sc_data_access_t::effect_radius_max( const uint32_t effect_id ) SC_CONST
{
  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->radius_max;
}

double sc_data_access_t::effect_pp_combo_points( const uint32_t effect_id ) SC_CONST
{
  assert( effect_exists( effect_id ) );

  return m_effects_index[ effect_id ]->pp_combo_points;
}

double sc_data_access_t::effect_average( const uint32_t effect_id, const player_type c, const uint32_t level ) SC_CONST
{
  assert( effect_exists( effect_id ) &&
         ( level > 0 ) && ( level <= MAX_LEVEL ) );

  uint32_t c_id = get_class_id( c );
  double* p_scale_ref = m_spell_scaling.ptr( level - 1, c_id );

  assert( ( c_id != 0 ) && ( p_scale_ref != NULL ) );

  return effect_m_average( effect_id ) * *p_scale_ref;

}

double sc_data_access_t::effect_delta( const uint32_t effect_id, const player_type c, const uint32_t level ) SC_CONST
{
  assert( effect_exists( effect_id ) &&
         ( level > 0 ) && ( level <= MAX_LEVEL ) );

  uint32_t c_id = get_class_id( c );
  double* p_scale_ref = m_spell_scaling.ptr( level - 1, c_id );

  assert( ( c_id != 0 ) && ( p_scale_ref != NULL ) );

  return effect_m_average( effect_id ) * effect_m_delta( effect_id ) * *p_scale_ref;
}

double sc_data_access_t::effect_unk( const uint32_t effect_id, const player_type c, const uint32_t level ) SC_CONST
{
  assert( effect_exists( effect_id ) &&
         ( level > 0 ) && ( level <= MAX_LEVEL ) );

  uint32_t c_id = get_class_id( c );
  double* p_scale_ref = m_spell_scaling.ptr( level - 1, c_id );

  assert( ( c_id != 0 ) && ( p_scale_ref != NULL ) );

  return effect_m_unk( effect_id ) * *p_scale_ref;
}

double sc_data_access_t::effect_min( const uint32_t effect_id, const player_type c, const uint32_t level ) SC_CONST
{
  assert( effect_exists( effect_id ) &&
         ( level > 0 ) && ( level <= MAX_LEVEL ) );

  double avg, delta;

  avg = effect_average( effect_id, c, level );
  delta = effect_delta( effect_id, c, level );

  return avg - ( delta / 2 );
}

double sc_data_access_t::effect_max( const uint32_t effect_id, const player_type c, const uint32_t level ) SC_CONST
{
  assert( effect_exists( effect_id ) &&
         ( level > 0 ) && ( level <= MAX_LEVEL ) );

  double avg, delta;

  avg = effect_average( effect_id, c, level );
  delta = effect_delta( effect_id, c, level );

  return avg + ( delta / 2 );
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

  return ( ( m_talents_index[ talent_id ]->flags & 0x02 ) == 0x02 );
}

void sc_data_access_t::talent_set_enabled( const uint32_t talent_id, const bool value )
{
  assert( talent_exists( talent_id ) );

  m_talents_index[ talent_id ]->flags &= (uint32_t) ~( (uint32_t)0x02 ); 
  m_talents_index[ talent_id ]->flags |= value ? 0x02 : 0x00;
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
  assert( talent_exists( talent_id ) && ( rank > 0 ) && ( rank <= MAX_RANK ) );

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
  double* s = m_class_combat_rating_scalar.ptr( c, r );

  return *v * *s;
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

/***************************** local functions *************************************/

static uint32_t get_class_id( const player_type c )
{
  switch ( c )
  {
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

static player_type get_pet_class_type( const pet_type_t c )
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

static uint32_t get_class_mask( const player_type c )
{
  uint32_t id = get_class_id( c );

  if ( id > 0 )
    return ( 1 << ( id - 1 ) );

  return 0x00;
}


static uint32_t get_race_id( const race_type r )
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

static uint32_t get_race_mask( const race_type r )
{
  uint32_t id = get_race_id( r );

  if ( id > 0 )
    return ( 1 << ( id - 1 ) );

  return 0x00;
}

static uint32_t get_pet_mask( const pet_type_t p )
{
  if ( p <= PET_FEROCITY )
    return 0x1;
  if ( p <= PET_TENACITY )
    return 0x2;
  if ( p <= PET_CUNNING )
    return 0x4;

  return 0x0;
}

static uint32_t get_pet_id( const pet_type_t p )
{
  uint32_t mask = get_pet_mask( p );

  switch ( mask )
  {
  case 0x1: return 1;
  case 0x2: return 2;
  case 0x3: return 3;
  }

  return 0;
}
