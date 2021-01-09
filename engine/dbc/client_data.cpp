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
  if ( SC_USE_PTR && ptr )
  {
#ifdef PTR_CLIENT_DATA_HOTFIX_DATE
    return PTR_CLIENT_DATA_HOTFIX_DATE;
#else
    return {};
#endif /* PTR_CLIENT_DATA_HOTFIX_DATE */
  }

#ifdef CLIENT_DATA_HOTFIX_DATE
  return CLIENT_DATA_HOTFIX_DATE;
#else
  return {};
#endif /* CLIENT_DATA_HOTFIX_DATE */
}

int dbc::hotfix_build_version( bool ptr )
{
  if ( SC_USE_PTR && ptr )
  {
#ifdef PTR_CLIENT_DATA_HOTFIX_BUILD
    return PTR_CLIENT_DATA_HOTFIX_BUILD;
#else
    return 0;
#endif /* PTR_CLIENT_DATA_HOTFIX_BUILD */
  }

#ifdef CLIENT_DATA_HOTFIX_BUILD
  return CLIENT_DATA_HOTFIX_BUILD;
#else
  return 0;
#endif /* CLIENT_DATA_HOTFIX_BUILD */
}

std::string dbc::hotfix_hash_str( bool ptr )
{
  if ( SC_USE_PTR && ptr )
  {
#ifdef PTR_CLIENT_DATA_HOTFIX_HASH
    return PTR_CLIENT_DATA_HOTFIX_HASH;
#else
    return "";
#endif  /* PTR_CLIENT_DATA_HOTFIX_HASH */
  }

#ifdef CLIENT_DATA_HOTFIX_HASH
  return CLIENT_DATA_HOTFIX_HASH;
#else
  return {};
#endif /* CLIENT_DATA_HOTFIX_HASH */
}

int dbc::client_data_build( bool ptr )
{
#if SC_USE_PTR == 1
  util::string_view __version = ptr ? PTR_CLIENT_DATA_WOW_VERSION : CLIENT_DATA_WOW_VERSION;
#else
  (void) ptr;
  util::string_view __version = CLIENT_DATA_WOW_VERSION;
#endif /* SC_USE_PTR */

  auto pos = __version.rfind( '.' );
  if ( pos == util::string_view::npos )
  {
    return -1;
  }

  return util::to_int( __version.substr( pos + 1 ) );
}
