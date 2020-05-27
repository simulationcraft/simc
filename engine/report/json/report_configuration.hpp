// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <string>

struct sim_t;

namespace report
{
namespace json
{
class report_configuration_t
{
  std::string _version;
  std::string _destination;

public:
  bool full_states;
  int decimal_places;

  report_configuration_t( std::string version, std::string destination );

  bool version_intersects( const std::string& version ) const;

  const std::string& version() const;
  const std::string& destination() const;
};

report_configuration_t create_report_entry( sim_t& sim, std::string version, std::string destination );
}  // namespace json
}  // namespace report