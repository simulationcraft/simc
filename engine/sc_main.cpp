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

  static void report( int signal )
  {
    const char* name = strsignal( signal );
    fprintf( stderr, "sim_signal_handler: %s! Iteration=%d Seed=%lu TargetHealth=%lu\n",
       name, global_sim -> current_iteration, global_sim -> seed,
       (uint64_t) global_sim -> target -> resources.initial[ RESOURCE_HEALTH ] );
    fflush( stderr );
  }

  static void sigint( int signal )
  {
    if ( global_sim )
    {
      report( signal );
      if( global_sim -> scaling -> calculate_scale_factors || global_sim -> reforge_plot -> current_reforge_sim || global_sim -> plot -> current_plot_stat != STAT_NONE )
      {
        global_sim -> cancel();
      }
      else if ( global_sim -> single_actor_batch )
      {
        global_sim -> cancel();
      }
      else
      {
        global_sim -> interrupt();
      }
    }
  }

  static void sigsegv( int signal )
  {
    if ( global_sim )
    {
      report( signal );
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

    sa.sa_handler = sigsegv;
    sigaction( SIGSEGV, &sa, nullptr );

    sa.sa_handler = sigint;
    sigaction( SIGINT,  &sa, nullptr );
  }

  ~sim_signal_handler_t()
  { global_sim = nullptr; }
};
sim_t* sim_signal_handler_t::global_sim = nullptr;
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

struct special_effect_initializer_t
{
  special_effect_initializer_t()
  {
    unique_gear::register_special_effects();
    unique_gear::sort_special_effects();
  }

  ~special_effect_initializer_t()
  { unique_gear::unregister_special_effects(); }
};

} // anonymous namespace ====================================================

// sim_t::main ==============================================================

int sim_t::main( const std::vector<std::string>& args )
{
  sim_signal_handler_t handler( this );

  cache_initializer_t cache_init( get_cache_directory() + "/simc_cache.dat" );
  dbc_initializer_t dbc_init;
  module_t::init();
  unique_gear::register_hotfixes();

  special_effect_initializer_t special_effect_init;

  sim_control_t control;

  try
  {
    control.options.parse_args(args);
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR! Incorrect option format: " << e.what() << std::endl;
    return 1;
  }

  // Hotfixes are applies right before the sim context (control) is created, and simulator setup
  // begins
  hotfix::apply();

  bool setup_success = true;
  std::string errmsg;
  try
  {
    setup( &control );
  }
  catch( const std::exception& e ){
    errmsg = e.what();
    setup_success = false;
  }

#if ! defined( SC_GIT_REV )
  util::printf("SimulationCraft %s for World of Warcraft %s %s (wow build %s)\n",
      SC_VERSION, dbc.wow_version(), dbc.wow_ptr_status(), util::to_string(dbc.build_level()).c_str());
#else
  util::printf("SimulationCraft %s for World of Warcraft %s %s (wow build %s, git build %s)\n",
      SC_VERSION, dbc.wow_version(), dbc.wow_ptr_status(), util::to_string(dbc.build_level()).c_str(), SC_GIT_REV);
#endif

  if ( display_hotfixes )
  {
    std::cout << hotfix::to_str( dbc.ptr );
    return 0;
  }

  if ( display_bonus_ids )
  {
    std::cout << dbc::bonus_ids_str( dbc );
    return 0;
  }

  if ( ! setup_success )
  {
    std::cerr <<  "ERROR! Setup failure: " << errmsg << std::endl;
    return 1;
  }

  if ( canceled ) return 1;

  std::cout << std::endl;

  if ( spell_query )
  {
    try
    {
      spell_query -> evaluate();
      print_spell_query();
    }
    catch( const std::exception& e ){
      std::cerr <<  "ERROR! Spell Query failure: " << e.what() << std::endl;
      return 1;
    }
  }
  else if ( need_to_save_profiles( this ) )
  {
    init();
    std::cout << "\nGenerating profiles... \n";
    report::print_profiles( this );
  }
  else
  {
    util::printf( "\nSimulating... ( iterations=%d, threads=%d, target_error=%.3f,  max_time=%.0f, vary_combat_length=%0.2f, optimal_raid=%d, fight_style=%s )\n\n",
      iterations, threads, target_error, max_time.total_seconds(), vary_combat_length, optimal_raid, fight_style.c_str() );

    set_sim_base_str( "Baseline" );
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
#if defined( SC_VS ) && SC_VS < 13
  // Ensure unified scientific notation exponent rules between different VS versions
  _set_output_format( _TWO_DIGIT_EXPONENT );
#endif

  sim_t sim;
  return sim.main( io::utf8_args( argc, argv ) );
}
