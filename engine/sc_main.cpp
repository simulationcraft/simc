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
    std::cerr << "sim_signal_handler: " << name << "!";
    const sim_t* crashing_child = nullptr;

#ifndef SC_NO_THREADING
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
#endif

    if ( crashing_child )
    {
      fmt::print(stderr, " Thread={} Iteration={} Seed={} ({}) TargetHealth={}\n",
          crashing_child -> thread_index, crashing_child -> current_iteration, crashing_child -> seed,
          ( crashing_child -> seed + crashing_child -> thread_index ),
          crashing_child -> target -> resources.initial[ RESOURCE_HEALTH ]);
    }
    else
    {
      fmt::print(stderr, " Iteration={} Seed={} TargetHealth={}\n",
          global_sim -> current_iteration, global_sim -> seed,
          global_sim -> target -> resources.initial[ RESOURCE_HEALTH ]);
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

#if !defined( SC_NO_NETWORKING )
struct apitoken_initializer_t
{
  apitoken_initializer_t()
  { bcp_api::token_load(); }

  ~apitoken_initializer_t()
  { bcp_api::token_save(); }
};
#endif

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

void print_version_info(const dbc_t& dbc)
{
  std::string build_info = fmt::format("wow build {}", dbc.build_level());
  if ( git_info::available() )
  {
    build_info += fmt::format(", git build {} {}", git_info::branch(), git_info::revision());
  }

  fmt::print("SimulationCraft {} for World of Warcraft {} {} ({})\n\n",
      SC_VERSION, dbc.wow_version(), dbc.wow_ptr_status(), build_info);
  std::flush(std::cout);
}

} // anonymous namespace ====================================================

// sim_t::main ==============================================================

int sim_t::main( const std::vector<std::string>& args )
{
  try
  {
    cache_initializer_t cache_init( get_cache_directory() + "/simc_cache.dat" );
#if !defined( SC_NO_NETWORKING )
    apitoken_initializer_t apitoken_init;
#endif
    dbc_initializer_t dbc_init;
    module_t::init();
    unique_gear::register_hotfixes();

    special_effect_initializer_t special_effect_init;

    print_version_info(dbc);

    sim_control_t control;

    try
    {
      control.options.parse_args(args);
    }
    catch (const std::exception& e) {

      std::throw_with_nested(std::invalid_argument("Incorrect option format"));
    }

    // Hotfixes are applies right before the sim context (control) is created, and simulator setup
    // begins
    hotfix::apply();

    try
    {
      setup( &control );
    }
    catch( const std::exception& e ){
      std::throw_with_nested(std::runtime_error("Setup failure"));
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

    if ( canceled )
    {
      return 1;
    }


    if ( spell_query )
    {
      try
      {
        spell_query -> evaluate();
        print_spell_query();
      }
      catch( const std::exception& e ){
        std::throw_with_nested(std::runtime_error("Spell Query Error"));
      }
    }
    else if ( need_to_save_profiles( this ) )
    {
      try
      {
        init();
        std::cout << "\nGenerating profiles... \n";
        report::print_profiles( this );
      }
      catch( const std::exception& e ){
        std::throw_with_nested(std::runtime_error("Generating profiles"));
      }
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

        if ( canceled == 0 && ! profilesets.iterate( this ))
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
  catch (const std::nested_exception& e) {
    // Only catch exception we have already re-thrown in init functions.
    std::cerr << "Error: ";
    util::print_chained_exception(e.nested_ptr());
    std::cerr << std::endl;
    return 1;
  }
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
