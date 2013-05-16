// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include <locale>

#ifdef SC_SIGACTION
#include <signal.h>
#endif

namespace { // anonymous namespace ==========================================

#ifdef SC_SIGACTION
// POSIX-only signal handler ================================================

struct sim_signal_handler_t
{
  static sim_t* global_sim;

  static void sigint( int )
  {
    if ( global_sim )
    {
      if ( global_sim -> canceled ) exit( 1 );
      global_sim -> cancel();
    }
  }

  static void callback( int signal )
  {
    if ( global_sim )
    {
      const char* name = strsignal( signal );
      fprintf( stderr, "sim_signal_handler: %s! Seed=%d Iteration=%d\n",
               name, global_sim -> seed, global_sim -> current_iteration );
      fflush( stderr );
    }
    exit( 1 );
  }

  sim_signal_handler_t( sim_t* sim )
  {
    assert ( ! global_sim );
    global_sim = sim;
    struct sigaction sa;
    sigemptyset( &sa.sa_mask );
    sa.sa_flags = 0;

    sa.sa_handler = callback;
    sigaction( SIGSEGV, &sa, 0 );

    sa.sa_handler = sigint;
    sigaction( SIGINT,  &sa, 0 );
  }

  ~sim_signal_handler_t()
  { global_sim = 0; }
};
sim_t* sim_signal_handler_t::global_sim = 0;
#else
struct sim_signal_handler_t
{
  sim_signal_handler_t( sim_t* ) {}
};
#endif

// need_to_save_profiles ====================================================

static bool need_to_save_profiles( sim_t* sim )
{
  if ( sim -> save_profiles ) return true;

  for ( size_t i = 0; i < sim -> player_list.size(); ++i )
  {
    player_t* p = sim -> player_list[ i ];
    if ( ! p -> report_information.save_str.empty() )
      return true;
  }

  return false;
}

} // anonymous namespace ====================================================

// sim_t::main ==============================================================

int sim_t::main( const std::vector<std::string>& args )
{
  sim_signal_handler_t handler( this );

  std::string cache_directory = ".";
#ifdef __linux__
  cache_directory = getenv( "XDG_CACHE_HOME" );
  if ( cache_directory.empty() )
  {
    cache_directory = getenv( "HOME" );
    if ( ! cache_directory.empty() )
      cache_directory += "/.cache";
    else
      cache_directory = "/tmp"; // back out
  }
#endif
#ifdef _WIN32
  cache_directory = getenv( "TMP" );
  if ( cache_directory.empty() )
  {
    cache_directory = getenv( "TEMP" );
    if ( cache_directory.empty() )
    {
      cache_directory = getenv( "HOME" );
      if ( cache_directory.empty() )
        cache_directory = "."; // back out
    }
  }
#endif
  http::cache_load( ( cache_directory + "/simc_cache.dat" ).c_str() );
  dbc::init();
  module_t::init();

  sim_control_t control;

  if ( ! control.options.parse_args( args ) )
  {
    errorf( "ERROR! Incorrect option format..\n" );
    return 1;
  }
  else if ( ! setup( &control ) )
  {
    errorf( "ERROR! Setup failure...\n" );
    return 1;
  }

  if ( challenge_mode ) scale_to_itemlevel = 463;

  if ( canceled ) return 1;

  util::fprintf( output_file, "\nSimulationCraft %s-%s for World of Warcraft %s %s (build level %s)\n",
                 SC_MAJOR_VERSION, SC_MINOR_VERSION, dbc.wow_version(), ( dbc.ptr ?
#if SC_BETA
                     "BETA"
#else
                     "PTR"
#endif
                     : "Live" ), util::to_string( dbc.build_level() ).c_str() );
  fflush( output_file );

  if ( spell_query )
  {
    spell_query -> evaluate();
    report::print_spell_query( this, spell_query_level );
  }
  else if ( need_to_save_profiles( this ) )
  {
    init();
    util::fprintf( stdout, "\nGenerating profiles... \n" ); fflush( stdout );
    report::print_profiles( this );
  }
  else
  {
    if ( max_time <= timespan_t::zero() )
    {
      util::fprintf( output_file, "simulationcraft: One of -max_time or -target_health must be specified.\n" );
      exit( 0 );
    }
    if ( fabs( vary_combat_length ) >= 1.0 )
    {
      util::fprintf( output_file, "\n |vary_combat_length| >= 1.0, overriding to 0.0.\n" );
      vary_combat_length = 0.0;
    }
    if ( confidence <= 0.0 || confidence >= 1.0 )
    {
      util::fprintf( output_file, "\nInvalid confidence, reseting to 0.95.\n" );
      confidence = 0.95;
    }

    util::fprintf( output_file,
                   "\nSimulating... ( iterations=%d, max_time=%.0f, vary_combat_length=%0.2f, optimal_raid=%d, fight_style=%s )\n",
                   iterations, max_time.total_seconds(), vary_combat_length, optimal_raid, fight_style.c_str() );
    fflush( output_file );

//    util::fprintf( stdout, "\nGenerating baseline... \n" ); fflush( stdout );

    sim_phase_str = "Generating baseline:   ";
    if ( execute() )
    {
      scaling      -> analyze();
      plot         -> analyze();
      reforge_plot -> analyze();
      util::fprintf( stdout, "\nGenerating reports...\n" ); fflush( stdout );
      report::print_suite( this );
    }
  }

  if ( output_file != stdout )
    fclose( output_file );
  output_file = 0;

  http::cache_save( ( cache_directory + "/simc_cache.dat" ).c_str() );
  dbc::de_init();

  return canceled;
}

// ==========================================================================
// MAIN
// ==========================================================================

int main( int argc, char** argv )
{
  std::locale::global( std::locale( "C" ) );

  sim_t sim;
  return sim.main( io::utf8_args( argc, argv ) );
}
