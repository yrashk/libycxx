// [std.modules]/3: std.compat exports what std exports and, in the global namespace, the names
// the C++ headers for C library facilities declare in std (3.1) and the declarations of
// <stdbit.h> and <stdckdint.h> (3.2); still no macros.
// MODULES: std.compat
import std.compat;
#include "module_check.hpp"

int main() {
  CHECK(::strlen("Hello modular world") == 19);
  ::size_t n = 3;
  ::uint8_t b = 0xF0;
  ::int64_t big = 1;
  CHECK(n == 3 && b == 0xF0 && big == 1);
  CHECK(::sqrt(9.0) == 3.0 && ::abs(-4) == 4 && ::fabs(-0.5) == 0.5 && ::isnan(::nan("")));
  ::FILE* out = nullptr;
  CHECK(out == nullptr);
  ::printf("::printf through std.compat: %d\n", 1);
  ::tm when{};
  when.tm_year = 100;
  CHECK(::time(nullptr) > 0);
  void* p = ::malloc(4);
  ::free(p);
  ::div_t d = ::div(9, 4);
  CHECK(d.quot == 2 && d.rem == 1);
  CHECK(::toupper('q') == 'Q' && ::memchr("abc", 'c', 3) != nullptr);
  ::max_align_t* m = nullptr;
  ::nullptr_t np = nullptr;
  CHECK(m == np);
  // <stdbit.h> and <stdckdint.h> ([std.modules]/3.2).
  CHECK(::stdc_count_ones(0xF0u) == 4 && ::stdc_bit_width_ui(8u) == 4);
  int sum = 0;
  CHECK(!::ckd_add(&sum, 2, 3) && sum == 5);
  CHECK(::ckd_mul(&sum, 2147483647, 2));
  // Everything of std too.
  std::vector<int> v{1, 2};
  CHECK(std::ranges::size(v) == 2 && std::strlen("x") == 1);
#if defined(EOF) || defined(NULL) || defined(UINT8_MAX) || defined(errno) || defined(assert) || defined(ckd_add)
  CHECK(!"a macro is defined after import std.compat;");
#endif
  return 0;
}
