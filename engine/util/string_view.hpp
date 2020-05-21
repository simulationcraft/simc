/*
 * string_view "very-lite"
 *
 * Requires C++14. Does not explicitly check for that though as it would
 * prevent its usage on some of our windows build bots (which can compile
 * it but don't advertise full C++14 compat).
 *
 * Based off the specification on cppreference.com
 *  [ https://en.cppreference.com/w/cpp/string/basic_string_view ]
 * And the initial implementation of string_view for libc++ by Marshal Clow:
 *  [ https://github.com/mclow/string_view ] (used as a skeleton)
 *
 * Explicitly not a template as we don't use anything but char and default
 * char traits. Also does not implement the full interface, lacks:
 *  - reverse iterators
 *  - rfind
 *  - find_last_of
 *  - find_first_not_of
 *  - find_last_not_of
 *  - output to streams
 * as we never found any uses for them.
 *
 * Both of the above also let us make the headers kinda lightweight.
 */

#ifndef SC_UTIL_STRING_VIEW_HPP_INCLUDED
#define SC_UTIL_STRING_VIEW_HPP_INCLUDED

#include <cassert>
#include <limits>
#include <string>

#include "fmt/format.h"

#include "config.hpp"

namespace util {
namespace detail {
// std::char_traits is not constexpr in C++14, so we have to supply our
// own ::length and ::compare as we do care about constexpr construction
// and comparisons

constexpr size_t strlen(const char* str) {
#if SC_CPP_LANG >= 201703L
  return std::char_traits<char>::length(str);
#elif defined(SC_GCC) || defined(SC_CLANG)
  return __builtin_strlen(str);
#else
  size_t len = 0;
  for (; *str != '\0'; ++str)
    len++;
  return len;
#endif
}

constexpr int strcmp(const char* s1, const char* s2, size_t n) {
#if SC_CPP_LANG >= 201703L
  return std::char_traits<char>::compare(s1, s2, n);
#elif defined(SC_GCC) || (defined(SC_CLANG) && SC_CLANG >= 40000)
  return __builtin_memcmp(s1, s2, n);
#else
  for (; n; --n, ++s1, ++s2) {
    const auto c1 = static_cast<unsigned char>(*s1);
    const auto c2 = static_cast<unsigned char>(*s2);
    if (c1 < c2) return -1;
    if (c2 < c1) return 1;
  }
  return 0;
#endif
}
} // namespace detail

class string_view {
public:
  // types
  using traits                 = std::char_traits<char>;
  using value_type             = char;
  using pointer                = char*;
  using const_pointer          = const char*;
  using reference              = char&;
  using const_reference        = const char&;
  using const_iterator         = const_pointer; // See [string.view.iterators]
  using iterator               = const_iterator;
  //using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  //using reverse_iterator       = const_reverse_iterator;
  using size_type              = std::size_t;
  using difference_type        = std::ptrdiff_t;
  static constexpr size_type npos = size_type(-1);

  // [string.view.cons], construct/copy
  constexpr string_view() noexcept
    : data_(nullptr), size_(0) {}

  constexpr string_view(const string_view&) noexcept = default;

  constexpr string_view& operator=(const string_view&) noexcept = default;

  constexpr string_view(const char* str, size_type len) noexcept
    : data_(str), size_(len) {
    assert(size_ == 0 || data_ != nullptr);
  }

  constexpr string_view(const char* str)
    : data_(str), size_(detail::strlen(str)) {
    assert(data_ != nullptr);
  }

  // XXX Extension: construction from a const std::string&
  template <typename Allocator>
  string_view(const std::basic_string<char, traits, Allocator>& str) noexcept
    : data_(str.data()), size_(str.length()) {}

  // [string.view.iterators], iterators
  constexpr const_iterator   begin() const noexcept { return data_; }
  constexpr const_iterator  cbegin() const noexcept { return data_; }
  constexpr const_iterator     end() const noexcept { return data_ + size_; }
  constexpr const_iterator    cend() const noexcept { return data_ + size_; }
  //const_reverse_iterator  rbegin() const noexcept;
  //const_reverse_iterator crbegin() const noexcept;
  //const_reverse_iterator    rend() const noexcept;
  //const_reverse_iterator   crend() const noexcept;

