// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_http.hpp"
#include "util/concurrency.hpp"
#include "util/io.hpp"
#include "util/util.hpp"

#include <iostream>
#include <cstring>

// Cross-Platform Support for HTTP-Download =================================

// ==========================================================================
// PLATFORM INDEPENDENT SECTION
// ==========================================================================

cache::cache_control_t cache::cache_control_t::singleton;

#ifdef SC_NO_NETWORKING
namespace http
{
void set_proxy( util::string_view, util::string_view, const unsigned) {}
const proxy_t& get_proxy() { static proxy_t p {};  return p; }
http_connection_pool_t* pool() { return nullptr; }

void cache_load( const std::string&) {}
void cache_save( const std::string&) {}
void clear_cache() {}

int get( std::string& /*result */,
         const std::string& /* url */,
         cache::behavior_e /* caching */,
         const std::string& /* confirmation */,
         const std::vector<std::string>& /* headers */ )
{
  return 503;
}
std::tuple<std::string, std::string> normalize_header( util::string_view )
{
  return {};
}
} // Namespace http ends
#else

#ifdef SC_USE_CURL

#include "sc_http_curl.hpp"
http::curl_connection_pool_t connection_pool;

#endif /* SC_USE_CURL */

#ifdef SC_USE_WININET
#include "sc_http_wininet.hpp"
http::wininet_connection_pool_t connection_pool;

#endif /* SC_USE_WININET */

namespace { // UNNAMED NAMESPACE ==========================================

http::proxy_t proxy;

const bool HTTP_CACHE_DEBUG = false;

mutex_t cache_mutex;

struct url_cache_entry_t
{
  // Not necessarily UTF-8; may contain zero bytes. Should really be vector<uint8_t>.
  std::string result;
  std::string last_modified_header;
  cache::cache_era modified, validated;

  url_cache_entry_t() :
    modified( cache::cache_era::INVALID ), validated( cache::cache_era::INVALID )
  {}
};

typedef std::unordered_map<std::string, url_cache_entry_t> url_db_t;
url_db_t url_db;

// cache_clear ==============================================================

void cache_clear()
{
  // writer lock
  auto_lock_t lock( cache_mutex );
  url_db.clear();
}

// download =================================================================

int download( url_cache_entry_t&              entry,
              std::string&                    result,
              const std::string&              url,
              const std::vector<std::string>& headers = {} )
{
  auto handle = connection_pool.handle( url );
  // Curl was not able to initialize, get out instantly
  if ( !handle->initialized() )
  {
    std::cerr << "Unable to initialize networking functionality" << std::endl;
    return 500;
  }

  handle->add_request_headers( headers );

  // Add If-Modified-Since
  if ( !entry.last_modified_header.empty() )
  {
    handle->add_request_header( "If-Modified-Since: " + entry.last_modified_header );
  }

  auto res = handle->get( url );
  int response_code = handle->response_code();

  if ( !res )
  {
    std::cerr << "Unable to fetch result (" << handle->error() << ")" << std::endl;
    return handle->response_code();
  }

  // Unconditionally return response body
  result.assign( handle->result() );

  // Cache results on successful fetch
  if ( response_code == 200 )
  {
    entry.last_modified_header.clear();

    entry.result.assign( handle->result() );

    auto response_headers = handle->headers();
    if ( response_headers.find( "last-modified" ) != response_headers.end() )
    {
      entry.last_modified_header = response_headers[ "last-modified" ];
    }
    entry.validated = cache::era();
    entry.modified = cache::era();
  }
  // Re-use cached copy, validate the cache
  else if ( response_code == 304 )
  {
    entry.validated = cache::era();

    result.assign( entry.result );
    // Everything is ok so return 200 instead of 304
    response_code = 200;
  }

  return response_code;
}

} // UNNAMED NAMESPACE ====================================================

// http::set_proxy ==========================================================

void http::set_proxy( util::string_view proxy_type,
                      util::string_view proxy_host,
                      const unsigned     proxy_port )
{
  proxy.type = std::string( proxy_type );
  proxy.host = std::string( proxy_host );
  proxy.port = proxy_port;
}

