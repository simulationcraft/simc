// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "player_stat_cache.hpp"
#include "player.hpp"
#include "action/action_state.hpp"
#include "action/action.hpp"


/**
 * Invalidate cache for ALL stats.
 */
void player_stat_cache_t::invalidate_all()
{
  if ( !active )
    return;

  range::fill( valid, false );
  range::fill( spell_power_valid, false );
  range::fill( player_mult_valid, false );
  range::fill( player_heal_mult_valid, false );
}

/**
 * Invalidate cache for a specific cache.
 */
void player_stat_cache_t::invalidate( cache_e c )
{
  switch ( c )
  {
    case CACHE_SPELL_POWER:
      range::fill( spell_power_valid, false );
      break;
    case CACHE_PLAYER_DAMAGE_MULTIPLIER:
      range::fill( player_mult_valid, false );
      break;
    case CACHE_PLAYER_HEAL_MULTIPLIER:
      range::fill( player_heal_mult_valid, false );
      break;
    default:
      valid[ c ] = false;
      break;
  }
}

/**
 * Get access attribute cache functions by attribute-enumeration.
 */
double player_stat_cache_t::get_attribute( attribute_e a ) const
{
  switch ( a )
  {
    case ATTR_STRENGTH:
      return strength();
    case ATTR_AGILITY:
      return agility();
    case ATTR_STAMINA:
      return stamina();
    case ATTR_INTELLECT:
      return intellect();
    case ATTR_SPIRIT:
      return spirit();
    default:
      assert( false );
      break;
  }
  return 0.0;
}

#if defined( SC_USE_STAT_CACHE )

double player_stat_cache_t::strength() const
{
  if ( !active || !valid[ CACHE_STRENGTH ] )
  {
    valid[ CACHE_STRENGTH ] = true;
    _strength               = player->strength();
  }
  else
    assert( _strength == player->strength() );
  return _strength;
}

double player_stat_cache_t::agility() const
{
  if ( !active || !valid[ CACHE_AGILITY ] )
  {
    valid[ CACHE_AGILITY ] = true;
    _agility               = player->agility();
  }
  else
    assert( _agility == player->agility() );
  return _agility;
}

double player_stat_cache_t::stamina() const
{
  if ( !active || !valid[ CACHE_STAMINA ] )
  {
    valid[ CACHE_STAMINA ] = true;
    _stamina               = player->stamina();
  }
  else
    assert( _stamina == player->stamina() );
  return _stamina;
}

double player_stat_cache_t::intellect() const
{
  if ( !active || !valid[ CACHE_INTELLECT ] )
  {
    valid[ CACHE_INTELLECT ] = true;
    _intellect               = player->intellect();
  }
  else
    assert( _intellect == player->intellect() );
  return _intellect;
}

double player_stat_cache_t::spirit() const
{
  if ( !active || !valid[ CACHE_SPIRIT ] )
  {
    valid[ CACHE_SPIRIT ] = true;
    _spirit               = player->spirit();
  }
  else
    assert( _spirit == player->spirit() );
  return _spirit;
}

double player_stat_cache_t::spell_power( school_e s ) const
{
  if ( !active || !spell_power_valid[ s ] )
  {
    spell_power_valid[ s ] = true;
    _spell_power[ s ]      = player->composite_spell_power( s );
  }
  else
    assert( _spell_power[ s ] == player->composite_spell_power( s ) );
  return _spell_power[ s ];
}

double player_stat_cache_t::attack_power() const
{
  if ( !active || !valid[ CACHE_ATTACK_POWER ] )
  {
    valid[ CACHE_ATTACK_POWER ] = true;
    _attack_power               = player->composite_melee_attack_power();
  }
  else
    assert( _attack_power == player->composite_melee_attack_power() );
  return _attack_power;
}

