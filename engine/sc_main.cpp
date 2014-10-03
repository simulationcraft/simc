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

bool need_to_save_profiles( sim_t* sim )
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

/* Obtain a platform specific place to store the http cache file
 */
std::string get_cache_directory()
{
  std::string s = ".";

  const char* env; // store desired environemental variable in here. getenv returns a null pointer if specified
  // environemental variable cannot be found.
#if defined(__linux__) || defined(__APPLE__)
  env = getenv( "XDG_CACHE_HOME" );
  if ( ! env )
  {
    env = getenv( "HOME" );
    if ( env )
      s = std::string( env ) + "/.cache";
    else
      s = "/tmp"; // back out
  }
  else
    s = std::string( env );
#endif
#ifdef _WIN32
  env = getenv( "TMP" );
  if ( !env )
  {
    env = getenv( "TEMP" );
    if ( ! env )
    {
      env = getenv( "HOME" );
    }
  }
  s = std::string( env );
#endif

  return s;
}

// RAII-wrapper for dbc init / de-init
struct dbc_initializer_t {
  dbc_initializer_t()
  { dbc::init(); }
  ~dbc_initializer_t()
  { dbc::de_init(); }
};

// RAII-wrapper for http cache load / save
struct cache_initializer_t {
  cache_initializer_t( const std::string& fn ) :
    _file_name( fn )
  { http::cache_load( _file_name ); }
  ~cache_initializer_t()
  { http::cache_save( _file_name ); }
private:
  std::string _file_name;
};

} // anonymous namespace ====================================================

// sim_t::main ==============================================================

int sim_t::main( const std::vector<std::string>& args )
{
  sim_signal_handler_t handler( this );

  cache_initializer_t cache_init( ( get_cache_directory() + "/simc_cache.dat" ).c_str() );
  dbc_initializer_t dbc_init;
  module_t::init();

  sim_control_t control;

  try
  {
    control.options.parse_args(args);
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR! Incorrect option format: " << e.what() << std::endl;
    return 1;
  }

  try
  {
    setup( &control );
  }
  catch( const std::exception& e ){
    std::cerr <<  "ERROR! Setup failure: " << e.what() << std::endl;
    return 1;
  }

  if ( canceled ) return 1;

  util::printf( "\nSimulationCraft %s for World of Warcraft %s %s (build level %s)\n",
                 SC_VERSION, dbc.wow_version(), dbc.wow_ptr_status(), util::to_string( dbc.build_level() ).c_str() );

  if ( spell_query )
  {
    spell_query -> evaluate();
    report::print_spell_query( this, spell_query_level );
  }
  else if ( need_to_save_profiles( this ) )
  {
    init();
    std::cout << "\nGenerating profiles... \n";
    report::print_profiles( this );
  }
  else
  {
    util::printf( "\nSimulating... ( iterations=%d, max_time=%.0f, vary_combat_length=%0.2f, optimal_raid=%d, fight_style=%s )\n",
                   iterations, max_time.total_seconds(), vary_combat_length, optimal_raid, fight_style.c_str() );

    std::cout << "\nGenerating baseline... " << std::endl;

    sim_phase_str = "Generating baseline:   ";
    if ( execute() )
    {
      scaling      -> analyze();
      plot         -> analyze();
      reforge_plot -> analyze();
      report::print_suite( this );
    }
    else
      canceled = 1;
  }

  std::cout << std::endl;

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
