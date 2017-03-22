// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef CONFIG_H
#define CONFIG_H

/* This file contains platform, compiler and general macros and defines,
 * as well as pre-C++11 macros
 */

/* This header defines eleven macro constants with alternative spellings for those C++ operators
 * not supported by the ISO646 standard character set.
 * eg. and == &&, or == ||, etc.
 */
#include <ciso646>

// ==========================================================================
// Platform
// ==========================================================================

#if defined(__APPLE__) || defined(__MACH__)
#  define SC_OSX
#endif

#if defined( WIN32 ) || defined( _WIN32 ) || defined( __WIN32 )
#  define SC_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  define VC_EXTRALEAN
#  ifndef _CRT_SECURE_NO_WARNINGS
#    define _CRT_SECURE_NO_WARNINGS
#  endif
#  ifndef UNICODE
#    define UNICODE
#  endif
#else
#  define SC_SIGACTION
#endif

// ==========================================================================
// Compiler Definitions
// ==========================================================================

#if defined( __GNUC__ ) && !defined( __clang__ ) // Do NOT define SC_GCC for Clang
#  define SC_GCC ( __GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ )
#endif
#if defined( __clang__ )
#  define SC_CLANG ( __clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__ )
#endif
#if defined( _MSC_VER )
#  define SC_VS ( _MSC_VER / 100 - 6 )
#pragma warning( disable : 4265)
#endif

#if defined( SC_WINDOWS ) && !defined( SC_VS )
#  define SC_MINGW
#endif

// ==========================================================================
// Compiler Minimal Limits
// ==========================================================================
#if defined( SC_VS ) && SC_VS < 12
#  error "Visual Studio 11 ( 2012 ) or lower not supported"
#endif

#if defined( SC_CLANG ) && SC_CLANG < 30100
#  error "clang++ below version 3.1 not supported"
#endif

#if defined( SC_GCC ) && SC_GCC < 40700
#  error "g++ below version 4.7 not supported"
#endif

#if defined( SC_GCC ) && SC_GCC < 40800
#  warning "g++ below version 4.8 is deprecated. Please update until 2017-05-01."
#endif

// ==========================================================================
// Compiler Workarounds
// ==========================================================================

// Workaround for LLVM/Clang 3.2+ using glibc headers.
#if defined( SC_CLANG ) && SC_CLANG >= 30200 && SC_CLANG < 30500
# define __extern_always_inline extern __always_inline __attribute__(( __gnu_inline__ ))
#endif

// ==========================================================================
// General Macros/Defines
// ==========================================================================

#if !defined(__GNUC__)
#  define __attribute__(x)
#endif

#define SC_PACKED_STRUCT      __attribute__((packed))

#ifndef SC_LINT // false negatives are irritating
#  define PRINTF_ATTRIBUTE(a,b) 
#else
#  if defined( SC_MINGW ) // printf wrongly points to vs_printf instead of gnu_printf on MinGW
#    define PRINTF_ATTRIBUTE(a,b) __attribute__((format(gnu_printf,a,b)))
#  else
#    define PRINTF_ATTRIBUTE(a,b) __attribute__((format(printf,a,b)))
#  endif
#endif

// ==========================================================================
// C99 fixed-width integral types & format specifiers
// ==========================================================================

#define __STDC_FORMAT_MACROS
#include <stdint.h>
#include <inttypes.h>

#ifndef PRIu64
#  if defined( SC_VS )
#    define PRIu64 "I64"
#    pragma message("C99 format specifiers not available")
#  else
#    define PRIu64 "zu"
#    warning "C99 format specifiers not available"
#  endif
#endif

// ==========================================================================
// Floating Point
// ==========================================================================

/* Ensure _USE_MATH_DEFINES is defined before any inclusion of <cmath>, so that
 * math constants are defined
 * */
#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
#define M_PI ( 3.14159265358979323846 )
#endif

#endif // CONFIG_H
