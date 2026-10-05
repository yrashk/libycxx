// [cfenv.syn]: fegetround/fesetround, feclearexcept/feraiseexcept/fetestexcept,
// fegetexceptflag/fesetexceptflag, fegetenv/fesetenv, feholdexcept/feupdateenv in namespace
// std with the meanings of ISO/IEC 9899:2024 7.6 (this implementation supports testing flags
// and setting rounding modes, Note 1); /2: "The floating-point environment has thread storage
// duration. The initial state for a thread's floating-point environment is the state of the
// floating-point environment of the thread that constructs the corresponding thread object
// ([thread.thread.class]) or jthread object ([thread.jthread.class]) at the time it
// constructed the object." Operands are volatile so that nothing is evaluated at compile time.
// FLAGS: -pthread -frounding-math
#include <cfenv>
#include <thread>
#include "check.hpp"

static volatile double one = 1.0, three = 3.0, zero = 0.0, big = 1e308;

static double third() { return one / three; }

int main() {
  CHECK(std::fegetround() == FE_TONEAREST);
  const double nearest = third();
  CHECK(std::fesetround(FE_UPWARD) == 0 && std::fegetround() == FE_UPWARD);
  const double up = third();
  CHECK(std::fesetround(FE_DOWNWARD) == 0);
  const double down = third();
  CHECK(std::fesetround(FE_TOWARDZERO) == 0);
  const double tz = third();
  CHECK(up > down && tz == down && (nearest == up || nearest == down));

  // a thread starts with the creating thread's environment; its changes stay its own
  CHECK(std::fesetround(FE_UPWARD) == 0);
  int seen = -1, after = -1;
  std::thread th([&] {
    seen = std::fegetround();
    std::fesetround(FE_DOWNWARD);
    after = std::fegetround();
  });
  th.join();
  CHECK(seen == FE_UPWARD && after == FE_DOWNWARD);
  CHECK(std::fegetround() == FE_UPWARD);
  std::jthread jt([&] { seen = std::fegetround(); });
  jt.join();
  CHECK(seen == FE_UPWARD);
  CHECK(std::fesetround(FE_TONEAREST) == 0);

  // exception flags
  CHECK(std::feclearexcept(FE_ALL_EXCEPT) == 0);
  CHECK(std::fetestexcept(FE_ALL_EXCEPT) == 0);
  volatile double r = one / zero;
  (void)r;
  CHECK(std::fetestexcept(FE_DIVBYZERO) == FE_DIVBYZERO && std::fetestexcept(FE_OVERFLOW) == 0);
  r = big * big;
  CHECK(std::fetestexcept(FE_OVERFLOW | FE_DIVBYZERO) == (FE_OVERFLOW | FE_DIVBYZERO));
  std::fexcept_t saved;
  CHECK(std::fegetexceptflag(&saved, FE_DIVBYZERO) == 0);
  CHECK(std::feclearexcept(FE_ALL_EXCEPT) == 0 && std::fetestexcept(FE_ALL_EXCEPT) == 0);
  CHECK(std::fesetexceptflag(&saved, FE_DIVBYZERO) == 0);
  CHECK(std::fetestexcept(FE_ALL_EXCEPT) == FE_DIVBYZERO);
  CHECK(std::feraiseexcept(FE_INVALID) == 0 && std::fetestexcept(FE_INVALID) == FE_INVALID);

  // flags in a thread are separate
  std::feclearexcept(FE_ALL_EXCEPT);
  int thread_flags = -1;
  std::thread t2([&] {
    volatile double q = one / zero;
    (void)q;
    thread_flags = std::fetestexcept(FE_DIVBYZERO);
  });
  t2.join();
  CHECK(thread_flags == FE_DIVBYZERO && std::fetestexcept(FE_DIVBYZERO) == 0);

  // whole environments
  std::fenv_t env;
  CHECK(std::fegetenv(&env) == 0);
  std::fesetround(FE_DOWNWARD);
  std::feraiseexcept(FE_INEXACT);
  CHECK(std::fesetenv(&env) == 0);
  CHECK(std::fegetround() == FE_TONEAREST && std::fetestexcept(FE_INEXACT) == 0);
  std::fenv_t held;
  std::feraiseexcept(FE_UNDERFLOW);
  CHECK(std::feholdexcept(&held) == 0 && std::fetestexcept(FE_ALL_EXCEPT) == 0);
  std::feraiseexcept(FE_OVERFLOW);
  CHECK(std::feupdateenv(&held) == 0);  // restores the held flags and re-raises the current ones
  CHECK(std::fetestexcept(FE_UNDERFLOW | FE_OVERFLOW) == (FE_UNDERFLOW | FE_OVERFLOW));
  CHECK(std::fesetenv(FE_DFL_ENV) == 0 && std::fegetround() == FE_TONEAREST && std::fetestexcept(FE_ALL_EXCEPT) == 0);
  return 0;
}
