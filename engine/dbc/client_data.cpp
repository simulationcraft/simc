#include "config.hpp"

#include "util/util.hpp"

#include "client_data.hpp"

#include "generated/client_data_version.inc"
#if SC_USE_PTR == 1
#include "generated/client_data_version_ptr.inc"
#endif

std::string dbc::client_data_version_str( bool ptr )
{
#if SC_USE_PTR == 1
  return ptr ? PTR_CLIENT_DATA_WOW_VERSION : CLIENT_DATA_WOW_VERSION;
#else
  (void) ptr;
  return CLIENT_DATA_WOW_VERSION;
#endif /* SC_USE_PTR */
}

std::string dbc::hotfix_date_str( bool ptr )
{
#if SC_USE_PTR == 1
  return ptr ? PTR_CLIENT_DATA_HOTFIX_DATE : CLIENT_DATA_HOTFIX_DATE;
#else
  (void) ptr;
  return CLIENT_DATA_HOTFIX_DATE;
#endif /* SC_USE_PTR */
}

int dbc::hotfix_build_version( bool ptr )
{
#if SC_USE_PTR == 1
  return ptr ? PTR_CLIENT_DATA_HOTFIX_BUILD : CLIENT_DATA_HOTFIX_BUILD;
#else
  (void) ptr;
  return CLIENT_DATA_HOTFIX_BUILD;
#endif /* SC_USE_PTR */
}

std::string dbc::hotfix_hash_str( bool ptr )
{
#if SC_USE_PTR == 1
  return ptr ? PTR_CLIENT_DATA_HOTFIX_HASH : CLIENT_DATA_HOTFIX_HASH;
#else
  (void) ptr;
  return CLIENT_DATA_HOTFIX_HASH;
#endif /* SC_USE_PTR */
}

int dbc::client_data_build( bool ptr )
{
#if SC_USE_PTR == 1
  std::string __version = ptr ? PTR_CLIENT_DATA_WOW_VERSION : CLIENT_DATA_WOW_VERSION;
#else
  (void) ptr;
  std::string __version = CLIENT_DATA_WOW_VERSION;
#endif /* SC_USE_PTR */

  auto pos = __version.rfind( '.' );
  if ( pos == std::string::npos )
  {
    return -1;
  }

  return util::to_int( __version.substr( pos + 1 ) );
}
