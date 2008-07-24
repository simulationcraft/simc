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

  t *= p -> haste;
  if( p -> buffs.bloodlust     ) t *= 0.7;
  if( p -> buffs.moonkin_haste ) t *= 0.8;

  static bool AFTER_3_0_0 = sim -> patch.after( 3, 0, 0 );
  if( AFTER_3_0_0 && p -> buffs.wrath_of_air ) t *= 0.9;

  return t;
}

// spell_t::duration =========================================================

double spell_t::duration()
{
  double t = base_duration;

  if( channeled ) 
  {
    t *= player -> haste;
    if( player -> buffs.bloodlust ) t *= 0.7;
  }

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

  report_t::debug( sim, "spell_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f", 
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

   report_t::debug( sim, "spell_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f power=%.2f penetration=%.0f", 
		    name(), target_multiplier, target_hit, target_crit, target_power, target_penetration );
}
   
// spell_t::level_based_miss_chance ==========================================

double spell_t::level_based_miss_chance( int8_t player, 
                                        int8_t target )
{
   int8_t diff = target - player;
   if( diff <= -3 ) return 0.01;
   if( diff == -2 ) return 0.02;
   if( diff == -1 ) return 0.03;
   if( diff ==  0 ) return 0.04;
   if( diff == +1 ) return 0.05;
   if( diff == +2 ) return 0.06;
   if( diff == +3 ) return 0.17;
   if( diff == +4 ) return 0.28;
   if( diff == +5 ) return 0.39;
   if( diff == +6 ) return 0.50;
   if( diff == +7 ) return 0.61;
   if( diff == +8 ) return 0.72;
   if( diff == +9 ) return 0.83;
   return 0.94;
}

// spell_t::caclulate_result ================================================

void spell_t::calculate_result()
{
  dd = dot = dot_tick = 0;

  result = RESULT_NONE;

  player_buff();
  target_debuff( DMG_DIRECT );

  if( may_miss )
  {
    double miss_chance = level_based_miss_chance( player -> level, sim -> target -> level );

    miss_chance -= base_hit + player_hit + target_hit;

    if( rand_t::roll( std::max( miss_chance, (double) 0.01 ) ) )
    {
      result = RESULT_MISS;
      return;
    }
  }

  if( may_resist )
  {
    if( binary || num_ticks )
    {
      if( rand_t::roll( resistance() ) )
      {
	result = RESULT_RESIST;
	return;
      }
    }
  }

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
