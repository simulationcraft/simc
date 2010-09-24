#include "simulationcraft.h"

#include "sc_scale_data.inc"
#include "sc_talent_data.inc"
#include "sc_spell_data.inc"
#include "sc_spell_lists.inc"
#include "sc_extra_data.inc"

#include "sc_scale_data_ptr.inc"
#include "sc_talent_data_ptr.inc"
#include "sc_spell_data_ptr.inc"
#include "sc_spell_lists_ptr.inc"
#include "sc_extra_data_ptr.inc"


void sc_data_t::set_parent( sc_data_t* p, const bool ptr )
{
  if ( p == this )
  {
    return;
  }

  m_parent = p;

  if ( m_parent == NULL )
  {
    if ( ptr )
    {
      m_spells.create_copy( ( spell_data_t * ) __ptr_spell_data, sizeof( __ptr_spell_data ) / sizeof( spell_data_t ) );
      m_effects.create_copy( ( spelleffect_data_t * ) __ptr_spelleffect_data, sizeof( __ptr_spelleffect_data ) / sizeof( spelleffect_data_t ) );
      m_talents.create_copy( ( talent_data_t * ) __ptr_talent_data, sizeof( __ptr_talent_data ) / sizeof( talent_data_t ) );
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
      m_talent_spec_spells.create_copy( ( uint32_t * ) __ptr_tree_specialization_data, TREE_SPECIALIZATION_SIZE, MAX_TALENT_TABS, sizeof( __ptr_tree_specialization_data ) / ( TREE_SPECIALIZATION_SIZE * MAX_TALENT_TABS * sizeof( uint32_t ) ) );
      m_racial_spells.create_copy( ( uint32_t * ) __ptr_race_ability_data, RACE_ABILITY_SIZE, CLASS_SIZE, sizeof( __ptr_race_ability_data ) / ( RACE_ABILITY_SIZE * CLASS_SIZE * sizeof( uint32_t ) ) );
      m_mastery_spells.create_copy( ( uint32_t * ) __ptr_class_mastery_ability_data, CLASS_MASTERY_ABILITY_SIZE, sizeof( __ptr_class_mastery_ability_data ) / ( CLASS_MASTERY_ABILITY_SIZE * sizeof( uint32_t ) ) );
      m_glyph_spells.create_copy( ( uint32_t * ) __ptr_glyph_abilities_data, GLYPH_ABILITIES_SIZE, 3, sizeof( __ptr_glyph_abilities_data ) / ( GLYPH_ABILITIES_SIZE * 3 * sizeof( uint32_t ) ) );
      m_dodge_base.create_copy( ( double * ) __ptr_gt_chance_to_dodge_base, sizeof( __ptr_gt_chance_to_dodge_base ) / sizeof( double ) );
      m_dodge_scale.create_copy( ( double * ) __ptr_gt_dodge_per_agi, MAX_LEVEL, sizeof( __ptr_gt_dodge_per_agi ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_base_mp5.create_copy( ( double * ) __ptr_gt_base_mp5, MAX_LEVEL, sizeof( __ptr_gt_base_mp5 ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_class_stats.create_copy( ( stat_data_t * ) __ptr_gt_class_stats_by_level, MAX_LEVEL, sizeof( __ptr_gt_class_stats_by_level ) / ( MAX_LEVEL * sizeof( stat_data_t ) ) );
      m_race_stats.create_copy( ( stat_data_t * ) __ptr_gt_race_stats, sizeof( __ptr_gt_race_stats ) / sizeof( stat_data_t ) );
    }
    else
    {
      m_spells.create_copy( ( spell_data_t * ) __spell_data, sizeof( __spell_data ) / sizeof( spell_data_t ) );
      m_effects.create_copy( ( spelleffect_data_t * ) __spelleffect_data, sizeof( __spelleffect_data ) / sizeof( spelleffect_data_t ) );
      m_talents.create_copy( ( talent_data_t * ) __talent_data, sizeof( __talent_data ) / sizeof( talent_data_t ) );
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
      m_dodge_base.create_copy( ( double * ) __gt_chance_to_dodge_base, sizeof( __gt_chance_to_dodge_base ) / sizeof( double ) );
      m_dodge_scale.create_copy( ( double * ) __gt_dodge_per_agi, MAX_LEVEL, sizeof( __gt_dodge_per_agi ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_base_mp5.create_copy( ( double * ) __gt_base_mp5, MAX_LEVEL, sizeof( __gt_base_mp5 ) / ( MAX_LEVEL * sizeof( double ) ) );
      m_class_stats.create_copy( ( stat_data_t * ) __gt_class_stats_by_level, MAX_LEVEL, sizeof( __gt_class_stats_by_level ) / ( MAX_LEVEL * sizeof( stat_data_t ) ) );
      m_race_stats.create_copy( ( stat_data_t * ) __gt_race_stats, sizeof( __gt_race_stats ) / sizeof( stat_data_t ) );
    }
  }
  else
  {
    m_copy( *p );
  }

  create_index();
}
