// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// Cross-Platform Support for HTTP-Download =================================

// ==========================================================================
// PLATFORM INDEPENDENT SECTION
// ==========================================================================

cache::cache_control_t cache::cache_control_t::singleton;

#ifdef SC_NO_NETWORKING
namespace http
{
void set_proxy( const std::string&, const std::string&, const unsigned) {}

void cache_load( const std::string&) {}
void cache_save( const std::string&) {}
bool clear_cache( sim_t*, const std::string&, const std::string&) { return true;}

int get( std::string& /*result */,
         const std::string& /* url */,
         cache::behavior_e /* caching */,
         const std::string& /* confirmation */,
         const std::vector<std::string>& /* headers */ )
{
  return 503;
}
}

#else

#include <curl/curl.h>

namespace { // UNNAMED NAMESPACE ==========================================

http::proxy_t proxy;

const bool HTTP_CACHE_DEBUG = false;
const std::string UA_STR = "Simulationcraft/" + std::string( SC_VERSION );

mutex_t cache_mutex;

struct url_cache_entry_t
{
  // Not necessarily UTF-8; may contain zero bytes. Should really be vector<uint8_t>.
  std::string result;
  std::string last_modified_header;
  cache::era_t modified, validated;

  url_cache_entry_t() :
    modified( cache::era_t::INVALID_ERA ), validated( cache::era_t::INVALID_ERA )
  {}
};

class curl_handle_cache_t;

class curl_handle_t
{
  CURL*                m_handle;
  curl_slist*          m_header_opts;

  std::string          m_buffer;
  std::unordered_map<std::string, std::string> m_headers;
  char                 m_error[ CURL_ERROR_SIZE ];

  size_t read_data( const char* data_in, size_t total_bytes )
  {
    m_buffer.append( data_in, total_bytes );
    return total_bytes;
  }

  size_t add_response_header( const char* header_str, size_t total_bytes )
  {
    std::string header = std::string( header_str, total_bytes );
    // Find first ':'
    auto pos = header.find( ':' );
    // Don't support foldable headers since they are deprecated anyhow
    if ( pos == std::string::npos || header[ 0 ] == ' ' )
    {
      return total_bytes;
    }

    std::string key = header.substr( 0, pos );
    std::string value = header.substr( pos + 1 );

    // Transform all header keys to lowercase to sanitize the input
    std::transform( key.begin(), key.end(), key.begin(), tolower );

    // Prune whitespaces from values
    while ( !value.empty() && ( value.back() == ' ' || value.back() == '\n' ||
            value.back() == '\t' || value.back() == '\r' ) )
    {
      value.pop_back();
    }

    while ( !value.empty() && ( value.front() == ' ' || value.front() == '\t' ) )
    {
      value.erase( value.begin() );
    }

    m_headers[ key ] = value;

    return total_bytes;
  }
public:
  static size_t data_cb( void* contents, size_t size, size_t nmemb, void* usr )
  {
    curl_handle_t* obj = reinterpret_cast<curl_handle_t*>( usr );
    return obj->read_data( reinterpret_cast<const char*>( contents ), size * nmemb );
  }

  static size_t header_cb( char* contents, size_t size, size_t nmemb, void* usr )
  {
    curl_handle_t* obj = reinterpret_cast<curl_handle_t*>( usr );
    return obj->add_response_header( contents, size * nmemb );
  }

  curl_handle_t() : m_handle( nullptr ), m_header_opts( nullptr )
  {
    m_error[ 0 ] = '\0';
  }

  curl_handle_t( CURL* handle ) : m_handle( handle ), m_header_opts( nullptr )
  {
    m_error[ 0 ] = '\0';

    curl_easy_setopt( handle, CURLOPT_HEADERFUNCTION, header_cb );
    curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, data_cb );
    curl_easy_setopt( handle, CURLOPT_HEADERDATA, reinterpret_cast<void*>( this ) );
    curl_easy_setopt( handle, CURLOPT_WRITEDATA, reinterpret_cast<void*>( this ) );
    curl_easy_setopt( handle, CURLOPT_ERRORBUFFER, m_error );
  }

