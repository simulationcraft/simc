// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_PROFILESET_HH
#define SC_PROFILESET_HH

#include <array>
#include <vector>
#include <string>

#ifndef SC_NO_THREADING
#include <thread>
#include <mutex>
#include <condition_variable>
#endif

#include "sc_option.hpp"
#include "util/chrono.hpp"
#include "util/generic.hpp"
#include "sc_enums.hpp"

struct sim_t;
struct sim_control_t;
struct player_t;
class extended_sample_data_t;
struct talent_data_t;

namespace js {
struct JsonOutput;
}

namespace report {
  class report_configuration_t;
}

namespace profileset
{
class profilesets_t;

#ifdef SC_NO_THREADING
class profile_set_t;
class profile_output_data_t;
struct statistical_data_t;
#endif

#ifndef SC_NO_THREADING
struct statistical_data_t
{
  double min, first_quartile, median, mean, third_quartile, max, std_dev, mean_std_dev;
};

class profile_result_t
{
  scale_metric_e m_metric;
  double         m_mean;
  double         m_median;
  double         m_min;
  double         m_max;
  double         m_1stquartile;
  double         m_3rdquartile;
  double         m_stddev;
  double         m_mean_stddev;
  size_t         m_iterations;

public:
  profile_result_t() : m_metric( SCALE_METRIC_NONE ), m_mean( 0 ), m_median( 0 ), m_min( 0 ),
    m_max( 0 ), m_1stquartile( 0 ), m_3rdquartile( 0 ), m_stddev( 0 ), m_mean_stddev(0), m_iterations( 0 )
  { }

  profile_result_t( scale_metric_e m ) : m_metric( m ), m_mean( 0 ), m_median( 0 ), m_min( 0 ),
    m_max( 0 ), m_1stquartile( 0 ), m_3rdquartile( 0 ), m_stddev( 0 ), m_mean_stddev(0), m_iterations( 0 )
  { }

  scale_metric_e metric() const
  { return m_metric; }

  double mean() const
  { return m_mean; }

  profile_result_t& mean( double v )
  { m_mean = v; return *this; }

  double median() const
  { return m_median; }

  profile_result_t& median( double v )
  { m_median = v; return *this; }

  double min() const
  { return m_min; }

  profile_result_t& min( double v )
  { m_min = v; return *this; }

  double max() const
  { return m_max; }

  profile_result_t& max( double v )
  { m_max = v; return *this; }

  double first_quartile() const
  { return m_1stquartile; }

  profile_result_t& first_quartile( double v )
  { m_1stquartile = v; return *this; }

  double third_quartile() const
  { return m_3rdquartile; }

  profile_result_t& third_quartile( double v )
  { m_3rdquartile = v; return *this; }

  double stddev() const
  { return m_stddev; }

  double mean_stddev() const
  {
    return m_mean_stddev;
  }

  profile_result_t& stddev( double v )
  { m_stddev = v; return *this; }

  profile_result_t& mean_stddev(double v)
  {
    m_mean_stddev = v; return *this;
  }

  size_t iterations() const
  { return m_iterations; }

  profile_result_t& iterations( size_t i )
  { m_iterations = i; return *this; }

  statistical_data_t statistical_data() const
  { return { m_min, m_1stquartile, m_median, m_mean, m_3rdquartile, m_max, m_stddev, m_mean_stddev }; }
};

class profile_output_data_item_t
{
  const char*                                      m_slot_name;
  unsigned                                         m_item_id;
  unsigned                                         m_item_level;
  std::vector<int>                                 m_bonus_id;
  unsigned                                         m_enchant_id;
  std::array<int, MAX_GEM_SLOTS>                   m_gem_id;
  std::array<std::vector<unsigned>, MAX_GEM_SLOTS> m_relic_data;
  std::array<unsigned, MAX_GEM_SLOTS>              m_relic_ilevel;
  std::array<unsigned, MAX_GEM_SLOTS>              m_relic_bonus_ilevel;

public:
  profile_output_data_item_t() : m_slot_name( nullptr ), m_item_id( 0 ), m_item_level( 0 ), m_enchant_id( 0 )
  { }

