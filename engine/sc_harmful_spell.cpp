// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Spell
// ==========================================================================

// spell_t::spell_t =========================================================

// == Harmful Spell Constructor ===============

spell_t::spell_t( const std::string&  token,
                  player_t*           p,
                  const spell_data_t* s ) :
  spell_base_t( ACTION_SPELL, token, p, s )
{
  may_miss = true;

  may_trigger_dtr                = true;

  crit_bonus = 1.0;

  if ( player -> meta_gem == META_AGILE_SHADOWSPIRIT         ||
       player -> meta_gem == META_AGILE_PRIMAL               ||
       player -> meta_gem == META_BURNING_SHADOWSPIRIT       ||
       player -> meta_gem == META_BURNING_PRIMAL             ||
       player -> meta_gem == META_CHAOTIC_SKYFIRE            ||
       player -> meta_gem == META_CHAOTIC_SKYFLARE           ||
       player -> meta_gem == META_CHAOTIC_SHADOWSPIRIT       ||
       player -> meta_gem == META_RELENTLESS_EARTHSIEGE      ||
       player -> meta_gem == META_RELENTLESS_EARTHSTORM      ||
       player -> meta_gem == META_REVERBERATING_SHADOWSPIRIT ||
       player -> meta_gem == META_REVERBERATING_PRIMAL )
  {
    crit_multiplier *= 1.03;
  }
}

// spell_t::target_debuff ===================================================

void spell_t::target_debuff( player_t* t, dmg_e type )
{
  spell_base_t::target_debuff( t, type );

  if ( ! no_debuffs )
  {
    target_crit += target -> composite_spell_crit_vulnerability();
  }

  if ( sim -> debug )
    sim -> output( "spell_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f",
                   name(), target_multiplier, target_hit, target_crit );
}

// spell_t::miss_chance =====================================================

double spell_t::miss_chance( double hit, int delta_level )
{
  double miss = 0;

  miss = 0.06 + ( delta_level * 0.03 );

  miss -= hit;

  if ( miss < 0.00 ) miss = 0.00;
  if ( miss > 0.99 ) miss = 0.99;

  return miss;
}

// spell_t::execute =========================================================

void spell_t::execute()
{
  spell_base_t::execute();

  if ( harmful && callbacks )
  {
    result_e r = execute_state ? execute_state -> result : result;

    if ( ! is_tick_action && r != RESULT_NONE  )
    {
      action_callback_t::trigger( player -> callbacks.harmful_spell[ r ], this );
      if ( execute_state && execute_state -> result_amount > 0 )
      {
        action_callback_t::trigger( player -> callbacks.direct_harmful_spell[ r ], this );
      }
    }
    if ( ! background ) // OnHarmfulSpellCast
    {
      action_callback_t::trigger( player -> callbacks.harmful_spell[ RESULT_NONE ], this );
    }
  }
}
