// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "proc_rng.hpp"

#include "dbc/dbc.hpp"
#include "item/item.hpp"
#include "player/player.hpp"
#include "sim/sim.hpp"
#include "util/rng.hpp"

proc_rng_t::proc_rng_t() : player( nullptr ), rng_type( rng_type_e::RNG_SIMPLE )
{}

proc_rng_t::proc_rng_t( std::string_view n, player_t* p, rng_type_e type )
  : name_str( n ), player( p ), rng_type( type )
{}

simple_proc_t::simple_proc_t( std::string_view n, player_t* p, double c )
  : proc_rng_t( n, p, rng_type_e::RNG_SIMPLE ), chance( c )
{}

bool simple_proc_t::trigger()
{
  return player->rng().roll( chance );
}

real_ppm_t::real_ppm_t()
  : proc_rng_t( "", nullptr, rng_type_e::RNG_RPPM ),
    freq( 0 ),
    modifier( 1.0 ),
    rppm( 0 ),
    scales_with( RPPM_NONE ),
    blp_state( BLP_ENABLED )
{}

real_ppm_t::real_ppm_t( std::string_view n, player_t* p, double f, double mod, unsigned s, blp b )
  : proc_rng_t( n, p, rng_type_e::RNG_RPPM ),
    freq( f ),
    modifier( mod ),
    rppm( freq * mod ),
    scales_with( s ),
    blp_state( b )
{}

real_ppm_t::real_ppm_t( std::string_view n, player_t* p, const spell_data_t* data, const item_t* item )
  : proc_rng_t( n, p, rng_type_e::RNG_RPPM ),
    freq( data->real_ppm() ),
    modifier( p->dbc->real_ppm_modifier( data->id(), player, item ? item->item_level() : 0 ) ),
    rppm( freq * modifier ),
    scales_with( p->dbc->real_ppm_scale( data->id() ) ),
    blp_state( BLP_ENABLED )
{}

double real_ppm_t::proc_chance()
{
  double coeff = 1.0;
  double seconds = std::min( player->sim->current_time() - last_trigger_attempt, max_interval ).total_seconds();

  if ( scales_with & RPPM_HASTE )
    coeff *= player->cache.rppm_haste_coeff();

  // This might technically be two separate crit values, but this should be sufficient for our cases. In any case, the
  // client data does not offer information which crit it is (attack or spell).
  if ( scales_with & RPPM_CRIT )
    coeff *= player->cache.rppm_crit_coeff();

  if ( scales_with & RPPM_AUTO_ATTACK_SPEED )
    coeff *= 1.0 / player->cache.auto_attack_speed();

  double real_ppm = rppm * coeff;
  double old_rppm_chance = real_ppm * ( seconds / 60.0 );
  double rppm_chance = 0;

  if ( blp_state == BLP_ENABLED )
  {
    // RPPM Extension added on 12. March 2013: http://us.battle.net/wow/en/blog/8953693?page=44
    // Formula see http://us.battle.net/wow/en/forum/topic/8197741003#1
    double last_success = std::min( accumulated_blp, max_bad_luck_prot ).total_seconds();
    double expected_average_proc_interval = 60.0 / real_ppm;

    rppm_chance =
      std::max( 1.0, 1 + ( ( last_success / expected_average_proc_interval - 1.5 ) * 3.0 ) ) * old_rppm_chance;
  }
  else
  {
    rppm_chance = old_rppm_chance;
  }

  if ( player->sim->debug )
  {
    player->sim->print_debug(
      "RPPM: {} base={:.3f} coeff={:.3f} last_trig={:.3f} last_proc={:.3f} scales={} blp={} chance={:.5f}%", name(),
      rppm, coeff, last_trigger_attempt.total_seconds(), accumulated_blp.total_seconds(), scales_with,
      blp_state == BLP_ENABLED ? "enabled" : "disabled", rppm_chance * 100.0 );
  }

  return rppm_chance;
}

void real_ppm_t::reset()
{
  last_trigger_attempt = 0_ms;
  accumulated_blp = 0_ms;
}

bool real_ppm_t::trigger()
{
  if ( freq <= 0 )
    return false;

  if ( last_trigger_attempt == player->sim->current_time() )
    return false;

  // 2020-10-11: Instead of using the aboslute time to the last successful proc, it appears
  // that the amount of time that is added on each trigger attempt is capped at max_interval
  accumulated_blp += std::min( player->sim->current_time() - last_trigger_attempt, max_interval );
  double chance = proc_chance();
  bool success = player->rng().roll( chance );

  last_trigger_attempt = player->sim->current_time();

  if ( success )
    accumulated_blp = 0_ms;

  return success;
}

shuffled_rng_t::shuffled_rng_t( std::string_view n, player_t* p, int success_entries, int total_entries )
  : proc_rng_t( n, p, rng_type_e::RNG_SHUFFLE ),
    success_entries( success_entries ),
    total_entries( total_entries ),
    success_entries_remaining( success_entries ),
    total_entries_remaining( total_entries )
{}

void shuffled_rng_t::reset()
{
  success_entries_remaining = success_entries;
  total_entries_remaining = total_entries;
}

bool shuffled_rng_t::trigger()
{
  if ( total_entries <= 0 || success_entries <= 0 )
    return false;

  if ( total_entries_remaining <= 0 )
    reset();  // Re-Shuffle the "Deck"

  bool result = false;

  if ( success_entries_remaining > 0 )
  {
    result = player->rng().roll( get_remaining_success_chance() );

    if ( result )
      success_entries_remaining--;
  }

  total_entries_remaining--;

  if ( total_entries_remaining <= 0 )
    reset();  // Re-Shuffle the "Deck"

  return result;
}

accumulated_rng_t::accumulated_rng_t( std::string_view n, player_t* p, double c,
                                      std::function<double( double, unsigned )> fn, unsigned initial_count )
  : proc_rng_t( n, p, rng_type_e::RNG_ACCUMULATE ),
    accumulator_fn( std::move( fn ) ),
    proc_chance( c ),
    initial_count( initial_count ),
    trigger_count( initial_count )
{}

void accumulated_rng_t::reset()
{
  trigger_count = initial_count;
}

bool accumulated_rng_t::trigger()
{
  if ( proc_chance <= 0 )
    return false;

  trigger_count++;

  auto chance = accumulator_fn ? accumulator_fn( proc_chance, trigger_count ) : proc_chance * trigger_count;
  auto result = player->rng().roll( chance );

  if ( player->sim->debug )
  {
    player->sim->print_debug( "Accumulated RNG: {}, base={:.3f} count={} chance={:.5f}%", name(), proc_chance,
                              trigger_count, chance * 100.0 );
  }

  if ( result )
    reset();

  return result;
}
