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

namespace js {
struct JsonOutput;
}

namespace profileset
{
class profile_result_t
{
  double         m_metric;
  double         m_stddev;
  size_t         m_iterations;

public:
  profile_result_t() : m_metric( 0 ), m_stddev( 0 ), m_iterations( 0 )
  { }

  profile_result_t( double metric, double stddev ) :
    m_metric( metric ), m_stddev( stddev )
  { }

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

  void done();

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


class profilesets_t
{
  using profileset_entry_t = std::unique_ptr<profile_set_t>;
  using profileset_vector_t = std::vector<profileset_entry_t>;

  profileset_vector_t m_profilesets;

  bool validate( sim_t* sim );

public:
  profilesets_t()
  { }

  size_t n_profilesets() const
  { return m_profilesets.size(); }

  bool parse( sim_t* );
  bool iterate( sim_t* parent_sim );

  void output( const sim_t& sim, js::JsonOutput& root ) const;
};

void create_options( sim_t* sim );

} /* Namespace profileset ends */

#endif /* SC_PROFILESET_HH */
