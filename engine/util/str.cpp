// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "config.hpp"

#include <stdint.h>
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iterator>
#include <limits>
#include <numeric>
#include <string>
#include <typeinfo>

#include "str.hpp"

// FORMAT_STRING ============================================================

static const char digits[] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

static void format_string( std::string& buffer, const char* str, int slen, int min_width, int max_width, bool left_justify, char pad )
{
  // pad left
  if( ! left_justify && slen < min_width )
  {
    buffer.resize( buffer.size() + (min_width - slen), pad );
  }

  buffer.append( str, (max_width != -1 && slen > max_width) ? max_width : slen  );

  // pad right
  if( left_justify && slen < min_width )
  {
    buffer.resize( buffer.size() + (min_width - slen), pad );
  }
}

static void format_char( std::string& buffer, char c, int min_width, int max_width, bool left_justify )
{
  char str[] = { c, '\0' };

  format_string( buffer, str, 1, min_width, max_width, left_justify, ' ' );
}

class backward_string {
public:
  backward_string( bool comma = false ) : _c( &_buf[0] + MAX_SIZE ), _comma(comma) { *--_c = '\0'; }
  ~backward_string()                            {}

  backward_string& operator-=( char c ) {
    if( _comma && size() % 4 == 3 ) *--_c = ',';
    *--_c = c;
    return *this;
  }

  const char* c_str() const          { return _c; }
  int         size() const           { return static_cast<int>( (&_buf[0] + MAX_SIZE) - _c - 1 ); }

  void toupper()                     { char* p = _c; while( *p ) { *p = ::toupper( *p ); ++p; } }

private:
  enum { MAX_SIZE = 64 };
  char _buf[ MAX_SIZE ];
  char* _c;
  bool  _comma;
};

template< typename T >
static void format_signed( std::string& buffer, T i, int min_width, int /*max_width*/,
         bool left_justify, char pad, char plus, bool comma )
{
  backward_string str( comma );
  T num = i < 0 ? -i : i;

  do
  {
    str -= digits[ (num % 10) ];
  }
  while( num /= 10 );

  if( i < 0 )
  {
    str -= '-';
  }
  else if( plus )
  {
    str -= plus;
  }

  format_string( buffer, str.c_str(), (int) str.size(), min_width, -1, left_justify, pad );
}

static void format_unsigned( std::string& buffer, unsigned long long i, int base, int min_width, int /*max_width*/,
           bool left_justify, bool alt_form, char pad, bool upper_case, bool comma )
{
  backward_string str( comma );
  unsigned long long num = i;

  do
  {
    str -= digits[ (num % base) ];
  }
  while( num /= base );

  if( alt_form )
  {
    if( base == 8 && i != 0 )
    {
      str -= '0';
    }
    else if( base == 16 )
    {
      str -= 'x';
      str -= '0';
    }
  }

  if( upper_case ) str.toupper();

  format_string( buffer, str.c_str(), (int) str.size(), min_width, -1, left_justify, pad );
}

