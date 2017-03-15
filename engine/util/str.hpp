// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef STR_HPP
#define STR_HPP

#include <cstdarg>
#include <string>

namespace str {

// These functions will concatenate to buffer.
std::string& format( std::string& buffer, const char *fmt, va_list args );
std::string& format( std::string& buffer, const char *fmt, ... );

// Please use the above two if at all convenient.
std::string format( const char *fmt, va_list args );
std::string format( const char *fmt, ... );

} // NAMESPACE STR

#endif // STR_HPP
