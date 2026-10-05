// [csetjmp.syn]: std::jmp_buf, [[noreturn]] std::longjmp and the setjmp macro with the
// meanings of ISO/IEC 9899:2024 7.13 (setjmp returns 0 when called directly and val, or 1 if
// val is 0, when returning through longjmp; objects of automatic storage duration local to the
// function containing setjmp that are volatile keep their values), restricted by /2 to jumps
// that would not run non-trivial destructors if replaced by throw/catch: only trivially
// destructible objects live between the setjmp and longjmp here. __STDC_VERSION_SETJMP_H__.
#include <csetjmp>
#include <type_traits>
#include "check.hpp"

static_assert(__STDC_VERSION_SETJMP_H__ == 202311L);
static_assert(std::is_array_v<std::jmp_buf> || std::is_class_v<std::jmp_buf>);

static std::jmp_buf env;
static int depth_reached = 0;

[[noreturn]] static void jump(int val) { std::longjmp(env, val); }

static void recurse(int n, int val) {
  int local[4] = {n, n, n, n};  // trivially destructible
  depth_reached = local[0];
  if (n == 0) jump(val);
  recurse(n - 1, val);
}

int main() {
  volatile int round = 0;
  volatile int sum = 0;
  volatile int r = 0;
  // setjmp only as the whole controlling expression of a switch (C 7.13.2.1/4)
  switch (setjmp(env)) {
    case 0: r = 0; break;
    case 7: r = 7; break;
    case 1: r = 1; break;
    case -5: r = -5; break;
    default: r = 99; break;
  }
  if (r == 0) {
    CHECK(round == 0);
    round = 1;
    recurse(50, 7);
  } else if (round == 1) {
    CHECK(r == 7 && depth_reached == 0 && sum == 0);
    round = 2;
    sum = 10;
    jump(0);  // val 0 makes setjmp return 1
  } else if (round == 2) {
    CHECK(r == 1 && sum == 10);
    round = 3;
    jump(-5);
  } else {
    CHECK(round == 3 && r == -5);
    return 0;
  }
  CHECK(false);
  return 1;
}
