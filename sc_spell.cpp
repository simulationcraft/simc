// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Spell
// ==========================================================================

// spell_t::spell_t ==========================================================

spell_t::spell_t( const char* n, player_t* p, int8_t r, int8_t s, int8_t t ) :
  action_t( ACTION_SPELL, n, p, r, s, t )
{
  may_miss = may_resist = true;
  base_crit_bonus = 0.5;
  trigger_gcd = p -> base_gcd;
  min_gcd = 1.0;
}   

// spell_t::haste ============================================================

double spell_t::haste()
{
  double h = player -> haste;
  if( player -> buffs.bloodlust         ) h *= 0.70;
  if( player -> buffs.moonkin_haste     ) h *= 0.80;
  if( player -> buffs.swift_retribution ) h *= 0.97;
  if( sim_t::WotLK && player -> buffs.wrath_of_air ) h *= 0.9;
  return h;
}

// spell_t::gcd =============================================================

double spell_t::gcd()
{
  double t = trigger_gcd;
  if( t == 0 ) return 0;

  t *= haste();
  if( t < min_gcd ) t = min_gcd;

  return t;
}

// spell_t::execute_time =====================================================

double spell_t::execute_time()
{
  player_t* p = player;

  if( p -> buffs.improved_moonkin_aura )
  {
    p -> uptimes.moonkin_haste -> update( p -> buffs.moonkin_haste );
  }
  
  if( base_execute_time == 0 ) return 0;

  double t = base_execute_time - p -> buffs.cast_time_reduction;
  if( t < 0 ) t = 0;

  t *= haste();

  return t;
}

// spell_t::duration =========================================================

double spell_t::duration()
{
  double t = base_duration;
  if( channeled ) t *= haste();
  return t;
}

// spell_t::player_buff ======================================================

void spell_t::player_buff()
{
  action_t::player_buff();

  player_t* p = player;

  player_hit   = p -> composite_spell_hit();
  player_crit  = p -> composite_spell_crit();
  player_power = p -> composite_spell_power( school );

  if( p -> gear.chaotic_skyfire ) player_crit_bonus *= 1.09;

  if( player -> buffs.elemental_oath )
  {
    player_crit_bonus *= 1.0 + p -> buffs.elemental_oath * 0.03;
  }

  if( sim -> debug ) report_t::log( sim, "spell_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f", 
		   name(), player_hit, player_crit, player_power, player_penetration );
}

// spell_t::target_debuff =====================================================

void spell_t::target_debuff( int8_t dmg_type )
{
  action_t::target_debuff( dmg_type );

   target_t* t = sim -> target;
   
   if( school == SCHOOL_FROST )
   {
      target_crit += ( t -> debuffs.winters_chill * 0.01 );
   }
   else if( school == SCHOOL_HOLY )
   {
     if( t -> debuffs.judgement_of_crusader ) target_power += 218;
   }      

   if( sim -> debug ) report_t::log( sim, "spell_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f power=%.2f penetration=%.0f", 
		    name(), target_multiplier, target_hit, target_crit, target_power, target_penetration );
}
   
// spell_t::level_based_miss_chance ==========================================

double spell_t::level_based_miss_chance( int8_t player, 
					 int8_t target )
{
  int8_t delta_level = target - player;
  double miss=0;

  if( sim_t::WotLK )
  {
    if( delta_level > 2 )
    {
      miss  = 0.07 + ( delta_level - 2 ) * 0.02;
    }
    else
    {
      miss = 0.05 + delta_level * 0.005;
    }
  }
  else
  {
    if( delta_level > 2 )
    {
      miss = 0.17 + ( delta_level - 3 ) * 0.11;
    }
    else
    {
      miss = 0.04 + delta_level * 0.01;
    }
  }

  if( miss < 0.01 ) miss = 0.01;
  if( miss > 0.99 ) miss = 0.99;

  return miss;
}

// spell_t::caclulate_result ================================================

void spell_t::calculate_result()
{
  dd = dot = dot_tick = 0;

  result = RESULT_NONE;

  player_buff();
  target_debuff( DMG_DIRECT );

  if( ( result == RESULT_NONE ) && may_miss )
  {
    double miss_chance = level_based_miss_chance( player -> level, sim -> target -> level );

    miss_chance -= base_hit + player_hit + target_hit;

    if( rand_t::roll( miss_chance ) )
    {
      result = RESULT_MISS;
    }
  }

  if( ( result == RESULT_NONE ) && may_resist )
  {
    if( binary || num_ticks )
    {
      if( rand_t::roll( resistance() ) )
      {
	result = RESULT_RESIST;
      }
    }
  }

  if( result == RESULT_NONE )
  {
    result = RESULT_HIT;

    if( may_crit )
    {
      double crit_chance = base_crit + player_crit + target_crit;

      if( rand_t::roll( crit_chance ) )
      {
	result = RESULT_CRIT;
      }
    }
  }

  if( sim -> debug ) report_t::log( sim, "%s result for %s is %s", player -> name(), name(), util_t::result_type_string( result ) );
}
