// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Spell
// ==========================================================================

// spell_t::spell_t =========================================================

void spell_t::init_spell_t_()
{
  may_miss = may_resist = true;

  may_trigger_dtr                = true;

  crit_bonus = 0.5;

  if ( player -> meta_gem == META_AGILE_SHADOWSPIRIT         ||
       player -> meta_gem == META_BURNING_SHADOWSPIRIT       ||
       player -> meta_gem == META_CHAOTIC_SKYFIRE            ||
       player -> meta_gem == META_CHAOTIC_SKYFLARE           ||
       player -> meta_gem == META_CHAOTIC_SHADOWSPIRIT       ||
       player -> meta_gem == META_RELENTLESS_EARTHSIEGE      ||
       player -> meta_gem == META_RELENTLESS_EARTHSTORM      ||
       player -> meta_gem == META_REVERBERATING_SHADOWSPIRIT )
  {
    crit_multiplier *= 1.03;
  }
}

spell_t::spell_t( const spell_id_t& s, talent_tree_type_e t ) :
  spell_base_t( ACTION_SPELL, s, t )
{
  init_spell_t_();
}

spell_t::spell_t( const std::string& n, player_t* p, resource_type_e r, school_type_e s, talent_tree_type_e t ) :
  spell_base_t( ACTION_SPELL, n, p, r, s, t )
{
  init_spell_t_();
}

spell_t::spell_t( const std::string& n, const char* sname, player_t* p, talent_tree_type_e t ) :
  spell_base_t( ACTION_SPELL, n, sname, p, t )
{
  init_spell_t_();
}

spell_t::spell_t( const std::string& n, const uint32_t id, player_t* p, talent_tree_type_e t ) :
  spell_base_t( ACTION_SPELL, n, id, p, t )
{
  init_spell_t_();
}

// spell_t::player_buff =====================================================

void spell_t::player_buff()
{
  spell_base_t::player_buff();

  if ( ! no_buffs )
  {
    player_hit  = player -> composite_spell_hit();

    player_penetration = player -> composite_spell_penetration();
  }

  if ( sim -> debug ) log_t::output( sim, "spell_t::player_buff: %s hit=%.2f penetration=%.2f",
                                     name(), player_hit, player_penetration );
}

// spell_t::target_debuff ===================================================

void spell_t::target_debuff( player_t* t, dmg_type_e type )
{
  spell_base_t::target_debuff( t, type );

  if ( ! no_debuffs )
  {
    target_crit += target -> composite_spell_crit_vulnerability();
  }

  if ( sim -> debug )
    log_t::output( sim, "spell_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f",
                   name(), target_multiplier, target_hit, target_crit );
}

// spell_t::miss_chance =====================================================

double spell_t::miss_chance( int delta_level ) const
{
  double miss = 0;

  if ( delta_level > 2 )
  {
    miss = 0.17 + ( delta_level - 3 ) * 0.11;
  }
  else
  {
    miss = 0.04 + delta_level * 0.01;
  }

  miss -= total_hit();

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
    if ( result != RESULT_NONE )
    {
      action_callback_t::trigger( player -> harmful_spell_callbacks[ result ], this );
    }
    if ( ! background ) // OnHarmfulSpellCast
    {
      action_callback_t::trigger( player -> harmful_spell_callbacks[ RESULT_NONE ], this );
    }
  }
}
