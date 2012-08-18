// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Rating
// ==========================================================================

// rating_t::init ===========================================================

void rating_t::init( sim_t* sim, dbc_t& dbc, int level, player_e type )
{
  if ( sim -> debug ) sim -> output( "rating_t::init: level=%d type=%s",
                                     level, util::player_type_string( type ) );

  if ( player_t::_is_enemy( type ) )
  {
    double max = +1.0E+50;
    spell_haste       = max;
    spell_hit         = max;
    spell_crit        = max;
    attack_haste      = max;
    attack_hit        = max;
    attack_crit       = max;
    ranged_haste      = max;
    ranged_hit        = max;
    ranged_crit       = max;
    expertise         = max;
    dodge             = max;
    parry             = max;
    block             = max;
    mastery           = max;
  }
  else
  {
    spell_haste       = dbc.combat_rating( RATING_SPELL_HASTE,  level );
    spell_hit         = dbc.combat_rating( RATING_SPELL_HIT,    level );
    spell_crit        = dbc.combat_rating( RATING_SPELL_CRIT,   level );
    attack_haste      = dbc.combat_rating( RATING_MELEE_HASTE,  level );
    attack_hit        = dbc.combat_rating( RATING_MELEE_HIT,    level );
    attack_crit       = dbc.combat_rating( RATING_MELEE_CRIT,   level );
    ranged_haste      = dbc.combat_rating( RATING_RANGED_HASTE, level );
    ranged_hit        = dbc.combat_rating( RATING_RANGED_HIT,   level );
    ranged_crit       = dbc.combat_rating( RATING_RANGED_CRIT,  level );
    expertise         = dbc.combat_rating( RATING_EXPERTISE,    level );
    dodge             = dbc.combat_rating( RATING_DODGE,        level );
    parry             = dbc.combat_rating( RATING_PARRY,        level );
    block             = dbc.combat_rating( RATING_BLOCK,        level );
    mastery           = dbc.combat_rating( RATING_MASTERY,      level ) / 100;
  }
}

// rating_t::interpolate ====================================================

double rating_t::interpolate( int    level,
                              double val_60,
                              double val_70,
                              double val_80,
                              double val_85 )
{
  if ( val_85 < 0 ) val_85 = val_80; // TODO
  if ( level <= 60 )
  {
    return val_60;
  }
  else if ( level == 70 )
  {
    return val_70;
  }
  else if ( level == 80 )
  {
    return val_80;
  }
  else if ( level >= 85 )
  {
    return val_85;
  }
  else if ( level < 70 )
  {
    // Assume linear progression for now.
    double adjust = ( level - 60 ) / 10.0;
    return val_60 + adjust * ( val_70 - val_60 );
  }
  else if ( level < 80 )
  {
    // Assume linear progression for now.
    double adjust = ( level - 70 ) / 10.0;
    return val_70 + adjust * ( val_80 - val_70 );
  }
  else // ( level < 85 )
  {
    // Assume linear progression for now.
    double adjust = ( level - 80 ) / 5.0;
    return val_80 + adjust * ( val_85 - val_80 );
  }
  assert( 0 );
  return 0;
}

// rating_t::get_attribute_base =============================================

double rating_t::get_attribute_base( sim_t* /* sim */, dbc_t& dbc, int level, player_e class_type, race_e race, base_stat_e stat )
{
  double res                       = 0.0;

  switch ( stat )
  {
  case BASE_STAT_STRENGTH:           res = dbc.race_base( race ).strength + dbc.attribute_base( class_type, level ).strength; break;
  case BASE_STAT_AGILITY:            res = dbc.race_base( race ).agility + dbc.attribute_base( class_type, level ).agility; break;
  case BASE_STAT_STAMINA:            res = dbc.race_base( race ).stamina + dbc.attribute_base( class_type, level ).stamina; break;
  case BASE_STAT_INTELLECT:          res = dbc.race_base( race ).intellect + dbc.attribute_base( class_type, level ).intellect; break;
  case BASE_STAT_SPIRIT:             res = dbc.race_base( race ).spirit + dbc.attribute_base( class_type, level ).spirit;
                                     if ( race == RACE_HUMAN ) res *= 1.03; break;
  case BASE_STAT_HEALTH:             res = dbc.health_base( class_type, level ); break;
  case BASE_STAT_MANA:               res = dbc.resource_base( class_type, level ); break;
  case BASE_STAT_MELEE_CRIT_PER_AGI: res = dbc.melee_crit_scaling( class_type, level ); break;
  case BASE_STAT_SPELL_CRIT_PER_INT: res = dbc.spell_crit_scaling( class_type, level ); break;
  case BASE_STAT_DODGE_PER_AGI:      res = dbc.dodge_scaling( class_type, level ); break;
  case BASE_STAT_MELEE_CRIT:         res = dbc.melee_crit_base( class_type, level ); break;
  case BASE_STAT_SPELL_CRIT:         res = dbc.spell_crit_base( class_type, level ); break;
  case BASE_STAT_MP5:                res = dbc.regen_base( class_type, level ); break;
  case BASE_STAT_SPI_REGEN:          res = dbc.regen_spirit( class_type, level ); break;
  default: break;
  }

  return res;
}