  profile_output_data_item_t( const char* slot_str, unsigned id, unsigned item_level ) :
    m_slot_name( slot_str ), m_item_id( id ), m_item_level( item_level )
  { }

  const char* slot_name() const
  { return m_slot_name; }

  profile_output_data_item_t& slot_name( const char* v )
  { m_slot_name = v; return *this; }

  unsigned item_id() const
  { return m_item_id; }

  profile_output_data_item_t& item_id( unsigned v )
  { m_item_id = v; return *this; }

  unsigned item_level() const
  { return m_item_level; }

  profile_output_data_item_t& item_level( unsigned v )
  { m_item_level = v; return *this; }

  const std::vector<int>& bonus_id() const
  { return m_bonus_id; }

  profile_output_data_item_t& bonus_id( const std::vector<int>& v )
  { m_bonus_id = v; return *this; }

  unsigned enchant_id() const
  { return m_enchant_id; }

  profile_output_data_item_t& enchant_id( unsigned v )
  { m_enchant_id = v; return *this; }

  const std::array<int, MAX_GEM_SLOTS>& gem_id() const
  { return m_gem_id; }

  profile_output_data_item_t& gem_id( const std::array<int, MAX_GEM_SLOTS>& v )
  { m_gem_id = v; return *this; }

  const std::array<std::vector<unsigned>, MAX_GEM_SLOTS>& relic_data() const
  { return m_relic_data; }

  profile_output_data_item_t& relic_data( const std::array<std::vector<unsigned>, MAX_GEM_SLOTS>& v )
  { m_relic_data = v; return *this; }

  const std::array<unsigned, MAX_GEM_SLOTS>& relic_ilevel() const
  { return m_relic_ilevel; }

  profile_output_data_item_t& relic_ilevel( const std::array<unsigned, MAX_GEM_SLOTS>& v )
  { m_relic_ilevel = v; return *this; }

  const std::array<unsigned, MAX_GEM_SLOTS>& relic_bonus_ilevel() const
  { return m_relic_bonus_ilevel; }

  profile_output_data_item_t& relic_bonus_ilevel( const std::array<unsigned, MAX_GEM_SLOTS>& v )
  { m_relic_bonus_ilevel = v; return *this; }
};

class profile_output_data_t
{
  race_e                                       m_race;
  std::vector<const talent_data_t*>            m_talents;
  std::vector<profile_output_data_item_t>      m_gear;

  double    m_crit_rating,
            m_crit_pct,
            m_haste_rating,
            m_haste_pct,
            m_mastery_rating,
            m_mastery_pct,
            m_versatility_rating,
            m_versatility_pct,
            m_agility,
            m_strength,
            m_intellect,
            m_stamina,
            m_avoidance_rating,
            m_avoidance_pct,
            m_leech_rating,
            m_leech_pct,
            m_speed_rating,
            m_speed_pct,
            m_corruption,
            m_corruption_resistance;

public:
  profile_output_data_t() : m_race ( RACE_NONE )
  { }

  race_e race() const
  { return m_race; }

  profile_output_data_t& race( race_e v )
  { m_race = v; return *this; }

  const std::vector<const talent_data_t*>& talents() const
  { return m_talents; }

  profile_output_data_t& talents( const std::vector<const talent_data_t*>& v )
  { m_talents = v; return *this; }

  const std::vector<profile_output_data_item_t>& gear() const
  { return m_gear; }

  profile_output_data_t& gear( const std::vector<profile_output_data_item_t>& v )
  { m_gear = v; return *this; }

  double crit_rating() const
  { return m_crit_rating; }

  profile_output_data_t& crit_rating( double d )
  { m_crit_rating = d; return *this; }

