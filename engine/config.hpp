// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef CONFIG_H
#define CONFIG_H

/* This file contains platform, compiler and general macros and defines,
 * as well as pre-C++11 macros
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
// Compiler
// ==========================================================================

#if defined( __GNUC__ ) && !defined( __clang__ ) // Do NOT define SC_GCC for Clang
#  define SC_GCC ( __GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__ )
#endif
#if defined( __clang__ )
#  define SC_CLANG ( __clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__ )
#endif
#if defined( _MSC_VER )
#  define SC_VS ( _MSC_VER / 100 - 6 )
#  if SC_VS < 10
#    error "Visual Studio 9 ( 2008 ) or lower not supported"
#  endif
#endif

// Workaround for LLVM/Clang 3.2+ using glibc headers.
#if defined( SC_CLANG ) && SC_CLANG >= 30200
# define __extern_always_inline extern __always_inline __attribute__(( __gnu_inline__ ))
#endif

// problem with gcc4.4 + -std=c++0x and including <cstdio>. Not sure if it affects 4.4.1 as well.
// http://stackoverflow.com/questions/3445312/swprintf-and-vswprintf-not-declared
#if defined(SC_GCC) && SC_GCC < 40500
#undef __STRICT_ANSI__
#endif



// ==========================================================================
// C++11
// ==========================================================================
#if __cplusplus < 201103L && ( ! defined( SC_GCC ) || ! __GXX_EXPERIMENTAL_CXX0X__ || SC_GCC < 40600 ) && ( ! defined( SC_VS ) )
namespace std {
class nullptr_t
{
public:
  template <typename T> operator T* () const { return 0; }
  template <typename T, typename U> operator T U::* () const { return 0; }
  operator bool () const { return false; }
};
}
#define nullptr ( std::nullptr_t() )
#endif

#if __cplusplus < 201103L && ( ! defined( SC_GCC ) || ! __GXX_EXPERIMENTAL_CXX0X__ || SC_GCC < 40700 ) && ( ! defined( SC_VS ) )
#define override
#endif

#if __cplusplus < 201103L && ( ! defined( SC_GCC ) || ! __GXX_EXPERIMENTAL_CXX0X__ || SC_GCC < 40700 ) && ( ! defined( SC_VS ) || SC_VS < 11 )
#define final
#endif

#if ( ! defined(_MSC_VER) || _MSC_VER < 1600 ) && __cplusplus < 201103L && ! defined(__GXX_EXPERIMENTAL_CXX0X__)
#define static_assert( condition, message )
#endif

#if ( ! ( ! defined( SC_OSX ) && ( defined( SC_VS ) || __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__) ) ) )
#define USE_TR1_NAMESPACE 1
#endif

#if ( defined( _LIBCPP_VERSION ) )
#ifdef USE_TR1_NAMESPACE
#undef USE_TR1_NAMESPACE
#endif
#endif



// ==========================================================================
// General Macros/Defines
// ==========================================================================

#if !defined(__GNUC__)
#  define __attribute__(x)
#endif

#define SC_PACKED_STRUCT      __attribute__((packed))
#define PRINTF_ATTRIBUTE(a,b) __attribute__((format(printf,a,b)))

#ifndef M_PI
#define M_PI ( 3.14159265358979323846 )
#endif



#endif // CONFIG_H
