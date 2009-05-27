// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// Cross-Platform Support for HTTP-Download =================================

// ==========================================================================
// PLATFORM INDEPENDENT SECTION
// ==========================================================================

namespace { // ANONYMOUS NAMESPACE ==========================================

// get_cache ================================================================

static bool get_cache( const std::string& result,
		       const std::string& url )
{
  return false;
}

// set_cache ================================================================

static void set_cache( const std::string& result,
		       const std::string& url )
{
}

// parse_url ================================================================

static bool parse_url( std::string& host,
		       std::string& path,
		       short&       port,
		       const char*  url )
{
  if( strncmp( url, "http://", 7 ) ) return false;

  char* buffer = (char*) alloca( strlen( url ) + 1 );
  strcpy( buffer, url+7 );

  char* port_start = strstr( buffer, ":" );
  char* path_start = strstr( buffer, "/" );

  if( ! path_start ) return false;

  *path_start = '\0';

  if( port_start )
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
  int64_t now = time( NULL );
  if( ( now - last ) < seconds ) 
  {
    sleep( 1 );
    last = time( NULL );
  }
}

} // ANONYMOUS NAMESPACE ====================================================

// http_t::load_cache =======================================================

bool http_t::load_cache( const std::string& file_name )
{
  thread_t::lock();

  // something interesting

  thread_t::unlock();

  return false;
}

// http_t::save_cache =======================================================

bool http_t::save_cache( const std::string& file_name )
{
  thread_t::lock();

  // something interesting

  thread_t::unlock();

  return false;
}

// http_t::get ==============================================================

bool http_t::get( std::string& result,
		  const std::string& url )
{
  result.clear();

  thread_t::lock();

  if( ! get_cache( result, url ) )
  {
    throttle( 1 );

    if( download( result, url ) )
    {
      set_cache( result, url );
    }
  }

  thread_t::unlock();

  return ! result.empty();
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

#include <windows.h>
#include <wininet.h>

// http_t::download =========================================================

bool http_t::download( std::string& result,
		       const std::string& url )
{
  HINTERNET hINet, hFile;

  hINet = InternetOpen( user_agent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
  if ( ! hINet )  return false;

  char buffer[ 2048 ];
  strcpy( buffer, url.c_str() );

  hFile = InternetOpenUrl( hINet, buffer, NULL, 0, INTERNET_FLAG_RELOAD, 0 );
  if ( ! hFile ) return false;

  result = "";

  DWORD amount;
  while( InternetReadFile( hFile, buffer, sizeof( buffer ), &amount ) )
  {
    if( amount > 0 )
    {
      buffer[ amount ] = '\0';
      result += buffer;
    }
    else break;
  }

  InternetCloseHandle( hFile );
  InternetCloseHandle( hINet );

  return result.size() > 0;
}

#else

// ==========================================================================
// HTTP-DOWNLOAD FOR POSIX COMPLIANT PLATFORMS
// ==========================================================================

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

// http_t::download =========================================================

bool http_t::download( std::string& result,
		       const std::string& url )
{
  std::string host;
  std::string path;
  short port;

  if( ! parse_url( host, path, port, url.c_str() ) ) return false;

  struct hostent* h = gethostbyname( host.c_str() );
  if( ! h ) return false;

  sockaddr_in a;
  a.sin_family = AF_INET;
  a.sin_addr = *(in_addr *)h->h_addr_list[0];
  a.sin_port = htons( port );

  int s;
  s = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  if( s < 0 ) return false;

  int r = ::connect( s, (sockaddr *)&a, sizeof( a ) );
  if( r < 0 )
  {
    close( s );
    return false;
  }
  
  char buffer[2048];
  sprintf( buffer, "GET %s HTTP/1.0\r\nUser-Agent: Firefox/3.0\r\nAccept: */*\r\nHost: %s\r\nConnection: close\r\n\r\n", path.c_str(), host.c_str() );
  r = ::send( s, buffer, int( strlen( buffer ) ), MSG_WAITALL );
  if( r != (int) strlen( buffer ) )
  {
    close( s );
    return false;
  }

  result = "";

  while( 1 )
  {
    r = ::recv( s, buffer, sizeof(buffer)-1, MSG_WAITALL );
    if( r > 0 )
    {
      buffer[ r ] = '\0';
      result += buffer;
    }
    else break;
  }
  
  ::close( s );

  std::string::size_type pos = result.find( "\r\n\r\n" );
  if( pos == result.npos ) 
  {
    result.clear();
    return false;
  }

  result.erase( result.begin(), result.begin() + pos + 4 );
  
  return true;
}

#endif

#ifdef UNIT_TEST

void thread_t::lock() {}
void thread_t::unlock() {}

int main( int argc, char** argv )
{
  std::string result;

  if( http_t::get( result, "http://www.wowarmory.com/character-sheet.xml?r=Llane&n=Pagezero" ) )
  {
    printf( "%s\n", result.c_str() );
  }
  else printf( "Unable to download armory data.\n" );

  if( http_t::get( result, "http://www.wowhead.com/?item=40328&xml" ) )
  {
    printf( "%s\n", result.c_str() );
  }
  else printf( "Unable to download wowhead data.\n" );

  return 0;
}

#endif