static void format_double( std::string& buffer, double d, int min_width, int precision,
                           bool left_justify, bool alt_form, char plus, char format_type )
{
  std::string str;
  double integer=0, fraction=0, digit=0;
  bool   negative = false;
  int    exponent=0;

  str.reserve( 32 );

#ifndef __FAST_MATH__
  if( std::isnan( d ) )
  {
    str         = "NaN";  // reverse
    precision   = 0;
    format_type = 'f';
  }
  else if ( !std::isfinite( d ) )
  {
    str         = "FNI";  // reverse
    precision   = 0;
    format_type = 'f';
    if( d < 0 ) negative = true;
  }
  else
#endif /* __FAST_MATH__ */
  {
    if( min_width == -1 ) min_width = 0;  // defaults
    if( precision == -1 ) precision = 6;

    if( d < 0.0 )
    {
      d = -d;
      negative = true;
    }

    // e: scientific
    if( format_type == 'e' || format_type == 'E' || format_type == 'g' || format_type == 'G' )
    {
      long double d_bak = d;

      exponent = 0;
      if( d > 10.0 )
      {
  while( d > 10.0 )
  {
    d /= 10;
    exponent++;
  }
      }
      else if( d < 1.0 && d != 0.0 )
      {
  // 0.5 for round off...
  while( d + 0.5 < 1.0  )
  {
    d *= 10;
    exponent--;
  }
      }

      if( format_type == 'g' || format_type == 'G' )
      {
  if( exponent < -4 || exponent >= precision )
  {
    format_type = format_type == 'g' ? 'e' : 'E';
    precision--; // digit on left of .
  }
  else
  {
    format_type = 'f';
    d = d_bak;
    if( exponent > 0  ) precision -= (exponent + 1);
    if( precision < 0 ) precision = 0;
  }
      }
    }

    d += (0.5 / pow(10.0, precision));

    fraction = modf( d, &integer );

    // integer part (reverse order)
    const size_t max_digits = 30;
    size_t n_digits = 0;
    do
    {
      digit = modf( integer / 10.0, &integer );
      str += digits[ int((digit * 10.0) + 0.5) ];
    }
    while( integer > 0.0 && ++n_digits < max_digits );
  }

  if( negative )
  {
    str += '-';
  }
  else if( plus )
  {
    str += plus;
  }

  int start = 0, end = (int) str.size() - 1;

  while( start < end )
  {
    std::swap( str[ start ], str[ end ] );
    start++;
    end--;
  }

  // dot
  if( precision > 0 || alt_form )
  {
    str += '.';
  }

  // fraction
  if( precision > 0 )
  {
    do
    {
      fraction = modf( fraction * 10.0, &digit );
      str += digits[ int(digit) ];
    }
    while( fraction > 0.0 && --precision > 0 );

    // make sure we have "precision" num of chars
    if( precision - 1 > 0 )
    {
      str.append( precision - 1, '0' );
    }
  }

  // e: scientific exponent
  if( format_type == 'e' || format_type == 'E' )
  {
    str += format_type;
    if( exponent >= 0 )
    {
      str += '+';
    }
    else
    {
      str += '-';
      exponent = -exponent;
    }

    if( exponent < 10 ) str += '0';

    // exponent part (reverse order)
    char exp_str[ 8 ];
    int  i = 0;
    do
    {
      exp_str[ i++ ] = digits[ (exponent % 10) ];
    }
    while( exponent /= 10 );

    while( --i >= 0 )
    {
      str += exp_str[ i ];
    }
  }

  format_string( buffer, str.c_str(), (int) str.size(), min_width, -1, left_justify, ' ' );
}

static int guess_reserve( const char* fmt )
{
  int len = 0;

  const char* c = fmt;
  while( *c )
  {
    if( *c == '%' )
    {
      len += 8;
    }
    c++;
  }
  len += static_cast<int>( c - fmt );

  return len;
}

