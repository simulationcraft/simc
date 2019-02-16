// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Attack
// ==========================================================================

attack_t::attack_t( const std::string& n, player_t* p, const spell_data_t* s )
  : action_t( ACTION_ATTACK, n, p, s ),
    base_attack_expertise( 0 ),
    attack_table()
{
  crit_bonus = 1.0;
  special = true; // Make sure to set this to false with autoattacks. 

  weapon_power_mod = 1.0 / WEAPON_POWER_COEFFICIENT;
  min_gcd          = p->min_gcd;
  hasted_ticks = false;

  crit_multiplier *= util::crit_multiplier( p->meta_gem );
}

void attack_t::execute()
{
  action_t::execute();
}

timespan_t attack_t::execute_time() const
{
  if ( base_execute_time == timespan_t::zero() )
    return timespan_t::zero();

  if ( !harmful && !player->in_combat )
    return timespan_t::zero();

  return base_execute_time * player->cache.attack_speed();
}

dmg_e attack_t::amount_type( const action_state_t*, bool periodic ) const
{
  if ( periodic )
    return DMG_OVER_TIME;
  else
    return DMG_DIRECT;
}

dmg_e attack_t::report_amount_type( const action_state_t* state ) const
{
  dmg_e result_type = state->result_type;

  if ( result_type == DMG_DIRECT )
  {
    // Direct ticks are direct damage, that are recorded as ticks
    if ( direct_tick )
    {
      result_type = DMG_OVER_TIME;
    }
    else
    {
      // With direct damage, we need to check if this action is a tick action of
      // someone. If so, then the damage should be recorded as periodic.
      if ( stats->action_list.front()->tick_action == this )
      {
        result_type = DMG_OVER_TIME;
      }
    }
  }

  return result_type;
}

double attack_t::miss_chance( double hit, player_t* t ) const
{
  if ( t->is_enemy() && sim->auto_attacks_always_land && !special )
  {
    return 0.0;
  }

  // cache.miss() contains the target's miss chance (3.0 base in almost all cases)
  double miss = t->cache.miss();

  // add or subtract 1.5% per level difference
  miss += ( t->level() - player->level() ) * 0.015;

  // asymmetric hit penalty for npcs attacking higher-level players
  if ( !t->is_enemy() )
    miss += std::max( t->level() - player->level() - 3, 0 ) * 0.015;

  // subtract the player's hit chance
  miss -= hit;

  return miss;
}

double attack_t::dodge_chance( double expertise, player_t* t ) const
{
  if ( t->is_enemy() && sim->auto_attacks_always_land && !special )
  {
    return 0.0;
  }

  // cache.dodge() contains the target's dodge chance (3.0 base, plus spec bonuses and rating)
  double dodge = t->cache.dodge();

  // WoD mechanics are unchanged from MoP add or subtract 1.5% per level difference
  dodge += ( t->level() - player->level() ) * 0.015;

  // subtract the player's expertise chance
  dodge -= expertise;

  return dodge;
}

double attack_t::block_chance( action_state_t* s ) const
{
  if ( s->target->is_enemy() && sim->auto_attacks_always_land && !special )
  {
    return 0.0;
  }

  // cache.block() contains the target's block chance (3.0 base for bosses, more for shield tanks)
  double block = s->target->cache.block();

  // add or subtract 1.5% per level difference -- Level difference does not seem to matter anymore.
  //block += ( s->target->level() - player->level() ) * 0.015;

  return block;
}

double attack_t::crit_block_chance( action_state_t* s ) const
{
  // This function is probably unnecessary, as we could just query
  // cache.crit_block() directly.
  // I'm leaving it for consistency with *_chance() and in case future changes
  // modify crit block mechanics

  // Crit Block does not suffer from level-based suppression, return cached
  // value directly
  return s->target->cache.crit_block();
}