double player_stat_cache_t::total_melee_attack_power() const
{
  if( !active || !valid[ CACHE_TOTAL_MELEE_ATTACK_POWER ] )
  {
    valid[ CACHE_TOTAL_MELEE_ATTACK_POWER ] = true;
    _total_melee_attack_power               = player->composite_melee_attack_power_by_type( player->default_ap_type() );
  }
  else
   assert( _total_melee_attack_power == player->composite_melee_attack_power_by_type( player->default_ap_type() ) );
  return _total_melee_attack_power;
}

double player_stat_cache_t::attack_expertise() const
{
  if ( !active || !valid[ CACHE_ATTACK_EXP ] )
  {
    valid[ CACHE_ATTACK_EXP ] = true;
    _attack_expertise         = player->composite_melee_expertise();
  }
  else
    assert( _attack_expertise == player->composite_melee_expertise() );
  return _attack_expertise;
}

double player_stat_cache_t::attack_hit() const
{
  if ( !active || !valid[ CACHE_ATTACK_HIT ] )
  {
    valid[ CACHE_ATTACK_HIT ] = true;
    _attack_hit               = player->composite_melee_hit();
  }
  else
  {
    if ( _attack_hit != player->composite_melee_hit() )
    {
      assert( false );
    }
    // assert( _attack_hit == player -> composite_attack_hit() );
  }
  return _attack_hit;
}

double player_stat_cache_t::attack_crit_chance() const
{
  if ( !active || !valid[ CACHE_ATTACK_CRIT_CHANCE ] )
  {
    valid[ CACHE_ATTACK_CRIT_CHANCE ] = true;
    _attack_crit_chance               = player->composite_melee_crit_chance();
  }
  else
    assert( _attack_crit_chance == player->composite_melee_crit_chance() );
  return _attack_crit_chance;
}

double player_stat_cache_t::attack_haste() const
{
  if ( !active || !valid[ CACHE_ATTACK_HASTE ] )
  {
    valid[ CACHE_ATTACK_HASTE ] = true;
    _attack_haste               = player->composite_melee_haste();
  }
  else
    assert( _attack_haste == player->composite_melee_haste() );
  return _attack_haste;
}

double player_stat_cache_t::attack_speed() const
{
  if ( !active || !valid[ CACHE_ATTACK_SPEED ] )
  {
    valid[ CACHE_ATTACK_SPEED ] = true;
    _attack_speed               = player->composite_melee_speed();
  }
  else
    assert( _attack_speed == player->composite_melee_speed() );
  return _attack_speed;
}

double player_stat_cache_t::spell_hit() const
{
  if ( !active || !valid[ CACHE_SPELL_HIT ] )
  {
    valid[ CACHE_SPELL_HIT ] = true;
    _spell_hit               = player->composite_spell_hit();
  }
  else
    assert( _spell_hit == player->composite_spell_hit() );
  return _spell_hit;
}

double player_stat_cache_t::spell_crit_chance() const
{
  if ( !active || !valid[ CACHE_SPELL_CRIT_CHANCE ] )
  {
    valid[ CACHE_SPELL_CRIT_CHANCE ] = true;
    _spell_crit_chance               = player->composite_spell_crit_chance();
  }
  else
    assert( _spell_crit_chance == player->composite_spell_crit_chance() );
  return _spell_crit_chance;
}

double player_stat_cache_t::rppm_haste_coeff() const
{
  if ( !active || !valid[ CACHE_RPPM_HASTE ] )
  {
    valid[ CACHE_RPPM_HASTE ] = true;
    _rppm_haste_coeff          = 1.0 / std::min( player->cache.spell_haste(), player->cache.attack_haste() );
  }
  else
  {
    assert( _rppm_haste_coeff == 1.0 / std::min( player->cache.spell_haste(), player->cache.attack_haste() ) );
  }
  return _rppm_haste_coeff;
}

