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
    last_trigger_attempt( 0_ms ),
    accumulated_blp( 0_ms ),
    scales_with( p->dbc->real_ppm_scale( data->id() ) ),
    blp_state( BLP_ENABLED )
{
}

double real_ppm_t::proc_chance( player_t* player, double PPM, timespan_t last_trigger,
                                timespan_t accumulated_blp, unsigned scales_with, blp blp_state )
{
  auto sim       = player->sim;
  double coeff   = 1.0;
  double seconds = std::min( sim->current_time() - last_trigger, max_interval() ).total_seconds();

  if ( scales_with & RPPM_HASTE )
    coeff *= player->cache.rppm_haste_coeff();

  // This might technically be two separate crit values, but this should be sufficient for our
  // cases. In any case, the client data does not offer information which crit it is (attack or
  // spell).
  if ( scales_with & RPPM_CRIT )
    coeff *= player->cache.rppm_crit_coeff();

  if ( scales_with & RPPM_ATTACK_SPEED )
    coeff *= 1.0 / player->cache.attack_speed();

  double real_ppm        = PPM * coeff;
  double old_rppm_chance = real_ppm * ( seconds / 60.0 );
  double rppm_chance     = 0;

  if ( blp_state == BLP_ENABLED )
  {
    // RPPM Extension added on 12. March 2013: http://us.battle.net/wow/en/blog/8953693?page=44
    // Formula see http://us.battle.net/wow/en/forum/topic/8197741003#1
    double last_success = std::min( accumulated_blp, max_bad_luck_prot() ).total_seconds();
    double expected_average_proc_interval = 60.0 / real_ppm;

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
        PPM, coeff, last_trigger.total_seconds(), accumulated_blp.total_seconds(), scales_with,
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

  // 2020-10-11: Instead of using the aboslute time to the last successful proc, it appears
  // that the amount of time that is added on each trigger attempt is capped at max_interval
  accumulated_blp += std::min( player->sim->current_time() - last_trigger_attempt, max_interval() );
  double chance = proc_chance( player, rppm, last_trigger_attempt, accumulated_blp, scales_with, blp_state );
  bool success  = player->rng().roll( chance );

  last_trigger_attempt = player->sim->current_time();

  if ( success )
    accumulated_blp = 0_ms;

  return success;
}
