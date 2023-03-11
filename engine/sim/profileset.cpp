// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "profileset.hpp"
#include "dbc/dbc.hpp"
#include "sim_control.hpp"
#include "sim.hpp"
#include "report/reports.hpp"
#include "player/player.hpp"
#include "player/player_talent_points.hpp"
#include "item/item.hpp"
#include "util/string_view.hpp"

#ifndef SC_NO_THREADING

#include <future>
#include <iostream>
#include <memory>
#include <sstream>

namespace
{
// Options that allow "appending" or "mapping" cannot really be overridden in the current
// climate. Note that this does not take into account class module options, and there's no
// easy way to do that, short of iterating over defined option parsers every single time.
// The simc option system really needs the "scoping" system fleshed out to understand
// where a specific option (i.e., a name, value tuple) is defined.
//
// Also, any "function" option is going to be undefined behavior whether it can be merged
// or not, but typically the function option semantics work as such that the next parsing
// of the same option will override whatever is being done.
bool overridable_option( const option_tuple_t& tuple )
{
  return tuple.value.rfind( '=' ) == std::string::npos &&
         tuple.name.rfind( "actions", 0 ) == std::string::npos &&
         tuple.name.rfind( "items", 0 ) == std::string::npos &&
         tuple.name.rfind( "raid_events", 0 ) == std::string::npos &&
         tuple.name.rfind( "class_talents", 0 ) == std::string::npos &&
         tuple.name.rfind( "spec_talents", 0 ) == std::string::npos;
}

std::string format_time( double seconds, bool milliseconds = true )
{
  std::stringstream s;

  if ( seconds == 0 )
  {
    return "0s";
  }
  // For milliseconds, just use a quick format
  else if ( seconds < 1 )
  {
    s << util::round( 1000.0 * seconds, 3 ) << "ms";
  }
  // Otherwise, do the whole thing
  else
  {
    int days = 0;
    int hours = 0;
    int minutes = 0;

    double remainder = seconds;

    if ( remainder >= 86400 )
    {
      days = static_cast<int>(remainder / 86400);
      remainder -= days * 86400;
    }

    if ( remainder >= 3600 )
    {
      hours = static_cast<int>(remainder / 3600);
      remainder -= hours * 3600;
    }

    if ( remainder >= 60 )
    {
      minutes = static_cast<int>(remainder / 60);
      remainder -= minutes * 60;
    }

    if ( days > 0 )
    {
      s << days << "d, ";
    }

    if ( hours > 0 )
    {
      s << hours << "h, ";
    }

    if ( minutes > 0 )
    {
      s << minutes << "m, ";
    }

    s << util::round( remainder, milliseconds ? 3 : 0 ) << "s";
  }

  return s.str();
}

// Deallocating profile_sim is the responsibility of the caller (i.e., profileset driver or
// worker_t)
void simulate_profileset( sim_t* parent, profileset::profile_set_t& set, sim_t*& profile_sim )
{
  // Reset random seed for the profileset sims
  profile_sim -> seed = 0;
  profile_sim -> profileset_enabled = true;
  profile_sim -> report_details = 0;
  if ( parent -> profileset_work_threads > 0 )
  {
    profile_sim -> threads = parent -> profileset_work_threads;
    // Disable reporting on parallel sims, instead, rely on parallel sims finishing to report
    // progress. For normal profileset simming we can rely on the normal progressbar updates
    profile_sim -> report_progress = false;
  }
  else
  {
    profile_sim -> progress_bar.set_base( "Profileset" );
    profile_sim -> progress_bar.set_phase( set.name() );
  }

  auto ret = profile_sim -> execute();
  if ( ret )
  {
    profile_sim -> progress_bar.restart();

    if ( set.has_output() )
    {
      report::print_suite( profile_sim );
    }
  }

  if ( !ret || profile_sim -> is_canceled() )
  {
    return;
  }

  const auto player = profile_sim -> player_no_pet_list.data().front();
  auto progress = profile_sim -> progress( nullptr, 0 );

  range::for_each( parent -> profileset_metric, [ & ]( scale_metric_e metric ) {
    auto data = profileset::metric_data( player, metric );

    set.result( metric )
      .min( data.min )
      .first_quartile( data.first_quartile )
      .median( data.median )
      .mean( data.mean )
      .third_quartile( data.third_quartile )
      .max( data.max )
      .stddev( data.std_dev )
      .mean_stddev( data.mean_std_dev )
      .iterations( progress.current_iterations );
  } );

  if ( ! parent -> profileset_output_data.empty() )
  {
    const auto parent_player = parent -> player_no_pet_list.data().front();
    range::for_each( parent -> profileset_output_data, [ & ]( const std::string& option ) {
        save_output_data( set, parent_player, player, option );
    } );
  }

  // Save global statistics back to parent sim
  parent -> elapsed_cpu  += profile_sim -> elapsed_cpu;
  parent -> init_time    += profile_sim -> init_time;
  parent -> merge_time   += profile_sim -> merge_time;
  parent -> analyze_time += profile_sim -> analyze_time;
  parent -> event_mgr.total_events_processed += profile_sim -> event_mgr.total_events_processed;

  set.cleanup_options();
}

// Figure out if the option is the beginning of a player-scope option
bool in_player_scope( const option_tuple_t& opt )
{
  static constexpr std::array<util::string_view, 15> player_scope_opts { {
    "demonhunter", "deathknight", "druid", "evoker", "hunter", "mage", "monk",
    "paladin", "priest", "rogue", "shaman", "warrior", "warlock",
    "armory", "local_json"
  } };

  return range::any_of( player_scope_opts, [ &opt ]( util::string_view name ) {
    return util::str_compare_ci( opt.name, name );
  } );
}

} // unnamed

