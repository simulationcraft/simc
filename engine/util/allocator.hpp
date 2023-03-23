// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include <util/span.hpp>

#include <cassert>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <vector>

namespace util {

// Allocates memory in an ever-growing pool
//
// Not strictly a true bump-pointer allocator as it uses backing pages instead
// of a boundless contigous heap. However, it is semantically pretty much equivalent
// as it uses a monotonically growing pool of memory.
//
// PageSize is the size of a single page.
//
// Allocating objects that do not fit a single page is a hard error, there is no
// separate allocation path for such cases. It is caught at compile time for the
// single object allocation case but only at runtime for the multi-object case.
template <size_t PageSize = 8192>
class bump_ptr_allocator_t
{
  static constexpr size_t PageAlignment = alignof(std::max_align_t);
public:
  bump_ptr_allocator_t() noexcept = default;

  bump_ptr_allocator_t(const bump_ptr_allocator_t&) = delete;
  bump_ptr_allocator_t& operator=(const bump_ptr_allocator_t&) = delete;

  // XXX: we could easily make it at least move-contructible /shrug
  bump_ptr_allocator_t(bump_ptr_allocator_t&&) = delete;
  bump_ptr_allocator_t& operator=(bump_ptr_allocator_t&&) = delete;

  template <typename T>
  void* allocate() {
    static_assert(fits_in_page(sizeof(T), alignof(T)), "The object does not fit in a single page.");

    return allocate(sizeof(T), alignof(T));
  }

  template <typename T>
  void* allocate(size_t n) {
    static_assert(fits_in_page(sizeof(T), alignof(T)), "Even a single object does not fit in a single page.");

    if (fits_in_page(sizeof(T) * n, alignof(T)))
      return allocate(sizeof(T) * n, alignof(T));

    throw std::bad_alloc();
  }

  // helpers for "fused" allocation + construction
  template <typename T, typename... Args>
  T* create(Args&&... args) {
    return ::new(allocate<T>()) T(std::forward<Args>(args)...);
  }

  template <typename T>
  util::span<T> create_n(size_t n, const T& value = {}) {
    T* p = reinterpret_cast<T*>(allocate<T>(n));
    std::uninitialized_fill_n(p, n, value);
    return {p, n};
  }

private:
  struct page_t {
    alignas(PageAlignment) char data[PageSize]; // NOLINT(modernize-avoid-c-arrays)
    page_t() noexcept = default;
  };

  void* allocate(size_t size, size_t alignment) {
    assert(fits_in_page(size, alignment) && "The allocation does not fit in a single page");

    if (std::align(alignment, size, current_, size_))
        return finalize_allocation(size);

    allocate_page();
    std::align(alignment, size, current_, size_);
    return finalize_allocation(size);
  }

  void* finalize_allocation(size_t size) {
    void* result = current_;
    current_ = static_cast<char*>(current_) + size;
    size_ -= size;
    return result;
  }

  void allocate_page() {
    pages_.push_back(std::make_unique<page_t>());
    current_ = pages_.back()->data;
    size_ = PageSize;
  }

  static constexpr bool fits_in_page(size_t size, size_t alignment) {
    if (alignment <= PageAlignment)
      return size <= PageSize;
    return size <= PageSize - alignment + PageAlignment;
  }

  void* current_ = nullptr; // current pointer inside the page
  size_t size_ = 0; // number of bytes left in the page
  std::vector<std::unique_ptr<page_t>> pages_;
};

} // namespace util
