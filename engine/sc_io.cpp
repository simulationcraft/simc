// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#ifdef SC_WINDOWS
#include <windows.h>
#include <shellapi.h>
#endif

namespace io { // ===========================================================

namespace { // anonymous namespace ==========================================

static const uint32_t invalid_character_replacement = '?';

std::wstring widen( const char* first, const char* last )
{
  std::wstring result;

  if ( sizeof( wchar_t ) == 2 )
    utf8::utf8to16( first, last, std::back_inserter( result ) );
  else
  {
    assert( sizeof( wchar_t ) == 4 );
    utf8::utf8to32( first, last, std::back_inserter( result ) );
  }

  return result;
}

std::string narrow( const wchar_t* first, const wchar_t* last )
{
  std::string result;
  utf8::utf16to8( first, last, std::back_inserter( result ) );
  return result;
}

} // anonymous namespace ====================================================

std::string narrow( const wchar_t* wstr )
{ return narrow( wstr, wstr + wcslen( wstr ) ); }

std::wstring widen( const char* str )
{ return widen( str, str + strlen( str ) ); }

std::wstring widen( const std::string& str )
{ return widen( str.c_str(), str.c_str() + str.size() ); }

// Unicode codepoints up to 0xFF are basically identical to Latin-1
// (and vaguely Windows codepage 1252) and codepoints up to 0x7F
// are identical to ASCII. In other words, ASCII is a subset of
// Latin-1 which is a subset of Unicode.

#if 0
std::string latin1_to_utf8( const std::string& str )
{
  std::string result;

  // Since the codepoints are identical, transcoding Latin-1 to UTF-8
  // only requires recoding each input byte as a UTF-8 sequence.
  for ( std::string::const_iterator i = str.begin(), e = str.end(); i != e; ++i )
    utf8::append( static_cast<unsigned char>( *i ), std::back_inserter( result ) );

  return result;
}

std::string utf8_to_latin1( const std::string& str )
{
  std::string result;

  std::string::const_iterator first = str.begin(), last = str.end();
  while ( first != last )
  {
    uint32_t codepoint = utf8::next( first, last );
    if ( codepoint > 0xFF )
    {
      // Codepoints > 0xFF aren't in Latin-1.
      codepoint = invalid_character_replacement;
    }
    result += static_cast<unsigned char>( codepoint );
  }

  return result;
}
#endif

std::string maybe_latin1_to_utf8( const std::string& str )
{
  typedef std::string::const_iterator iterator;
  std::string result;

  try
  {
    // Validate input as UTF-8 while we copy.
    for ( iterator first = str.begin(), last = str.end(); first != last ; )
      utf8::append( utf8::next( first, last ), std::back_inserter( result ) );
  }
  catch ( utf8::exception& )
  {
    // We hit something invalid. Start over, treating the whole string as Latin-1.
    result.clear();
    for ( iterator first = str.begin(), last = str.end(); first != last ; ++first )
      utf8::append( static_cast<unsigned char>( *first ), std::back_inserter( result ) );
  }

  return result;
}

#ifdef SC_WINDOWS
FILE* fopen( const std::string& filename, const char* mode )
{ return _wfopen( widen( filename ).c_str(), widen( mode ).c_str() ); }
#endif

ofstream& ofstream::printf( const char* format, ... )
{
  char buffer[ 16384 ];

  va_list fmtargs;
  va_start( fmtargs, format );
  int rval = ::vsnprintf( buffer, sizeof( buffer ), format, fmtargs );
  va_end( fmtargs );

  if ( rval >= 0 )
    assert( static_cast<size_t>( rval ) < sizeof( buffer ) );

  *this << buffer;
  return *this;
}

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
// FIXME: #elif SC_MINGW
#else
  std::ofstream::open( name, mode );
#endif
}

void ofstream::open( sim_t* sim, const std::string& filename, openmode mode )
{
  open( filename, mode );

  if ( fail() )
  {
    sim -> errorf( "Failed to open output file '%s'.", filename.c_str() );
    return;
  }

  exceptions( failbit | badbit );
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

#else

utf8_args::utf8_args( int argc, char** argv ) :
  std::vector<std::string>( argv + 1, argv + argc ) {}
#endif

} // namespace io ===========================================================
