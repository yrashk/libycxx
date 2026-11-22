// libycxx's documented behaviour (STATUS.md) for an operation that throws, which violates
// resize_and_overwrite's precondition: when the string had to grow it is unchanged; otherwise
// its size is unchanged and it is still null-terminated.
// REQUIRES: exceptions
#include <cstddef>
#include <string>
#include "check.hpp"
#include "test_allocators.hpp"

using S = std::basic_string<char, std::char_traits<char>, CountingAlloc<char>>;

int main() {
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