const http::proxy_t& http::get_proxy()
{
  return proxy;
}

http::http_handle_t::http_handle_t()
{
}

http::http_handle_t::~http_handle_t()
{
}

http::http_connection_pool_t::http_connection_pool_t()
{
}

http::http_connection_pool_t::~http_connection_pool_t()
{
}

// http::clear_cache ========================================================

void http::clear_cache()
{
  cache_clear();
}

// http::cache_load =========================================================

namespace {
std::string cache_get( std::istream& is )
{
  std::string result;
  while ( is )
  {
    unsigned char c = is.get();
    if ( ! c )
      break;
    result += c;
  }
  return result;
}

void cache_put( std::ostream& os, const std::string& s )
{ os.write( s.c_str(), s.size() + 1 ); }

void cache_put( std::ostream& os, const char* s )
{ os.write( s, std::strlen( s ) + 1 ); }

} // unnamed namespace

void http::cache_load( const std::string& file_name )
{
  auto_lock_t lock( cache_mutex );

  try
  {
    io::ifstream file;
    file.open( file_name, std::ios::binary );
    if ( !file ) return;
    file.exceptions( std::ios::eofbit | std::ios::failbit | std::ios::badbit );
    file.unsetf( std::ios::skipws );

    if ( cache_get( file ) != SC_VERSION )
    {
      // invalid version, GTFO
      return;
    }

    std::string content;
    content.reserve( 16 * 1024 );

    while ( ! file.eof() )
    {
      std::string url = cache_get( file );
      std::string last_modified = cache_get( file );

      uint32_t size;
      file.read( reinterpret_cast<char*>( &size ), sizeof( size ) );
      content.resize( size );
      file.read( &content[ 0 ], size );

      url_cache_entry_t& c = url_db[ url ];
      c.result = content;
      c.last_modified_header = last_modified;
      c.modified = c.validated = cache::cache_era::IN_THE_BEGINNING;
    }
  }
  catch ( ... )
  {}
}

// http::cache_save =========================================================

void http::cache_save( const std::string& file_name )
{
  auto_lock_t lock( cache_mutex );

  try
  {
    io::ofstream file;
    file.open( file_name, std::ios::binary );
    if ( ! file ) return;
    file.exceptions( std::ios::eofbit | std::ios::failbit | std::ios::badbit );

    cache_put( file, SC_VERSION );

    for ( url_db_t::const_iterator p = url_db.begin(), e = url_db.end(); p != e; ++p )
    {
      if ( p -> second.validated == cache::cache_era::INVALID )
        continue;

      cache_put( file, p -> first );
      cache_put( file, p -> second.last_modified_header );

      uint32_t size = as<uint32_t>( p -> second.result.size() );
      file.write( reinterpret_cast<const char*>( &size ), sizeof( size ) );
      file.write( p -> second.result.data(), size );
    }
  }
  catch ( ... )
  {}
}

// http::pool ===============================================================

http::http_connection_pool_t* http::pool()
{
  return &connection_pool;
}

// http::get ================================================================

