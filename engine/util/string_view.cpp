// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "string_view.hpp"

#include <stdexcept>
#include <algorithm>

util::string_view::size_type util::string_view::copy(char* s, size_type n, size_type pos) const {
  if (pos > size_)
    throw_out_of_range("string_view::copy");
  const size_type rlen = (std::min)(n, size_ - pos);
  (void)traits::copy(s, data_ + pos, rlen);
  return rlen;
}

util::string_view::size_type util::string_view::find(string_view sv, size_type pos) const noexcept {
  assert(sv.size() == 0 || sv.data() != nullptr);
  if (pos >= size_)
    return npos;
  auto it = std::search(cbegin() + pos, cend(), sv.cbegin(), sv.cend(), traits::eq);
  return it == cend() ? npos : it - cbegin();
}

util::string_view::size_type util::string_view::find(char c, size_type pos) const noexcept {
  if (pos >= size_)
    return npos;
  auto it = std::find(cbegin() + pos, cend(), c);
  return it == cend() ? npos : it - cbegin();
}

util::string_view::size_type util::string_view::rfind(string_view sv, size_type pos) const noexcept {
  assert(sv.size() == 0 || sv.data() != nullptr);
  pos = (std::min)(size_, pos);
  if (sv.size() < size_ - pos)
    pos += sv.size();
  else
    pos = size_;
  auto last = cbegin() + pos;
  auto it = std::find_end(cbegin(), last, sv.cbegin(), sv.cend(), traits::eq);
  if (sv.size() > 0 && it == last)
    return npos;
  return it - cbegin();
}

util::string_view::size_type util::string_view::find_first_of(string_view sv, size_type pos) const noexcept {
  assert(sv.size() == 0 || sv.data() != nullptr);
  if (pos >= size_)
    return npos;
  auto it = std::find_first_of(cbegin() + pos, cend(), sv.cbegin(), sv.cend(), traits::eq);
  return it == cend() ? npos : it - cbegin();
}

/* static */ void util::string_view::throw_out_of_range(const char* msg) {
  throw std::out_of_range(msg);
}
