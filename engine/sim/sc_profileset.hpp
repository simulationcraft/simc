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
  double         m_variance;

public:
  profile_result_t() :
    m_metric_type( SCALE_METRIC_NONE ), m_metric( 0 ), m_variance( 0 )
  { }

  profile_result_t( scale_metric_e metric_type, double metric, double variance ) :
    m_metric_type( metric_type ), m_metric( metric ), m_variance( variance )
  { }

  scale_metric_e metric_type() const
  { return m_metric_type; }

  double metric() const
  { return m_metric; }

  double variance() const
  { return m_variance; }
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

  void set_result( const profile_result_t& result )
  { m_result = result; }

  static sim_control_t* create_sim_options( const sim_control_t*,
      const std::vector<std::string>& opts );
};


bool parse_profilesets( sim_t* sim );
bool iterate_profilesets( sim_t* sim );
bool validate_profileset( sim_t* profileset_sim );

void create_options( sim_t* sim );

} /* Namespace profileset ends */

#endif /* SC_PROFILESET_HH */
