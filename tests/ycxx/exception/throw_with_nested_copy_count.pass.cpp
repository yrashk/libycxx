// [except.nested]/8: the thrown exception is "constructed from std::forward<T>(t)" (wrapped
// case), "otherwise std::forward<T>(t)" (thrown as-is). So an rvalue argument is never copied
// (only moved), and an lvalue argument is copied (exactly once: the only copy source is t).
#include <exception>
#include <utility>
#include "check.hpp"

template <bool Final>
struct CountedBase {
  static inline int copies = 0;
  static inline int moves = 0;
  int v;
  explicit CountedBase(int x) : v(x) {}
  CountedBase(const CountedBase& o) : v(o.v) { ++copies; }
  CountedBase(CountedBase&& o) noexcept : v(o.v) { ++moves; }
  virtual ~CountedBase() = default;
  static void reset() { copies = moves = 0; }
};
struct Open : CountedBase<false> {
  using CountedBase::CountedBase;
};
struct Closed final : CountedBase<true> {
  using CountedBase::CountedBase;
};

template <class X, class Arg>
int run(Arg&& a) {
  int got = 0;
  try {
    try {
      throw 1;
    } catch (...) {
      std::throw_with_nested(std::forward<Arg>(a));
    }
  } catch (const X& x) {
    got = x.v;
  }
  return got;
}

int main() {
  {
    Open o(5);
    Open::reset();
    CHECK(run<Open>(std::move(o)) == 5);
    CHECK(Open::copies == 0);
    CHECK(Open::moves >= 1);

    Open l(6);
    Open::reset();
    CHECK(run<Open>(l) == 6);
    CHECK(Open::copies == 1);

    const Open c(7);
    Open::reset();
    CHECK(run<Open>(c) == 7);
    CHECK(Open::copies == 1);
  }
  {
    Closed o(5);
    Closed::reset();
    CHECK(run<Closed>(std::move(o)) == 5);
    CHECK(Closed::copies == 0);
    CHECK(Closed::moves >= 1);

    Closed l(6);
    Closed::reset();
    CHECK(run<Closed>(l) == 6);
    CHECK(Closed::copies == 1);
  }
  return 0;
}
