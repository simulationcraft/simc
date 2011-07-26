// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"
#include "utf8.h"

// Cross-Platform Support for HTTP-Download =================================

// ==========================================================================
// PLATFORM INDEPENDENT SECTION
// ==========================================================================

// http_t::proxy_* ==========================================================

std::string http_t::proxy_type = "none";
std::string http_t::proxy_host = "";
int         http_t::proxy_port = 0;

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

static const char*  url_cache_file = "simc_cache.dat";
static const double url_cache_version = 2.0;

static void* cache_mutex = 0;

struct url_cache_t
{
  std::string url;
  std::string result;
  int64_t timestamp;
  url_cache_t() : timestamp( 0 ) {}
  url_cache_t( const std::string& u, const std::string& r, int64_t t ) : url( u ), result( r ), timestamp( t ) {}
};

static std::vector<url_cache_t> url_cache_db;

// parse_url ================================================================

static bool parse_url( std::string&    host,
                       std::string&    path,
                       unsigned short& port,
                       const char*     url )
{
  if ( strncmp( url, "http://", 7 ) ) return false;

  char buffer[ 2048 ];
  strcpy( buffer, url+7 );

  char* port_start = strstr( buffer, ":" );
  char* path_start = strstr( buffer, "/" );

  if ( ! path_start ) return false;

  *path_start = '\0';

  if ( port_start )
  {
    *port_start = '\0';
    port = atoi( port_start + 1 );
  }
  else port = 80;

  host = buffer;

  path = "/";
  path += ( path_start + 1 );

  return true;
}

// build_request ============================================================

static std::string build_request( std::string&   host,
                                  std::string&   path,
                                  unsigned short port )
{
  // reference : http://tools.ietf.org/html/rfc2616#page-36
  char buffer[2048];

  if ( http_t::proxy_type == "http" )
  {
    // append port info only if not the standard port
    char portbuff[7] = "\0";
    if ( port != 80 )
    {
      snprintf( portbuff, sizeof( portbuff ), ":%i", port );
    }
    // building a proxified request : absoluteURI without Host header
    snprintf( buffer, sizeof( buffer ),
              "GET http://%s%s%s HTTP/1.0\r\n"
              "User-Agent: Firefox/3.0\r\n"
              "Accept: */*\r\n"
              "Cookie: loginChecked=1\r\n"
              "Cookie: cookieLangId=en_US\r\n"
              // Skip arenapass 2011 advertisement .. can we please have a sensible
              // API soon?
              "Cookie: int-WOW=1\r\n"
              "Cookie: int-WOW-arenapass2011=1\r\n"
              "Cookie: int-WOW-epic-savings-promo=1\r\n"
              "Connection: close\r\n"
              "\r\n",
              host.c_str(),
              portbuff,
              path.c_str() );
  }
  else
  {
    // building a direct request : using abs_path and Host header
    snprintf( buffer, sizeof( buffer ),
              "GET %s HTTP/1.0\r\n"
              "User-Agent: Firefox/3.0\r\n"
              "Accept: */*\r\n"
              "Host: %s\r\n"
              "Cookie: loginChecked=1\r\n"
              "Cookie: cookieLangId=en_US\r\n"
              // Skip arenapass 2011 advertisement .. can we please have a sensible
              // API soon?
              "Cookie: int-WOW=1\r\n"
              "Cookie: int-WOW-arenapass2011=1\r\n"
              "Cookie: int-WOW-epic-savings-promo=1\r\n"
              "Connection: close\r\n"
              "\r\n",
              path.c_str(),
              host.c_str() );
  }
  return std::string( buffer );
}

// throttle =================================================================

static void throttle( int seconds )
{
  static int64_t last = 0;
  int64_t now;
  do { now = time( NULL ); }
  while ( ( now - last ) < seconds );
  last = now;
}

} // ANONYMOUS NAMESPACE ====================================================

// http_t::cache_load =======================================================

