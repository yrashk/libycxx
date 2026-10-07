// [assertions.assert]/2.3: a failed assert writes a diagnostic to the standard error stream that
// contains #__VA_ARGS__ and the names of the source file and the enclosing function, then calls
// abort(). The enclosing function here is a function template specialization whose name runs to
// kilobytes (as in Abseil's raw_hash_set tests, whose death tests look for the asserted
// expression's text); the expression must still be there in full, after the name.
#include <cassert>
#include <csignal>
#include <string>
#include "child_process.hpp"
#include "check.hpp"

template <int N>
struct Long {
  using type = typename Long<N - 1>::type;
};
template <>
struct Long<0> {
  using type = int;
};
// With a negative argument: the name of the function, as the assert macro sees it (GCC spells
// out the template arguments, Clang gives only "fail").
template <class... Ts>
const char* fail(int x) {
  if (x < 0)
    return __builtin_FUNCTION();
  assert(x == 42 && "the text after a very long function name");
  return nullptr;
}
template <int... Ns>
constexpr const char* (*many)(int) = fail<Long<Ns>...>;
constexpr auto victim = many<1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
                    24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
                    45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
                    66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80>;

int main(int argc, char** argv) {
  if (child_mode_is("fail")) {
    victim(argc);
    return 0;
  }
  (void)argv;
  ChildResult r = run_self("fail");
  CHECK(r.status == 1000 + SIGABRT);
  CHECK(r.err.find("x == 42 && \"the text after a very long function name\"") != std::string::npos);
  CHECK(r.err.find("assert_long_function_name.pass.cpp") != std::string::npos);
  const std::string name = victim(-1);
  CHECK(r.err.find(name + ": Assertion") != std::string::npos);
#if defined(__GNUC__) && !defined(__clang__)
  CHECK(name.size() > 600); // the case this test is about: longer than a 512-byte buffer
#endif
  return 0;
}
