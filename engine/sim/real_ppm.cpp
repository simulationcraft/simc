// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "real_ppm.hpp"
#include "dbc/dbc.hpp"
#include "player/sc_player.hpp"
#include "sim/sc_sim.hpp"
#include "util/rng.hpp"
#include "item/item.hpp"

real_ppm_t::real_ppm_t( util::string_view name, player_t* p, const spell_data_t* data, const item_t* item )
  : player( p ),
    name_str( name ),
    freq( data->real_ppm() ),
    modifier( p->dbc->real_ppm_modifier( data->id(), player, item ? item->item_level() : 0 ) ),
    rppm( freq * modifier ),
    last_trigger_attempt( timespan_t::zero() ),
    last_successful_trigger( timespan_t::zero() ),
    mean_trigger_attempt( 0 ),
    trigger_attempts( 0 ),
    mean_base_trigger_chance( 0 ),
    scales_with( p->dbc->real_ppm_scale( data->id() ) ),
    blp_state( BLP_ENABLED )
{
}

double real_ppm_t::proc_chance()
{
  auto sim                    = player->sim;
  double coeff                = 1.0;
  double last_trigger_seconds = ( sim->current_time() - last_trigger_attempt ).total_seconds();
  double seconds              = std::min( last_trigger_seconds, max_interval() );

  if ( scales_with & RPPM_HASTE )
    coeff *= player->cache.rppm_haste_coeff();

  // This might technically be two separate crit values, but this should be sufficient for our
  // cases. In any case, the client data does not offer information which crit it is (attack or
  // spell).
  if ( scales_with & RPPM_CRIT )
    coeff *= player->cache.rppm_crit_coeff();

  if ( scales_with & RPPM_ATTACK_SPEED )
    coeff *= 1.0 / player->cache.attack_speed();

  double real_ppm        = rppm * coeff;
  double old_rppm_chance = real_ppm * ( seconds / 60.0 );
  double rppm_chance     = 0;

  if ( blp_state == BLP_ENABLED )
  {
    // RPPM Extension added on 12. March 2013: http://us.battle.net/wow/en/blog/8953693?page=44
    // Formula see http://us.battle.net/wow/en/forum/topic/8197741003#1
    double last_success =
        std::min( ( sim->current_time() - last_successful_trigger ).total_seconds(), max_bad_luck_prot() );

    // 2020-10-05: The old implementation was fine when all trigger attempts were within max_interval() of each other,
    // but was wrong in cases where less frequent trigger attempts occur. This is most noticeable with effects that
    // also have ICDs. Instead, the calculation needs to take the distribution of recent trigger attempts into account
    // in some way to estimate how many procs should be expected. In game, this expected interval may be calculated in
    // a different way because these averages don't seem to perfectly match observed behavior, although they are close.
    mean_base_trigger_chance = ( mean_base_trigger_chance * trigger_attempts + old_rppm_chance ) / ( trigger_attempts + 1 );
    mean_trigger_attempt = ( mean_trigger_attempt * trigger_attempts + last_trigger_seconds ) / ( trigger_attempts + 1 );
    double expected_average_proc_interval = mean_trigger_attempt / mean_base_trigger_chance;

    rppm_chance =
        std::max( 1.0, 1 + ( ( last_success / expected_average_proc_interval - 1.5 ) * 3.0 ) ) * old_rppm_chance;
  }
  else
  {
    rppm_chance = old_rppm_chance;
  }

  if ( sim->debug )
  {
    sim->out_debug.print(
        "base={:.3f} coeff={:.3f} last_trig={:.3f} last_proc={:.3f}"
        " scales={} blp={} chance={:.5f}%",
        rppm, coeff, last_trigger_attempt.total_seconds(), last_successful_trigger.total_seconds(), scales_with,
        blp_state == BLP_ENABLED ? "enabled" : "disabled", rppm_chance * 100.0 );
  }

  return rppm_chance;
}

bool real_ppm_t::trigger()
{
  if ( freq <= 0 )
  {
    return false;
  }

  if ( last_trigger_attempt == player->sim->current_time() )
    return false;

  double chance = proc_chance();
  bool success  = player->rng().roll( chance );

  last_trigger_attempt = player->sim->current_time();
  trigger_attempts++;

  if ( success )
  {
    mean_base_trigger_chance = 0;
    mean_trigger_attempt = 0;
    trigger_attempts = 0;
    last_successful_trigger = player->sim->current_time();
  }
  return success;
}
