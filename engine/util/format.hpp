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
                              std::declval<::fmt::format_context&>() ) ) > > : std::true_type {};
} } // namespace util::fmt_detail

namespace fmt {

/**
 * Generic fmt::formatter that supports any type T that has adl-discoverable
 *  format_to( const T&, fmt::format_context& )
 * overload.
 *
 * The function must conform to `fmt::formatter<>::format()` api -it must
 * return `fmt::format_context::iterator`.
 *
 */
template <typename T>
struct formatter<T, std::enable_if_t<::util::fmt_detail::has_format_to<T>::value, char>> {
  constexpr auto parse( format_parse_context& ctx ) { return ctx.begin(); }
  format_context::iterator format( const T& v, format_context& ctx ) {
    static_assert(std::is_same<decltype( format_to( v, ctx ) ), format_context::iterator>::value,
                  "format_to() must return format_context iterator");
    return format_to( v, ctx );
  }
};

} // namespace fmt

#endif // SC_UTIL_FORMAT_HPP_INCLUDED
