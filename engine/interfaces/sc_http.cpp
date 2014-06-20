// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// Cross-Platform Support for HTTP-Download =================================

// ==========================================================================
// PLATFORM INDEPENDENT SECTION
// ==========================================================================

// proxy ====================================================================

http::proxy_t proxy;

cache::cache_control_t cache::cache_control_t::singleton;

namespace { // UNNAMED NAMESPACE ==========================================

const bool HTTP_CACHE_DEBUG = false;

mutex_t cache_mutex;

const unsigned int NETBUFSIZE = 1 << 15;

struct url_cache_entry_t
{
  // Not necessarily UTF-8; may contain zero bytes. Should really be vector<uint8_t>.
  std::string result;
  std::string last_modified_header;
  cache::era_t modified, validated;

  url_cache_entry_t() :
    modified( cache::INVALID_ERA ), validated( cache::INVALID_ERA )
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

const char* const cookies =
  "Cookie: loginChecked=1\r\n"
  "Cookie: cookieLangId=en_US\r\n"
  // Skip arenapass 2011 advertisement .. can we please have a sensible
  // API soon?
  "Cookie: int-WOW=1\r\n"
  "Cookie: int-WOW-arenapass2011=1\r\n"
  "Cookie: int-WOW-epic-savings-promo=1\r\n"
  "Cookie: int-WOW-anniversary=1\r\n"
  "Cookie: int-EuropeanInvitational2011=1\r\n"
  "Cookie: int-dec=1\r\n";

#if defined( NO_HTTP )

// ==========================================================================
// NO HTTP-DOWNLOAD SUPPORT
// ==========================================================================

// download =================================================================

bool download( url_cache_entry_t&,
                      const std::string& )
{
  return false;
}

#elif defined( SC_WINDOWS )

// ==========================================================================
// HTTP-DOWNLOAD FOR WINDOWS
// ==========================================================================
#include <windows.h>
#include <wininet.h>

// download =================================================================

bool download( url_cache_entry_t& entry,
                      const std::string& url )
{
  // Requires cache_mutex to be held.

  class InetWrapper : public noncopyable
  {
  public:
    HINTERNET handle;

    explicit InetWrapper( HINTERNET handle_ ) : handle( handle_ ) {}
    ~InetWrapper() { if ( handle ) InternetCloseHandle( handle ); }
    operator HINTERNET () const { return handle; }
  };

  static HINTERNET hINet;
  if ( !hINet )
  {
    // hINet = InternetOpen( L"simulationcraft", INTERNET_OPEN_TYPE_PROXY, "proxy-server", NULL, 0 );
    hINet = InternetOpenW( L"simulationcraft", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
    if ( ! hINet )
      return false;
  }

  std::wstring headers = io::widen( cookies );

  if ( ! entry.last_modified_header.empty() )
  {
    headers += L"If-Modified-Since: ";
    headers += io::widen( entry.last_modified_header );
    headers += L"\r\n";
  }

  InetWrapper hFile( InternetOpenUrlW( hINet, io::widen( url ).c_str(), headers.data(), static_cast<DWORD>( headers.length() ),
                                       INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0 ) );
  if ( ! hFile )
    return false;

  union
  {
    char chars[ NETBUFSIZE ];
    wchar_t wchars[ ( NETBUFSIZE + 1 ) / 2 ];
  } buffer;
  buffer.chars[ 0 ] = '\0';
  DWORD amount = sizeof( buffer );
  if ( ! HttpQueryInfoA( hFile, HTTP_QUERY_STATUS_CODE, buffer.chars, &amount, 0 ) )
    return false;

  if ( ! std::strcmp( buffer.chars, "304" ) )
  {
    entry.validated = cache::era();
    return true;
  }

  entry.result.clear();
  while ( InternetReadFile( hFile, buffer.chars, sizeof( buffer ), &amount ) )
  {
    if ( amount == 0 )
      break;
    entry.result.append( buffer.chars, buffer.chars + amount );
  }

  entry.modified = entry.validated = cache::era();

  entry.last_modified_header.clear();
  amount = sizeof( buffer );
  DWORD index = 0;
  if ( HttpQueryInfoW( hFile, HTTP_QUERY_LAST_MODIFIED, buffer.wchars, &amount, &index ) )
    entry.last_modified_header = io::narrow( buffer.wchars );

  return true;
}

#else

// ==========================================================================
// HTTP-DOWNLOAD FOR POSIX COMPLIANT PLATFORMS
// ==========================================================================

#if defined( SC_MINGW )
// Keep this code compiling on MinGW just for the hell of it.
#include <windows.h>
#include <wininet.h>
#include <Winsock2.h>
#include <io.h> // for POSIX ::close

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#endif

struct SocketWrapper
{
  int fd;

  SocketWrapper() : fd( -1 ) {}
  ~SocketWrapper() { if ( fd >= 0 ) ::close( fd ); }

  operator int () const { return fd; }

  int connect( const std::string& host, unsigned short port );

  int send( const char* buf, std::size_t size )
  { return ::send( fd, buf, size, 0 ); }

  int recv( char* buf, std::size_t size )
  { return ::recv( fd, buf, size, 0 ); }

  void close()
  { ::close( fd ); fd = -1; }
};

int SocketWrapper::connect( const std::string& host, unsigned short port )
{
  struct hostent* h;
  sockaddr_in a;

  a.sin_family = AF_INET;

  if ( proxy.type == "http" || proxy.type == "https" )
  {
    h = gethostbyname( proxy.host.c_str() );
    a.sin_port = htons( proxy.port );
  }
  else
  {
    h = gethostbyname( host.c_str() );
    a.sin_port = htons( port );
  }
  if ( ! h ) return -1;

  if ( ( fd = ::socket( PF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 )
    return -1;

  std::memcpy( &a.sin_addr, h -> h_addr_list[ 0 ], sizeof( a.sin_addr ) );

  return ::connect( fd, reinterpret_cast<const sockaddr*>( &a ), sizeof( a ) );
}

#ifdef SC_USE_OPENSSL
#include <openssl/ssl.h>

struct SSLWrapper
{
  static SSL_CTX* ctx;

  static void init()
  {
    if ( ! ctx )
    {
      SSL_library_init();
      ctx = SSL_CTX_new( SSLv23_client_method() );
      SSL_CTX_set_mode( ctx, SSL_MODE_AUTO_RETRY );
    }
  }

  SSL* s;
  SSLWrapper() : s( SSL_new( ctx ) ) {}

  ~SSLWrapper() { if ( s ) close(); }

  int open( int fd )
  { return ( ! SSL_set_fd( s, fd ) || SSL_connect( s ) <= 0 ) ? -1 : 0; }

  int read( char* buffer, std::size_t size )
  { return SSL_read( s, buffer, size ); }

  int write( const char* buffer, std::size_t size )
  { return SSL_write( s, buffer, size ); }

  void close()
  { SSL_shutdown( s ); SSL_free( s ); s = 0; }
};

SSL_CTX* SSLWrapper::ctx = 0;
#endif

// parse_url ================================================================

struct url_t
{
  std::string protocol;
  std::string host;
  unsigned short port;
  std::string path;

  static bool split( url_t& split, const std::string& url );
};

bool url_t::split( url_t&             split,
                   const std::string& url )
{
  typedef std::string::size_type pos_t;

  pos_t pos = url.find_first_of( ':' );
  if ( pos == url.npos || pos + 2 > url.length() ||
       url[ pos + 1 ] != '/' || url[ pos + 2 ] != '/' )
    return false;
  split.protocol.assign( url, 0, pos );

  pos += 3;
  pos_t end = url.find_first_of( ":/", pos );
  split.host.assign( url, pos, end - pos );
  pos = end;

  if ( split.protocol == "https" )
    split.port = 443;
  else
    split.port = 80;

  split.path = '/';
  if ( pos >= url.length() )
    return true;

  if ( url[ pos ] == ':' )
  {
    ++pos;
    split.port = static_cast<unsigned short>( strtol( &url[ pos ], 0, 10 ) );
    pos = url.find_first_of( '/', pos );
    if ( pos == url.npos )
      return true;
  }

  ++pos;
  split.path.append( url, pos, url.npos );
  return true;
}

// build_request ============================================================

std::string build_request( const url_t&       url,
                                  const std::string& last_modified )
{
  // reference : http://tools.ietf.org/html/rfc2616#page-36
  std::stringstream request;
  bool use_proxy = ( proxy.type == "http" || proxy.type == "https" );

  request << "GET ";

  if ( use_proxy )
  {
    request << url.protocol << "://" << url.host;

    // append port info only if not the standard port
    if ( url.port != 80 )
      request << ':' << url.port;
  }

  request << url.path << " HTTP/1.0\r\n"
          "User-Agent: Firefox/3.0\r\n"
          "Accept: */*\r\n";

  if ( ! use_proxy )
    request << "Host: " << url.host << "\r\n";

  request << cookies;

  if ( ! last_modified.empty() )
    request << "If-Modified-Since: " << last_modified << "\r\n";

  request << "Connection: close\r\n"
          "\r\n";

  return request.str();
}

// download =================================================================

bool download( url_cache_entry_t& entry,
                      const std::string& url )
{
#if defined( SC_MINGW )

  static bool initialized = false;
  if ( ! initialized )
  {
    WSADATA wsa_data;
    WSAStartup( MAKEWORD( 2, 2 ), &wsa_data );
    initialized = true;
  }

#endif

#ifdef SC_USE_OPENSSL
  SSLWrapper::init();
#endif

  std::string current_url = url;
  unsigned int redirect = 0;
  static const unsigned int redirect_max = 8;
  bool ssl_proxy = ( proxy.type == "https" );
  const bool use_proxy = ( ssl_proxy || proxy.type == "http" );

  url_t split_url;
  if ( ! url_t::split( split_url, current_url ) )
    return false;

  // get a page and if we find a redirect update current_url and loop
  while ( true )
  {
    bool use_ssl = ssl_proxy || ( ! use_proxy && ( split_url.protocol == "https" ) );
#ifndef SC_USE_OPENSSL
    if ( use_ssl )
    {
      // FIXME: report unable to use SSL
      return false;
    }
#endif

    SocketWrapper s;
    if ( s.connect( split_url.host, split_url.port ) < 0 )
      return false;

    std::string request = build_request( split_url, entry.last_modified_header );

    std::string result;
    char buffer[ NETBUFSIZE ];

#ifdef SC_USE_OPENSSL
    if ( use_ssl )
    {
      SSLWrapper ssl;

      if ( ssl.open( s ) < 0 )
        return false;

      if ( ssl.write( request.data(), request.size() ) != static_cast<int>( request.size() ) )
        return false;

      while ( true )
      {
        int r = ssl.read( buffer, sizeof( buffer ) );
        if ( r <= 0 )
          break;
        result.append( &buffer[ 0 ], &buffer[ r ] );
      }
    }
    else
#endif
    {
      if ( s.send( request.data(), request.size() ) != static_cast<int>( request.size() ) )
        return false;

      while ( true )
      {
        int r = s.recv( buffer, sizeof( buffer ) );
        if ( r <= 0 )
          break;
        result.append( &buffer[ 0 ], &buffer[ r ] );
      }
    }

    s.close();

    std::string::size_type pos = result.find( "\r\n\r\n" );
    if ( pos == result.npos )
      return false;

    // reference : http://tools.ietf.org/html/rfc2616#section-14.30
    // Checking for redirects via "Location:" in headers, which applies at least
    // to 301 Moved Permanently, 302 Found, 303 See Other, 307 Temporary Redirect
    std::string::size_type pos_location = result.find( "\r\nLocation: " );
    if ( pos_location < pos )
    {
      if ( ++redirect > redirect_max )
        return false;

      pos_location += 12;
      std::string::size_type pos_line_end = result.find( "\r\n", pos_location );

      std::string redirect_url = result.substr( pos_location, pos_line_end - pos_location );
      url_t tmp_split_url;
      if ( url_t::split( tmp_split_url, redirect_url ) )
      {
        // Absolute url, move tmp_split_url to split_url
        split_url = tmp_split_url;

      }
      else
      {
        // Assume Relative URL, just write it into the path part of the already splitted url
        split_url.path = redirect_url;
      }
    }
    else
    {
      entry.validated = cache::era();
      {
        std::string::size_type pos_304 = result.find( "304" );
        std::string::size_type line_end = result.find( "\r\n" );
        if ( pos_304 < line_end )
        {
          // Item is not modified
          return true;
        }
      }

      entry.modified = cache::era();
      entry.last_modified_header.clear();
      std::string::size_type pos_l_m = result.find( "\r\nLast-Modified: " );
      if ( pos_l_m < pos )
      {
        pos_l_m += 17;
        std::string::size_type pos_l_m_end = result.find( "\r\n", pos_l_m );
        if ( pos_l_m_end < pos )
          entry.last_modified_header.assign( result, pos_l_m, pos_l_m_end - pos_l_m );
      }
      entry.result.assign( result, pos + 4, result.npos );
      return true;
    }
  }
}

#endif

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

namespace cache {
std::string get( std::istream& is )
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

void put( std::ostream& os, const std::string& s )
{ os.write( s.c_str(), s.size() + 1 ); }

void put( std::ostream& os, const char* s )
{ os.write( s, std::strlen( s ) + 1 ); }
}

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

    if ( cache::get( file ) != SC_VERSION )
    {
      // invalid version, GTFO
      return;
    }

    std::string content;
    content.reserve( 16 * 1024 );

    while ( ! file.eof() )
    {
      std::string url = cache::get( file );
      std::string last_modified = cache::get( file );

      uint32_t size;
      file.read( reinterpret_cast<char*>( &size ), sizeof( size ) );
      content.resize( size );
      file.read( &content[ 0 ], size );

      url_cache_entry_t& c = url_db[ url ];
      c.result = content;
      c.last_modified_header = last_modified;
      c.modified = c.validated = cache::IN_THE_BEGINNING;
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

    cache::put( file, SC_VERSION );

    for ( url_db_t::const_iterator p = url_db.begin(), e = url_db.end(); p != e; ++p )
    {
      if ( p -> second.validated == cache::INVALID_ERA )
        continue;

      cache::put( file, p -> first );
      cache::put( file, p -> second.last_modified_header );

      uint32_t size = as<uint32_t>( p -> second.result.size() );
      file.write( reinterpret_cast<const char*>( &size ), sizeof( size ) );
      file.write( p -> second.result.data(), size );
    }
  }
  catch ( ... )
  {}
}


// http::get ================================================================

bool http::get( std::string&       result,
                const std::string& url,
                cache::behavior_e  caching,
                const std::string& confirmation )
{
  result.clear();

  std::string encoded_url = url;
  util::urlencode( encoded_url );

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

      if ( entry.validated != cache::INVALID_ERA )
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
           ( entry.validated == cache::INVALID_ERA ||
             ( caching == cache::CURRENT && entry.validated < cache::era() ) ) )
        http_log << " download";
      http_log << "]\n";
    }
  }

  if ( entry.validated < cache::era() && ( caching == cache::CURRENT || entry.validated == cache::INVALID_ERA ) )
  {
    if ( caching == cache::ONLY )
      return false;

    util::printf( "@" ); fflush( stdout );

    if ( ! download( entry, encoded_url ) )
      return false;

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
      return false;
    }
  }

  result = entry.result;
  return true;
}

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

#endif