  // Curl handle object
  CURL* handle() const
  { return m_handle; }

  // Curl descriptive error (when fetch() returns != CURLE_OK)
  std::string error() const
  { return { m_error }; }

  // Response body
  const std::string& result() const
  { return m_buffer; }

  // Response headers
  const std::unordered_map<std::string, std::string>& headers() const
  { return m_headers; }

  void add_request_headers( const std::vector<std::string>& headers )
  {
    range::for_each( headers, [ this ]( const std::string& header ) {
      m_header_opts = curl_slist_append( m_header_opts, header.c_str() );
    } );

    curl_easy_setopt( m_handle, CURLOPT_HTTPHEADER, m_header_opts );
  }

  void add_request_header( const std::string& header )
  {
    m_header_opts = curl_slist_append( m_header_opts, header.c_str() );
    curl_easy_setopt( m_handle, CURLOPT_HTTPHEADER, m_header_opts );
  }

  // Fetch an url
  CURLcode fetch( const std::string& url ) const
  {
    curl_easy_setopt( m_handle, CURLOPT_URL, url.c_str() );
    return curl_easy_perform( m_handle );
  }

  int response_code() const
  {
    long response_code = 0;
    curl_easy_getinfo( m_handle, CURLINFO_RESPONSE_CODE, &response_code );
    return as<int>( response_code );
  }

  ~curl_handle_t();
};

class curl_handle_cache_t
{
  CURL* m_handle = nullptr;

public:
  curl_handle_cache_t()
  {
    curl_global_init( CURL_GLOBAL_ALL );
  }

  curl_handle_t get()
  {
    if ( !m_handle )
    {
      bool ssl_proxy = ( proxy.type == "https" );
      bool use_proxy = ( ssl_proxy || proxy.type == "http" );

      m_handle = curl_easy_init();
      if ( !m_handle )
      {
        return {};
      }

      if ( HTTP_CACHE_DEBUG )
      {
        curl_easy_setopt( m_handle, CURLOPT_VERBOSE, 1L);
      }

      curl_easy_setopt( m_handle, CURLOPT_TIMEOUT, 15L );
      curl_easy_setopt( m_handle, CURLOPT_FOLLOWLOCATION, 1L );
      curl_easy_setopt( m_handle, CURLOPT_MAXREDIRS, 5L );
      curl_easy_setopt( m_handle, CURLOPT_ACCEPT_ENCODING, "");
      curl_easy_setopt( m_handle, CURLOPT_USERAGENT, UA_STR.c_str() );

      if ( use_proxy )
      {
        char error_buffer[ CURL_ERROR_SIZE ];
        error_buffer[ 0 ] = '\0';
// Note, won't cover the case where curl is compiled without USE_SSL
#ifdef CURL_VERSION_HTTPS_PROXY
        auto ret = curl_easy_setopt( m_handle, CURLOPT_PROXYTYPE,
            ssl_proxy ? CURLPROXY_HTTPS : CURLPROXY_HTTP );
#else
        if ( ssl_proxy )
        {
          std::cerr << "Libcurl does not support HTTPS proxies, aborting." << std::endl;
          return {};
        }
        auto ret = curl_easy_setopt( m_handle, CURLOPT_PROXYTYPE, CURLPROXY_HTTP );
#endif /* CURL_VERSION_HTTPS_PROXY */
        if ( ret != CURLE_OK )
        {
          std::cerr << "Unable setup proxy (" << error_buffer << ")" << std::endl;
          curl_easy_cleanup( m_handle );
          m_handle = nullptr;
          return {};
        }

        curl_easy_setopt( m_handle, CURLOPT_PROXY, proxy.host.c_str() );
        curl_easy_setopt( m_handle, CURLOPT_PROXYPORT, proxy.port );
      }
    }

    return { m_handle };
  }

