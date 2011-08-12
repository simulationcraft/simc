// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"
#include "utf8.h"

#include <fstream>

#ifdef USE_TR1
#include <unordered_map>
#else
#include <map>
#endif

// Cross-Platform Support for HTTP-Download =================================

// ==========================================================================
// PLATFORM INDEPENDENT SECTION
// ==========================================================================

// http_t::proxy_* ==========================================================

std::string http_t::proxy_type;
std::string http_t::proxy_host;
int         http_t::proxy_port = 0;

cache::cache_control_t cache::cache_control_t::singleton;

// http_t::clear_cache ======================================================

bool http_t::clear_cache( sim_t* sim,
                          const std::string& name,
                          const std::string& value )
{
  assert( name == "http_clear_cache" );
  if ( value != "0" && ! sim -> parent ) http_t::cache_clear();
  return true;
}

namespace { // ANONYMOUS NAMESPACE ==========================================

static const char* const url_cache_file = "simc_cache.dat";
static const double url_cache_version = 3.0;

static void* cache_mutex = 0;

static const unsigned int NETBUFSIZE = 1 << 15;

struct url_cache_entry_t
{
  std::string result;
  cache::era_t era;
  url_cache_entry_t() : era( cache::IN_THE_BEGINNING ) {}
  url_cache_entry_t( const std::string& result_, cache::era_t era_ ) :
    result( result_ ), era( era_ ) {}
};

#ifdef USE_TR1
typedef std::tr1::unordered_map<std::string, url_cache_entry_t> url_db_t;
#else
typedef std::map<std::string, url_cache_entry_t> url_db_t;
#endif // USE_TR1
static url_db_t url_db;

// throttle =================================================================

static void throttle( int seconds )
{
  static int64_t last = 0;
  int64_t now;
  do { now = time( NULL ); }
  while ( ( now - last ) < seconds );
  last = now;
}

// cache_set ================================================================

static void cache_set( const std::string& url,
                       const std::string& result,
                       int era )
{
  // writer lock
  thread_t::auto_lock_t lock( cache_mutex );
  url_cache_entry_t& c = url_db[ url ];
  c.result = result;
  c.era = era;
}

#if defined( NO_HTTP )

// ==========================================================================
// NO HTTP-DOWNLOAD SUPPORT
// ==========================================================================

// download =================================================================

static bool download( std::string& result,
                      const std::string& url )
{
  return false;
}

#elif defined( _MSC_VER )

// ==========================================================================
// HTTP-DOWNLOAD FOR WINDOWS (MS Visual C++ Only)
// ==========================================================================
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <wininet.h>

// download =================================================================

static bool download( std::string& result,
                      const std::string& url )
{
  class InternetWrapper
  {
  public:
    HINTERNET handle;

    explicit InternetWrapper( HINTERNET handle_ ) : handle( handle_ ) {}
    ~InternetWrapper() { if ( handle ) InternetCloseHandle( handle ); }
    operator HINTERNET () const { return handle; }
  };

  result.clear();

  // InternetWrapper hINet( InternetOpen( L"Firefox/3.0", INTERNET_OPEN_TYPE_PROXY, "proxy-server", NULL, 0 ) );
  InternetWrapper hINet( InternetOpen( L"Firefox/3.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 ) );
  if ( ! hINet )
    return false;

  std::wstring wURL( url.length(), L' ' );
  std::copy( url.begin(), url.end(), wURL.begin() );

  std::wstring wHeaders;
  wHeaders += L"Cookie: loginChecked=1\r\n";
  wHeaders += L"Cookie: cookieLangId=en_US\r\n";
  // Skip arenapass 2011 advertisement .. can we please have a sensible
  // API soon?
  wHeaders += L"Cookie: int-WOW=1\r\n";
  wHeaders += L"Cookie: int-WOW-arenapass2011=1\r\n";
  wHeaders += L"Cookie: int-WOW-epic-savings-promo=1\r\n";
  wHeaders += L"Cookie: int-EuropeanInvitational2011=1\r\n";

  InternetWrapper hFile( InternetOpenUrl( hINet, wURL.c_str(), wHeaders.c_str(), 0, INTERNET_FLAG_RELOAD, 0 ) );
  if ( ! hFile )
    return false;

  char buffer[ NETBUFSIZE ];
  DWORD amount;
  while ( InternetReadFile( hFile, buffer, sizeof( buffer ), &amount ) )
  {
    if ( amount == 0 )
      break;
    result.append( &buffer[ 0 ], &buffer[ amount ] );
  }

  return result.size() > 0;
}

#else

// ==========================================================================
// HTTP-DOWNLOAD FOR WINDOWS (MinGW Only) and FOR POSIX COMPLIANT PLATFORMS
// ==========================================================================

#if defined( __MINGW32__ )

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

// parse_url ================================================================

static bool parse_url( std::string&    host,
                       std::string&    path,
                       unsigned short& port,
                       const char*     url )
{
  if ( strncmp( url, "http://", 7 ) ) return false;
  url += 7;

  port = 80;

  host.clear();
  while( *url )
  {
    if ( *url == ':' )
    {
      port = static_cast<unsigned short>( strtol( url + 1, const_cast<char**>( &url ), 10 ) );
      break;
    }

    if ( *url == '/' )
      break;

    host.push_back( *url++ );
  }

  path = '/';

  if ( !*url )
    return true;

  if ( *url++ != '/' )
    return false;

  path.append( url );
  return true;
}

// build_request ============================================================

static std::string build_request( const std::string&   host,
                                  const std::string&   path,
                                  unsigned short port )
{
  // reference : http://tools.ietf.org/html/rfc2616#page-36
  std::stringstream request;
  bool use_proxy = ( http_t::proxy_type == "http" );

  request << "GET ";

  if ( use_proxy )
  {
    request << "http://" << host;

    // append port info only if not the standard port
    if ( port != 80 )
      request << ':' << port;
  }

  request << path << " HTTP/1.0\r\n"
             "User-Agent: Firefox/3.0\r\n"
             "Accept: */*\r\n";

  if ( ! use_proxy )
    request << "Host: " << host << "\r\n";

  request << "Cookie: loginChecked=1\r\n"
             "Cookie: cookieLangId=en_US\r\n"
             // Skip arenapass 2011 advertisement .. can we please have a sensible
             // API soon?
             "Cookie: int-WOW=1\r\n"
             "Cookie: int-WOW-arenapass2011=1\r\n"
             "Cookie: int-WOW-epic-savings-promo=1\r\n"
             "Cookie: int-EuropeanInvitational2011=1\r\n"
             "Connection: close\r\n"
             "\r\n";

  return request.str();
}

// download =================================================================

static bool download( std::string& result,
                      const std::string& url )
{

#if defined( __MINGW32__ )

  static bool initialized = false;
  if ( ! initialized )
  {
    WSADATA wsa_data;
    WSAStartup( MAKEWORD( 2,2 ), &wsa_data );
    initialized = true;
  }

#endif

  std::string current_url = url;
  std::string host;
  std::string path;
  unsigned short port;
  int redirect = 0, redirect_max = 5;

  // get a page and if we find a redirect update current_url and loop
  while ( redirect < redirect_max )
  {
    if ( ! parse_url( host, path, port, current_url.c_str() ) ) return false;

    std::string request = build_request( host, path, port );

    struct hostent* h;
    sockaddr_in a;

    if ( http_t::proxy_type == "http" )
    {
      h = gethostbyname( http_t::proxy_host.c_str() );
      a.sin_port = htons( http_t::proxy_port );
    }
    else
    {
      h = gethostbyname( host.c_str() );
      a.sin_port = htons( port );
    }

    if ( ! h ) return false;
    a.sin_family = AF_INET;
    a.sin_addr = *( in_addr * )h -> h_addr_list[0];

    int s = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( s < 0 ) return false;

    if ( ::connect( s, ( sockaddr * )&a, sizeof( a ) ) < 0 )
    {
      ::close( s );
      return false;
    }

    if ( ::send( s, request.c_str(), request.size(), 0 ) != static_cast<int>( request.size() ) )
    {
      ::close( s );
      return false;
    }

    result.clear();
    char buffer[ NETBUFSIZE ];
    while ( true )
    {
      int r = ::recv( s, buffer, sizeof( buffer ), 0 );
      if ( r <= 0 )
        break;
      result.append( &buffer[ 0 ], &buffer[ r ] );
    }

    ::close( s );

    std::string::size_type pos = result.find( "\r\n\r\n" );
    if ( pos == result.npos )
    {
      result.clear();
      return false;
    }

    // reference : http://tools.ietf.org/html/rfc2616#section-14.30
    // Checking for redirects via "Location:" in headers, which applies at least
    // to 301 Moved Permanently, 302 Found, 303 See Other, 307 Temporary Redirect
    std::string::size_type pos_location = result.find( "Location: " );
    if ( pos_location != result.npos && pos_location < pos )
    {
      if ( redirect < redirect_max )
      {
        std::string::size_type pos_line_end = result.find( "\r\n", pos_location + 10 );
        current_url = result.substr( pos_location + 10, pos_line_end - ( pos_location + 10 ) );
        redirect ++;
      }
      else return false;
    }
    else
    {
      result.erase( 0, pos + 4 );
      return true;
    }
  }
  return false;
}

#endif

} // ANONYMOUS NAMESPACE ====================================================

// http_t::cache_load =======================================================

void http_t::cache_load()
{
  try
  {
    std::ifstream file( url_cache_file, std::ios::binary );
    if ( !file ) return;
    file.exceptions( std::ios::eofbit | std::ios::failbit | std::ios::badbit );
    file.unsetf( std::ios::skipws );

    double version;
    file.read( reinterpret_cast<char*>( &version ) , sizeof( version ) );
    if ( version != url_cache_version )
      return;

    std::string url, result;

    while ( ! file.eof() )
    {
      uint32_t size;

      file.read( reinterpret_cast<char*>( &size ), sizeof( size ) );
      url.resize( size );
      file.read( &url[ 0 ], size );

      file.read( reinterpret_cast<char*>( &size ), sizeof( size ) );
      result.resize( size );
      file.read( &result[ 0 ], size );

      cache_set( url, result, 0 );
    }
  }
  catch( ... )
  {}
}

// http_t::cache_save =======================================================

void http_t::cache_save()
{
  try
  {
    std::ofstream file( url_cache_file, std::ios::binary );
    if ( ! file ) return;
    file.exceptions( std::ios::eofbit | std::ios::failbit | std::ios::badbit );

    file.write( reinterpret_cast<const char*>( &url_cache_version ), sizeof( url_cache_version ) );

    for ( url_db_t::const_iterator p = url_db.begin(), e = url_db.end(); p != e; ++p )
    {
      uint32_t size = p -> first.size();
      file.write( reinterpret_cast<const char*>( &size ), sizeof( size ) );
      file.write( p -> first.data(), size );

      size = p -> second.result.size();
      file.write( reinterpret_cast<const char*>( &size ), sizeof( size ) );
      file.write( p -> second.result.data(), size );
    }
  }
  catch ( ... )
  {}
}

// http_t::cache_clear ======================================================

void http_t::cache_clear()
{
  // writer lock
  thread_t::auto_lock_t lock( cache_mutex );
  url_db.clear();
}


// http_t::get ==============================================================

bool http_t::get( std::string&       result,
                  const std::string& url,
                  const std::string& confirmation,
                  cache::behavior_t  caching,
                  int                throttle_seconds )
{
  result.clear();

  static void* mutex = 0;
  thread_t::auto_lock_t lock( mutex );

  if ( false )
  {
    std::ofstream http_log( "simc_http_log.txt", std::ios::app );
    std::ostream::sentry s( http_log );
    if ( s )
    {
      http_log << cache::era() << ": get(\"" << url << "\") = ";

      thread_t::auto_lock_t lock( cache_mutex );
      url_db_t::const_iterator p = url_db.find( url );
      if ( p != url_db.end() )
      {
        if ( p -> second.era >= cache::era() )
          http_log << "hot";
        else if ( caching != cache::CURRENT )
          http_log << "warm";
        else
          http_log << "cold";
        http_log << ": " << p -> second.era;
      } else
        http_log << "miss";
      if ( caching != cache::ONLY && ( p == url_db.end() || ( caching == cache::CURRENT && p -> second.era < cache::era() ) ) )
        http_log << " [download]";
      http_log << '\n';
    }
  }

  {
    // reader lock
    thread_t::auto_lock_t lock( cache_mutex );

    url_db_t::const_iterator p = url_db.find( url );
    if ( p != url_db.end() && ( ( caching != cache::CURRENT ) || p -> second.era >= cache::era() ) )
    {
      result = p -> second.result;
      return true;
    }
  }

  if ( caching == cache::ONLY )
    return false;

  util_t::printf( "@" ); fflush( stdout );
  throttle( throttle_seconds );

  std::string encoded_url;
  if ( ! download( result, format( encoded_url, url ) ) )
    return false;

  if ( confirmation.size() && ( result.find( confirmation ) == std::string::npos ) )
  {
    //util_t::printf( "\nsimulationcraft: HTTP failed on '%s'\n", url.c_str() );
    //util_t::printf( "%s\n", ( result.empty() ? "empty" : result.c_str() ) );
    //fflush( stdout );
    return false;
  }

  cache_set( url, result, cache::era() );

  return true;
}

// http_t::format ===========================================================

void http_t::format_( std::string& encoded_url,
                      const std::string& url )
{
  encoded_url = url;
  util_t::urlencode( util_t::str_to_utf8( encoded_url ) );
}

#ifdef UNIT_TEST

void thread_t::mutex_lock( void*& mutex ) {}
void thread_t::mutex_unlock( void*& mutex ) {}

int main( int argc, char** argv )
{
  std::string result;

  if ( http_t::get( result, "http://us.battle.net/wow/en/character/llane/pagezero/advanced" ) )
  {
    util_t::printf( "%s\n", result.c_str() );
  }
  else util_t::printf( "Unable to download armory data.\n" );


  if ( http_t::get( result, "http://www.wowhead.com/item=40328&xml" ) )
  {
    util_t::printf( "%s\n", result.c_str() );
  }
  else util_t::printf( "Unable to download wowhead data.\n" );

  return 0;
}

#endif