double player_stat_cache_t::rppm_crit_coeff() const
{
  if ( !active || !valid[ CACHE_RPPM_CRIT ] )
  {
    valid[ CACHE_RPPM_CRIT ] = true;
    _rppm_crit_coeff          = 1.0 + std::max( player->cache.attack_crit_chance(), player->cache.spell_crit_chance() );
  }
  else
  {
    assert( _rppm_crit_coeff == 1.0 + std::max( player->cache.attack_crit_chance(), player->cache.spell_crit_chance() ) );
  }
  return _rppm_crit_coeff;
}

double player_stat_cache_t::spell_haste() const
{
  if ( !active || !valid[ CACHE_SPELL_HASTE ] )
  {
    valid[ CACHE_SPELL_HASTE ] = true;
    _spell_haste               = player->composite_spell_haste();
  }
  else
    assert( _spell_haste == player->composite_spell_haste() );
  return _spell_haste;
}

double player_stat_cache_t::spell_speed() const
{
  if ( !active || !valid[ CACHE_SPELL_SPEED ] )
  {
    valid[ CACHE_SPELL_SPEED ] = true;
    _spell_speed               = player->composite_spell_speed();
  }
  else
    assert( _spell_speed == player->composite_spell_speed() );
  return _spell_speed;
}

double player_stat_cache_t::dodge() const
{
  if ( !active || !valid[ CACHE_DODGE ] )
  {
    valid[ CACHE_DODGE ] = true;
    _dodge               = player->composite_dodge();
  }
  else
    assert( _dodge == player->composite_dodge() );
  return _dodge;
}

double player_stat_cache_t::parry() const
{
  if ( !active || !valid[ CACHE_PARRY ] )
  {
    valid[ CACHE_PARRY ] = true;
    _parry               = player->composite_parry();
  }
  else
    assert( _parry == player->composite_parry() );
  return _parry;
}

double player_stat_cache_t::block() const
{
  if ( !active || !valid[ CACHE_BLOCK ] )
  {
    valid[ CACHE_BLOCK ] = true;
    _block               = player->composite_block();
  }
  else
    assert( _block == player->composite_block() );
  return _block;
}

double player_stat_cache_t::crit_block() const
{
  if ( !active || !valid[ CACHE_CRIT_BLOCK ] )
  {
    valid[ CACHE_CRIT_BLOCK ] = true;
    _crit_block               = player->composite_crit_block();
  }
  else
    assert( _crit_block == player->composite_crit_block() );
  return _crit_block;
}

double player_stat_cache_t::crit_avoidance() const
{
  if ( !active || !valid[ CACHE_CRIT_AVOIDANCE ] )
  {
    valid[ CACHE_CRIT_AVOIDANCE ] = true;
    _crit_avoidance               = player->composite_crit_avoidance();
  }
  else
    assert( _crit_avoidance == player->composite_crit_avoidance() );
  return _crit_avoidance;
}

double player_stat_cache_t::miss() const
{
  if ( !active || !valid[ CACHE_MISS ] )
  {
    valid[ CACHE_MISS ] = true;
    _miss               = player->composite_miss();
  }
  else
    assert( _miss == player->composite_miss() );
  return _miss;
}

double player_stat_cache_t::armor() const
{
  if ( !active || !valid[ CACHE_ARMOR ] || !valid[ CACHE_BONUS_ARMOR ] )
  {
    valid[ CACHE_ARMOR ] = true;
    _armor               = player->composite_armor();
  }
  else
    assert( _armor == player->composite_armor() );
  return _armor;
}

double player_stat_cache_t::mastery() const
{
  if ( !active || !valid[ CACHE_MASTERY ] )
  {
    valid[ CACHE_MASTERY ] = true;
    _mastery               = player->composite_mastery();
    _mastery_value         = player->composite_mastery_value();
  }
  else
    assert( _mastery == player->composite_mastery() );
  return _mastery;
}

/**
 * This is composite_mastery * specialization_mastery_coefficient !
 *
 * If you need the pure mastery value, use player_t::composite_mastery
 */
