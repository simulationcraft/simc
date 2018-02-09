// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "util/git_info.hpp"
#include "sim/sc_profileset.hpp"
#include <locale>

#ifdef SC_SIGACTION
#include <csignal>
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
    const sim_t* crashing_child = nullptr;
    if ( signal == SIGSEGV )
    {
      for ( auto child : global_sim -> children )
      {
        if ( std::this_thread::get_id() == child ->  thread_id() )
        {
          crashing_child = child;
          break;
        }
      }
    }

    std::cerr << "sim_signal_handler: " << name << "!";
    if ( crashing_child )
    {
      std::cerr << " Thread=" << crashing_child -> thread_index
                << " Iteration=" << crashing_child -> current_iteration
                << " Seed=" << crashing_child -> seed << " (" << ( crashing_child -> seed + crashing_child -> thread_index ) << ")"
                << " TargetHealth=" << crashing_child -> target -> resources.initial[ RESOURCE_HEALTH ];
    }
    else
    {
      std::cerr << " Iteration=" << global_sim -> current_iteration
                << " Seed=" << global_sim -> seed
                << " TargetHealth=" << global_sim -> target -> resources.initial[ RESOURCE_HEALTH ];
    }

    auto profileset = global_sim -> profilesets.current_profileset_name();
    if ( ! profileset.empty() )
    {
      std::cerr << " ProfileSet=" << profileset;
    }
    std::cerr << std::endl;
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
      else if ( ! global_sim -> profileset_map.empty() )
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
    exit( signal );
  }

  sim_signal_handler_t()
  {
    assert ( ! global_sim );

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
#else
struct sim_signal_handler_t
{
  static sim_t* global_sim;
};
#endif

sim_t* sim_signal_handler_t::global_sim = nullptr;

sim_signal_handler_t handler;

// need_to_save_profiles ====================================================

bool need_to_save_profiles( sim_t* sim )
{
  if ( sim -> save_profiles ) { return true;
}

  for ( auto& player : sim -> player_list )
  {
    if ( ! player -> report_information.save_str.empty() ) {
      return true;
}
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
    if ( env ) {
      s = std::string( env ) + "/.cache";
    } else {
      s = "/tmp"; // back out
}
  }
  else {
    s = std::string( env );
}
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

  if ( !git_info::available() )
  {
  util::printf("SimulationCraft %s for World of Warcraft %s %s (wow build %s)\n",
      SC_VERSION, dbc.wow_version(), dbc.wow_ptr_status(), util::to_string(dbc.build_level()).c_str());
  }
  else
  {
  util::printf("SimulationCraft %s for World of Warcraft %s %s (wow build %s, git build %s %s)\n",
      SC_VERSION, dbc.wow_version(), dbc.wow_ptr_status(), util::to_string(dbc.build_level()).c_str(), git_info::branch(), git_info::revision());
  }

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

  if ( canceled ) { return 1;
}

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

    progress_bar.set_base( "Baseline" );
    if ( execute() )
    {
      scaling      -> analyze();
      plot         -> analyze();
      reforge_plot -> analyze();

      if ( canceled == 0 && ! profilesets.iterate( this ) )
      {
        canceled = 1;
      }
      else
      {
        report::print_suite( this );
      }
    }
    else
    {
      util::printf("Simulation was canceled.\n");
      canceled = 1;
    }
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
  sim_signal_handler_t::global_sim = &sim;

  return sim.main( io::utf8_args( argc, argv ) );
}
