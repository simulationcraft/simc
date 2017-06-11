// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_PROFILESET_HH
#define SC_PROFILESET_HH

#include <vector>
#include <string>

#include "util/generic.hpp"
#include "sc_enums.hpp"

struct sim_t;
struct sim_control_t;
struct player_t;
class extended_sample_data_t;

namespace js {
struct JsonOutput;
}

namespace profileset
{
class profile_result_t
{
  double         m_mean;
  double         m_median;
  double         m_min;
  double         m_max;
  double         m_1stquartile;
  double         m_3rdquartile;
  double         m_stddev;
  size_t         m_iterations;

public:
  profile_result_t() : m_mean( 0 ), m_median( 0 ), m_min( 0 ), m_max( 0 ), m_1stquartile( 0 ),
    m_3rdquartile( 0 ), m_stddev( 0 ), m_iterations( 0 )
  { }

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

  profile_result_t& stddev( double v )
  { m_stddev = v; return *this; }

  size_t iterations() const
  { return m_iterations; }

  profile_result_t& iterations( size_t i )
  { m_iterations = i; return *this; }
};

class profile_set_t
{
  std::string      m_name;
  sim_control_t*   m_options;
  profile_result_t m_result;
  bool             m_has_output;

public:
  profile_set_t( const std::string& name, sim_control_t* opts, bool has_ouput );

  ~profile_set_t();

  void done();

  const std::string& name() const
  { return m_name; }

  sim_control_t* options() const;

  const profile_result_t& result() const
  { return m_result; }

  profile_result_t& result()
  { return m_result; }

  bool has_output() const
  { return m_has_output; }

  static sim_control_t* create_sim_options( const sim_control_t*,
      const std::vector<std::string>& opts );
};


class profilesets_t
{
  using profileset_entry_t = std::unique_ptr<profile_set_t>;
  using profileset_vector_t = std::vector<profileset_entry_t>;

  profileset_vector_t m_profilesets;

  bool validate( sim_t* sim );

  int max_name_length() const;

public:
  profilesets_t()
  { }

  size_t n_profilesets() const
  { return m_profilesets.size(); }

  bool parse( sim_t* );
  bool iterate( sim_t* parent_sim );

  void output( const sim_t& sim, js::JsonOutput& root ) const;
  void output( const sim_t& sim, FILE* out ) const;
};

struct statistical_data_t
{
  double min, first_quartile, median, mean, third_quartile, max, std_dev;
};

void create_options( sim_t* sim );

statistical_data_t collect( const extended_sample_data_t& c );
statistical_data_t metric_data( const player_t* player );

} /* Namespace profileset ends */

#endif /* SC_PROFILESET_HH */