double player_stat_cache_t::mastery_value() const
{
  if ( !active || !valid[ CACHE_MASTERY ] )
  {
    valid[ CACHE_MASTERY ] = true;
    _mastery               = player->composite_mastery();
    _mastery_value         = player->composite_mastery_value();
  }
  else
    assert( _mastery_value == player->composite_mastery_value() );
  return _mastery_value;
}

double player_stat_cache_t::bonus_armor() const
{
  if ( !active || !valid[ CACHE_BONUS_ARMOR ] )
  {
    valid[ CACHE_BONUS_ARMOR ] = true;
    _bonus_armor               = player->composite_bonus_armor();
  }
  else
    assert( _bonus_armor == player->composite_bonus_armor() );
  return _bonus_armor;
}

double player_stat_cache_t::damage_versatility() const
{
  if ( !active || !valid[ CACHE_DAMAGE_VERSATILITY ] )
  {
    valid[ CACHE_DAMAGE_VERSATILITY ] = true;
    _damage_versatility               = player->composite_damage_versatility();
  }
  else
    assert( _damage_versatility == player->composite_damage_versatility() );
  return _damage_versatility;
}

double player_stat_cache_t::heal_versatility() const
{
  if ( !active || !valid[ CACHE_HEAL_VERSATILITY ] )
  {
    valid[ CACHE_HEAL_VERSATILITY ] = true;
    _heal_versatility               = player->composite_heal_versatility();
  }
  else
    assert( _heal_versatility == player->composite_heal_versatility() );
  return _heal_versatility;
}

double player_stat_cache_t::mitigation_versatility() const
{
  if ( !active || !valid[ CACHE_MITIGATION_VERSATILITY ] )
  {
    valid[ CACHE_MITIGATION_VERSATILITY ] = true;
    _mitigation_versatility               = player->composite_mitigation_versatility();
  }
  else
    assert( _mitigation_versatility == player->composite_mitigation_versatility() );
  return _mitigation_versatility;
}

double player_stat_cache_t::leech() const
{
  if ( !active || !valid[ CACHE_LEECH ] )
  {
    valid[ CACHE_LEECH ] = true;
    _leech               = player->composite_leech();
  }
  else
    assert( _leech == player->composite_leech() );
  return _leech;
}

double player_stat_cache_t::run_speed() const
{
  if ( !active || !valid[ CACHE_RUN_SPEED ] )
  {
    valid[ CACHE_RUN_SPEED ] = true;
    _run_speed               = player->composite_movement_speed();
  }
  else
    assert( _run_speed == player->composite_movement_speed() );
  return _run_speed;
}

double player_stat_cache_t::avoidance() const
{
  if ( !active || !valid[ CACHE_AVOIDANCE ] )
  {
    valid[ CACHE_AVOIDANCE ] = true;
    _avoidance               = player->composite_avoidance();
  }
  else
    assert( _avoidance == player->composite_avoidance() );
  return _avoidance;
}

double player_stat_cache_t::corruption() const
{
  if ( !active || !valid[ CACHE_CORRUPTION ] )
  {
    valid[ CACHE_CORRUPTION ] = true;
    _corruption               = player->composite_corruption();
  }
  else
    assert( _corruption == player->composite_corruption() );
  return _corruption;
}

double player_stat_cache_t::corruption_resistance() const
{
  if ( !active || !valid[ CACHE_CORRUPTION_RESISTANCE ] )
  {
    valid[ CACHE_CORRUPTION_RESISTANCE ] = true;
    _corruption_resistance               = player->composite_corruption_resistance();
  }
  else
    assert( _corruption_resistance == player->composite_corruption_resistance() );
  return _corruption_resistance;
}

double player_stat_cache_t::player_multiplier( school_e s ) const
{
  if ( !active || !player_mult_valid[ s ] )
  {
    player_mult_valid[ s ] = true;
    _player_mult[ s ]      = player->composite_player_multiplier( s );
  }
  else
    assert( _player_mult[ s ] == player->composite_player_multiplier( s ) );
  return _player_mult[ s ];
}