void attack_t::attack_table_t::build_table( double miss_chance,
                                            double dodge_chance,
                                            double parry_chance,
                                            double glance_chance,
                                            double crit_chance, sim_t* sim )
{
  sim->print_debug("attack_t::build_table: miss={} dodge={} parry={} glance={} crit={}",
        miss_chance, dodge_chance, parry_chance, glance_chance, crit_chance );

  assert( crit_chance >= 0 && crit_chance <= 1.0 );

  double sum = miss_chance + dodge_chance + parry_chance + crit_chance;

  // Only recalculate attack table if the stats have changed
  if ( attack_table_sum == sum )
    return;
  attack_table_sum = sum;

  double limit = 1.0;
  double total = 0;

  num_results = 0;

  if ( miss_chance > 0 && total < limit )
  {
    total += miss_chance;
    if ( total > limit )
      total                = limit;
    chances[ num_results ] = total;
    results[ num_results ] = RESULT_MISS;
    num_results++;
  }
  if ( dodge_chance > 0 && total < limit )
  {
    total += dodge_chance;
    if ( total > limit )
      total                = limit;
    chances[ num_results ] = total;
    results[ num_results ] = RESULT_DODGE;
    num_results++;
  }
  if ( parry_chance > 0 && total < limit )
  {
    total += parry_chance;
    if ( total > limit )
      total                = limit;
    chances[ num_results ] = total;
    results[ num_results ] = RESULT_PARRY;
    num_results++;
  }
  if ( glance_chance > 0 && total < limit )
  {
    total += glance_chance;
    if ( total > limit )
      total                = limit;
    chances[ num_results ] = total;
    results[ num_results ] = RESULT_GLANCE;
    num_results++;
  }
  if ( crit_chance > 0 && total < limit )
  {
    total += crit_chance;
    if ( total > limit )
      total                = limit;
    chances[ num_results ] = total;
    results[ num_results ] = RESULT_CRIT;
    num_results++;
  }
  if ( total < 1.0 )
  {
    chances[ num_results ] = 1.0;
    results[ num_results ] = RESULT_HIT;
    num_results++;
  }
}

result_e attack_t::calculate_result( action_state_t* s ) const
{
  result_e result = RESULT_NONE;

  if ( !harmful || !may_hit || !s->target )
    return RESULT_NONE;

  int delta_level = s->target->level() - player->level();

  double miss = may_miss ? miss_chance( composite_hit(), s->target ) : 0;
  double dodge =
      may_dodge ? dodge_chance( composite_expertise(), s->target ) : 0;
  double parry = may_parry && player->position() == POSITION_FRONT
                     ? parry_chance( composite_expertise(), s->target )
                     : 0;
  double crit = may_crit ? std::max( s->composite_crit_chance() +
                                         s->target->cache.crit_avoidance(),
                                     0.0 )
                         : 0;

  // Specials are 2-roll calculations, so only pass crit chance to
  // build_table for non-special attacks

  attack_table.build_table( miss, dodge, parry,
                            may_glance ? glance_chance( delta_level ) : 0.0,
                            !special ? std::min( 1.0, crit ) : 0.0, sim );

  if ( attack_table.num_results == 1 )
  {
    result = attack_table.results.front();
  }
  else
  {
    // 1-roll attack table with true RNG

    double random = rng().real();

    for ( int i = 0; i < attack_table.num_results; ++i )
    {
      if ( random <= attack_table.chances[ i ] )
      {
        result = attack_table.results[ i ];
        break;
      }
    }
  }

  assert( result != RESULT_NONE );

  // if we have a special, make a second roll for hit/crit
  if ( result == RESULT_HIT && special && may_crit )
  {
    if ( rng().roll( crit ) )
      result = RESULT_CRIT;
  }

  return result;
}

void attack_t::init()
{
  action_t::init();

  if ( special )
    may_glance = false;
}

void attack_t::reschedule_auto_attack( double old_swing_haste )
{
  if ( player->cache.attack_speed() == old_swing_haste )
  {
    return;
  }

  // Note that if attack -> swing_haste() > old_swing_haste, this could
  // probably be handled by rescheduling, but the code is slightly simpler if
  // we just cancel the event and make a new one.
  if ( execute_event && execute_event->remains() > timespan_t::zero() )
  {
    timespan_t time_to_hit = execute_event->occurs() - sim->current_time();
    timespan_t new_time_to_hit =
        time_to_hit * player->cache.attack_speed() / old_swing_haste;

    if ( time_to_hit == new_time_to_hit )
    {
      return;
    }

    sim->print_debug("Haste change, reschedule {} attack from {} to {} (speed {} -> {}, remains {})",
        name(), execute_event->remains(), new_time_to_hit, old_swing_haste, player->cache.attack_speed(),
        time_to_hit);

    if ( new_time_to_hit > time_to_hit )
      execute_event->reschedule( new_time_to_hit );
    else
    {
      event_t::cancel( execute_event );
      execute_event = start_action_execute_event( new_time_to_hit );
    }
  }
}

// ==========================================================================
// Melee Attack
// ==========================================================================

