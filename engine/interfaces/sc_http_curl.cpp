#include "config.hpp"

#ifdef SC_USE_CURL

#include "sc_http_curl.hpp"

#include "util/generic.hpp"
#include "util/util.hpp"
#include "fmt/format.h"

#include <iostream>

namespace
{
const std::string UA_STR = "Simulationcraft+CURL/" + std::string( SC_VERSION );
} /* Namespace anonymous ends */

namespace http
{

void curl_handle_t::set_proxy()
{
  const auto& proxy = http::get_proxy();

  if ( m_handle == nullptr )
  {
    return;
  }

  bool ssl_proxy = util::str_compare_ci( proxy.type, "https" );
  bool use_proxy = ssl_proxy || util::str_compare_ci( proxy.type, "http" );

  if ( !use_proxy )
  {
    return;
  }

  // Note, won't cover the case where curl is compiled without USE_SSL
#ifdef CURL_VERSION_HTTPS_PROXY
  auto ret = curl_easy_setopt( m_handle, CURLOPT_PROXYTYPE,
      ssl_proxy ? CURLPROXY_HTTPS : CURLPROXY_HTTP );
#else
  if ( ssl_proxy )
  {
    throw std::runtime_error( "Libcurl does not support HTTPS proxies" );
  }
  auto ret = curl_easy_setopt( m_handle, CURLOPT_PROXYTYPE, CURLPROXY_HTTP );
#endif /* CURL_VERSION_HTTPS_PROXY */
  if ( ret != CURLE_OK )
  {
    throw std::runtime_error( fmt::format( "Unable to setup proxy: {}", m_error ) );
  }

  if ( curl_easy_setopt( m_handle, CURLOPT_PROXY, proxy.host.c_str() ) != CURLE_OK )
  {
    throw std::runtime_error( fmt::format( "Unable to setup proxy: {}", m_error ) );
  }

  if ( curl_easy_setopt( m_handle, CURLOPT_PROXYPORT, proxy.port ) != CURLE_OK )
  {
    throw std::runtime_error( fmt::format( "Unable to setup proxy: {}", m_error ) );
  }
}

size_t curl_handle_t::read_data( const char* data_in, size_t total_bytes )
{
  m_buffer.append( data_in, total_bytes );
  return total_bytes;
}

size_t curl_handle_t::add_response_header( const char* header_str, size_t total_bytes )
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

size_t curl_handle_t::data_cb( void* contents, size_t size, size_t nmemb, void* usr )
{
  curl_handle_t* obj = reinterpret_cast<curl_handle_t*>( usr );
  return obj->read_data( reinterpret_cast<const char*>( contents ), size * nmemb );
}

size_t curl_handle_t::header_cb( char* contents, size_t size, size_t nmemb, void* usr )
{
  curl_handle_t* obj = reinterpret_cast<curl_handle_t*>( usr );
  return obj->add_response_header( contents, size * nmemb );
}

curl_handle_t::curl_handle_t() : http_handle_t(), m_handle( nullptr ), m_header_opts( nullptr )
{
  m_error[ 0 ] = '\0';
}

curl_handle_t::curl_handle_t( CURL* handle ) :
  http_handle_t(), m_handle( handle ), m_header_opts( nullptr )
{
  m_error[ 0 ] = '\0';

  auto ret = curl_easy_setopt( handle, CURLOPT_ERRORBUFFER, m_error );
  if ( ret != CURLE_OK )
  {
    throw std::runtime_error( fmt::format( "Unable to setup CURL: {}", curl_easy_strerror( ret ) ) );
  }

  if ( curl_easy_setopt( handle, CURLOPT_TIMEOUT, 15L ) != CURLE_OK )
  {
    throw std::runtime_error( fmt::format( "Unable to setup CURL: {}", m_error ) );
  }

  if ( curl_easy_setopt( handle, CURLOPT_FOLLOWLOCATION, 1L ) != CURLE_OK )
  {
    throw std::runtime_error( fmt::format( "Unable to setup CURL: {}", m_error ) );
  }

  if ( curl_easy_setopt( handle, CURLOPT_MAXREDIRS, 5L ) != CURLE_OK )
  {
    throw std::runtime_error( fmt::format( "Unable to setup CURL: {}", m_error ) );
  }

  if ( curl_easy_setopt( handle, CURLOPT_ACCEPT_ENCODING, "" ) != CURLE_OK )
  {
    throw std::runtime_error( fmt::format( "Unable to setup CURL: {}", m_error ) );
  }

  if ( curl_easy_setopt( handle, CURLOPT_USERAGENT, UA_STR.c_str() ) != CURLE_OK )
  {
    throw std::runtime_error( fmt::format( "Unable to setup CURL: {}", m_error ) );
  }

  if ( curl_easy_setopt( handle, CURLOPT_HEADERFUNCTION, header_cb ) != CURLE_OK )
  {
    throw std::runtime_error( fmt::format( "Unable to setup CURL: {}", m_error ) );
  }

  if ( curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, data_cb ) != CURLE_OK )
  {
    throw std::runtime_error( fmt::format( "Unable to setup CURL: {}", m_error ) );
  }

