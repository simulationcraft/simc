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
  int _version;
  std::string _destination;

public:
  report_configuration_t( int version, std::string destination );

  bool is_version_greater_than( int min_version ) const;
  bool is_version_between( int min_version, int max_version ) const;
  bool is_version_less_than( int max_version ) const;

  int version() const;
  std::string destination() const;
};

report_configuration_t create_report_entry( sim_t& sim, int version, std::string destination );
}  // namespace json
}  // namespace report