bool http_t::cache_load()
{
  FILE* file = fopen( url_cache_file, "rb" );

  if ( file )
  {
    double version;
    uint32_t num_records, max_size;

    if ( fread( &version,     sizeof( double   ), 1, file ) &&
         fread( &num_records, sizeof( uint32_t ), 1, file ) &&
         fread( &max_size,    sizeof( uint32_t ), 1, file ) )
    {
      if ( version == url_cache_version )
      {
        std::string url, result;
        char* buffer = new char[ max_size+1 ];

        for ( unsigned i=0; i < num_records; i++ )
        {
          int64_t timestamp;
          uint32_t url_size, result_size;

          if ( fread( &timestamp,   sizeof(  int64_t ), 1, file ) &&
               fread( &url_size,    sizeof( uint32_t ), 1, file ) &&
               fread( &result_size, sizeof( uint32_t ), 1, file ) )
          {
            assert( url_size > 0 && url_size <= max_size );
            assert( result_size > 0 && result_size <= max_size );

            if ( fread( buffer, sizeof( char ), url_size, file ) )
            {
              buffer[ url_size ] = '\0';
              url = buffer;
            }
            else break;

            if ( fread( buffer, sizeof( char ), result_size, file ) )
            {
              buffer[ result_size ] = '\0';
              result = buffer;
            }
            else break;

            cache_set( url, result, timestamp );
          }
          else break;
        }
        delete[] buffer;
      }
    }
    fclose( file );
  }

  return true;
}

// http_t::cache_save =======================================================

bool http_t::cache_save()
{
  FILE* file = fopen( url_cache_file, "wb" );
  if ( file )
  {
    uint32_t num_records = ( uint32_t ) url_cache_db.size();
    uint32_t max_size=0;

    for ( unsigned i=0; i < num_records; i++ )
    {
      url_cache_t& c = url_cache_db[ i ];
      uint32_t size = ( uint32_t ) std::max( c.url.size(), c.result.size() );
      if ( size > max_size ) max_size = size;
    }

    if ( 1 != fwrite( &url_cache_version, sizeof( double   ), 1, file ) ||
         1 != fwrite( &num_records,       sizeof( uint32_t ), 1, file ) ||
         1 != fwrite( &max_size,          sizeof( uint32_t ), 1, file ) )
    {
      perror( "fwrite failed while saving cache file" );
    }

    for ( unsigned i=0; i < num_records; i++ )
    {
      url_cache_t& c = url_cache_db[ i ];

      uint32_t    url_size = ( uint32_t ) c.   url.size();
      uint32_t result_size = ( uint32_t ) c.result.size();

      if ( 1 != fwrite( &( c.timestamp ), sizeof(  int64_t ), 1, file ) ||
           1 != fwrite( &url_size,        sizeof( uint32_t ), 1, file ) ||
           1 != fwrite( &result_size,     sizeof( uint32_t ), 1, file ) ||
           url_size    != fwrite( c.   url.c_str(), sizeof( char ),    url_size, file ) ||
           result_size != fwrite( c.result.c_str(), sizeof( char ), result_size, file ) )
      {
        perror( "fwrite failed while saving cache file" );
      }
    }

    fclose( file );
  }

  return true;
}

// http_t::cache_clear ======================================================

void http_t::cache_clear()
{
  thread_t::mutex_lock( cache_mutex );
  url_cache_db.clear();
  thread_t::mutex_unlock( cache_mutex );
}


// http_t::cache_get ========================================================

bool http_t::cache_get( std::string&       result,
                        const std::string& url,
                        int64_t            timestamp )
{
  if ( timestamp < 0 ) return false;

  thread_t::mutex_lock( cache_mutex );

  bool success = false;

  int num_records = ( int ) url_cache_db.size();
  for ( int i=0; i < num_records && ! success; i++ )
  {
    url_cache_t& c = url_cache_db[ i ];

    if ( url == c.url )
    {
      if ( timestamp > 0 )
        if ( timestamp > c.timestamp )
          break;

      result = c.result;
      success = true;
    }
  }

  thread_t::mutex_unlock( cache_mutex );

  return success;
}

// http_t::cache_set ========================================================

void http_t::cache_set( const std::string& url,
                        const std::string& result,
                        int64_t            timestamp )
{
  thread_t::mutex_lock( cache_mutex );

  int num_records = ( int ) url_cache_db.size();
  bool success = false;

  for ( int i=0; i < num_records; i++ )
  {
    url_cache_t& c = url_cache_db[ i ];

    if ( url == c.url )
    {
      c.url = url;
      c.result = result;
      c.timestamp = timestamp;
      success = true;
      break;
    }
  }

  if ( ! success ) url_cache_db.push_back( url_cache_t( url, result, timestamp ) );

  thread_t::mutex_unlock( cache_mutex );
}

// http_t::get ==============================================================

