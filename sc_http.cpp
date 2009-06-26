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

static const char*  url_cache_file = "url_cache.dat";
static const double url_cache_version = 1.0;
static const uint32_t expiration_seconds = 12 * 60 * 60;

static void* cache_mutex = 0;

struct url_cache_t
{
  std::string url;
  std::string result;
  uint32_t timestamp;
  url_cache_t() : timestamp(0) {}
  url_cache_t( const std::string& u, const std::string& r, uint32_t t ) : url(u), result(r), timestamp(t) {}
};

static std::vector<url_cache_t> url_cache_db;

// parse_url ================================================================

static bool parse_url( std::string& host,
                       std::string& path,
                       short&       port,
                       const char*  url )
{
  if( strncmp( url, "http://", 7 ) ) return false;

  char buffer[ 2048 ];
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
  int64_t now;
  do { now = time( NULL ); } while( ( now - last ) < seconds );
  last = now;
}

} // ANONYMOUS NAMESPACE ====================================================

// http_t::cache_load =======================================================

bool http_t::cache_load()
{
  FILE* file = fopen( url_cache_file, "rb" );

  if( file )
  {
    double version;
    uint32_t num_records, max_size;

    if( fread( &version,     sizeof( double   ), 1, file ) &&
        fread( &num_records, sizeof( uint32_t ), 1, file ) &&
        fread( &max_size,    sizeof( uint32_t ), 1, file ) )
    {
      if( version == url_cache_version )
      {
        std::string url, result;
        char* buffer = new char[ max_size+1 ];
    
        for( unsigned i=0; i < num_records; i++ )
        {
          uint32_t timestamp, url_size, result_size;
          
          if( fread( &timestamp,   sizeof( uint32_t ), 1, file ) &&
              fread( &url_size,    sizeof( uint32_t ), 1, file ) &&
              fread( &result_size, sizeof( uint32_t ), 1, file ) )
          {
            assert( url_size > 0 && result_size > 0 );

            if( fread( buffer, sizeof( char ), url_size, file ) )
            {
              buffer[ url_size ] = '\0';
              url = buffer;
            }
            else break;

            if( fread( buffer, sizeof( char ), result_size, file ) )
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
  if( file )
  {
    uint32_t num_records = url_cache_db.size();
    uint32_t max_size=0;

    for( unsigned i=0; i < num_records; i++ )
    {
      url_cache_t& c = url_cache_db[ i ];
      uint32_t size = std::max( c.url.size(), c.result.size() );
      if( size > max_size ) max_size = size;
    }

    if(sizeof(double) != fwrite( &url_cache_version, sizeof( double   ), 1, file ) ||
    	sizeof(uint32_t) != fwrite( &num_records,       sizeof( uint32_t ), 1, file ) ||
    	sizeof(uint32_t) != fwrite( &max_size,          sizeof( uint32_t ), 1, file ))
    {
    	perror("fwrite failed while saving cache file"); 
    }

    for( unsigned i=0; i < num_records; i++ )
    {
      url_cache_t& c = url_cache_db[ i ];
      
      uint32_t    url_size = c.   url.size();
      uint32_t result_size = c.result.size();

      if(
      	sizeof(uint32_t) != fwrite( &( c.timestamp ), sizeof( uint32_t ), 1, file ) ||
      	sizeof(uint32_t) != fwrite(    &url_size, sizeof( uint32_t ), 1, file ) ||
      	sizeof(uint32_t) != fwrite( &result_size, sizeof( uint32_t ), 1, file ) ||

      	sizeof(char)*url_size != fwrite( c.   url.c_str(), sizeof( char ),    url_size, file ) ||
      	sizeof(char)*result_size != fwrite( c.result.c_str(), sizeof( char ), result_size, file ))
      {
			perror("fwrite failed while saving cache file"); 
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

bool http_t::cache_get( std::string& result,
                        const std::string& url,
                        bool force )
{
  thread_t::mutex_lock( cache_mutex );

  uint32_t now = (uint32_t) time( NULL );
  bool success = false;

  int num_records = url_cache_db.size();
  for( int i=0; i < num_records && ! success; i++ )
  {
    url_cache_t& c = url_cache_db[ i ];

    if( url == c.url )
    {
      if( ! force )
        if( ( now - c.timestamp ) > expiration_seconds ) 
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
                        uint32_t           timestamp )
{
  thread_t::mutex_lock( cache_mutex );

  if( timestamp == 0 ) timestamp = (uint32_t) time( NULL );

  int num_records = url_cache_db.size();
  bool success = false;

  for( int i=0; i < num_records; i++ )
  {
    url_cache_t& c = url_cache_db[ i ];

    if( url == c.url )
    {
      c.url = url;
      c.result = result;
      c.timestamp = timestamp;
      success = true;
      break;
    }
  }

  if( ! success ) url_cache_db.push_back( url_cache_t( url, result, timestamp ) );

  thread_t::mutex_unlock( cache_mutex );
}

// http_t::get ==============================================================

bool http_t::get( std::string& result,
                  const std::string& url,
                  const std::string& confirmation,
                  int throttle_seconds )
{
  static void* mutex = 0;

  thread_t::mutex_lock( mutex );

  result.clear();

  bool success = cache_get( result, url, false );

  if( ! success )
  {
    printf( "@" ); fflush( stdout );
    throttle( throttle_seconds );
    success = download( result, url );

    if( success )
    {
      if( confirmation.size() > 0 && ( result.find( confirmation ) == std::string::npos ) )
      {
        printf( "X" ); fflush( stdout );
        success = cache_get( result, url, true );
      }
      else
      {
        cache_set( url, result, 0 );
      }
    }
  }

  thread_t::mutex_unlock( mutex );

  return success;
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
  if( ! initialized )
  {
    WSADATA wsa_data; 
    WSAStartup( MAKEWORD(2,2), &wsa_data );
    initialized = true;
  }

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

  uint32_t s;
  s = ::socket( AF_INET, SOCK_STREAM, getprotobyname( "tcp" ) -> p_proto );
  if( s == 0xffffffffU ) return false;

  int r = ::connect( s, (sockaddr *)&a, sizeof( a ) );
  if( r < 0 )
  {
    return false;
  }
  
  char buffer[2048];
  sprintf( buffer, "GET %s HTTP/1.0\r\nUser-Agent: Firefox/3.0\r\nAccept: */*\r\nHost: %s\r\nConnection: close\r\n\r\n", path.c_str(), host.c_str() );
  r = ::send( s, buffer, int( strlen( buffer ) ), 0 );
  if( r != (int) strlen( buffer ) )
  {
    return false;
  }

  result = "";

  while( 1 )
  {
    r = ::recv( s, buffer, sizeof(buffer)-1, 0 );
    if( r > 0 )
    {
      buffer[ r ] = '\0';
      result += buffer;
    }
    else break;
  }
  
  std::string::size_type pos = result.find( "\r\n\r\n" );
  if( pos == result.npos ) 
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
  result = "";
  HINTERNET hINet, hFile;
  hINet = InternetOpen( L"Firefox/3.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
  if( hINet )
  {
    std::wstring wURL( url.length(), L' ' );
    std::copy( url.begin(), url.end(), wURL.begin() );
    hFile = InternetOpenUrl( hINet, wURL.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0 );
    if ( hFile )
    {
      char buffer[ 20000 ];
      DWORD amount;
      while( InternetReadFile( hFile, buffer, sizeof(buffer)-2, &amount ) )
      {
        if( amount > 0 )
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
  http_t::load_cache();

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

  http_t::save_cache();

  return 0;
}

#endif