std::string& str::format( std::string& buffer, const char *fmt, va_list args )
{
  const char* c = fmt;

  if( buffer.capacity() == 0 ) buffer.reserve( guess_reserve( fmt ) );

  while( *c )
  {
    if( *c != '%' )
    {
      buffer += *c++;
    }
    else
    {
      c++; // skip %

      bool flags_alt_form     = false;
      bool flags_left_justify = false;
      char flags_zero_pad     = ' ';
      char flags_plus_pad     = 0;
      bool flags_comma        = false;
      bool mod_char           = false;
      bool mod_short          = false;
      bool mod_long           = false; // for ptrs only
      bool mod_64             = false;
      bool mod_size           = false;
      int  width              = -1;
      int  precision          = -1;
      const char* str;
      unsigned long long unum;

      // %[#0- +][#*][.][#*][hlL][dioxXucsfeEgGpn%]

      // flags [#0- +] our extensions [,]
      bool reading_flags = true;
      while( reading_flags )
      {
        switch( *c )
        {
        case '#':
          flags_alt_form = true;
          c++;
          break;
        case '0':
          if( !flags_left_justify ) flags_zero_pad = '0';
          c++;
          break;
        case '-':
          flags_left_justify = true;
          flags_zero_pad     = ' '; // overrides
          c++;
          break;
        case ' ':
          if( !flags_plus_pad ) flags_plus_pad = ' ';
          c++;
          break;
        case '+':
          flags_plus_pad = '+'; // overrides
          c++;
          break;
        case ',':
          flags_comma = true;
          c++;
          break;
        default:
          reading_flags = false;
          break;
        }
      }
      
      // width [#*]
      if( *c == '*' || isdigit( *c ) )
      {
        if( *c == '*' )
        {
          width = va_arg( args, int );
          c++;
        }
        else
        {
          char* end;
          width = (int)strtol( c, &end, 10 );
          c = end;
        }

        if( width < 0 )
        {
          flags_left_justify = true;
          flags_zero_pad     = ' '; // overrides
          width              = -width;
        }
      }

      // dot [.]
      if( *c == '.' ) c++;

      // right width [#*]
      if( *c == '*' || isdigit( *c ) )
      {
        if( *c == '*' )
        {
          precision = va_arg( args, int );
          c++;
        }
        else
        {
          char* end;
          precision = (int)strtol( c, &end, 10 );
          c = end;
        }

        if( precision < 0 )
        {
          precision = -precision;
        }
      }

      // modifiers [hlLz]
      switch( *c )
      {
      case 'h':
        c++;
        if( *c == 'h' )
        {
          c++;
          mod_char = true;
        }
        else
        {
          mod_short = true;
        }
        break;
      case 'l':
        c++;
        if( *c == 'l' )
        {
          c++;
        }
        mod_64 = true;
        break;
      case 'L':
        c++;
        mod_64 = true;
        break;
      case 'z':
        c++;
        mod_size = true;
        break;
      }

      // type [dioxXucsfeEgGpn%]
      switch( *c )
      {
      case '%':
        buffer += '%';
        c++;
        break;
      case 'd':
      case 'i':
        if( mod_64 )
        {
          format_signed<int64_t>( buffer, va_arg( args, int64_t ), width, precision,
                                  flags_left_justify, flags_zero_pad, flags_plus_pad, flags_comma );
        }
        else if( mod_short ) // elipsis auto-promote
        {
          format_signed<short>( buffer, (short) va_arg( args, int ), width, precision,
        flags_left_justify, flags_zero_pad, flags_plus_pad, flags_comma );
        }
        else if( mod_char ) // elipsis auto-promote
        {
          format_signed<char>( buffer, (char) va_arg( args, int ), width, precision,
                              flags_left_justify, flags_zero_pad, flags_plus_pad, flags_comma );
        }
        else
        {
          format_signed<int>( buffer, va_arg( args, int ), width, precision,
                              flags_left_justify, flags_zero_pad, flags_plus_pad, flags_comma );
        }
        c++;
        break;
      case 's':
        str = va_arg( args, char* );
        if( ! str ) str =  "";
        format_string( buffer, str, static_cast<int>(strlen(str)), width, precision,
                       flags_left_justify, flags_zero_pad );
        c++;
        break;
      case 'f':
        format_double( buffer, va_arg( args, double ), width, precision,
                       flags_left_justify, flags_alt_form, flags_plus_pad, 'f' );
        c++;
        break;
      case 'c':
        format_char( buffer, va_arg( args, int ), width, precision, flags_left_justify );
        c++;
        break;
      case 'o':
  unum = ( mod_64    ? va_arg( args, uint64_t       ) :
     mod_size  ? va_arg( args, size_t         ) :
     mod_short ? va_arg( args, unsigned int   ) : // elipsis auto-promote
     mod_char  ? va_arg( args, unsigned int   ) : // elipsis auto-promote
                 va_arg( args, unsigned int   ) );
  format_unsigned( buffer, unum, 8, width, precision,
       flags_left_justify, flags_alt_form, flags_zero_pad, false, false );
        c++;
        break;
      case 'p':
        flags_alt_form = true;
        mod_long = true;
        // fall thru because size(void*) == sizeof(unsigned long)
      case 'x':
      case 'X':
  unum = ( mod_64    ? va_arg( args, uint64_t       ) :
     mod_size  ? va_arg( args, size_t         ) :
     mod_long  ? va_arg( args, unsigned long  ) :
     mod_short ? va_arg( args, unsigned int   ) : // elipsis auto-promote
     mod_char  ? va_arg( args, unsigned int   ) : // elipsis auto-promote
                 va_arg( args, unsigned int   ) );
  format_unsigned( buffer, unum, 16, width, precision,
       flags_left_justify, flags_alt_form, flags_zero_pad, *c == 'X', false );
        c++;
        break;
      case 'u':
  unum = ( mod_64    ? va_arg( args, uint64_t       ) :
     mod_size  ? va_arg( args, size_t         ) :
     mod_short ? va_arg( args, unsigned int   ) : // elipsis auto-promote
     mod_char  ? va_arg( args, unsigned int   ) : // elipsis auto-promote
                 va_arg( args, unsigned int   ) );
  format_unsigned( buffer, unum, 10, width, precision,
       flags_left_justify, false, flags_zero_pad, false, flags_comma );
        c++;
        break;
      case 'e':
      case 'E':
      case 'g':
      case 'G':
        format_double( buffer, va_arg( args, double ), width, precision,
                       flags_left_justify, flags_alt_form, flags_plus_pad, *c );
        c++;
        break;
      case 'n':
        *(va_arg( args, int* )) = (int) buffer.size();
        c++;
        break;
      default: // % followed by an unknown char, printf() seems to add a % in this case
        buffer += '%';
        break;
      }
    }
  }
  return buffer;
}

std::string& str::format( std::string& buffer, const char *fmt, ... )
{
  va_list args;
  va_start( args, fmt );
  str::format( buffer, fmt, args );
  va_end( args );
  return buffer;
}

// Oh lords of reference counting and rvo, please don't f*ck this up!

std::string str::format( const char *fmt, va_list args )
{
  std::string buffer;
  str::format( buffer, fmt, args );
  return buffer;
}

std::string str::format( const char *fmt, ... )
{
  std::string buffer;
  va_list args;
  va_start( args, fmt );
  str::format( buffer, fmt, args );
  va_end( args );
  return buffer;
}

