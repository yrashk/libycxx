// [std.modules]/2: `import std;` exports the declarations in namespace std of the C++ headers for
// C library facilities (Table 25), but none of their macros ([module.import]/7) and none of the
// global C names (those are std.compat's, import_std_compat.pass.cpp).
// MODULES: std
import std;
#include "module_check.hpp"

int main() {
  const char text[] = "module";
  CHECK(std::strlen(text) == 6 && std::strcmp(text, "module") == 0);
  char copy[8];
  std::memcpy(copy, text, sizeof text);
  CHECK(std::memcmp(copy, text, sizeof text) == 0);
  std::size_t n = sizeof(std::int64_t);
  std::uint32_t u = 7;
  std::ptrdiff_t diff = 2;
  std::intmax_t im = std::imaxabs(-3);
  CHECK(n == 8 && u == 7 && diff == 2 && im == 3);
  CHECK(std::sqrt(16.0) == 4.0 && std::abs(-2) == 2 && std::fabs(-1.5) == 1.5 && std::isnan(std::nan("")));
  CHECK(std::div(7, 2).quot == 3 && std::strtol("12", nullptr, 10) == 12);
  CHECK(std::toupper('a') == 'A' && std::iswdigit(L'4'));
  std::time_t now = std::time(nullptr);
  CHECK(now > 0 && std::difftime(now, now) == 0);
  void* p = std::malloc(16);
  CHECK(p != nullptr);
  std::free(p);
  std::printf("std::printf through the module: %d\n", 1);
  char buf[32];
  CHECK(std::snprintf(buf, sizeof buf, "%s", "x") == 1);
  std::mbstate_t st{};
  CHECK(std::mbsinit(&st));
  std::max_align_t* none = nullptr;
  std::nullptr_t np = nullptr;
  CHECK(none == np);
  std::jmp_buf jb;
  (void)jb;
  std::sig_atomic_t sig = 0;
  std::fenv_t env;
  CHECK(std::fegetenv(&env) == 0 && sig == 0);
  std::lconv* lc = std::localeconv();
  CHECK(lc != nullptr);
  std::va_list* vl = nullptr;
  CHECK(vl == nullptr);
  CHECK(std::isinf(std::numeric_limits<double>::infinity()));
  // Macros are not exported: none of these is defined.
#if defined(EOF) || defined(NULL) || defined(INT_MAX) || defined(SIZE_MAX) || defined(errno) || \
    defined(assert) || defined(offsetof) || defined(va_arg) || defined(EXIT_SUCCESS) || defined(HUGE_VAL) || \
    defined(SEEK_SET) || defined(__cpp_lib_ranges) || defined(__cpp_lib_modules)
  CHECK(!"a macro of the library is defined after import std;");
#endif
  return 0;
}
