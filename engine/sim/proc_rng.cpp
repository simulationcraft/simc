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

proc_rng_t::proc_rng_t( rng_type_e type_ ) : player( nullptr ), rng_type_( type_ )
{}

proc_rng_t::proc_rng_t( rng_type_e type_, std::string_view n, player_t* p )
  : name_str( n ), player( p ), rng_type_( type_ )
{}

simple_proc_t::simple_proc_t( std::string_view n, player_t* p, double c )
  : proc_rng_t( rng_type, n, p ), chance( c )
{}

int simple_proc_t::trigger( action_state_t* )
{
  return player->rng().roll( chance );
}

shuffled_rng_t::shuffled_rng_t( std::string_view n, player_t* p, initializer data )
  : proc_rng_t( rng_type, n, p )
{
  init( data );
}

shuffled_rng_t::shuffled_rng_t( std::string_view n, player_t* p, int success_entries, int total_entries )
  : proc_rng_t( rng_type, n, p)
{
  assert( total_entries >= success_entries );
  init( { { FAIL, total_entries - success_entries }, { SUCCESS, success_entries } } );
}

void shuffled_rng_t::init( initializer data )
{
  // CXX23: use append_range instead of nested loops
  for ( const auto& [ key, count ] : data )
  {
    assert( count >= 0 );
    for ( int i = 0; i < count; ++i )
      entries.emplace_back( key );
  }

  if ( entries.empty() )
    entries.emplace_back( shuffled_rng_e::FAIL );
}

void shuffled_rng_t::reset( reset_type_e /* reset_type */)
{
  player->rng().shuffle( entries.begin(), entries.end() );
  position = entries.begin();
}

int shuffled_rng_t::trigger( action_state_t* )
{
  if ( position == entries.end() )
    reset( reset_type_e::COMBAT );

  return *position++;
}

int shuffled_rng_t::count_remains( int key )
{
  return as<int>( std::count( position, entries.end(), key ) );
}

int shuffled_rng_t::entry_remains()
{
  return as<int>( std::distance( position, entries.end() ) );
}

accumulated_rng_t::accumulated_rng_t( std::string_view n, player_t* p, double c, unsigned cap,
                                      accumulated_rng_fn fn, unsigned initial_count )
  : proc_rng_t( rng_type, n, p ),
    accumulator_fn( std::move( fn ) ),
    proc_chance( c ),
    max_count( cap ),
    initial_count( initial_count ),
    trigger_count( initial_count )
{}

void accumulated_rng_t::reset( reset_type_e /* reset_type */)
{
  trigger_count = initial_count;
}

int accumulated_rng_t::trigger( action_state_t* state )
{
  if ( proc_chance <= 0 )
    return ARNG_FAIL;

  trigger_count++;

  double chance;
  if ( accumulator_fn )
    chance = accumulator_fn( proc_chance, trigger_count, state );
  else
    chance = max_count > 0 && trigger_count >= max_count ? 1.0 : proc_chance * trigger_count;

  assert( !std::isnan( chance ) ); // nan check
  bool result = player->rng().roll( chance );

  if ( player->sim->debug )
  {
    player->sim->print_debug( "Accumulated RNG: {}, base={:.3f} cap={} count={} chance={:.5f}%", name(),
                              proc_chance, max_count, trigger_count, chance * 100.0 );
  }

  if ( result )
  {
    reset( reset_type_e::COMBAT );
    return chance >= 1.0 ? ARNG_GUARANTEED : ARNG_SUCCESS;
  }
  else
  {
    return ARNG_FAIL;
  }
}

threshold_rng_t::threshold_rng_t( std::string_view n, player_t* p, double increment_max, threshold_rng_fn fn,
                                  bool random_initial_state, bool roll_over )
  : proc_rng_t( rng_type, n, p ),
    accumulator_fn( std::move( fn ) ),
    increment_max( increment_max ),
    accumulated_chance( random_initial_state ? player->rng().real() : 0 ),
    random_initial_state( random_initial_state ),
    roll_over( roll_over )
{}

void threshold_rng_t::reset( reset_type_e /* reset_type */)
{
  accumulated_chance = random_initial_state ? player->rng().real() : 0;
}

double threshold_rng_t::get_accumulated_chance()
{
  return accumulated_chance;
}

double threshold_rng_t::get_increment_max()
{
  return increment_max;
}

int threshold_rng_t::trigger( action_state_t* state )
{
  if ( increment_max <= 0 )
    return false;

  auto result = accumulator_fn ? accumulator_fn( increment_max, state ) : player->rng().range( increment_max );

  if ( player->sim->debug )
  {
    player->sim->print_debug( "Threshold RNG: {}, increment_max={:.3f} accumulated={:.5f}% result={:.5f}%", name(),
                              increment_max, accumulated_chance * 100.0, result * 100.0 );
  }

  accumulated_chance += result;

  if ( accumulated_chance >= 1 )
  {
    accumulated_chance = roll_over ? accumulated_chance - 1 : 0;

    if ( player->sim->debug )
    {
      player->sim->print_debug( "Threshold RNG: {}, triggered. roll_over={}, new_accumulated={:.5f}%", name(),
                                roll_over, accumulated_chance * 100.0 );
    }

    return true;
  }

  return false;
}