bool http_t::get( std::string& result,
                  const std::string& url,
                  const std::string& confirmation,
                  int64_t timestamp,
                  int throttle_seconds )
{
  static void* mutex = 0;

  thread_t::mutex_lock( mutex );

  result.clear();

  bool success = cache_get( result, url, timestamp );

  if ( ! success )
  {
    util_t::printf( "@" ); fflush( stdout );
    throttle( throttle_seconds );
    std::string encoded_url;
    success = download( result, format( encoded_url, url ) );

    if ( success )
    {
      if ( confirmation.size() > 0 && ( result.find( confirmation ) == std::string::npos ) )
      {
        //util_t::printf( "\nsimulationcraft: HTTP failed on '%s'\n", url.c_str() );
        //util_t::printf( "%s\n", ( result.empty() ? "empty" : result.c_str() ) );
        fflush( stdout );
        success = false;
      }
      else
      {
        cache_set( url, result, timestamp );
      }
    }
  }

  thread_t::mutex_unlock( mutex );

  return success;
}

// http_t::format ===========================================================

std::string& http_t::format( std::string& encoded_url,
                             const std::string& url )
{
  encoded_url.clear();
  bool is_utf8 = utf8::is_valid( url.begin(), url.end() );
  encoded_url = url;

  if ( is_utf8 )
    util_t::urlencode( encoded_url );
  else
    util_t::urlencode( util_t::str_to_utf8( encoded_url ) );

  return encoded_url;
}

#if defined( NO_HTTP )

// ==========================================================================
// NO HTTP-DOWNLOAD SUPPORT
// ==========================================================================

// http_t::download =========================================================

bool http_t::download( std::string& result,
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

// http_t::download =========================================================

bool http_t::download( std::string& result,
                       const std::string& url )
{
  // hINet = InternetOpen( L"Firefox/3.0", INTERNET_OPEN_TYPE_PROXY, "proxy-server", NULL, 0 );

  result = "";
  HINTERNET hINet, hFile;
  hINet = InternetOpen( L"Firefox/3.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
  if ( hINet )
  {
    std::wstring wURL( url.length(), L' ' );
    std::copy( url.begin(), url.end(), wURL.begin() );

    std::wstring wHeaders = L"";
    wHeaders += L"Cookie: loginChecked=1\r\n";
    wHeaders += L"Cookie: cookieLangId=en_US\r\n";
    // Skip arenapass 2011 advertisement .. can we please have a sensible
    // API soon?
    wHeaders += L"Cookie: int-WOW=1\r\n";
    wHeaders += L"Cookie: int-WOW-arenapass2011=1\r\n";
    wHeaders += L"Cookie: int-WOW-epic-savings-promo=1\r\n";

    hFile = InternetOpenUrl( hINet, wURL.c_str(), wHeaders.c_str(), 0, INTERNET_FLAG_RELOAD, 0 );
    if ( hFile )
    {
      char buffer[ 20000 ];
      DWORD amount;
      while ( InternetReadFile( hFile, buffer, sizeof( buffer )-2, &amount ) )
      {
        if ( amount > 0 )
        {
          buffer[ amount ] = '\0';
          result += buffer;
        }
        else break;
      }
      InternetCloseHandle( hFile );
    }
    InternetCloseHandle( hINet );
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

// http_t::download =========================================================

bool http_t::download( std::string& result,
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

    if ( proxy_type == "http" )
    {
      h = gethostbyname( proxy_host.c_str() );
      a.sin_port = htons( proxy_port );
    }
    else
    {
      h = gethostbyname( host.c_str() );
      a.sin_port = htons( port );
    }

    if ( ! h ) return false;
    a.sin_family = AF_INET;
    a.sin_addr = *( in_addr * )h->h_addr_list[0];

    int s;
    s = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( s < 0 ) return false;

    int r = ::connect( s, ( sockaddr * )&a, sizeof( a ) );
    if ( r < 0 )
    {
      ::close( s );
      return false;
    }

    r = ::send( s, request.c_str(), request.size(), 0 );
    if ( r != ( int ) request.size() )
    {
      ::close( s );
      return false;
    }

    result = "";
    char buffer[2048];
    while ( 1 )
    {
      r = ::recv( s, buffer, sizeof( buffer )-1, 0 );
      if ( r > 0 )
      {
        buffer[ r ] = '\0';
        result += buffer;
      }
      else break;
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
      result.erase( result.begin(), result.begin() + pos + 4 );
      return true;
    }
  }
  return false;
}

#endif

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


  if ( http_t::get( result, "http://www.wowhead.com/?item=40328&xml" ) )
  {
    util_t::printf( "%s\n", result.c_str() );
  }
  else util_t::printf( "Unable to download wowhead data.\n" );

  return 0;
}

#endif