int http::get( std::string&       result,
                const std::string& url,
                cache::behavior_e  caching,
                const std::string& confirmation,
                const std::vector<std::string>& headers )
{
  result.clear();

  std::string encoded_url = url;
  util::urlencode( encoded_url );
  int response_code = 200;

  auto_lock_t lock( cache_mutex );

  url_cache_entry_t& entry = url_db[ encoded_url ];

  if ( HTTP_CACHE_DEBUG )
  {
    io::ofstream http_log;
    http_log.open( "simc_http_log.txt", std::ios::app );
    std::ostream::sentry s( http_log );
    if ( s )
    {
      fmt::print( http_log, "{}: get(\"{}\") [", cache::era(), url );

      if ( entry.validated != cache::cache_era::INVALID )
      {
        if ( entry.validated >= cache::era() )
          http_log << "hot";
        else if ( caching != cache::CURRENT )
          http_log << "warm";
        else
          http_log << "cold";
        fmt::print( http_log, ": ({},{})", entry.modified, entry.validated );
      }
      else
        http_log << "miss";
      if ( caching != cache::ONLY &&
           ( entry.validated == cache::cache_era::INVALID ||
             ( caching == cache::CURRENT && entry.validated < cache::era() ) ) )
        http_log << " download";
      http_log << "]\n";
    }
  }

  if ( entry.validated < cache::era() && ( caching == cache::CURRENT || entry.validated == cache::cache_era::INVALID ) )
  {
    if ( caching == cache::ONLY )
      return 404;

    fmt::print( "@" ); fflush( stdout );

    response_code = download( entry, result, encoded_url, headers );

    if ( HTTP_CACHE_DEBUG && entry.modified < entry.validated )
    {
      io::ofstream http_log;
      http_log.open( "simc_http_log.txt", std::ios::app );
      fmt::print( http_log, "{}: Unmodified ({},{})\n", cache::era(), entry.modified, entry.validated );
    }

    if ( !confirmation.empty() && ( entry.result.find( confirmation ) == std::string::npos ) )
    {
      //fmt::print( "\nsimulationcraft: HTTP failed on '{}'\n", url );
      //fmt::print( "{}\n", ( result.empty() ? "empty" : result.c_str() ) );
      //fflush( stdout );
      return 409; // "Conflict"
    }
  }

  // No result from the download process, grab it from the cache, only if the download process was
  // returned OK status.
  if ( result.empty() && response_code == 200 )
  {
    result = entry.result;
  }

  return response_code;
}

std::tuple<std::string, std::string> http::normalize_header( util::string_view header_str )
{
  // Find first ':'
  auto pos = header_str.find( ':' );
  // Don't support foldable headers since they are deprecated anyhow
  if ( pos == std::string::npos || header_str[ 0 ] == ' ' )
  {
    return {};
  }

  auto key   = std::string( header_str.substr( 0, pos ) );
  auto value = std::string( header_str.substr( pos + 1 ) );

  // Transform all header keys to lowercase to sanitize the input
  std::transform( key.begin(), key.end(), key.begin(), tolower );

  // Prune whitespaces from values
  while ( !value.empty() &&
          ( value.back() == ' ' || value.back() == '\n' || value.back() == '\t' || value.back() == '\r' ) )
  {
    value.pop_back();
  }

  while ( !value.empty() && ( value.front() == ' ' || value.front() == '\t' ) )
  {
    value.erase( value.begin() );
  }

  return std::make_tuple( key, value );
}



#endif /* NO_SC_NETWORKING */

#ifdef UNIT_TEST

uint32_t dbc::get_school_mask( school_e ) { return 0; }
void sim_t::errorf( const char*, ... ) { }

int main( int argc, char* argv[] )
{
  if ( argc > 1 )
  {
    for ( int i = 1; i < argc; ++i )
    {
      if ( !strcmp( argv[ i ], "--dump" ) )
      {
        url_db.clear();
        const char* const url_cache_file = "simc_cache.dat";
        http::cache_load( url_cache_file );

        for ( auto& i : url_db )
        {
          std::cout << "URL: \"" << i.first << "\" (" << i.second.last_modified_header << ")\n"
                    << i.second.result << '\n';
        }
      }
      else
      {
        std::string result;
        if ( http::get( result, argv[ i ], cache::CURRENT ) )
          std::cout << result << '\n';
        else
          std::cout << "Unable to download \"" << argv[ i ] << "\".\n";
      }
    }
  }
  else
  {
    std::string result;

    if ( http::get( result, "http://us.battle.net/wow/en/character/llane/pagezero/advanced", cache::CURRENT ) )
      std::cout << result << '\n';
    else
      std::cout << "Unable to download armory data.\n";

    if ( http::get( result, "http://www.wowhead.com/list=1564664", cache::CURRENT ) )
      std::cout << result << '\n';
    else
      std::cout << "Unable to download wowhead data.\n";
  }

  return 0;
}

#endif /* UNIT_TEST */
