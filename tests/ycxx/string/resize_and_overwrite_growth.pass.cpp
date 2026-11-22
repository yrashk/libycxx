// [string.capacity]: resize_and_overwrite and reserve grow the capacity geometrically, like
// append, so that the append idioms s.resize_and_overwrite(s.size() + k, op) and
// s.reserve(s.size() + k); s.append(...) take amortized O(k) per call, not a reallocation each.
#include <cstddef>
#include <memory>
#include <string>
#include "check.hpp"

namespace {
int allocations = 0;

template <class T>
struct GrowthAllocator {
  using value_type = T;
  GrowthAllocator() = default;
  template <class U>
  GrowthAllocator(const GrowthAllocator<U>&) noexcept {}
  T* allocate(std::size_t n) {
    ++allocations;
    return std::allocator<T>{}.allocate(n);
  }
  void deallocate(T* p, std::size_t n) noexcept { std::allocator<T>{}.deallocate(p, n); }
  friend bool operator==(const GrowthAllocator&, const GrowthAllocator&) noexcept { return true; }
};

using S = std::basic_string<char, std::char_traits<char>, GrowthAllocator<char>>;
}

int main() {
  {
    S s;
    allocations = 0;
    for (int i = 0; i != 10000; ++i)
      s.resize_and_overwrite(s.size() + 16, [](char* p, std::size_t n) {
        for (std::size_t j = n - 16; j != n; ++j)
          p[j] = static_cast<char>('a' + j % 26);
        return n;
      });
    CHECK(s.size() == 160000 && s[0] == 'a' && s[159999] == static_cast<char>('a' + 159999 % 26));
    CHECK(s.data()[s.size()] == '\0');
    CHECK_SAY(allocations <= 20, "%d allocations for 10000 appends", allocations);
  }
  {
    S s;
    allocations = 0;
    for (int i = 0; i != 10000; ++i) {
      s.reserve(s.size() + 16);
      s.append(16, 'x');
    }
    CHECK(s.size() == 160000);
    CHECK_SAY(allocations <= 20, "%d allocations for 10000 appends", allocations);
  }
  {
    S s;
    s.reserve(100);
    CHECK(s.capacity() >= 100);
  }
}
