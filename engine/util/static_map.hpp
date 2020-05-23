// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#ifndef SC_UTIL_STATIC_MAP_HPP_INCLUDED
#define SC_UTIL_STATIC_MAP_HPP_INCLUDED

#include <cassert>
#include <cstddef>
#include <utility>

namespace util {
namespace detail {

// minimal std::array clone, std::array non-const operator[] is constexpr only in c++17
template <typename T, size_t N>
struct array {
  T data_[N] {};

  constexpr T& operator[](size_t n) { return data_[n]; }
  constexpr const T& operator[](size_t n) const { return data_[n]; }

  constexpr const T* begin() const { return data_; }
  constexpr const T* end() const { return data_ + N; }
};

template <typename T, typename U, size_t N, size_t... I>
constexpr array<T, N> to_array_impl(const U (&init)[N], std::index_sequence<I...>) {
  return {{ static_cast<T>(init[I])... }};
}

template <typename T, typename U, size_t N>
constexpr array<T, N> to_array(const U (&init)[N]) {
  return to_array_impl<T>(init, std::make_index_sequence<N>{});
}

// *very* minimal std::pair clone, std::pair operator= is constexpr only in c++20
template <typename T, typename U>
struct pair {
  T first;
  U second;
};

struct less {
  template <typename T, typename U>
  constexpr bool operator()(const T& lhs, const U& rhs) const {
    return lhs < rhs;
  }
  template <typename T, typename U>
  constexpr bool operator()(const pair<T, U>& lhs, const pair<T, U>& rhs) const {
    return lhs.first < rhs.first;
  }
  template <typename T, typename T2, typename U2>
  constexpr bool operator()(const T& lhs, const pair<T2, U2>& rhs) const {
    return lhs < rhs.first;
  }
};

template <typename T, size_t N, typename Compare = less>
constexpr array<T, N> sorted(const array<T, N>& items, Compare cmp = {}) {
  // simple insertion sort
  array<T, N> res = items;
  for (size_t i = 1; i < N; i++) {
    T temp = res[i];
    ptrdiff_t j = i - 1;
    for (; j >= 0 && cmp(temp, res[j]); j--)
      res[j + 1] = res[j];
    res[j + 1] = temp;
  }
  return res;
}

template <typename T, size_t N, typename U, typename Compare = less>
constexpr const T* lower_bound(const array<T, N>& items, const U& value, Compare cmp = {}) {
  size_t l = 0;
  size_t r = N - 1;
  while (l != r) {
    const size_t mid = (l + r + 1) / 2;
    if (cmp(value, items[mid]))
      r = mid - 1;
    else
      l = mid;
  }
  return &items[l];
}

template <typename T, size_t N, typename Compare = less>
constexpr bool is_unique(const array<T, N>& items, Compare cmp = {}) {
  if (N <= 1)
    return true;
  for (size_t i = 1; i < N; i++) {
    if (!cmp(items[i-1], items[i]))
      return false;
  }
  return true;
}

struct dummy {};

template <typename T, size_t N>
class static_set_base {
  static_assert(N > 0, "Empty maps/sets are not supported.");
public:
  using value_type      = T;
  using size_type       = size_t;
  using difference_type = ptrdiff_t;
  using reference       = const value_type&;
  using const_reference = reference;
  using pointer         = const value_type*;
  using const_pointer   = pointer;
  using iterator        = pointer;
  using const_iterator  = pointer;

  constexpr static_set_base(const array<value_type, N>& items)
    : data_(sorted(items))
  {
    assert(is_unique(data_));
  }

  constexpr iterator begin() const { return cbegin(); }
  constexpr iterator cbegin() const { return data_.begin(); }

  constexpr iterator end() const { return cend(); }
  constexpr iterator cend() const { return data_.end(); }

  constexpr size_t size() const { return N; }
  constexpr bool empty() const { return false; }

protected:
  array<value_type, N> data_;
};

template <typename T>
class static_set_base<T, 0> {
public:
  using value_type      = T;
  using size_type       = size_t;
  using difference_type = ptrdiff_t;
  using reference       = const value_type&;
  using const_reference = reference;
  using pointer         = const value_type*;
  using const_pointer   = pointer;
  using iterator        = pointer;
  using const_iterator  = pointer;

  constexpr static_set_base() = default;

  constexpr iterator begin() const { return nullptr; }
  constexpr iterator cbegin() const { return nullptr; }

  constexpr iterator end() const { return nullptr; }
  constexpr iterator cend() const { return nullptr; }

  constexpr size_t size() const { return 0; }
  constexpr bool empty() const { return true; }

  template <typename U>
  constexpr iterator find(const U&) const { return end(); }
  template <typename U>
  constexpr bool contains(const U&) const { return false; }
};

} // namespace detail

template <typename Key, typename T, size_t N>
class static_map : public detail::static_set_base<detail::pair<Key, T>, N> {
  using base = detail::static_set_base<detail::pair<Key, T>, N>;
public:
  using key_type    = Key;
  using mapped_type = T;

  using base::base;

  constexpr typename base::iterator find(const Key& key) const {
    auto it = detail::lower_bound(this->data_, key);
    return it->first == key ? it : this->end();
  }

  constexpr bool contains(const Key& key) const {
    return find(key) != this->end();
  }
};

template <typename Key, typename T>
class static_map<Key, T, 0> : public detail::static_set_base<detail::pair<Key, T>, 0> {
public:
  using key_type    = Key;
  using mapped_type = T;
};

template <typename T, typename U>
constexpr auto make_static_map(detail::dummy = {}) -> static_map<T, U, 0> {
  return {};
}

template <typename T, typename U, size_t N>
constexpr auto make_static_map(const detail::pair<T, U> (&items)[N]) -> static_map<T, U, N> {
  return { detail::to_array<detail::pair<T, U>>(items) };
}

template <typename T, size_t N>
class static_set : public detail::static_set_base<T, N> {
  using base = detail::static_set_base<T, N>;
public:
  using base::base;

  constexpr typename base::iterator find(const T& value) const {
    auto it = detail::lower_bound(this->data_, value);
    return *it == value ? it : this->end();
  }

  constexpr bool contains(const T& value) const {
    return find(value) != this->end();
  }
};

template <typename T>
class static_set<T, 0> : public detail::static_set_base<T, 0> {};

template <typename T>
constexpr auto make_static_set(detail::dummy = {}) -> static_set<T, 0> {
  return {};
}

template <typename T, typename U, size_t N>
constexpr auto make_static_set(const U (&items)[N]) -> static_set<T, N> {
  return { detail::to_array<T>(items) };
}

} // namespace util

#endif // SC_UTIL_STATIC_MAP_HPP_INCLUDED
