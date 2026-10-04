// [charconv.syn]/1: "When a function is specified with a type placeholder of
// floating-point-type, the implementation provides overloads for all cv-unqualified
// floating-point types ([basic.fundamental]) in lieu of floating-point-type" -- including
// the extended floating-point types of <stdfloat> the implementation defines. Each round
// trips through to_chars / from_chars in every format.
#include <charconv>
#include <stdfloat>
#include <string_view>
#include <system_error>
#include "check.hpp"

template <class T>
void check_type(T v, std::string_view shortest) {
  char buf[128];
  auto r = std::to_chars(buf, buf + sizeof buf, v);
  CHECK(r.ec == std::errc{});
  CHECK(std::string_view(buf, r.ptr) == shortest);
  for (auto f : {std::chars_format::general, std::chars_format::fixed, std::chars_format::scientific,
                 std::chars_format::hex}) {
    r = std::to_chars(buf, buf + sizeof buf, v, f);
    CHECK(r.ec == std::errc{});
    T back{};
    auto p = std::from_chars(buf, r.ptr, back, f);
    CHECK(p.ec == std::errc{} && p.ptr == r.ptr && back == v);
    r = std::to_chars(buf, buf + sizeof buf, v, f, 4);
    CHECK(r.ec == std::errc{});
  }
  T x{};
  std::string_view s = "1.5";
  auto p = std::from_chars(s.data(), s.data() + s.size(), x);
  CHECK(p.ec == std::errc{} && x == T(1.5));
}

int main() {
#ifdef __STDCPP_FLOAT16_T__
  check_type<std::float16_t>(0.1f16, "0.1");
#endif
#ifdef __STDCPP_FLOAT32_T__
  check_type<std::float32_t>(0.1f32, "0.1");
#endif
#ifdef __STDCPP_FLOAT64_T__
  check_type<std::float64_t>(0.1f64, "0.1");
#endif
#ifdef __STDCPP_BFLOAT16_T__
  check_type<std::bfloat16_t>(0.5bf16, "0.5");
#endif
  return 0;
}