melee_attack_t::melee_attack_t( const std::string& n, player_t* p, const spell_data_t* s )
  : attack_t( n, p, s )
{
  may_miss = may_dodge = may_parry = may_glance = may_block = true;

  // Prevent melee from being scheduled when player is moving
  if ( range < 0 )
    range = 5;
}

void melee_attack_t::init()
{
  attack_t::init();

  if ( special )
    may_glance = false;
}

double melee_attack_t::parry_chance( double expertise, player_t* t ) const
{
  if ( t->is_enemy() && sim->auto_attacks_always_land && !special )
  {
    return 0.0;
  }

  // cache.parry() contains the target's parry chance (3.0 base, plus spec
  // bonuses and rating)
  double parry = t->cache.parry();

  // WoD mechanics are similar to MoP
  // add or subtract 1.5% per level difference
  parry += ( t->level() - player->level() ) * 0.015;

  // 3% additional parry for attacking a level+3 or higher NPC
  if ( t->is_enemy() && ( t->level() - player->level() ) > 2 )
    parry += 0.03;

  // subtract the player's expertise chance - no longer depends on dodge
  parry -= expertise;

  return parry;
}

double melee_attack_t::glance_chance( int delta_level ) const
{
  double glance = 0;

  // TODO-WOD: Glance chance increase per 4+ level delta?
  if ( delta_level > 3 )
  {
    glance += 0.10 + 0.10 * delta_level;
  }

  return glance;
}

proc_types melee_attack_t::proc_type() const
{
  if ( s_data->ok() )
  {
    switch ( s_data->dmg_class() )
    {
      case SPELL_TYPE_NONE:   return PROC1_NONE_SPELL;
      case SPELL_TYPE_MAGIC:  return PROC1_MAGIC_SPELL;
      case SPELL_TYPE_MELEE:  return special ? PROC1_MELEE_ABILITY : PROC1_MELEE;
      case SPELL_TYPE_RANGED: return special ? PROC1_RANGED_ABILITY : PROC1_RANGED;
    }
  }

  if ( special )
    return PROC1_MELEE_ABILITY;
  else
    return PROC1_MELEE;
}

// ==========================================================================
// Ranged Attack
// ==========================================================================

ranged_attack_t::ranged_attack_t( const std::string& token, player_t* p, const spell_data_t* s )
  : attack_t( token, p, s )
{
  may_miss  = true;
  may_dodge = true;
}

// Ranged attacks are identical to melee attacks, but cannot be parried or dodged.
// all of the inherited *_chance() methods are accurate.

double ranged_attack_t::composite_target_multiplier( player_t* target ) const
{
  double v = attack_t::composite_target_multiplier( target );

  return v;
}

void ranged_attack_t::schedule_execute( action_state_t* execute_state )
{
  sim->print_log( "{} schedules execute for {} ({})",
      player->name(), name(), player->resources.current[ player->primary_resource() ] );

  time_to_execute = execute_time();

  execute_event = start_action_execute_event( time_to_execute, execute_state );

  if ( trigger_gcd > timespan_t::zero() )
    player->off_gcdactions.clear();

  if ( !background )
  {
    if ( player->queueing == this )
    {
      player->queueing = nullptr;
    }

    player->executing      = this;
    player->gcd_ready      = sim->current_time() + gcd();
    player->gcd_haste_type = gcd_haste;
    switch ( gcd_haste )
    {
      case HASTE_SPELL:
        player->gcd_current_haste_value = player->cache.spell_haste();
        break;
      case HASTE_ATTACK:
        player->gcd_current_haste_value = player->cache.attack_haste();
        break;
      default:
        break;
    }

    if ( player->action_queued && sim->strict_gcd_queue )
    {
      player->gcd_ready -= sim->queue_gcd_reduction;
    }
  }
}

proc_types ranged_attack_t::proc_type() const
{
  if ( s_data->ok() )
  {
    switch ( s_data->dmg_class() )
    {
      case SPELL_TYPE_NONE:   return PROC1_NONE_SPELL;
      case SPELL_TYPE_MAGIC:  return PROC1_MAGIC_SPELL;
      case SPELL_TYPE_MELEE:  return special ? PROC1_MELEE_ABILITY : PROC1_MELEE;
      case SPELL_TYPE_RANGED: return special ? PROC1_RANGED_ABILITY : PROC1_RANGED;
    }
  }

  if ( special )
    return PROC1_RANGED_ABILITY;
  else
    return PROC1_RANGED;
}
