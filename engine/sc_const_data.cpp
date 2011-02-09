#include "simulationcraft.h"

#include "sc_scale_data.inc"
#include "sc_talent_data.inc"
#include "sc_spell_data.inc"
#include "sc_spell_lists.inc"
#include "sc_extra_data.inc"
#include "sc_item_data.inc"

#if SC_USE_PTR
#include "sc_scale_data_ptr.inc"
#include "sc_talent_data_ptr.inc"
#include "sc_spell_data_ptr.inc"
#include "sc_spell_lists_ptr.inc"
#include "sc_extra_data_ptr.inc"
#include "sc_item_data_ptr.inc"
#endif

static bool use_ptr = false;
static spell_data_t       nil_sd;
static spelleffect_data_t nil_sed;
static talent_data_t      nil_td;

bool dbc_t::get_ptr()
{
  return use_ptr;
}

void dbc_t::set_ptr( bool use )
{
#if SC_USE_PTR
  use_ptr = use;
#else
  assert(!use);
#endif
}

const char* dbc_t::build_level()
{
#if SC_USE_PTR
  return use_ptr ? "13596" : "13596";
#else
  return "13596";
#endif
}

void dbc_t::init()
{
  memset( &nil_sd,  0x00, sizeof( spell_data_t )       );
  memset( &nil_sed, 0x00, sizeof( spelleffect_data_t ) );
  memset( &nil_td,  0x00, sizeof( talent_data_t )      );

  nil_sd.effect1 = &nil_sed;
  nil_sd.effect2 = &nil_sed;
  nil_sd.effect3 = &nil_sed;

  nil_sed.spell         = &nil_sd;
  nil_sed.trigger_spell = &nil_sd;

  nil_td.spell1 = &nil_sd;
  nil_td.spell2 = &nil_sd;
  nil_td.spell3 = &nil_sd;

  set_ptr( false );
  spell_data_t::link();
  spelleffect_data_t::link();
  talent_data_t::link();

#if SC_USE_PTR
  set_ptr( true );
  spell_data_t::link();
  spelleffect_data_t::link();
  talent_data_t::link();
  set_ptr( false );
#endif
}

int dbc_t::glyphs( std::vector<unsigned>& glyph_ids, int cid )
{
  for( int i=0; i < GLYPH_MAX; i++ )
  {
    for( int j=0; j < GLYPH_ABILITIES_SIZE; j++ )
    {
      unsigned id = __glyph_abilities_data[ cid ][ i ][ j ];
      if( ! id ) break;
      glyph_ids.push_back( id );
    }
  }
  return glyph_ids.size();
}

spell_data_t* spell_data_t::list() 
{ 
#if SC_USE_PTR
  return use_ptr ? __ptr_spell_data : __spell_data; 
#else
  return __spell_data; 
#endif
}

spelleffect_data_t* spelleffect_data_t::list() 
{ 
#if SC_USE_PTR
  return use_ptr ? __ptr_spelleffect_data : __spelleffect_data; 
#else
  return __spelleffect_data; 
#endif
}

talent_data_t* talent_data_t::list()
{ 
#if SC_USE_PTR
  return use_ptr ? __ptr_talent_data : __talent_data; 
#else
  return __talent_data; 
#endif
}

spell_data_t* spell_data_t::nil() 
{ 
  return &nil_sd;
}

spelleffect_data_t* spelleffect_data_t::nil() 
{ 
  return &nil_sed;
}

talent_data_t* talent_data_t::nil() 
{ 
  return &nil_td;
}

spell_data_t* spell_data_t::find( unsigned id, const std::string& confirmation ) 
{ 
  spell_data_t* spell_data = spell_data_t::list();

  for( int i=0; spell_data[ i ].name; i++ )
  {
    if( spell_data[ i ].id == id )
    {
      if( ! confirmation.empty() ) assert( confirmation == spell_data[ i ].name );
      return spell_data + i;
    }
  }

  return 0;
}

