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

namespace profileset
{
class profile_result_t
{
  scale_metric_e m_metric_type;
  double         m_metric;
  double         m_stddev;
  size_t         m_iterations;

public:
  profile_result_t() :
    m_metric_type( SCALE_METRIC_NONE ), m_metric( 0 ), m_stddev( 0 ), m_iterations( 0 )
  { }

  profile_result_t( scale_metric_e type ) :
    m_metric_type( type ), m_metric( 0 ), m_stddev( 0 )
  { }

  profile_result_t( scale_metric_e metric_type, double metric, double stddev ) :
    m_metric_type( metric_type ), m_metric( metric ), m_stddev( stddev )
  { }

  scale_metric_e metric_type() const
  { return m_metric_type; }

  profile_result_t& metric_type( scale_metric_e t )
  { m_metric_type = t; return *this; }

  double metric() const
  { return m_metric; }

  profile_result_t& metric( double m )
  { m_metric = m; return *this; }

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

public:
  profile_set_t( const std::string& name, sim_control_t* opts );

  ~profile_set_t();

  const std::string& name() const
  { return m_name; }

  sim_control_t* options() const;

  const profile_result_t& result() const
  { return m_result; }

  profile_result_t& result()
  { return m_result; }

  void set_result( const profile_result_t& result )
  { m_result = result; }

  static sim_control_t* create_sim_options( const sim_control_t*,
      const std::vector<std::string>& opts );
};

using profileset_vector_t = std::vector<std::unique_ptr<profile_set_t>>;

class profilesets_t
{
  profileset_vector_t m_profilesets;

public:
  profilesets_t()
  { }
};

bool parse_profilesets( sim_t* sim );
bool iterate_profilesets( sim_t* sim );
bool validate_profileset( sim_t* profileset_sim );

void create_options( sim_t* sim );

} /* Namespace profileset ends */

#endif /* SC_PROFILESET_HH */