  double haste_rating() const
  { return m_haste_rating; }

  profile_output_data_t& haste_rating( double d )
  { m_haste_rating = d; return *this; }

  double mastery_rating() const
  { return m_mastery_rating; }

  profile_output_data_t& mastery_rating( double d )
  { m_mastery_rating = d; return *this; }

  double versatility_rating() const
  { return m_versatility_rating; }

  profile_output_data_t& versatility_rating( double d )
  { m_versatility_rating = d; return *this; }

  double crit_pct() const
  { return m_crit_pct; }

  profile_output_data_t& crit_pct( double d )
  { m_crit_pct = d; return *this; }

  double haste_pct() const
  { return m_haste_pct; }

  profile_output_data_t& haste_pct( double d )
  { m_haste_pct = d; return *this; }

  double mastery_pct() const
  { return m_mastery_pct; }

  profile_output_data_t& mastery_pct( double d )
  { m_mastery_pct = d; return *this; }

  double versatility_pct() const
  { return m_versatility_pct; }

  profile_output_data_t& versatility_pct( double d )
  { m_versatility_pct = d; return *this; }

  double agility() const
  { return m_agility; }

  profile_output_data_t& agility( double d )
  { m_agility = d; return *this; }

  double strength() const
  { return m_strength; }

  profile_output_data_t& strength( double d )
  { m_strength = d; return *this; }

  double intellect() const
  { return m_intellect; }

  profile_output_data_t& intellect( double d )
  { m_intellect = d; return *this; }

  double stamina() const
  { return m_stamina; }

  profile_output_data_t& stamina( double d )
  { m_stamina = d; return *this; }

  double avoidance_rating() const
  { return m_avoidance_rating; }

  profile_output_data_t& avoidance_rating( double d )
  { m_avoidance_rating = d; return *this; }

  double avoidance_pct() const
  { return m_avoidance_pct; }

  profile_output_data_t& avoidance_pct( double d )
  { m_avoidance_pct = d; return *this; }

  double leech_rating() const
  { return m_leech_rating; }

  profile_output_data_t& leech_rating( double d )
  { m_leech_rating = d; return *this; }

  double leech_pct() const
  { return m_leech_pct; }

  profile_output_data_t& leech_pct( double d )
  { m_leech_pct = d; return *this; }

  double speed_rating() const
  { return m_speed_rating; }

  profile_output_data_t& speed_rating( double d )
  { m_speed_rating = d; return *this; }

  double speed_pct() const
  { return m_speed_pct; }

  profile_output_data_t& speed_pct( double d )
  { m_speed_pct = d; return *this; }

  double corruption() const
  { return m_corruption; }

  profile_output_data_t& corruption( double d )
  { m_corruption = d; return *this; }

  double corruption_resistance() const
  { return m_corruption_resistance; }

  profile_output_data_t& corruption_resistance( double d )
  { m_corruption_resistance = d; return *this; }
};

class profile_set_t
{
  std::string                            m_name;
  sim_control_t*                         m_options;
  bool                                   m_has_output;
  std::vector<profile_result_t>          m_results;
  std::unique_ptr<profile_output_data_t> m_output_data;

public:
  profile_set_t( const std::string& name, sim_control_t* opts, bool has_output );

  ~profile_set_t();

  void cleanup_options();

  const std::string& name() const
  { return m_name; }

  sim_control_t* options() const;

  bool has_output() const
  { return m_has_output; }

  const profile_result_t& result( scale_metric_e metric = SCALE_METRIC_NONE ) const;

  profile_result_t& result( scale_metric_e metric );

  size_t results() const
  { return m_results.size(); }

  profile_output_data_t& output_data()
  {
    if ( ! m_output_data )
    {
      m_output_data = std::unique_ptr<profile_output_data_t>( new profile_output_data_t() );
    }

    return *m_output_data;
  }
};

class worker_t
{
  bool           m_done;
  sim_t*         m_parent;
  profilesets_t* m_master;

