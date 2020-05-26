// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "report_configuration.hpp"

#include "sim/sc_sim.hpp"

namespace
{
struct report_version_settings_t
{
  int minimum_version;
  int minimum_non_deprecated_version;
  int maximum_version;
};

constexpr report_version_settings_t get_report_settings()
{
  return report_version_settings_t{ 2, 2, 3 };
}
}  // namespace

namespace report
{
namespace json
{
report_configuration_t::report_configuration_t( int version, std::string destination )
  : _version( version ), _destination( std::move( destination ) )
{
}

bool report_configuration_t::is_version_greater_than( int min_version ) const
{
  return _version > min_version;
}

bool report_configuration_t::is_version_between( int min_version, int max_version ) const
{
  return _version > min_version && _version < max_version;
}

bool report_configuration_t::is_version_less_than( int max_version ) const
{
  return _version < max_version;
}

int report_configuration_t::version() const
{
  return _version;
}

std::string report_configuration_t::destination() const
{
  return _destination;
}

report_configuration_t create_report_entry( sim_t& sim, int version, std::string destination )
{
  auto settings = get_report_settings();
  if (version <= 0 )
  {
    // if no version > 0 is specified, use current max version available.
    version = settings.maximum_version;
  }

  if ( version < settings.minimum_version )
  {
    throw std::invalid_argument(
        fmt::format( "Cannot generate JSON report with version {}, which is less than the supported minimum version {}",
                     version, settings.minimum_version ) );
  }
  if ( version < settings.minimum_non_deprecated_version )
  {
    sim.error( "JSON Report with version {} is deprecated. Minimum non-deprecated version is {}.", version,
               settings.minimum_non_deprecated_version );
  }
  if ( version > settings.maximum_version )
  {
    throw std::invalid_argument(
        fmt::format( "Cannot generate JSON report with version {}, which greater than the supported maximum version {}",
                     version, settings.maximum_version ) );
  }

  return report_configuration_t( version, destination );
}

}  // namespace json
}  // namespace report