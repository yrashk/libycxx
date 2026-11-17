// [string.capacity]: resize_and_overwrite and reserve grow the capacity geometrically, like
// append, so that the append idioms s.resize_and_overwrite(s.size() + k, op) and
// s.reserve(s.size() + k); s.append(...) take amortized O(k) per call, not a reallocation each.
// And libycxx's documented behaviour (STATUS.md) for an operation that throws, which violates
// resize_and_overwrite's precondition: when the string had to grow it is unchanged; otherwise
// its size is unchanged and it is still null-terminated.
#include <cstddef>
#include <string>
#include "check.hpp"
#include "test_allocators.hpp"

using S = std::basic_string<char, std::char_traits<char>, CountingAlloc<char>>;

int main() {
  {
    S s;
    alloc_counters.allocations = 0;
    for (int i = 0; i != 10000; ++i)
      s.resize_and_overwrite(s.size() + 16, [](char* p, std::size_t n) {
        for (std::size_t j = n - 16; j != n; ++j)
          p[j] = static_cast<char>('a' + j % 26);
        return n;
      });
    CHECK(s.size() == 160000 && s[0] == 'a' && s[159999] == static_cast<char>('a' + 159999 % 26));
    CHECK(s.data()[s.size()] == '\0');
    CHECK_SAY(alloc_counters.allocations <= 20, "%d allocations for 10000 appends", alloc_counters.allocations);
  }
  {
    S s;
    alloc_counters.allocations = 0;
    for (int i = 0; i != 10000; ++i) {
      s.reserve(s.size() + 16);
      s.append(16, 'x');
    }
    CHECK(s.size() == 160000);
    CHECK_SAY(alloc_counters.allocations <= 20, "%d allocations for 10000 appends", alloc_counters.allocations);
  }
  {
    S s;
    s.reserve(100);
    CHECK(s.capacity() >= 100);
  }
  {
    // Growing: the operation writes into the new block, which is discarded.
    S s(40, 'a');
    const std::size_t cap = s.capacity();
    const char* data = s.data();
    long outstanding = alloc_counters.outstanding;
    bool threw = false;
    try {
      s.resize_and_overwrite(cap + 100, [](char* p, std::size_t n) -> std::size_t {
        for (std::size_t i = 0; i != n; ++i)
          p[i] = 'X';
        throw 1;
      });
    } catch (int) {
      threw = true;
    }
    CHECK(threw && s == S(40, 'a') && s.capacity() == cap && s.data() == data && s.data()[40] == '\0');
    CHECK(alloc_counters.outstanding == outstanding);
  }
  {
    // In place: the size and the terminator survive.
    S s(40, 'a');
    s.reserve(100);
    bool threw = false;
    try {
      s.resize_and_overwrite(80, [](char* p, std::size_t n) -> std::size_t {
        for (std::size_t i = 0; i != n; ++i)
          p[i] = 'X';
        throw 1;
      });
    } catch (int) {
      threw = true;
    }
    CHECK(threw && s.size() == 40 && s.data()[40] == '\0');
  }
}