  // [string.view.capacity], capacity
  constexpr size_type size()     const noexcept { return size_; }
  constexpr size_type length()   const noexcept { return size_; }
  constexpr size_type max_size() const noexcept { return (std::numeric_limits<size_type>::max)(); }
  constexpr bool empty()         const noexcept { return size_ == 0; }

  // [string.view.access], element access
  constexpr const_reference operator[](size_type pos) const noexcept { return data_[pos]; }

  constexpr const_reference at(size_t pos) const {
    if (pos >= size_)
      throw_out_of_range("string_view::at");
    return data_[pos];
  }

  constexpr const_reference front() const noexcept {
    assert(!empty());
    return data_[0];
  }

  constexpr const_reference back() const noexcept {
    assert(!empty());
    return data_[size_-1];
  }

  constexpr const_pointer data() const noexcept { return data_; }

  // [string.view.modifiers], modifiers:
  constexpr void remove_prefix(size_type n) noexcept {
    if (n > size_)
      n = size_;
    data_ += n;
    size_ -= n;
  }

  constexpr void remove_suffix(size_type n) noexcept {
    if ( n > size_ )
      n = size_;
    size_ -= n;
  }

  constexpr void swap(string_view& other) noexcept {
    const char *ptr = data_;
    data_ = other.data_;
    other.data_ = ptr;

    size_type len = size_;
    size_ = other.size_;
    other.size_ = len;
  }

  size_type copy(char* s, size_type n, size_type pos = 0) const;

  constexpr string_view substr(size_type pos, size_type n = npos) const {
    if (pos > size_)
      throw_out_of_range("string_view::substr");
    const size_type rlen = size_ - pos;
    return string_view(data_ + pos, n < rlen ? n : rlen);
  }

  constexpr int compare(string_view sv) const noexcept {
    const size_t rlen = size_ < sv.size_ ? size_ : sv.size_;
    const int cmp = detail::strcmp(data_, sv.data_, rlen);
    return cmp != 0 ? cmp : (size_ == sv.size_ ? 0 : size_ < sv.size_ ? -1 : 1);
  }
  constexpr int compare(size_type pos1, size_type n1, string_view sv) const {
    return substr(pos1, n1).compare(sv);
  }
  constexpr int compare(                size_type pos1, size_type n1,
                        string_view sv, size_type pos2, size_type n2) const {
    return substr(pos1, n1).compare(sv.substr(pos2, n2));
  }
  constexpr int compare(const char* s) const noexcept {
    return compare(string_view(s));
  }
  constexpr int compare(size_type pos1, size_type n1, const char* s) const {
      return substr(pos1, n1).compare(string_view(s));
  }
  constexpr int compare(size_type pos1, size_type n1, const char* s, size_type n2) const {
      return substr(pos1, n1).compare(string_view(s, n2));
  }

  // find
  /* constexpr */ size_type find(string_view sv, size_type pos = 0) const noexcept;
  /* constexpr */ size_type find(char c, size_type pos = 0) const noexcept;
  /* constexpr */ size_type find(const char* s, size_type pos, size_type n) const {
    return find(string_view(s, n), pos);
  }
  /* constexpr */ size_type find(const char* s, size_type pos = 0) const {
    return find(string_view(s), pos);
  }

  // rfind
  //constexpr size_type rfind(basic_string_view s, size_type pos = npos) const noexcept;
  //constexpr size_type rfind(charT c, size_type pos = npos) const noexcept;
  //constexpr size_type rfind(const charT* s, size_type pos, size_type n) const;
  //constexpr size_type rfind(const charT* s, size_type pos = npos) const;