  ~curl_handle_cache_t()
  {
    curl_easy_cleanup( m_handle );
    curl_global_cleanup();
  }
};

curl_handle_t::~curl_handle_t()
{
  // Reset headers before giving the handle to the user
  curl_easy_setopt( m_handle, CURLOPT_HTTPHEADER, nullptr );
  // Free headers
  curl_slist_free_all( m_header_opts );
}

typedef std::unordered_map<std::string, url_cache_entry_t> url_db_t;
url_db_t url_db;

curl_handle_cache_t curl_db;

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
  auto handle = curl_db.get();
  // Curl was not able to initialize, get out instantly
  if ( !handle.handle() )
  {
    std::cerr << "Unable to initialize CURL" << std::endl;
    return 500;
  }

  handle.add_request_headers( headers );

  // Add If-Modified-Since
  if ( !entry.last_modified_header.empty() )
  {
    handle.add_request_header( ( "If-Modified-Since: " + entry.last_modified_header ).c_str() );
  }

  auto res = handle.fetch( url );
  int response_code = handle.response_code();

  if ( res != CURLE_OK )
  {
    std::cerr << "Unable to fetch result (" << handle.error() << ")" << std::endl;
    return handle.response_code();
  }

  // Unconditionally return response body
  result.assign( handle.result() );

  // Cache results on successful fetch
  if ( response_code == 200 )
  {
    entry.last_modified_header.clear();

    entry.result.assign( handle.result() );

    auto response_headers = handle.headers();
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

void http::set_proxy( const std::string& proxy_type,
                      const std::string& proxy_host,
                      const unsigned     proxy_port )
{
  proxy.type = proxy_type;
  proxy.host = proxy_host;
  proxy.port = proxy_port;
}

// http::clear_cache ========================================================

bool http::clear_cache( sim_t* sim,
                        const std::string& name,
                        const std::string& value )
{
  assert( name == "http_clear_cache" ); ( void )name;
  if ( value != "0" && ! sim -> parent ) cache_clear();
  return true;
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
      c.modified = c.validated = cache::era_t::IN_THE_BEGINNING;
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
      if ( p -> second.validated == cache::era_t::INVALID_ERA )
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
      http_log << cache::era() << ": get(\"" << url << "\") [";

      if ( entry.validated != cache::era_t::INVALID_ERA )
      {
        if ( entry.validated >= cache::era() )
          http_log << "hot";
        else if ( caching != cache::CURRENT )
          http_log << "warm";
        else
          http_log << "cold";
        http_log << ": (" << entry.modified << ", " << entry.validated << ')';
      }
      else
        http_log << "miss";
      if ( caching != cache::ONLY &&
           ( entry.validated == cache::era_t::INVALID_ERA ||
             ( caching == cache::CURRENT && entry.validated < cache::era() ) ) )
        http_log << " download";
      http_log << "]\n";
    }
  }

  if ( entry.validated < cache::era() && ( caching == cache::CURRENT || entry.validated == cache::era_t::INVALID_ERA ) )
  {
    if ( caching == cache::ONLY )
      return 404;

    util::printf( "@" ); fflush( stdout );

    response_code = download( entry, result, encoded_url, headers );

    if ( HTTP_CACHE_DEBUG && entry.modified < entry.validated )
    {
      io::ofstream http_log;
      http_log.open( "simc_http_log.txt", std::ios::app );
      http_log << cache::era() << ": Unmodified (" << entry.modified << ", " << entry.validated << ")\n";
    }

    if ( confirmation.size() && ( entry.result.find( confirmation ) == std::string::npos ) )
    {
      //util::printf( "\nsimulationcraft: HTTP failed on '%s'\n", url.c_str() );
      //util::printf( "%s\n", ( result.empty() ? "empty" : result.c_str() ) );
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

#endif /* NO_SC_NETWORKING */

#ifdef UNIT_TEST

#include <iostream>

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