spelleffect_data_t* spelleffect_data_t::find( unsigned id ) 
{ 
  spelleffect_data_t* spelleffect_data = spelleffect_data_t::list();

  for( int i=0; spelleffect_data[ i ].id; i++ )
    if( spelleffect_data[ i ].id == id )
      return spelleffect_data + i;

  return 0;
}

talent_data_t* talent_data_t::find( unsigned id, const std::string& confirmation ) 
{ 
  talent_data_t* talent_data = talent_data_t::list();

  for( int i=0; talent_data[ i ].name; i++ )
  {
    if( talent_data[ i ].id == id )
    {
      if( ! confirmation.empty() ) assert( confirmation == talent_data[ i ].name );
      return talent_data + i;
    }
  }

  return 0;
}

spell_data_t* spell_data_t::find( const std::string& name ) 
{ 
  spell_data_t* spell_data = spell_data_t::list();

  for( int i=0; spell_data[ i ].name; i++ )
    if( name == spell_data[ i ].name )
      return spell_data + i;

  return 0;
}

talent_data_t* talent_data_t::find( const std::string& name ) 
{ 
  talent_data_t* talent_data = talent_data_t::list();

  for( int i=0; talent_data[ i ].name; i++ )
    if( name == talent_data[ i ].name )
      return talent_data + i;

  return 0;
}

void spell_data_t::link()
{
  spell_data_t* spell_data = spell_data_t::list();

  spelleffect_data_t* spelleffect_data = spelleffect_data_t::list();

  for( int i=0; spell_data[ i ].name; i++ )
  {
    spell_data_t& sd = spell_data[ i ];

    spelleffect_data_t** effects[] = { &( sd.effect1 ), &( sd.effect2 ), &( sd.effect3 ) };

    for( int j=0; j < 3; j++ )
    {
      unsigned id = sd.effect[ j ];
      spelleffect_data_t** addr = effects[ j ];
      *addr = spelleffect_data_t::nil();
      if( id > 0 )
      {
        for( int k=0; spelleffect_data[ k ].id; k++ )
        {
          if( id == spelleffect_data[ k ].id )
          {
            *addr = spelleffect_data + k;
            break;
          }
        }
      }
    }
  }
}

void spelleffect_data_t::link()
{
  spell_data_t* spell_data = spell_data_t::list();

  spelleffect_data_t* spelleffect_data = spelleffect_data_t::list();

  for( int i=0; spelleffect_data[ i ].id; i++ )
  {
    spelleffect_data_t& ed = spelleffect_data[ i ];

    ed.spell         = spell_data_t::nil();
    ed.trigger_spell = spell_data_t::nil();

    if( ed.spell_id > 0 )
    {
      for( int j=0; spell_data[ j ].name; j++ )
      {
        if( ed.spell_id == spell_data[ j ].id )
        {
          ed.spell = spell_data + j;
          break;
        }
      }
    }
    if( ed.trigger_spell_id > 0 )
    {
      for( int j=0; spell_data[ j ].name; j++ )
      {
        if( ed.trigger_spell_id == (int) spell_data[ j ].id )
        {
          ed.trigger_spell = spell_data + j;
          break;
        }
      }
    }
  }
}

void talent_data_t::link()
{ 
  spell_data_t* spell_data = spell_data_t::list();

  talent_data_t* talent_data = talent_data_t::list();

  for( int i=0; talent_data[ i ].name; i++ )
  {
    talent_data_t& td = talent_data[ i ];

    spell_data_t** rank_spells[] = { &( td.spell1 ), &( td.spell2 ), &( td.spell3 ) };

    for( int j=0; j < 3; j++ )
    {
      unsigned id = td.rank_id[ j ];
      spell_data_t** addr = rank_spells[ j ];
      *addr = spell_data_t::nil();
      if( id > 0 )
      {
        for( int k=0; spell_data[ k ].name; k++ )
        {
          if( id == spell_data[ k ].id )
          {
            *addr = spell_data + k;
            break;
          }
        }
      }
    }
  }
}

