// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Attack
// ==========================================================================

// attack_t::attack_t =======================================================

attack_t::attack_t( const char* n, player_t* p, int8_t r, int8_t s, int8_t t ) :
  action_t( ACTION_ATTACK, n, p, r, s, t ),
  base_expertise(0), player_expertise(0), target_expertise(0)
{
  may_miss = may_resist = may_dodge = may_parry = may_glance = may_block = may_crit = true;

  if( player -> position == POSITION_BACK )
  {
    may_parry = false;
    may_block = false;
  }

  base_crit_bonus = 1.0;

  direct_power_mod = tick_power_mod = 1.0 / 14;

  trigger_gcd = p -> base_gcd;
  min_gcd = 1.0;
}
  
// attack_t::haste ==========================================================

double attack_t::haste()
{
  double h = player -> haste;

  if( player -> buffs.bloodlust ) h *= 1.0 / ( 1.0 + 0.30 );

  if(      player -> buffs.swift_retribution     ) h *= 1.0 / ( 1.0 + 0.03 );
  else if( player -> buffs.improved_moonkin_aura ) h *= 1.0 / ( 1.0 + 0.02 );

  if( player -> buffs.windfury_totem != 0 )
  {
    h *= 1.0 / ( 1.0 + player -> buffs.windfury_totem );
  }

  if( player -> buffs.mongoose_mh ) h *= 1.0 / ( 1.0 + 0.02 );
  if( player -> buffs.mongoose_oh ) h *= 1.0 / ( 1.0 + 0.02 );

  return h;
}

// attack_t::player_buff ====================================================

void attack_t::player_buff()
{
  action_t::player_buff();

  player_t* p = player;

  player_hit       = p -> composite_attack_hit();
  player_expertise = p -> composite_attack_expertise();
  player_crit      = p -> composite_attack_crit();
  player_power     = p -> composite_attack_power();

  if( sim -> debug ) 
    report_t::log( sim, "attack_t::player_buff: %s hit=%.2f expertise=%.2f crit=%.2f power=%.2f penetration=%.0f", 
		   name(), player_hit, player_expertise, player_crit, player_power, player_penetration );
}

// attack_t::target_debuff ==================================================

void attack_t::target_debuff( int8_t dmg_type )
{
  action_t::target_debuff( dmg_type );
  target_expertise = 0;
}

// attack_t::build_table ====================================================

void attack_t::build_table( std::vector<double>& chances, 
			    std::vector<int>&    results )
{
  chances.clear();
  results.clear();

  double miss=0, dodge=0, parry=0, glance=0, block=0, crit=0;

  double expertise = total_expertise();

  int delta_level = sim -> target -> level - player -> level;

  if( may_miss  ) 
  {
    if( delta_level > 2 )
    {
      miss  = 0.07 + ( delta_level - 2 ) * 0.02;
    }
    else
    {
      miss = 0.05 + delta_level * 0.005;
    }
    miss -= total_hit();
  }

  if( may_dodge ) 
  {
    dodge  = 0.05 + delta_level * 0.005;
    dodge -= expertise * 0.25;
  }

  if( may_parry ) 
  {
    parry  = 0.05 + delta_level * 0.005;
    parry -= expertise * 0.25;
  }

  if( may_glance ) 
  {
    glance = 0.24;
  }

  if( may_block && sim -> target -> shield ) 
  {
    block = 0.05 + delta_level * 0.005;
  }

  if( may_crit )
  {
    crit = total_crit();
  }

  if( sim -> debug ) report_t::log( sim, "attack_t::build_table: %s miss=%.3f dodge=%.3f parry=%.3f glance=%.3f block=%.3f crit=%.3f",
		   name(), miss, dodge, parry, glance, block, crit );
  
  double total = 0;

  if( miss > 0 )
  {
    total += miss;
    chances.push_back( total );
    results.push_back( RESULT_MISS );
  }
  if( dodge > 0 )
  {
    total += dodge;
    chances.push_back( total );
    results.push_back( RESULT_DODGE );
  }
  if( parry > 0 )
  {
    total += parry;
    chances.push_back( total );
    results.push_back( RESULT_PARRY );
  }
  if( glance > 0 )
  {
    total += glance;
    chances.push_back( total );
    results.push_back( RESULT_GLANCE );
  }
  if( block > 0 )
  {
    total += block;
    chances.push_back( total );
    results.push_back( RESULT_BLOCK );
  }
  if( crit > 0 )
  {
    total += crit;
    chances.push_back( total );
    results.push_back( RESULT_CRIT );
  }
  if( total < 1.0 )
  {
    chances.push_back( 1.0 );
    results.push_back( RESULT_HIT );
  }
}

// attack_t::caclulate_result ===============================================

void attack_t::calculate_result()
{
  direct_dmg = tick_dmg = 0;

  static std::vector<double> chances;
  static std::vector<int>    results;

  result = RESULT_NONE;

  build_table( chances, results );

  double random = rand_t::gen_float();
  int num_results = results.size();

  for( int i=0; i < num_results; i++ )
  {
    if( random <= chances[ i ] )
    {
      result = results[ i ];
      break;
    }
  }
  assert( result != RESULT_NONE );

  if( binary && result_is_hit() )
  {
    if( rand_t::roll( resistance() ) )
    {
      result = RESULT_RESIST;
    }
  }

  if( sim -> debug ) report_t::log( sim, "%s result for %s is %s", player -> name(), name(), util_t::result_type_string( result ) );
}