  if ( curl_easy_setopt( handle, CURLOPT_HEADERDATA, reinterpret_cast<void*>( this ) ) != CURLE_OK )
  {
    throw std::runtime_error( fmt::format( "Unable to setup CURL: {}", m_error ) );
  }

  if ( curl_easy_setopt( handle, CURLOPT_WRITEDATA, reinterpret_cast<void*>( this ) ) != CURLE_OK )
  {
    throw std::runtime_error( fmt::format( "Unable to setup CURL: {}", m_error ) );
  }

#ifdef CURL_DEBUG
  if ( curl_easy_setopt( handle, CURLOPT_VERBOSE, 1L ) != CURLE_OK )
  {
    throw std::runtime_error( fmt::format( "Unable to setup CURL: {}", m_error ) );
  }
#endif

  set_proxy();
}

curl_handle_t::~curl_handle_t()
{
  curl_easy_reset( m_handle );
  curl_slist_free_all( m_header_opts );
}

bool curl_handle_t::initialized() const
{
  return m_handle != nullptr;
}

// Curl descriptive error (when fetch() returns != CURLE_OK)
std::string curl_handle_t::error() const
{
  return { m_error };
}

// Response body
const std::string& curl_handle_t::result() const
{
  return m_buffer;
}

// Response headers
const std::unordered_map<std::string, std::string>& curl_handle_t::headers() const
{
  return m_headers;
}

void curl_handle_t::add_request_headers( const std::vector<std::string>& headers )
{
  range::for_each( headers, [ this ]( const std::string& header ) {
    m_header_opts = curl_slist_append( m_header_opts, header.c_str() );
  } );

  curl_easy_setopt( m_handle, CURLOPT_HTTPHEADER, m_header_opts );
}

void curl_handle_t::add_request_header( const std::string& header )
{
  m_header_opts = curl_slist_append( m_header_opts, header.c_str() );
  curl_easy_setopt( m_handle, CURLOPT_HTTPHEADER, m_header_opts );
}

bool curl_handle_t::set_basic_auth( const std::string& userpwd )
{
  return curl_easy_setopt( m_handle, CURLOPT_USERPWD, userpwd.c_str() ) == CURLE_OK;
}

bool curl_handle_t::set_basic_auth( const std::string& user, const std::string& pwd )
{
  return set_basic_auth( user + ":" + pwd );
}

// Fetch an url
bool curl_handle_t::get( const std::string& url )
{
  if ( url.empty() )
  {
    return false;
  }

  if ( curl_easy_setopt( m_handle, CURLOPT_URL, url.c_str() ) != CURLE_OK )
  {
    std::cerr << fmt::format( "Unable to set URL {}: {}", url, m_error ) << std::endl;
    return false;
  }

  if ( curl_easy_perform( m_handle ) != CURLE_OK )
  {
    std::cerr << fmt::format( "Unable to perform operation: {}", m_error ) << std::endl;
    return false;
  }

  return true;
}

bool curl_handle_t::post( const std::string& url, const std::string& data, const std::string& content_type )
{
  if ( !content_type.empty() )
  {
    add_request_header( "Content-Type: " + content_type );
  }

  if ( curl_easy_setopt( m_handle, CURLOPT_POSTFIELDS, data.c_str() ) != CURLE_OK )
  {
    std::cerr << fmt::format( "Unable to set POST data \"{}\": {}", data, m_error ) << std::endl;
    return false;
  }

  return get( url );
}

int curl_handle_t::response_code() const
{
  long response_code = 0;
  if ( curl_easy_getinfo( m_handle, CURLINFO_RESPONSE_CODE, &response_code ) != CURLE_OK )
  {
    std::cerr << fmt::format( "Unable to retrieve response code: {}", m_error ) << std::endl;
    return -1;
  }

  return as<int>( response_code );
}

curl_connection_pool_t::curl_connection_pool_t() : http_connection_pool_t(), m_handle( nullptr )
{
  auto ret = curl_global_init( CURL_GLOBAL_ALL );
  if ( ret != CURLE_OK )
  {
    throw std::runtime_error( fmt::format( "Unable to initialize libcurl: {}",
      curl_easy_strerror( ret ) ) );
  }
}

curl_connection_pool_t::~curl_connection_pool_t()
{
  curl_easy_cleanup( m_handle );
  curl_global_cleanup();
}

std::unique_ptr<http_handle_t> curl_connection_pool_t::handle( const std::string& /* url */ )
{
  if ( m_handle )
  {
    return std::unique_ptr<http_handle_t>( new curl_handle_t( m_handle ) );
  }

  m_handle = curl_easy_init();
  if ( !m_handle )
  {
    throw std::runtime_error( "Unable to create CURL handle" );
  }

  return std::unique_ptr<http_handle_t>( new curl_handle_t( m_handle ) );
}

} /* Namespace http ends */

#endif /* SC_USE_CURL */
