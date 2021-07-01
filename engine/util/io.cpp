// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "io.hpp"
#include "utf8-cpp/utf8.h"
#include "gsl-lite/gsl-lite.hpp"
#include <cassert>
#include <cstring>
#include <cstdarg>

#ifdef SC_WINDOWS
#include <windows.h>
#include <shellapi.h>
#endif

namespace io { // ===========================================================

namespace { // anonymous namespace ==========================================

std::wstring _widen( gsl::cstring_span s )
{
  std::wstring result;
  result.reserve( s.size() / 2 + 1);

  if ( sizeof( wchar_t ) == 2 )
    utf8::utf8to16( s.begin(), s.end(), std::back_inserter( result ) );
  else
  {
    assert( sizeof( wchar_t ) == 4 );
    utf8::utf8to32( s.begin(), s.end(), std::back_inserter( result ) );
  }

  return result;
}

std::string _narrow( gsl::cwstring_span s )
{
  std::string result;
  result.reserve(s.size() * 2 );

  utf8::utf16to8( s.begin(), s.end(), std::back_inserter( result ) );
  return result;
}

#if defined( SC_MINGW )
bool contains_non_ascii( const std::string& s )
{
  for (const auto & elem : s)
  {
    if ( elem < 0 || ! std::isprint( elem ) )
      return true;
  }

  return false;
}
#endif

} // anonymous namespace ====================================================

std::string narrow( const wchar_t* wstr )
{ return _narrow( wstr ); }

std::wstring widen( const char* str )
{ return _widen( str ); }

std::wstring widen( const std::string& str )
{ return _widen( str ); }

std::string maybe_latin1_to_utf8( util::string_view str )
{
  std::string result;

  try
  {
    // Validate input as UTF-8 while we copy.
    for ( auto first = str.begin(), last = str.end(); first != last ; )
      utf8::append( utf8::next( first, last ), std::back_inserter( result ) );
  }
  catch ( utf8::exception& )
  {
    // We hit something invalid. Start over, treating the whole string as Latin-1.
    result.clear();
    for (const auto & elem : str)
      utf8::append( static_cast<unsigned char>( elem ), std::back_inserter( result ) );
  }

  return result;
}

// MinGW 4.6+ now properly supports _wfopen as well
#if defined(SC_WINDOWS)
FILE* fopen( const std::string& filename, const char* mode )
{
  return _wfopen( widen( filename ).c_str(), widen( mode ).c_str() );
}
#endif

void ofstream::open( const char* name, openmode mode )
{
#ifdef _MSC_VER
  // This is intentionally "_MSC_VER" instead of "SC_WINDOWS".
  // MS's c++ std library provides
  //   std::fstream::open( const wchar_t*, openmode )
  // as an extension to open files with Unicode names.
  // The MinGW c++ std library does not. Since neither
  // library will accept UTF-8 encoded names, there is
  // no way to open a file with a Unicode filename using
  // std::fstream in MinGW.
  std::ofstream::open( io::widen( name ).c_str(), mode );
#elif defined( SC_MINGW )
  if ( contains_non_ascii( name ) )
  {
    throw std::invalid_argument(fmt::format("File '{}' with non-ascii characters cannot be opened when built with MinGW.", name));
  }
  std::ofstream::open( name, mode );
#else
  std::ofstream::open( name, mode );
#endif
}

/* Attempts to open a file named 'filename' with the help of a given list of prefixes.
 * If a file handle could be obtained, returns true, otherwise false
 */
bool ofstream::open( const std::string& filename, const std::vector<std::string>& prefix_list, openmode mode )
{
  for( auto& prefix : prefix_list )
  {
    open( prefix + filename, mode );
    if ( !fail() )
      return true;
  }
  return false;
}

void ifstream::open( const char* name, openmode mode )
{
#ifdef _MSC_VER
  // This is intentionally "_MSC_VER" instead of "SC_WINDOWS".
  // MS's c++ std library provides
  //   std::fstream::open( const wchar_t*, openmode )
  // as an extension to open files with Unicode names.
  // The MinGW c++ std library does not. Since neither
  // library will accept UTF-8 encoded names, there is
  // no way to open a file with a Unicode filename using
  // std::fstream in MinGW.
  std::ifstream::open( io::widen( name ).c_str(), mode );
#elif defined( SC_MINGW )
  if ( contains_non_ascii( name ) )
  {
    throw std::invalid_argument(fmt::format("File '{}' with non-ascii characters cannot be opened when built with MinGW.", name));
  }
  std::ifstream::open( name, mode );
#else
  std::ifstream::open( name, mode );
#endif
}

/* Attempts to open a file named 'filename' with the help of a given list of prefixes.
 * If a file handle could be obtained, returns true, otherwise false
 */
bool ifstream::open( const std::string& filename, const std::vector<std::string>& prefix_list, openmode mode )
{
  for( auto& prefix : prefix_list )
  {
    open( prefix + filename, mode );
    if ( !fail() )
      return true;
  }
  return false;
}

#ifdef SC_WINDOWS
utf8_args::utf8_args( int, char** )
{
  int wargc;
  wchar_t** wargv = CommandLineToArgvW( GetCommandLineW(), &wargc );
  for ( int i = 1; i < wargc; ++i )
    push_back( io::narrow( wargv[ i ] ) );
  LocalFree( wargv );
}

std::string ansi_to_utf8( util::string_view src )
{
  int len = MultiByteToWideChar( CP_ACP, 0, src.data(), src.size(), NULL, 0 );
  auto wstr = std::vector<wchar_t>(len);
  MultiByteToWideChar( CP_ACP, 0, src.data(), src.size(), wstr.data(), wstr.size() );
  len = WideCharToMultiByte( CP_UTF8, 0, wstr.data(), wstr.size(), NULL, 0, NULL, NULL );
  auto str = std::vector<char>(len);
  WideCharToMultiByte( CP_UTF8, 0, wstr.data(), wstr.size(), str.data(), str.size(), NULL, NULL );
  return std::string(str.data(), str.size());
}
#else

utf8_args::utf8_args( int argc, char** argv ) :
  std::vector<std::string>( argv + 1, argv + argc ) {}
#endif



} // namespace io ===========================================================
