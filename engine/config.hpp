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

#if defined(__linux) || defined(__linux__) || defined(linux)
#  define SC_LINUX
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
#  define SC_VS _MSC_VER
#pragma warning( disable : 4265)
#endif

#if defined( SC_WINDOWS ) && !defined( SC_VS )
#  define SC_MINGW
#endif

// ==========================================================================
// Compiler Minimal Limits
// ==========================================================================


// Last updated 2023-09: Support gcc7 / clang 5 / MSVS 19.14 which have (~full) C++17 support
// Ubuntu LTS EOL is +5 years, Debian has ~+3year EOL, +5 years with separate LTS support
// Debian 10 (Buster) (Release 2019-07): gcc 8.3 clang 7
// Ubuntu 20.04: gcc 9.3 clang 10
// Debian 11 (Bullseye) (Release 2021-08): gcc 10.2 clang 11
// Ubuntu 22.04: gcc 11.2 clang 14
// Debian 12 (Bookworm) (Release 2023-06): gcc 12.2 clang 14
//
// CI setup currently only supports Ubuntu 20.04+, which has clang 7 as the lowest installable version -
// so clang 5/6 builds are not actually tested in CI.
#if defined( SC_CLANG ) && SC_CLANG < 50000
#  error "clang++ below version 5 not supported"
#endif
#if defined( SC_GCC ) && SC_GCC < 70000
#  error "g++ below version 7 not supported"
#endif
#if defined( SC_VS ) && SC_VS < 1914
#  error "Visual Studio 2017 below version 15.7 not supported"
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

#if !defined(__GNUC__) && !defined(__clang__)
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

#if defined( _MSVC_LANG )
#  define SC_CPP_LANG _MSVC_LANG
#else
#  define SC_CPP_LANG __cplusplus
#endif

// ==========================================================================
// Networking library
// ==========================================================================

#ifndef SC_NO_NETWORKING
constexpr bool SC_NO_NETWORKING_ON = false;
#else
constexpr bool SC_NO_NETWORKING_ON = true;
#endif

#if !defined( SC_NO_NETWORKING ) && !defined( SC_WINDOWS )
#  define SC_USE_CURL
#elif !defined( SC_NO_NETWORKING ) && defined( SC_WINDOWS )
#  define SC_USE_WININET
#endif

// ==========================================================================
// Floating Point
// ==========================================================================

/**
 * Define our own m_pi since M_PI constant is actually only in POSIX math.h
 * */
constexpr double m_pi = 3.14159265358979323846;

// ==========================================================================
// Simc related compilation defines
// ==========================================================================

#define SC_USE_STAT_CACHE

#ifndef NDEBUG
#define ACTOR_EVENT_BOOKKEEPING
#endif

#define RAPIDJSON_HAS_STDSTRING 1
#define RAPIDJSON_NOMEMBERITERATORCLASS 1

#if defined(_MSC_VER) && defined(_M_ARM64)
#    define RAPIDJSON_ENDIAN RAPIDJSON_LITTLEENDIAN
#endif

#ifndef SC_USE_PTR
#define SC_USE_PTR 1
#endif

// ==========================================================================
// Simc related value definitions
// ==========================================================================

#define SC_MAJOR_VERSION "1100"
#define SC_MINOR_VERSION "02"
#define SC_VERSION ( SC_MAJOR_VERSION "-" SC_MINOR_VERSION )
#define SC_BETA 0
#if SC_BETA
#define SC_BETA_STR "thewarwithin"
#endif

constexpr int MAX_LEVEL = 80;
constexpr int MAX_SCALING_LEVEL = 80;
constexpr int MAX_ILEVEL = 1300;
constexpr int MAX_CLASS = 14;

#endif // CONFIG_H
