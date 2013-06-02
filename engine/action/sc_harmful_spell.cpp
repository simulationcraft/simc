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
}

// spell_t::miss_chance =====================================================

double spell_t::miss_chance( double hit, int delta_level )
{
  double miss = 0.06 + ( delta_level * 0.03 );

  miss -= hit;

  return miss;
}

// spell_t::assess_damage ====================================================

void spell_t::assess_damage( dmg_e type,
                             action_state_t* s )
{
  spell_base_t::assess_damage( type, s );

  if ( type == DMG_DIRECT )
  {
    if ( s -> result_amount > 0.0 )
    {
      if ( direct_tick_callbacks )
      {
        action_callback_t::trigger( player -> callbacks.spell_tick_damage[ school ], this, s );
      }
      else
      {
        if ( callbacks ) action_callback_t::trigger( player -> callbacks.spell_direct_damage[ school ], this, s );
      }
    }
  }
  else // DMG_OVER_TIME
  {
    if ( callbacks && s -> result_amount > 0.0 ) action_callback_t::trigger( player -> callbacks.spell_tick_damage[ school ], this, s );
  }
}

// spell_t::execute =========================================================

void spell_t::execute()
{
  spell_base_t::execute();

  if ( harmful && callbacks )
  {
    result_e r = execute_state ? execute_state -> result : result;

    if ( r != RESULT_NONE  )
    {
      action_callback_t::trigger( player -> callbacks.harmful_spell[ r ], this );
      if ( execute_state && execute_state -> result_amount > 0 && ! direct_tick_callbacks )
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

void spell_t::init()
{
  spell_base_t::init();

  if ( harmful )
    procs_courageous_primal_diamond = false;

}
