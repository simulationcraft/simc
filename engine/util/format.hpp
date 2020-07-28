// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_UTIL_FORMAT_HPP_INCLUDED
#define SC_UTIL_FORMAT_HPP_INCLUDED

#include <type_traits>

#include "fmt/core.h"

namespace util { namespace fmt_detail {
template <typename...> using void_t = void;
template <typename T, typename = void> struct has_format_to : std::false_type {};
template <typename T>
struct has_format_to<T,
  void_t<decltype( format_to( std::declval<const T&>(),
                              std::declval<::fmt::format_context::iterator>() ) ) > > : std::true_type {};
} } // namespace util::fmt_detail

namespace fmt {

/**
 * Generic fmt::formatter that supports any type T that has adl-discoverable
 *  format_to( const T&, fmt::format_context::iterator )
 * overload.
 */
template <typename T>
struct formatter<T, char, std::enable_if_t<::util::fmt_detail::has_format_to<T>::value>> {
  constexpr auto parse( format_parse_context& ctx ) { return ctx.begin(); }
  format_context::iterator format( const T& v, format_context& ctx ) {
    auto out = ctx.out();
    format_to( v, out );
    return out;
  }
};

} // namespace fmt

#endif // SC_UTIL_FORMAT_HPP_INCLUDED