  // find_first_of
  /* constexpr */ size_type find_first_of(string_view sv, size_type pos = 0) const noexcept;
  /* constexpr */ size_type find_first_of(char c, size_type pos = 0) const noexcept {
    return find(c, pos);
  }
  /* constexpr */ size_type find_first_of(const char* s, size_type pos, size_type n) const {
    assert(n == 0 || s != nullptr);
    return find_first_of(string_view(s, n), pos);
  }
  /* constexpr */ size_type find_first_of(const char* s, size_type pos = 0) const {
    assert(s != nullptr);
    return find_first_of(string_view(s), pos);
  }

  // find_last_of
  //constexpr size_type find_last_of(basic_string_view s, size_type pos = npos) const noexcept;
  //constexpr size_type find_last_of(charT c, size_type pos = npos) const noexcept;
  //constexpr size_type find_last_of(const charT* s, size_type pos, size_type n) const;
  //constexpr size_type find_last_of(const charT* s, size_type pos = npos) const;

  // find_first_not_of
  //constexpr size_type find_first_not_of(basic_string_view s, size_type pos = 0) const noexcept;
  //constexpr size_type find_first_not_of(charT c, size_type pos = 0) const noexcept;
  //constexpr size_type find_first_not_of(const charT* s, size_type pos, size_type n) const;
  //constexpr size_type find_first_not_of(const charT* s, size_type pos = 0) const;

  // find_last_not_of
  //constexpr size_type find_last_not_of(basic_string_view s, size_type pos = npos) const noexcept;
  //constexpr size_type find_last_not_of(charT c, size_type pos = npos) const noexcept;
  //constexpr size_type find_last_not_of(const charT* s, size_type pos, size_type n) const;
  //constexpr size_type find_last_not_of(const charT* s, size_type pos = npos) const;

  // [string.view.comparison]
  friend constexpr bool operator==(string_view lhs, string_view rhs) noexcept {
    return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
  }
  friend constexpr bool operator!=(string_view lhs, string_view rhs) noexcept {
    return !(lhs == rhs);
  }
  friend constexpr bool operator<(string_view lhs, string_view rhs) noexcept {
    return lhs.compare(rhs) < 0;
  }
  friend constexpr bool operator>(string_view lhs, string_view rhs) noexcept {
    return lhs.compare(rhs) > 0;
  }
  friend constexpr bool operator<=(string_view lhs, string_view rhs) noexcept {
    return lhs.compare(rhs) <= 0;
  }
  friend constexpr bool operator>=(string_view lhs, string_view rhs) noexcept {
    return lhs.compare(rhs) >= 0;
  }

  // XXX Extension: explicit conversion to std::string
  template <typename Allocator>
  explicit operator std::basic_string<char, traits, Allocator>() const {
    return std::basic_string<char, traits, Allocator>(data_, size_);
  }

private:
  /* [[noreturn]] */ static void throw_out_of_range(const char* msg);

  const char* data_;
  std::size_t size_;
};

// C++20 extension. Implemented as free functions as it's not available in C++17 so it's easier to port.
constexpr bool starts_with(string_view sv, string_view s) noexcept {
  return sv.size() >= s.size() && sv.compare(0, s.size(), s) == 0;
}
constexpr bool starts_with(string_view sv, char c) noexcept {
  return !sv.empty() && string_view::traits::eq(sv.front(), c);
}
constexpr bool ends_with(string_view sv, string_view s) noexcept {
  return sv.size() >= s.size() && sv.compare(sv.size() - s.size(), string_view::npos, s) == 0;
}
constexpr bool ends_with(string_view sv, char c) noexcept {
  return !sv.empty() && string_view::traits::eq(sv.back(), c);
}

// explicit conversion to string
inline std::string to_string(string_view sv) {
  return std::string(sv);
}

// fmtlib support
constexpr fmt::string_view to_string_view(string_view str) {
  return fmt::string_view(str.data(), str.size());
}

} // namespace util

// fmtlib support
namespace fmt {
template <>
struct formatter<util::string_view> : public formatter<string_view> {
  template <typename FormatContext>
  auto format(util::string_view str, FormatContext& ctx) -> decltype(ctx.out()) {
    return formatter<string_view>::format(to_string_view(str), ctx);
  }
};
} // namespace fmt

#endif // SC_UTIL_STRING_VIEW_HPP_INCLUDED