void sc_data_t::set_parent( sc_data_t* p, const bool ptr )
{
  if ( p == this )
  {
    return;
  }

  m_parent = p;

  if ( m_parent == NULL )
  {
#if SC_USE_PTR
    if ( ptr )
    {
      m_spells.create_copy( ( spell_data_t * ) __ptr_spell_data, sizeof( __ptr_spell_data ) / sizeof( spell_data_t ) - 1 );
      m_effects.create_copy( ( spelleffect_data_t * ) __ptr_spelleffect_data, sizeof( __ptr_spelleffect_data ) / sizeof( spelleffect_data_t ) - 1 );
      m_talents.create_copy( ( talent_data_t * ) __ptr_talent_data, sizeof( __ptr_talent_data ) / sizeof( talent_data_t ) - 1 );
      m_melee_crit_base.create_copy( ( double * ) __ptr_gt_chance_to_melee_crit_base, sizeof( __ptr_gt_chance_to_melee_crit_base ) / sizeof( double ) );
      m_spell_crit_base.create_copy( ( double *) __ptr_gt_chance_to_spell_crit_base, sizeof( __ptr_gt_chance_to_spell_crit_base ) / sizeof( double ) );
      m_spell_scaling.create_copy( ( double * ) __ptr_gt_spell_scaling, MAX_LEVEL, sizeof( __ptr_gt_spell_scaling ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_melee_crit_scale.create_copy( ( double * ) __ptr_gt_chance_to_melee_crit, MAX_LEVEL, sizeof( __ptr_gt_chance_to_melee_crit ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_spell_crit_scale.create_copy( ( double * ) __ptr_gt_chance_to_spell_crit, MAX_LEVEL, sizeof( __ptr_gt_chance_to_spell_crit ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_regen_spi.create_copy( ( double * ) __ptr_gt_regen_mpper_spt, MAX_LEVEL, sizeof( __ptr_gt_regen_mpper_spt ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_octregen.create_copy( ( double * ) __ptr_gt_octregen_mp, MAX_LEVEL, sizeof( __ptr_gt_octregen_mp ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_combat_ratings.create_copy( ( double * ) __ptr_gt_combat_ratings, MAX_LEVEL, sizeof( __ptr_gt_combat_ratings ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_class_combat_rating_scalar.create_copy( ( double * ) __ptr_gt_octclass_combat_rating_scalar, CLASS_SIZE, sizeof( __ptr_gt_octclass_combat_rating_scalar ) / ( CLASS_SIZE * sizeof( double ) ) );
      m_class_spells.create_copy( ( uint32_t * ) __ptr_class_ability_data, PTR_CLASS_ABILITY_SIZE, PTR_CLASS_ABILITY_TREE_SIZE, sizeof( __ptr_class_ability_data ) / ( PTR_CLASS_ABILITY_SIZE * PTR_CLASS_ABILITY_TREE_SIZE * sizeof( uint32_t ) ) );
      m_talent_spec_spells.create_copy( ( uint32_t * ) __ptr_tree_specialization_data, PTR_TREE_SPECIALIZATION_SIZE, MAX_TALENT_TABS, sizeof( __ptr_tree_specialization_data ) / ( PTR_TREE_SPECIALIZATION_SIZE * MAX_TALENT_TABS * sizeof( uint32_t ) ) );
      m_racial_spells.create_copy( ( uint32_t * ) __ptr_race_ability_data, PTR_RACE_ABILITY_SIZE, CLASS_SIZE, sizeof( __ptr_race_ability_data ) / ( PTR_RACE_ABILITY_SIZE * CLASS_SIZE * sizeof( uint32_t ) ) );
      m_mastery_spells.create_copy( ( uint32_t * ) __ptr_class_mastery_ability_data, PTR_CLASS_MASTERY_ABILITY_SIZE, sizeof( __ptr_class_mastery_ability_data ) / ( PTR_CLASS_MASTERY_ABILITY_SIZE * sizeof( uint32_t ) ) );
      m_glyph_spells.create_copy( ( uint32_t * ) __ptr_glyph_abilities_data, PTR_GLYPH_ABILITIES_SIZE, 3, sizeof( __ptr_glyph_abilities_data ) / ( PTR_GLYPH_ABILITIES_SIZE * 3 * sizeof( uint32_t ) ) );
      m_set_bonus_spells.create_copy( ( uint32_t * ) __ptr_tier_bonuses_data, PTR_TIER_BONUSES_SIZE, 12, sizeof( __ptr_tier_bonuses_data ) / ( PTR_TIER_BONUSES_SIZE * 12 * sizeof( uint32_t ) ) );
      m_dodge_base.create_copy( ( double * ) __ptr_gt_chance_to_dodge_base, sizeof( __ptr_gt_chance_to_dodge_base ) / sizeof( double ) );
      m_dodge_scale.create_copy( ( double * ) __ptr_gt_dodge_per_agi, MAX_LEVEL, sizeof( __ptr_gt_dodge_per_agi ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_base_mp5.create_copy( ( double * ) __ptr_gt_base_mp5, MAX_LEVEL, sizeof( __ptr_gt_base_mp5 ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_class_stats.create_copy( ( stat_data_t * ) __ptr_gt_class_stats_by_level, MAX_LEVEL, sizeof( __ptr_gt_class_stats_by_level ) / ( MAX_LEVEL * sizeof( stat_data_t ) ) );
      m_race_stats.create_copy( ( stat_data_t * ) __ptr_gt_race_stats, sizeof( __ptr_gt_race_stats ) / sizeof( stat_data_t ) );
      
      m_random_property_data.create_copy( ( random_prop_data_t * ) __ptr_rand_prop_points_data, sizeof( __ptr_rand_prop_points_data ) / sizeof( random_prop_data_t ) - 1 );
      m_random_suffixes.create_copy( ( random_suffix_data_t * ) __ptr_rand_suffix_data, sizeof( __ptr_rand_suffix_data ) / sizeof( random_suffix_data_t ) - 1 );
      m_item_enchantments.create_copy( ( item_enchantment_data_t * ) __ptr_spell_item_ench_data, sizeof( __ptr_spell_item_ench_data ) / sizeof( item_enchantment_data_t ) - 1 );
    }
    else
    {
#endif
      m_spells.create_copy( ( spell_data_t * ) __spell_data, sizeof( __spell_data ) / sizeof( spell_data_t ) - 1 );
      m_effects.create_copy( ( spelleffect_data_t * ) __spelleffect_data, sizeof( __spelleffect_data ) / sizeof( spelleffect_data_t ) - 1 );
      m_talents.create_copy( ( talent_data_t * ) __talent_data, sizeof( __talent_data ) / sizeof( talent_data_t ) - 1 );
      m_melee_crit_base.create_copy( ( double * ) __gt_chance_to_melee_crit_base, sizeof( __gt_chance_to_melee_crit_base ) / sizeof( double ) );
      m_spell_crit_base.create_copy( ( double *) __gt_chance_to_spell_crit_base, sizeof( __gt_chance_to_spell_crit_base ) / sizeof( double ) );
      m_spell_scaling.create_copy( ( double * ) __gt_spell_scaling, MAX_LEVEL, sizeof( __gt_spell_scaling ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_melee_crit_scale.create_copy( ( double * ) __gt_chance_to_melee_crit, MAX_LEVEL, sizeof( __gt_chance_to_melee_crit ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_spell_crit_scale.create_copy( ( double * ) __gt_chance_to_spell_crit, MAX_LEVEL, sizeof( __gt_chance_to_spell_crit ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_regen_spi.create_copy( ( double * ) __gt_regen_mpper_spt, MAX_LEVEL, sizeof( __gt_regen_mpper_spt ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_octregen.create_copy( ( double * ) __gt_octregen_mp, MAX_LEVEL, sizeof( __gt_octregen_mp ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_combat_ratings.create_copy( ( double * ) __gt_combat_ratings, MAX_LEVEL, sizeof( __gt_combat_ratings ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_class_combat_rating_scalar.create_copy( ( double * ) __gt_octclass_combat_rating_scalar, CLASS_SIZE, sizeof( __gt_octclass_combat_rating_scalar ) / ( CLASS_SIZE * sizeof( double ) ) );
      m_class_spells.create_copy( ( uint32_t * ) __class_ability_data, CLASS_ABILITY_SIZE, CLASS_ABILITY_TREE_SIZE, sizeof( __class_ability_data ) / ( CLASS_ABILITY_SIZE * CLASS_ABILITY_TREE_SIZE * sizeof( uint32_t ) ) );
      m_talent_spec_spells.create_copy( ( uint32_t * ) __tree_specialization_data, TREE_SPECIALIZATION_SIZE, MAX_TALENT_TABS, sizeof( __tree_specialization_data ) / ( TREE_SPECIALIZATION_SIZE * MAX_TALENT_TABS * sizeof( uint32_t ) ) );
      m_racial_spells.create_copy( ( uint32_t * ) __race_ability_data, RACE_ABILITY_SIZE, CLASS_SIZE, sizeof( __race_ability_data ) / ( RACE_ABILITY_SIZE * CLASS_SIZE * sizeof( uint32_t ) ) );
      m_mastery_spells.create_copy( ( uint32_t * ) __class_mastery_ability_data, CLASS_MASTERY_ABILITY_SIZE, sizeof( __class_mastery_ability_data ) / ( CLASS_MASTERY_ABILITY_SIZE * sizeof( uint32_t ) ) );
      m_glyph_spells.create_copy( ( uint32_t * ) __glyph_abilities_data, GLYPH_ABILITIES_SIZE, 3, sizeof( __glyph_abilities_data ) / ( GLYPH_ABILITIES_SIZE * 3 * sizeof( uint32_t ) ) );
      m_set_bonus_spells.create_copy( ( uint32_t * ) __tier_bonuses_data, TIER_BONUSES_SIZE, 12, sizeof( __tier_bonuses_data ) / ( TIER_BONUSES_SIZE * 12 * sizeof( uint32_t ) ) );
      m_dodge_base.create_copy( ( double * ) __gt_chance_to_dodge_base, sizeof( __gt_chance_to_dodge_base ) / sizeof( double ) );
      m_dodge_scale.create_copy( ( double * ) __gt_dodge_per_agi, MAX_LEVEL, sizeof( __gt_dodge_per_agi ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_base_mp5.create_copy( ( double * ) __gt_base_mp5, MAX_LEVEL, sizeof( __gt_base_mp5 ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_class_stats.create_copy( ( stat_data_t * ) __gt_class_stats_by_level, MAX_LEVEL, sizeof( __gt_class_stats_by_level ) / ( MAX_LEVEL * sizeof( stat_data_t ) ) );
      m_race_stats.create_copy( ( stat_data_t * ) __gt_race_stats, sizeof( __gt_race_stats ) / sizeof( stat_data_t ) );

      m_random_property_data.create_copy( ( random_prop_data_t * ) __rand_prop_points_data, sizeof( __rand_prop_points_data ) / sizeof( random_prop_data_t ) - 1 );
      m_random_suffixes.create_copy( ( random_suffix_data_t * ) __rand_suffix_data, sizeof( __rand_suffix_data ) / sizeof( random_suffix_data_t ) - 1 );
      m_item_enchantments.create_copy( ( item_enchantment_data_t * ) __spell_item_ench_data, sizeof( __spell_item_ench_data ) / sizeof( item_enchantment_data_t ) - 1 );
#if SC_USE_PTR
    }
#endif
  }
  else
  {
    m_copy( *p );
  }

  create_index();
}
