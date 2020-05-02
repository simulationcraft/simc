#include "simulationcraft.hpp"

#ifdef SC_USE_WININET

#include <windows.h>
#include <wincrypt.h>
#include <WinBase.h>

#include "sc_http_wininet.hpp"

namespace
{
  static const std::string UA_STR = "Simulationcraft+WININET/" + std::string( SC_VERSION );

  std::string error_str( const std::string& module_str )
  {
    DWORD err = GetLastError();
    if ( err == 0L )
    {
      return "";
    }

    LPSTR buffer = nullptr;

    auto n_chars = FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM,
      GetModuleHandleA( module_str.c_str() ), err, 0, reinterpret_cast< LPSTR >( &buffer ), 256, NULL );

    if ( n_chars <= 1L )
    {
      LocalFree( buffer );
      return fmt::format( "FormatMessage error={:#08x} on error code {:#08x}", GetLastError(), err );
    }

    std::string error_str{ buffer };

    LocalFree( buffer );

    return error_str;
  }

}  // namespace

namespace http
{
bool parse_url( const std::string& url_in, URL_COMPONENTSA& url_out )
{
  memset( &url_out, 0, sizeof( url_out ) );
  url_out.dwStructSize = sizeof( url_out );

  url_out.dwSchemeLength    = 1;
  url_out.dwHostNameLength  = 1;
  url_out.dwUrlPathLength   = 1;
  url_out.dwExtraInfoLength = 1;

  if ( !InternetCrackUrlA( url_in.c_str(), as<DWORD>( url_in.length() ), 0, &url_out ) )
  {
    return false;
  }

  return true;
}

void wininet_connection_pool_t::initialize()
{
  if ( m_root_handle )
  {
    return;
  }

  // Figure out proxying setup here
  const auto& proxy = http::get_proxy();

  if ( !proxy.type.empty() )
  {
    std::string proxy_url = proxy.host;
    if ( proxy.port > 0 )
    {
      proxy_url += ":" + util::to_string( proxy.port );
    }

    m_root_handle = InternetOpenA( UA_STR.c_str(), INTERNET_OPEN_TYPE_PROXY, proxy_url.c_str(), NULL, 0 );
  }
  else
  {
    m_root_handle = InternetOpenA( UA_STR.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
  }

  if ( m_root_handle == NULL )
  {
    throw std::runtime_error( fmt::format( "Unable to initialize wininet: errorcode={} error={}",
      GetLastError(), error_str( "wininet.dll" ) ) );
  }
}

wininet_connection_pool_t::wininet_connection_pool_t() : http_connection_pool_t(), m_root_handle( nullptr )
{
}

wininet_connection_pool_t::~wininet_connection_pool_t()
{
  range::for_each( m_handle_db, []( const std::pair<std::string, HINTERNET>& v ) {
    InternetCloseHandle( v.second );
  } );

  InternetCloseHandle( m_root_handle );
}

std::unique_ptr<http_handle_t> wininet_connection_pool_t::handle( const std::string& url_str )
{
  URL_COMPONENTSA url;
  if ( !parse_url( url_str, url ) )
  {
    throw std::runtime_error( fmt::format( "Unable to parse url \"{}\": {}",
      url_str, error_str( "wininet.dll" ) ) );
  }

  initialize();

  std::string scheme { url.lpszScheme, url.dwSchemeLength };
  std::string host { url.lpszHostName, url.dwHostNameLength };

  std::string key = scheme + ":" + host + ":" + util::to_string( url.nPort );

  auto it = m_handle_db.find( key );

  if ( it != m_handle_db.end() )
  {
    return std::unique_ptr<http_handle_t>( new wininet_handle_t( it->second ) );
  }

  auto port_number = url.nPort != 0 ? url.nPort : INTERNET_INVALID_PORT_NUMBER;
  auto new_connection = InternetConnectA( m_root_handle, host.c_str(), port_number, NULL, NULL,
    INTERNET_SERVICE_HTTP, 0, NULL );

  if ( new_connection == nullptr )
  {
    throw std::runtime_error( fmt::format( "Unable to create a connection to \"{}\": {}",
      host, error_str( "wininet.dll" ) ) );
  }

  m_handle_db[ key ] = new_connection;

  return std::unique_ptr<http_handle_t>( new wininet_handle_t( new_connection ) );
}

HINTERNET wininet_handle_t::create_handle( const std::string& verb, const std::string& url_str )
{
  URL_COMPONENTSA url;

  if ( !parse_url( url_str, url ) )
  {
    throw std::runtime_error( fmt::format( "Unable to parse url \"{}\": {}",
      url_str, error_str( "wininet.dll" ) ) );
  }

  // Needs to be added manually for some reason, InternetOpen() agent disappears from the set of 
  // headers added to the request
  add_request_header( "User-Agent: " + UA_STR );

  std::string scheme { url.lpszScheme, url.dwSchemeLength };
  std::string path { url.lpszUrlPath, url.dwUrlPathLength };

  if ( url.dwExtraInfoLength > 0 )
  {
    path += url.lpszExtraInfo;
  }

  auto secure = util::str_compare_ci( scheme, "https" );
  DWORD flags = INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_NO_AUTH | INTERNET_FLAG_NO_UI |
                INTERNET_FLAG_KEEP_CONNECTION | ( secure ? INTERNET_FLAG_SECURE : 0 );

  const char* accept_types[] { "*/*", NULL };

  auto handle = HttpOpenRequestA( m_connection_handle, verb.c_str(), path.c_str(), NULL, NULL,
    accept_types, flags, NULL );

  if ( handle == NULL )
  {
    throw std::runtime_error( fmt::format( "Unable to open request to \"{}\": {}",
      url_str, error_str( "wininet.dll" ) ) );
    return NULL;
  }

  m_request_handle = handle;

  return handle;
}

bool wininet_handle_t::read_data()
{
  std::array<char, 65536> buffer;

  DWORD bytes_read = 0L;
  bool ret         = false;

  do
  {
    ret = InternetReadFile( m_request_handle, reinterpret_cast<void*>( buffer.data() ), as<DWORD>( buffer.size() ),
                            &bytes_read );

    if ( ret && bytes_read > 0L )
    {
      m_buffer.append( buffer.data(), bytes_read );
    }

  } while ( bytes_read > 0L && ret );

  if ( !ret )
  {
    std::cerr << fmt::format( "Unable to read data: {}", error_str( "wininet.dll" ) ) << std::endl;
  }

  return ret;
}

bool wininet_handle_t::parse_headers()
{
  std::vector<char> buffer;

  DWORD buffer_length = 65536;
  DWORD index         = 0L;
  bool ret            = false;

  // 65K buffer should be enough for HTTP headers
  buffer.resize( as<size_t>( buffer_length ) );

  do
  {
    ret = HttpQueryInfoA( m_request_handle, HTTP_QUERY_RAW_HEADERS_CRLF, 
      reinterpret_cast<LPVOID>( buffer.data() ), &buffer_length, &index );

    if ( ret == false && GetLastError() == ERROR_INSUFFICIENT_BUFFER )
    {
      buffer.resize( as<size_t>( buffer_length ) );
    }

  } while ( ret == false && GetLastError() == ERROR_INSUFFICIENT_BUFFER );

  if ( ret )
  {
    auto headers_str  = std::string{buffer.begin(), buffer.end()};
    auto header_split = util::string_split( headers_str, "\r\n" );

    range::for_each( header_split, [this]( const std::string& header_str ) {
      auto normalized_header = http::normalize_header( header_str );
      if ( !std::get<0>( normalized_header ).empty() )
      {
        m_headers[ std::get<0>( normalized_header ) ] = std::get<1>( normalized_header );
      }
    } );
  }
  else
  {
    std::cerr << fmt::format( "Unable to parse response headers: {}", error_str( "wininet.dll" ) ) << std::endl;
  }

  return ret;
}

bool wininet_handle_t::set_headers()
{
  if ( !m_request_handle )
  {
    return false;
  }

  std::string headers = util::string_join( m_request_headers, "" );
  auto ret = HttpAddRequestHeadersA( m_request_handle, headers.c_str(), as<DWORD>( headers.length() ),
    HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE );

  if ( !ret )
  {
    std::cerr << fmt::format( "Unable to add request headers: {}", error_str( "wininet.dll" ) ) << std::endl;
  }

  return ret;
}

wininet_handle_t::wininet_handle_t() : m_connection_handle( NULL ), m_request_handle( NULL )
{
}

wininet_handle_t::wininet_handle_t( HINTERNET handle ) : m_connection_handle( handle ), m_request_handle( NULL )
{
}

wininet_handle_t::~wininet_handle_t()
{
  if ( m_request_handle )
  {
    InternetCloseHandle( m_request_handle );
  }
}

bool wininet_handle_t::initialized() const
{
  return true;
}

std::string wininet_handle_t::error() const
{
  return error_str( "wininet.dll" );
}

int wininet_handle_t::response_code() const
{
  if ( m_request_handle == NULL )
  {
    return -1;
  }

  DWORD response_code = -1;
  DWORD buflen        = sizeof( response_code );

  auto ret = HttpQueryInfoA( m_request_handle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
    &response_code, &buflen, 0 );

  if ( !ret )
  {
    std::cerr << fmt::format( "Unable to determine response code: {}", error_str( "wininet.dll" ) ) << std::endl;
    return -1;
  }

  return as<int>( response_code );
}

const std::string& wininet_handle_t::result() const
{
  return m_buffer;
}

const std::unordered_map<std::string, std::string>& wininet_handle_t::headers() const
{
  return m_headers;
}

void wininet_handle_t::add_request_header( const std::string& header )
{
  if ( header.size() < 2 )
  {
    return;
  }

  // Wininet wants the headers to have \r\n at the end ...
  auto normalized_header = header;

  // Presume there's either nothing or \r\n
  if ( normalized_header.size() > 2 && normalized_header.back() != '\n' )
  {
    normalized_header += "\r\n";
  }

  m_request_headers.push_back( normalized_header );
}

void wininet_handle_t::add_request_headers( const std::vector<std::string>& headers )
{
  range::for_each( headers, [this]( const std::string& header_str ) {
    add_request_header( header_str );
  } );
}

bool wininet_handle_t::set_basic_auth( const std::string& user, const std::string& pwd )
{
  return set_basic_auth( user + ":" + pwd );
}

bool wininet_handle_t::set_basic_auth( const std::string& userpwd )
{
  DWORD str_len = 0L;
  std::vector<BYTE> data_in { userpwd.begin(), userpwd.end() };
  std::vector<CHAR> data_out;

  // Calculate length
  auto ret = CryptBinaryToStringA( data_in.data(), as<DWORD>( data_in.size() ),
    CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &str_len );

  if ( !ret )
  {
    std::cerr << fmt::format( "Unable to determine encoded length: {}", error_str( "crypt32.dll" ) ) << std::endl;
    return false;
  }

  data_out.resize( str_len );

  ret = CryptBinaryToStringA( data_in.data(), as<DWORD>( data_in.size() ),
    CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, data_out.data(), &str_len );

  if ( !ret )
  {
    std::cerr << fmt::format( "Unable to base64 encode data: {}", error_str( "crypt32.dll" ) ) << std::endl;
    return false;
  }

  // -1 because length includes the \x00 byte at the end
  std::string header_str { data_out.begin(), data_out.end() - 1 };
  add_request_header( "Authorization: Basic " + header_str );

  return ret;
}

bool wininet_handle_t::get( const std::string& url )
{
  if ( !create_handle( "GET", url ) )
  {
    return false;
  }

  if ( !set_headers() )
  {
    return false;
  }

  if ( !HttpSendRequestA( m_request_handle, NULL, 0L, NULL, 0L ) )
  {
    std::cerr << fmt::format( "Unable to send request to \"{}\": {}",
      url, error_str( "wininet.dll" ) ) << std::endl;
    return false;
  }

  if ( !parse_headers() )
  {
    return false;
  }

  if ( !read_data() )
  {
    return false;
  }

  return true;
}

bool wininet_handle_t::post( const std::string& url, const std::string& data, const std::string& content_type )
{
  if ( data.empty() || !create_handle( "POST", url ) )
  {
    return false;
  }

  if ( !content_type.empty() )
  {
    add_request_header( "Content-Type: " + content_type );
  }

  if ( !set_headers() )
  {
    return false;
  }

  std::vector<char> data_vector { data.begin(), data.end() };

  if ( !HttpSendRequestA( m_request_handle, NULL, 0L,
       reinterpret_cast<void*>( data_vector.data() ), as<DWORD>( data_vector.size() ) ) )
  {
    std::cerr << fmt::format( "Unable to send request to \"{}\": {}",
      url, error_str( "wininet.dll" ) ) << std::endl;
    return false;
  }

  if ( !parse_headers() )
  {
    return false;
  }

  if ( !read_data() )
  {
    return false;
  }
  
  return true;
}

}  // namespace http

#endif /* SC_USE_WININET */