namespace profileset
{
size_t profilesets_t::done_profilesets() const
{
  if ( m_work_index <= n_workers() )
  {
    return 0;
  }

  return m_work_index - n_workers();
}

sim_control_t* profilesets_t::create_sim_options( const sim_control_t*            original,
                                                  const std::vector<std::string>& opts )
{
  if ( original == nullptr )
  {
    return nullptr;
  }

  sim_control_t new_options;

  try
  {
    new_options.options.parse_args( opts );
  }
  catch ( const std::exception& e ) {
    std::cerr << "ERROR! Incorrect option format: " << e.what() << std::endl;
    return nullptr;
  }

  // Find the insertion index only once, and cache the position to speed up init. 0 denotes "no
  // enemy found".
  if ( m_insert_index == -1 )
  {
    // Find a suitable player-scope variable to start looking for an "enemy" option. "spec" option
    // must be always defined, so we can start the search below from it.
    auto it = range::find_if( original -> options, []( const option_tuple_t& opt ) {
      return in_player_scope( opt );
    } );

    if ( it == original -> options.end() )
    {
      std::cerr << "ERROR! No start of player-scope defined for the simulation" << std::endl;
      return nullptr;
    }

    // Then, find the first enemy= line from the original options. The profileset options need to be
    // inserted after the original player definition, but before any enemy options are defined.
    auto enemy_it = std::find_if( it, original -> options.end(), []( const option_tuple_t& opt ) {
      return util::str_compare_ci( opt.name, "enemy" );
    } );

    if ( enemy_it == original -> options.end() )
    {
      m_insert_index = 0;
    }
    else
    {
      m_insert_index = std::distance( original -> options.begin(), enemy_it );
    }
  }

  auto options_copy = new sim_control_t( *original );
  std::vector<option_tuple_t> filtered_opts;

  // Filter profileset options so that any option overridable in the base options is
  // overriden, and the rest are inserted at the correct position
  for ( const option_tuple_t& t : new_options.options )
  {
    if ( in_player_scope( t ) )
    {
      std::cerr << fmt::format("ERROR! Profilesets cannot define additional actors: {}={}", t.name, t.value) << std::endl;
      return nullptr;
    }

    if ( !overridable_option( t ) )
    {
      filtered_opts.push_back( t );
    }
    // Option that can be overridden in the copy, check if it exists in the base options
    // set and replace if so
    else
    {
      // Note, replace the last occurrence of the option to ensure the profileset option
      // will be set
      auto it = std::find_if( options_copy->options.rbegin(), options_copy->options.rend(),
        [&t]( const option_tuple_t& orig_t ) {
          return orig_t.name == t.name;
      } );

      if ( it != options_copy->options.rend() )
      {
        it->value = t.value;
      }
      else
      {
        filtered_opts.push_back( t );
      }
    }
  }

  // No enemy option defined, insert filtered profileset options to the end of the
  // original options
  if ( m_insert_index == 0 )
  {
    options_copy -> options.insert( options_copy -> options.end(),
                                    filtered_opts.begin(), filtered_opts.end() );
  }
  // Enemy option found, insert filtered profileset options just before the enemy option
  else
  {
    options_copy -> options.insert( options_copy -> options.begin() + m_insert_index,
                                    filtered_opts.begin(), filtered_opts.end() );
  }

  return options_copy;
}

profilesets_t::profilesets_t() : m_state( STARTED ), m_mode( SEQUENTIAL ),
    m_original( nullptr ), m_insert_index( -1 ),
    m_work_index( 0 ),
    m_control_lock( m_mutex, std::defer_lock ),
    m_max_workers( 0 ), 
    m_work_lock( m_work_mutex, std::defer_lock ),
    m_total_elapsed()
{ 

}

profilesets_t::~profilesets_t()
{
  range::for_each( m_thread, []( std::thread& thread ) {
    if ( thread.joinable() )
    {
      thread.join();
    }
  } );

  range::for_each( m_current_work, []( std::unique_ptr<worker_t>& worker ) { worker -> thread().join(); } );
}

profile_set_t::profile_set_t( std::string name, sim_control_t* opts, bool has_output ) :
  m_name( std::move(name) ), m_options( opts ), m_has_output( has_output ), m_output_data( nullptr )
{
}

sim_control_t* profile_set_t::options() const
{
  return m_options;
}

void profile_set_t::cleanup_options()
{
  delete m_options;
  m_options = nullptr;
}

profile_set_t::~profile_set_t()
{
  delete m_options;
}

const profile_result_t& profile_set_t::result( scale_metric_e metric ) const
{
  static const profile_result_t __default {};

  if ( m_results.empty() )
  {
    return __default;
  }

  if ( metric == SCALE_METRIC_NONE )
  {
    return m_results.front();
  }

  auto it = range::find_if( m_results, [ metric ]( const profile_result_t& result ) {
    return metric == result.metric();
  } );

  if ( it != m_results.end() )
  {
    return *it;
  }

  return __default;
}

profile_result_t& profile_set_t::result( scale_metric_e metric )
{
  assert( metric != SCALE_METRIC_NONE );

  auto it = range::find_if( m_results, [ metric ]( profile_result_t& result ) {
    return metric == result.metric();
  } );

  if ( it != m_results.end() )
  {
    return *it;
  }

  m_results.emplace_back( metric );

  return m_results.back();
}

worker_t::worker_t( profilesets_t* master, sim_t* p, profile_set_t* ps ) :
  m_done( false ), m_parent( p ), m_master( master ), m_sim( nullptr ), m_profileset( ps ),
  m_thread( nullptr )
{
  m_thread = new std::thread( [this] { execute(); } );
}

worker_t::~worker_t()
{
  delete m_sim;
  delete m_thread;
}

std::thread& worker_t::thread()
{
  return *m_thread;
}

const std::thread& worker_t::thread() const
{
  return *m_thread;
}

sim_t* worker_t::sim() const
{
  return m_sim;
}

void worker_t::execute()
{
  try
  {
    m_sim = new sim_t( m_parent, 0, m_profileset -> options() );

    simulate_profileset( m_parent, *m_profileset, m_sim );
  }
  catch (const std::exception& e )
  {
    fmt::print( stderr, "\n\nError in profileset worker: " );
    util::print_chained_exception( e, stderr );
    fmt::print( stderr, "\n\n" );
    std::fflush( stderr );
    // TODO: find out how to cancel profilesets without deadlock.
  }

  m_done = true;

  m_master -> notify_worker();
}

// Count the number of running workers
size_t profilesets_t::n_workers() const
{
  return range::count_if( m_current_work, []( const std::unique_ptr<worker_t>& worker ) {
    return ! worker -> is_done();
  } );
}

// Note, we must own the work mutex here.
void profilesets_t::cleanup_work()
{
  assert( m_work_lock.owns_lock() );

  auto it = m_current_work.begin();

  while ( it != m_current_work.end() )
  {
    if ( ( *it ) -> is_done() )
    {
      ( *it ) -> thread().join();

      auto sim = ( *it ) -> sim();

      // Pure iterative time
      m_total_elapsed += sim -> elapsed_time;

      it = m_current_work.erase( it );
    }
    else
    {
      ++it;
    }
  }
}

// Wait until we have all the work done
void profilesets_t::finalize_work()
{
  // Nothing to finalize for sequential profileset model
  if ( m_mode == SEQUENTIAL )
  {
    return;
  }

  while ( !m_current_work.empty() )
  {
    assert( ! m_work_lock.owns_lock() );

    m_work_lock.lock();

    // If we still have active workers around, wait for them to signal their finish
    // TODO: Potential race? (wait vs notify_worker() called from thread)
    if ( n_workers() > 0 )
    {
      m_work.wait( m_work_lock );
    }

    // Aand clean up finished sims
    cleanup_work();

    m_work_lock.unlock();
  }
}

void profilesets_t::generate_work( sim_t* parent, std::unique_ptr<profile_set_t>& ptr_set )
{
  if ( m_mode == SEQUENTIAL )
  {
    auto original_opts = parent -> control;

    parent -> control = ptr_set -> options();

    sim_t* profile_sim = new sim_t( parent );

    parent -> control = original_opts;

    simulate_profileset( parent, *ptr_set, profile_sim );

    delete profile_sim;
  }
  // Parallel processing
  else
  {
    m_work_lock.lock();

    while ( ! is_done() && m_max_workers - n_workers() == 0 )
    {
      m_work.wait( m_work_lock );
    }

    if ( ! is_done() )
    {
      cleanup_work();

      // Output profileset progressbar whenever we finish anything
      output_progressbar( parent );

      m_current_work.push_back( std::make_unique<worker_t>( this, parent, ptr_set.get() ) );
    }

    m_work_lock.unlock();
  }
}

bool profilesets_t::validate( sim_t* ps_sim )
{
  if ( ps_sim -> player_no_pet_list.size() > 1 )
  {
    ps_sim -> errorf( "Profileset simulations must have only one actor in the baseline sim" );
    return false;
  }

  return true;
}

// Ensure profileset options are valid, and also perform basic simulator initialization for the
// profileset to ensure that we can launch it when the time comes
bool profilesets_t::parse( sim_t* sim )
{
  while ( true )
  {
    if ( sim -> canceled )
    {
      set_state( DONE );
      m_control.notify_one();
      return false;
    }

    m_mutex.lock();

    if ( m_init_index == sim -> profileset_map.cend() )
    {
      m_mutex.unlock();
      break;
    }

    const auto& profileset_name = m_init_index -> first;
    const auto& profileset_opts = m_init_index -> second;

    ++m_init_index;

    m_mutex.unlock();

    auto control = create_sim_options( m_original.get(), profileset_opts );
    if ( control == nullptr )
    {
      set_state( DONE );
      m_control.notify_one();
      return false;
    }

    auto has_output_opts = range::any_of( profileset_opts, []( util::string_view opt ) {
      auto name_end = opt.find( "=" );
      if ( name_end == std::string::npos )
      {
        return false;
      }

      auto name = opt.substr( 0, name_end );

      return util::str_compare_ci( name, "output" ) ||
             util::str_compare_ci( name, "html" ) ||
             util::str_compare_ci( name, "json2" );
    } );

    // Test that profileset options are OK, up to the simulation initialization
    try
    {
      std::unique_ptr<sim_t> test_sim = std::make_unique<sim_t>();
      test_sim -> profileset_enabled = true;

      test_sim -> setup( control );
      test_sim -> init();
    }
    catch ( const std::exception& e )
    {
      fmt::print( stderr, "ERROR! Profileset '{}' Setup failure: ", profileset_name );
      util::print_chained_exception( e, stderr );
      fmt::print( stderr, "\n" );
      set_state( DONE );
      m_control.notify_one();
      delete control;
      return false;
    }

    m_mutex.lock();
    m_profilesets.push_back( std::make_unique<profile_set_t>(
        profileset_name, control, has_output_opts ) );
    m_control.notify_one();
    m_mutex.unlock();
  }

  set_state( RUNNING );

  return true;
}

void profilesets_t::initialize( sim_t* sim )
{
  if ( sim -> profileset_enabled || sim -> parent || sim -> thread_index > 0 )
  {
    return;
  }

  if ( sim -> profileset_map.empty() )
  {
    set_state( DONE );
    return;
  }

  if ( ! validate( sim ) )
  {
    set_state( DONE );
    return;
  }

  if ( sim -> profileset_init_threads < 1 )
  {
    sim -> errorf( "No profileset init threads given, profilesets cannot continue" );
    return;
  }

  // Figure out how many workers can we have by looking at how many threads we have, and how many
  // worker threads the user wants
  if ( sim -> profileset_work_threads > 0 )
  {
    size_t workers = as<size_t>( sim -> threads / sim -> profileset_work_threads );
    if ( workers == 0 )
    {
      sim -> errorf( "More worker threads defined than simulator threads, reverting to sequential behavior" );
      sim -> profileset_work_threads = 0;
    }

    m_max_workers = workers;
  }

  // Go parallel mode if we have any workers .. including one
  if ( m_max_workers > 0 )
  {
    m_mode = PARALLEL;
  }

  m_profilesets.reserve( sim -> profileset_map.size() + 1 );

  // Generate a copy of the original control, and remove any and all profileset. options from it
  m_original = std::make_unique<sim_control_t>( );

  // Copy non-profileset. options to use as a base option setup for each profileset
  range::copy_if( sim -> control -> options, std::back_inserter( m_original -> options ),
    []( const option_tuple_t& opt ) {
    return ! util::str_in_str_ci( opt.name, "profileset." );
  } );

  // Spawn initialization threads, and start parsing through the profilesets
  set_state( INITIALIZING );

  m_init_index = sim -> profileset_map.cbegin();

  for ( int i = 0; i < sim -> profileset_init_threads; ++i )
  {
    m_thread.emplace_back([ this, sim ]() {
      if ( ! parse( sim ) )
      {
        sim -> cancel();
      }
    } );
  }
}

void profilesets_t::cancel()
{
  if ( ! is_done() )
  {
    range::for_each( m_thread, []( std::thread& thread ) {
      if ( thread.joinable() )
      {
        thread.join();
      }
    } );
  }

  set_state( DONE );
}

void profilesets_t::set_state( state new_state )
{
  m_mutex.lock();

  m_state = new_state;

  m_mutex.unlock();
}

std::string profilesets_t::current_profileset_name()
{
  m_control_lock.lock();
  if ( is_done() || m_work_index == 0 )
  {
    m_control_lock.unlock();
    return {};
  }

  std::string profileset_name = m_profilesets[ m_work_index - 1 ] -> name();
  m_control_lock.unlock();

  return profileset_name;
}

bool profilesets_t::iterate( sim_t* parent )
{
  if ( parent -> profileset_map.empty() )
  {
    return true;
  }

  auto original_opts = parent -> control;

  m_start_time = chrono::wall_clock::now();

  while ( ! is_done() )
  {
    m_control_lock.lock();

    // Wait until we have at least something to sim
    while ( is_initializing() && m_profilesets.size() - m_work_index == 0 )
    {
      m_control.wait( m_control_lock );
    }

    // Break out of iteration loop if all work has been done
    if ( is_running() )
    {
      if ( m_work_index == m_profilesets.size() )
      {
        m_control_lock.unlock();
        break;
      }
    }

    auto& set = m_profilesets[ m_work_index++ ];

    m_control_lock.unlock();

    generate_work( parent, set );
  }

  // Wait until the tail-end of the parallel work has been done. Non-parallel processing mode will
  // not need to finalize any work (all work has been done by the loop above)
  finalize_work();

  // Output profileset progressbar whenever we finish anything
  output_progressbar( parent );

  // Update parent elapsed_time
  parent -> elapsed_time += chrono::elapsed( m_start_time );

  parent -> control = original_opts;

  set_state( DONE );

  return true;
}

void profilesets_t::notify_worker()
{
  m_work.notify_one();
}

int profilesets_t::max_name_length() const
{
  size_t len = 0;

  range::for_each( m_profilesets, [ &len ]( const profileset_entry_t& profileset ) {
    if ( profileset -> name().size() > len )
    {
      len = profileset -> name().size();
    }
  } );

  return as<int>(len);
}

void profilesets_t::output_progressbar( const sim_t* parent ) const
{
  if ( m_max_workers == 0 )
  {
    return;
  }

  std::stringstream s;

  s << "Profilesets (" << m_max_workers << "*" << parent -> profileset_work_threads << "): ";

  auto done = done_profilesets();
  auto pct = done / as<double>( m_profilesets.size() );

  s << done << "/" << m_profilesets.size() << " ";

  std::string status = "[";
  status.insert( 1, parent -> progress_bar.steps, '.' );
  status += "]";

  int length = as<int>( std::lround( parent -> progress_bar.steps * pct ) );
  for ( int i = 1; i < length + 1; ++i )
  {
    status[ i ] = '=';
  }

  if ( length > 0 )
  {
    status[ length ] = '>';
  }

  s << status;

  auto average_per_sim = chrono::to_fp_seconds(m_total_elapsed) / as<double>( done );
  auto elapsed = chrono::elapsed_fp_seconds( m_start_time );
  auto work_left = m_profilesets.size() - done;
  auto time_left = work_left * ( average_per_sim / m_max_workers );

  // Average time per done simulation
  s << " avg=" << format_time( average_per_sim / as<double>( m_max_workers ) );

  // Elapsed time
  s << " done=" << format_time( elapsed, false );

  // Estimated time left, based on average time per done simulation, elapsed time, and the number of
  // workers
  s << " left=" << format_time( time_left, false );

  // Cleanups
  s << "     ";

  s << '\r';

  std::cout << s.str();
  std::fflush( stdout );
}

std::vector<const profile_set_t*> profilesets_t::generate_sorted_profilesets( bool mean ) const
{
  std::vector<const profile_set_t*> out;
  range::transform( m_profilesets, std::back_inserter( out ), []( const profileset_entry_t& p ) {
    return p.get();
  } );

  // Sort to descending with mean value
  range::sort( out, [mean]( const profile_set_t* l, const profile_set_t* r ) {
    double lv = mean ? l->result().mean() : l->result().median();
    double rv = mean ? r->result().mean() : r->result().median();
    if ( lv == rv )
    {
      return l->name() < r->name();
    }

    return lv > rv;
  } );
  return out;
}

size_t profilesets_t::n_profilesets() const
{
  return m_profilesets.size();
}

const profilesets_t::profileset_vector_t& profilesets_t::profilesets() const
{
  return m_profilesets;
}

bool profilesets_t::is_initializing() const
{
  return m_state == INITIALIZING;
}

bool profilesets_t::is_running() const
{
  return m_state == RUNNING;
}

bool profilesets_t::is_done() const
{
  return m_state == DONE;
}

bool profilesets_t::is_sequential() const
{
  return m_mode == SEQUENTIAL;
}

bool profilesets_t::is_parallel() const
{
  return m_mode == PARALLEL;
}

void create_options( sim_t* sim )
{
  sim -> add_option( opt_map_list( "profileset.", sim -> profileset_map ) );
  sim -> add_option( opt_func( "profileset_metric", []( sim_t*             sim,
                                                        util::string_view,
                                                        util::string_view value ) {
    sim -> profileset_metric.clear();

    auto split = util::string_split<util::string_view>( value, "/:," );
    for ( const auto& v : split )
    {
      auto metric = util::parse_scale_metric( v );
      if ( metric == SCALE_METRIC_NONE )
      {
        sim -> error( "Invalid profileset metric '{}'", v );
        return false;
      }

      sim -> profileset_metric.push_back( metric );
    }

    return true;
  } ) );
  sim -> add_option( opt_func( "profileset_output_data", []( sim_t*             sim,
                                                        util::string_view,
                                                        util::string_view value ) {
    sim -> profileset_output_data.clear();

    for ( auto&& v : util::string_split( value, "/:," ) )
    {
      sim -> profileset_output_data.push_back( std::move( v ) );
    }

    return true;
  } ) );

  sim -> add_option( opt_int( "profileset_work_threads", sim -> profileset_work_threads ) );
  sim -> add_option( opt_int( "profileset_init_threads", sim -> profileset_init_threads ) );
}

statistical_data_t collect( const extended_sample_data_t& c )
{
  return { c.min(), c.percentile( 0.25 ), c.percentile( 0.5 ), c.mean(),
           c.percentile( 0.75 ), c.max(), c.std_dev, c.mean_std_dev };
}

statistical_data_t metric_data( const player_t* player, scale_metric_e metric )
{
  const auto& d = player -> collected_data;

  switch ( metric )
  {
    case SCALE_METRIC_DPS:       return collect( d.dps );
    case SCALE_METRIC_DPSE:      return collect( d.dpse );
    case SCALE_METRIC_HPS:       return collect( d.hps );
    case SCALE_METRIC_HPSE:      return collect( d.hpse );
    case SCALE_METRIC_APS:       return collect( d.aps );
    case SCALE_METRIC_DPSP:      return collect( d.prioritydps );
    case SCALE_METRIC_DTPS:      return collect( d.dtps );
    case SCALE_METRIC_DMG_TAKEN: return collect( d.dmg_taken );
    case SCALE_METRIC_HTPS:      return collect( d.htps );
    case SCALE_METRIC_TMI:       return collect( d.theck_meloree_index );
    case SCALE_METRIC_ETMI:      return collect( d.effective_theck_meloree_index );
    case SCALE_METRIC_DEATHS:    return collect( d.deaths );
    case SCALE_METRIC_TIME:      return collect( d.fight_length );
    case SCALE_METRIC_HAPS:
    {
      auto hps = collect( d.hps );
      auto aps = collect( d.aps );
      return {
        hps.min + aps.min,
        hps.first_quartile + aps.first_quartile,
        hps.median + aps.median,
        hps.mean + aps.mean,
        hps.third_quartile + aps.first_quartile,
        hps.max + aps.max,
        sqrt( d.aps.variance + d.hps.variance ),
        sqrt( d.aps.mean_variance + d.hps.mean_variance )
      };
    }
    default:                     return { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
  }
}

void save_output_data( profile_set_t& profileset, const player_t* parent_player, const player_t* player, const std::string& option )
{
  // TODO: Make an enum to proper use a switch instead of if/else
  if ( option == "race") {
    if ( parent_player -> race != player -> race )
    {
      profileset.output_data().race( player -> race );
    }
  } else if ( option == "talents" ) {
    if ( parent_player -> talents_str != player -> talents_str ) {
      std::vector<const talent_data_t*> saved_talents;
      for ( auto talent_row = 0; talent_row < MAX_TALENT_ROWS; talent_row++ )
      {
        const auto& talent_col = player -> talent_points->choice( talent_row );
        if ( talent_col == -1 )
        {
          continue;
        }

        auto* talent_data = talent_data_t::find( player -> type, talent_row, talent_col, player -> specialization(), player -> dbc->ptr );
        if ( talent_data == nullptr )
        {
          continue;
        }

        const auto& p_talent_col = parent_player -> talent_points->choice( talent_row );
        auto* p_talent_data = talent_data_t::find( parent_player -> type, talent_row, talent_col, parent_player -> specialization(), parent_player -> dbc->ptr );
        if ( p_talent_col == -1 || p_talent_data == nullptr || p_talent_col != talent_col )
        {
          saved_talents.push_back( talent_data );
        }
      }
      if ( !saved_talents.empty() )
      {
        profileset.output_data().talents( saved_talents );
      }
    }
  } else if ( option == "gear" ) {
    const auto& parent_items = parent_player -> items;
    const auto& items = player -> items;
    std::vector<profile_output_data_item_t> saved_gear;
    for ( slot_e slot = SLOT_MIN; slot < SLOT_MAX; slot++ )
    {
      const auto& item = items[ slot ];
      if ( ! item.active() || ! item.has_stats() )
      {
        continue;
      }
      const auto& parent_item = parent_items[ slot ];
      if ( parent_item.parsed.data.id != item.parsed.data.id ) {
        profile_output_data_item_t saved_item {
          item.slot_name(), item.parsed.data.id, item.item_level()
        };

        // saved_item.bonus_id( item.parsed.bonus_id );

        saved_gear.push_back( saved_item );
      }
    }
    if ( !saved_gear.empty() )
    {
      profileset.output_data().gear( saved_gear );
    }
  } else if ( option == "stats" ) {
    const auto& buffed_stats = player -> collected_data.buffed_stats_snapshot;

    // primary stats

    profileset.output_data().stamina( util::floor( buffed_stats.attribute[ ATTR_STAMINA ] ) );
    profileset.output_data().agility( util::floor( buffed_stats.attribute[ ATTR_AGILITY ] ) );
    profileset.output_data().intellect( util::floor( buffed_stats.attribute[ ATTR_INTELLECT ] ) );
    profileset.output_data().strength( util::floor( buffed_stats.attribute[ ATTR_STRENGTH ] ) );

    // secondary stats

    profileset.output_data().crit_rating( util::floor( player -> composite_melee_crit_rating() > player -> composite_spell_crit_rating()
                       ? player -> composite_melee_crit_rating()
                       : player -> composite_spell_crit_rating() ) );
    profileset.output_data().crit_pct( buffed_stats.attack_crit_chance > buffed_stats.spell_crit_chance
                       ? buffed_stats.attack_crit_chance
                       : buffed_stats.spell_crit_chance );

    profileset.output_data().haste_rating( util::floor( player -> composite_melee_haste_rating() > player -> composite_spell_haste_rating()
                       ? player -> composite_melee_haste_rating()
                       : player -> composite_spell_haste_rating() ) );

    double attack_haste_pct = 1 / buffed_stats.attack_haste - 1;
    double spell_haste_pct = 1 / buffed_stats.spell_haste - 1;
    profileset.output_data().haste_pct( attack_haste_pct > spell_haste_pct ? attack_haste_pct : spell_haste_pct );

    profileset.output_data().mastery_rating( util::floor( player -> composite_mastery_rating() ) );
    profileset.output_data().mastery_pct( buffed_stats.mastery_value );

    profileset.output_data().versatility_rating( util::floor( player -> composite_damage_versatility_rating() ) );
    profileset.output_data().versatility_pct( buffed_stats.damage_versatility );

    // tertiary stats

    profileset.output_data().avoidance_rating( player -> composite_avoidance_rating() );
    profileset.output_data().avoidance_pct( buffed_stats.avoidance );
    profileset.output_data().leech_rating( player -> composite_leech_rating() );
    profileset.output_data().leech_pct( buffed_stats.leech );
    profileset.output_data().speed_rating( player -> composite_spell_haste_rating() );
    profileset.output_data().speed_pct( buffed_stats.run_speed );

    profileset.output_data().corruption( buffed_stats.corruption );
    profileset.output_data().corruption_resistance( buffed_stats.corruption_resistance );
  }
}

sim_control_t* filter_control( const sim_control_t* control )
{
  if ( control == nullptr )
  {
    return nullptr;
  }

  auto clone = new sim_control_t();

  for ( size_t i = 0, end = control -> options.size(); i < end; ++i )
  {
    auto pos = control -> options[ i ].name.find( "profileset" );
    if ( pos != 0 )
    {
      clone -> options.add( control -> options[ i ].scope,
                            control -> options[ i ].name,
                            control -> options[ i ].value );
    }
  }

  return clone;
}

} /* Namespace profileset ends */

#else

namespace profileset
{
profilesets_t::profilesets_t() {}
profilesets_t::~profilesets_t() {}
void create_options( sim_t* ) {}
sim_control_t* filter_control( const sim_control_t* ) { return nullptr; }
void profilesets_t::initialize( sim_t* ) {}
std::string profilesets_t::current_profileset_name() { return "DUMMY"; }
void profilesets_t::cancel() {}
bool profilesets_t::iterate( sim_t*  ) { return true ;}
void profilesets_t::output_html( const sim_t&, std::ostream& ) const {}
void profilesets_t::output_text( const sim_t&, std::ostream& ) const {}
size_t profilesets_t::n_profilesets() const { return 0; }
bool profilesets_t::is_running() const { return false; }
}

#endif
