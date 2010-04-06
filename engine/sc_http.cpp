// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// Cross-Platform Support for HTTP-Download =================================

// ==========================================================================
// PLATFORM INDEPENDENT SECTION
// ==========================================================================

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

static bool parse_url( std::string& host,
                       std::string& path,
                       short&       port,
                       const char*  url )
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
            assert( url_size > 0 && result_size > 0 );

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

  int size = ( int ) url.size();
  for ( int i=0; i < size; i++ )
  {
    char c = url[ i ];

    if ( c == '+' || c == ' ' )
    {
      encoded_url += "%20";
    }
    else if ( c == '\'' )
    {
      encoded_url += "%27";
    }
    else if ( ( unsigned char ) c > 127 )
    {
      std::string temp = "";
      temp += c;
      util_t::ascii_binary_to_utf8_hex( temp );
      encoded_url += temp;
    }
    else encoded_url += c;
  }

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

#elif defined( __MINGW32__ )

// ==========================================================================
// HTTP-DOWNLOAD FOR WINDOWS (MinGW Only)
// ==========================================================================

#include <windows.h>
#include <wininet.h>
#include <Winsock2.h>

// http_t::download =========================================================

bool http_t::download( std::string& result,
                       const std::string& url )
{
  static bool initialized = false;
  if ( ! initialized )
  {
    WSADATA wsa_data;
    WSAStartup( MAKEWORD( 2,2 ), &wsa_data );
    initialized = true;
  }

  std::string host;
  std::string path;
  short port;

  if ( ! parse_url( host, path, port, url.c_str() ) ) return false;

  struct hostent* h = gethostbyname( host.c_str() );
  if ( ! h ) return false;

  sockaddr_in a;
  a.sin_family = AF_INET;
  a.sin_addr = *( in_addr * )h->h_addr_list[0];
  a.sin_port = htons( port );

  uint32_t s;
  s = ::socket( AF_INET, SOCK_STREAM, getprotobyname( "tcp" ) -> p_proto );
  if ( s == 0xffffffffU ) return false;

  int r = ::connect( s, ( sockaddr * )&a, sizeof( a ) );
  if ( r < 0 )
  {
    return false;
  }

  char buffer[2048];
  sprintf( buffer, 
	   "GET %s HTTP/1.0\r\n"
	   "User-Agent: Firefox/3.0\r\n"
	   "Accept: */*\r\n"
	   "Host: %s\r\n"
	   "Cookie: loginChecked=1\r\n"
	   "Cookie: cookieLangId=en_US\r\n"
	   "Connection: close\r\n"
	   "\r\n", 
	   path.c_str(), 
	   host.c_str() );

  r = ::send( s, buffer, int( strlen( buffer ) ), 0 );
  if ( r != ( int ) strlen( buffer ) )
  {
    return false;
  }

  result = "";

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

  std::string::size_type pos = result.find( "\r\n\r\n" );
  if ( pos == result.npos )
  {
    result.clear();
    return false;
  }

  result.erase( result.begin(), result.begin() + pos + 4 );

  return true;
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
// HTTP-DOWNLOAD FOR POSIX COMPLIANT PLATFORMS
// ==========================================================================

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

// http_t::download =========================================================

bool http_t::download( std::string& result,
                       const std::string& url )
{
  std::string host;
  std::string path;
  short port;

  if ( ! parse_url( host, path, port, url.c_str() ) ) return false;

  struct hostent* h = gethostbyname( host.c_str() );
  if ( ! h ) return false;

  sockaddr_in a;
  a.sin_family = AF_INET;
  a.sin_addr = *( in_addr * )h->h_addr_list[0];
  a.sin_port = htons( port );

  int s;
  s = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  if ( s < 0 ) return false;

  int r = ::connect( s, ( sockaddr * )&a, sizeof( a ) );
  if ( r < 0 )
  {
    close( s );
    return false;
  }

  char buffer[2048];
  sprintf( buffer, 
	   "GET %s HTTP/1.0\r\n"
	   "User-Agent: Firefox/3.0\r\n"
	   "Accept: */*\r\n"
	   "Host: %s\r\n"
	   "Cookie: loginChecked=1\r\n"
	   "Cookie: cookieLangId=en_US\r\n"
	   "Connection: close\r\n"
	   "\r\n", 
	   path.c_str(), 
	   host.c_str() );

  r = ::send( s, buffer, int( strlen( buffer ) ), MSG_WAITALL );
  if ( r != ( int ) strlen( buffer ) )
  {
    close( s );
    return false;
  }

  result = "";

  while ( 1 )
  {
    r = ::recv( s, buffer, sizeof( buffer )-1, MSG_WAITALL );
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

  result.erase( result.begin(), result.begin() + pos + 4 );

  return true;
}

#endif

#ifdef UNIT_TEST

void thread_t::mutex_lock( void*& mutex ) {}
void thread_t::mutex_unlock( void*& mutex ) {}

int main( int argc, char** argv )
{
  std::string result;

  if ( http_t::get( result, "http://www.wowarmory.com/character-sheet.xml?r=Llane&n=Pagezero" ) )
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

