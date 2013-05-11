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
    pvp_power         = max;
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
    pvp_power         = dbc.combat_rating( RATING_PVP_POWER,    level );
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
