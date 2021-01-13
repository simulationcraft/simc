// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "report_configuration.hpp"

#include <stdexcept>  // included because cpp-semver seems to lack this internal dependency
#include <utility>


// GNU C library defines macros major & minor in <sys/types.h>
// While it does not seem to mess with the use of these names in cpp-semver both gcc and
// clang complain with a warning. This seems to shut up them both.
#ifdef minor
#  undef minor
#endif
#ifdef major
#  undef major
#endif

#include "lib/cpp-semver/cpp-semver.hpp"
#include "sim/sc_sim.hpp"

namespace
{
struct report_version_settings_t
{
  std::string version;
  bool is_deprecated;
};

std::vector<report_version_settings_t> get_report_settings()
{
  return std::vector<report_version_settings_t>{ { "3.0.0-alpha1", false }, { "2.0.0", false } };
}
}  // namespace

namespace report
{
namespace json
{
report_configuration_t::report_configuration_t( std::string version, std::string destination )
  : _version( std::move( version ) ),
    _destination( std::move( destination ) ),
    full_states( false ),
    pretty_print( false ),
    decimal_places( 0 )
{
}

bool report_configuration_t::version_intersects( const std::string& version ) const
{
  return semver::intersects( _version, version );
}

const std::string& report_configuration_t::version() const
{
  return _version;
}

const std::string& report_configuration_t::destination() const
{
  return _destination;
}

report_configuration_t create_report_entry( sim_t& sim, std::string version, std::string destination )
{
  // valid reports, ordered from current to oldest
  auto valid_report_versions = get_report_settings();
  auto max_report            = valid_report_versions.front();
  std::vector<std::string> available_non_deprecated_versions;
  for ( const auto& entry : valid_report_versions )
  {
    if ( !entry.is_deprecated )
    {
      available_non_deprecated_versions.push_back( entry.version );
    }
  }
  auto available_non_deprecated_version_string = util::string_join( available_non_deprecated_versions, " / " );

  if ( version.empty() )
  {
    // if no version is specified, use current max version available.
    version = valid_report_versions.front().version;
  }

  if ( !semver::valid( version ) )
  {
    throw std::invalid_argument(
        fmt::format( "Specified JSON report version {} is not a valid semver version.", version ) );
  }
  auto parsed_version = semver::parse_simple( version );

  auto it = range::find_if( valid_report_versions, [ &version ]( const auto& entry ) {
    return semver::intersects( entry.version, version );
  } );
  if ( it == valid_report_versions.end() )
  {
    throw std::invalid_argument(
        fmt::format( "Cannot generate JSON report with version {}. Available non-deprecated versions: {}.", version,
                     available_non_deprecated_version_string ) );
  }
  auto selected_entry = *it;
  if ( selected_entry.is_deprecated )
  {
    sim.error( "JSON Report with version {} ({} requested) is deprecated. Available non-deprecated versions: {}.",
               selected_entry.version, version, available_non_deprecated_version_string );
  }

  return report_configuration_t( selected_entry.version, std::move(destination) );
}

}  // namespace json
}  // namespace report