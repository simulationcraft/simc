// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef CONFIG_H
#define CONFIG_H

/* This file contains platform, compiler and general macros and defines,
 * etc.
 */

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
#  define NOMINMAX
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
#if defined( SC_VS ) && SC_VS < 13
#  error "Visual Studio 12 ( 2013 ) or lower not supported"
#endif

// Last updated 2017-11-07: Support gcc4.8 with full C++11 support for now
// Debian 8 (Jessie) with gcc 4.9 and clang3.5
// Ubuntu 16.04: gcc 5.3, clang3.8
// Debian 9 (Stretch): gcc 6.3 clang 3.8
#if defined( SC_CLANG ) && SC_CLANG < 30500
#  error "clang++ below version 3.5 not supported"
#endif
#if defined( SC_GCC ) && SC_GCC < 50000
#  error "g++ below version 5 not supported"
#endif


// ==========================================================================
// Compiler Workarounds
// ==========================================================================

/* This header defines eleven macro constants with alternative spellings for those C++ operators
 * not supported by the ISO646 standard character set.
 * eg. and == &&, or == ||, etc.
 * This is required for MSVC, which without the /ZA option does not conform to the standard, but which
 * we don't want to use for other reasons.
 */
#include <ciso646>


// ==========================================================================
// General Macros/Defines
// ==========================================================================

#if !defined(__GNUC__)
#  define __attribute__(x)
#endif

#define SC_PACKED_STRUCT      __attribute__((packed))

#ifndef __has_cpp_attribute
#  define __has_cpp_attribute(x) 0
#endif

#if __cplusplus > 201402L && __has_cpp_attribute(fallthrough)
#  define SC_FALLTHROUGH [[fallthrough]]
#elif __has_cpp_attribute(gnu::fallthrough)
#  define SC_FALLTHROUGH [[gnu::fallthrough]]
#elif __has_cpp_attribute(clang::fallthrough)
#  define SC_FALLTHROUGH [[clang::fallthrough]]
#elif defined( _MSC_VER ) && _MSC_VER >= 1911 && _MSVC_LANG >= 201703L
#  define SC_FALLTHROUGH [[fallthrough]]
#else
#  define SC_FALLTHROUGH
#endif

// ==========================================================================
// Floating Point
// ==========================================================================

/**
 * Define our own m_pi since M_PI constant is actually only in POSIX math.h
 * */
constexpr double m_pi = 3.14159265358979323846;

#endif // CONFIG_H