  sim_t*         m_sim;
  profile_set_t* m_profileset;
  std::thread*   m_thread;

public:
  worker_t( profilesets_t*, sim_t*, profile_set_t* );
  ~worker_t();

  const std::thread& thread() const;
  std::thread& thread();
  void execute();

  bool is_done() const
  { return m_done == true; }

  sim_t* sim() const;
};
#endif

class profilesets_t
{
public:
  
  using profileset_entry_t = std::unique_ptr<profile_set_t>;
  using profileset_vector_t = std::vector<profileset_entry_t>;
private:
  enum state
  {
    STARTED,            // Initial state
    INITIALIZING,       // Initializing/constructing profile sets
    RUNNING,            // Finished initializing, running through profilesets
    DONE                // Finished profileset iterating
  };
#ifndef SC_NO_THREADING
  enum simulation_mode
  {
    SEQUENTIAL = 0,
    PARALLEL
  };

  static const size_t MAX_CHART_ENTRIES = 500;

  state                                  m_state;
  simulation_mode                        m_mode;
  profileset_vector_t                    m_profilesets;
  std::unique_ptr<sim_control_t>         m_original;
  int64_t                                m_insert_index;
  size_t                                 m_work_index;
  std::mutex                             m_mutex;
  std::unique_lock<std::mutex>           m_control_lock;
  std::condition_variable                m_control;
  std::vector<std::thread>               m_thread;

  // Shared iterator for threaded init workers
  opts::map_list_t::const_iterator       m_init_index;

  // Parallel profileset worker information
  size_t                                 m_max_workers;
  std::vector<std::unique_ptr<worker_t>> m_current_work;

  // Parallel profileset worker control
  std::mutex                             m_work_mutex;
  std::unique_lock<std::mutex>           m_work_lock;
  std::condition_variable                m_work;

  // Parallel profileset stats collection
  chrono::wall_clock::time_point         m_start_time;
  chrono::wall_clock::duration           m_total_elapsed;
#endif

  bool validate( sim_t* sim );

  int max_name_length() const;

  bool generate_chart( const sim_t& sim, std::ostream& out ) const;
  void generate_sorted_profilesets( std::vector<const profile_set_t*>& out, bool mean = false ) const;

  void output_progressbar( const sim_t* ) const;

  void set_state( state new_state );

  size_t n_workers() const;
  void generate_work( sim_t*, std::unique_ptr<profile_set_t>& );
  void cleanup_work();
  void finalize_work();

  sim_control_t* create_sim_options( const sim_control_t*, const std::vector<std::string>& opts );
public:
  profilesets_t();

  ~profilesets_t();

  size_t n_profilesets() const;

  const profileset_vector_t& profilesets() const;

  size_t done_profilesets() const;

  // Worker sim finished
  void notify_worker();

  std::string current_profileset_name();

  bool parse( sim_t* );
  void initialize( sim_t* );
  void cancel();
  bool iterate( sim_t* parent_sim );

  void output_text( const sim_t& sim, std::ostream& out ) const;
  void output_html( const sim_t& sim, std::ostream& out ) const;

  bool is_initializing() const;

  bool is_running() const;

  bool is_done() const;

  bool is_sequential() const;

  bool is_parallel() const;
};

void create_options( sim_t* sim );

statistical_data_t collect( const extended_sample_data_t& c );
statistical_data_t metric_data( const player_t* player, scale_metric_e metric );
void save_output_data( profile_set_t& profileset, const player_t* parent_player, const player_t* player, const std::string& option );
void fetch_output_data( const profile_output_data_t& output_data, js::JsonOutput& ovr );

// Filter non-profilest options into a new control object, caller is responsible for deleting the
// newly created control object.
sim_control_t* filter_control( const sim_control_t* );
} /* Namespace profileset ends */

#endif /* SC_PROFILESET_HH */
