#ifndef SC_UTIL_STRING_VIEW_HPP_INCLUDED
#define SC_UTIL_STRING_VIEW_HPP_INCLUDED

#include <string_view>

namespace util {

using string_view = std::string_view;

// C++20 extension. Implemented as free functions as it's not available in C++17 so it's easier to port.
constexpr bool starts_with(string_view sv, string_view s) noexcept {
  return sv.size() >= s.size() && sv.compare(0, s.size(), s) == 0;
}
constexpr bool starts_with(string_view sv, char c) noexcept {
  return !sv.empty() && string_view::traits_type::eq(sv.front(), c);
}
constexpr bool ends_with(string_view sv, string_view s) noexcept {
  return sv.size() >= s.size() && sv.compare(sv.size() - s.size(), string_view::npos, s) == 0;
}
constexpr bool ends_with(string_view sv, char c) noexcept {
  return !sv.empty() && string_view::traits_type::eq(sv.back(), c);
}

} // namespace util

#endif // SC_UTIL_STRING_VIEW_HPP_INCLUDED