double player_stat_cache_t::player_heal_multiplier( const action_state_t* s ) const
{
  school_e sch = s->action->get_school();

  if ( !active || !player_heal_mult_valid[ sch ] )
  {
    player_heal_mult_valid[ sch ] = true;
    _player_heal_mult[ sch ]      = player->composite_player_heal_multiplier( s );
  }
  else
    assert( _player_heal_mult[ sch ] == player->composite_player_heal_multiplier( s ) );
  return _player_heal_mult[ sch ];
}

#else
  // Passthrough cache stat functions for inactive cache
  double player_stat_cache_t::strength() const  { return _player -> strength();  }
  double player_stat_cache_t::agility() const   { return _player -> agility();   }
  double player_stat_cache_t::stamina() const   { return _player -> stamina();   }
  double player_stat_cache_t::intellect() const { return _player -> intellect(); }
  double player_stat_cache_t::spirit() const    { return _player -> spirit();    }
  double player_stat_cache_t::spell_power( school_e s ) const { return _player -> composite_spell_power( s ); }
  double player_stat_cache_t::attack_power() const            { return _player -> composite_melee_attack_power();   }
  double player_stat_cache_t::total_melee_attack_power() const { return _player->composite_melee_attack_power_by_type( _player -> default_ap_type() ); }
  double player_stat_cache_t::attack_expertise() const { return _player -> composite_melee_expertise(); }
  double player_stat_cache_t::attack_hit() const       { return _player -> composite_melee_hit();       }
  double player_stat_cache_t::attack_crit_chance() const      { return _player -> composite_melee_crit_chance();      }
  double player_stat_cache_t::attack_haste() const     { return _player -> composite_melee_haste();     }
  double player_stat_cache_t::attack_speed() const     { return _player -> composite_melee_speed();     }
  double player_stat_cache_t::spell_hit() const        { return _player -> composite_spell_hit();       }
  double player_stat_cache_t::spell_crit_chance() const       { return _player -> composite_spell_crit_chance();      }
  double player_stat_cache_t::spell_haste() const      { return _player -> composite_spell_haste();     }
  double player_stat_cache_t::spell_speed() const      { return _player -> composite_spell_speed();     }
  double player_stat_cache_t::dodge() const            { return _player -> composite_dodge();      }
  double player_stat_cache_t::parry() const            { return _player -> composite_parry();      }
  double player_stat_cache_t::block() const            { return _player -> composite_block();      }
  double player_stat_cache_t::crit_block() const       { return _player -> composite_crit_block(); }
  double player_stat_cache_t::crit_avoidance() const   { return _player -> composite_crit_avoidance();       }
  double player_stat_cache_t::miss() const             { return _player -> composite_miss();       }
  double player_stat_cache_t::armor() const            { return _player -> composite_armor();           }
  double player_stat_cache_t::mastery() const          { return _player -> composite_mastery();   }
  double player_stat_cache_t::mastery_value() const    { return _player -> composite_mastery_value();   }
  double player_stat_cache_t::damage_versatility() const { return _player -> composite_damage_versatility(); }
  double player_stat_cache_t::heal_versatility() const { return _player -> composite_heal_versatility(); }
  double player_stat_cache_t::mitigation_versatility() const { return _player -> composite_mitigation_versatility(); }
  double player_stat_cache_t::leech() const { return _player -> composite_leech(); }
  double player_stat_cache_t::run_speed() const { return _player -> composite_run_speed(); }
  double player_stat_cache_t::avoidance() const { return _player -> composite_avoidance(); }
  double player_stat_cache_t::corruption() const { return _player -> composite_corruption(); }
  double player_stat_cache_t::corruption_resistance() const { return _player -> composite_corruption_resistance(); }
#endif
