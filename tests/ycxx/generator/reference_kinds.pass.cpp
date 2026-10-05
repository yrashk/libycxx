// [coro.generator.class]: yielded is reference if it is a reference type, else const
// reference&; [coro.generator.promise]/4: yield_value(yielded val) makes value_ point to val
// itself; /6-9: for an rvalue-reference yielded type, co_yield of an lvalue (or of a const
// rvalue) selects yield_value(const remove_reference_t<yielded>&), which stores a copy
// "direct-non-list-initialized with lval" in the awaitable -- the coroutine's object is not
// moved from by a consumer that moves from *it; /8: "Throws: Any exception thrown by the
// initialization of the stored object" -- thrown at the co_yield inside the coroutine body,
// where the coroutine itself can catch it. [coro.generator.iterator]/5: operator* returns
// static_cast<reference>(*p.value_).
// So: generator<T&> lets the consumer modify the coroutine's object; generator<const T&>
// yields the object itself (same address) or a temporary that lives until the coroutine
// resumes; generator<T&&> with co_yield std::move(x) hands out x itself (a consumer that moves
// from it empties x), with co_yield x a copy; generator<T> behaves like generator<T&&>;
// generator<string_view, string> (non-reference Ref) yields const string_view&.
// REQUIRES: exceptions
#include <generator>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include "check.hpp"

static const char* const longs = "a string that is too long for any small-string buffer, 0123456789";

struct Counted {
  static inline int copies = 0, moves = 0;
  std::string s;
  explicit Counted(std::string x) : s(std::move(x)) {}
  Counted(const Counted& o) : s(o.s) { ++copies; }
  Counted(Counted&& o) noexcept : s(std::move(o.s)) { ++moves; }
};

std::generator<int&> lref(int& out_seen) {
  int x = 1;
  co_yield x;      // consumer adds 10
  out_seen = x;
  co_yield x;
}

std::generator<const std::string&> cref(const std::string*& addr, bool& still_same) {
  std::string local = longs;
  addr = &local;
  co_yield local;  // the object itself
  still_same = (local == longs);
  co_yield std::string("temporary, long enough to need the heap 0123456789");  // a temporary
}

struct Box {  // a move leaves -1 behind
  int v;
  explicit Box(int x) : v(x) {}
  Box(const Box& o) : v(o.v) {}
  Box(Box&& o) noexcept : v(o.v) { o.v = -1; }
};

std::generator<Box&&> rref(int& after_lvalue, int& after_xvalue) {
  Box a(1);
  co_yield a;  // lvalue: a copy is yielded
  after_lvalue = a.v;
  Box b(2);
  co_yield std::move(b);  // xvalue: b itself
  after_xvalue = b.v;
}

std::generator<Counted> by_value(int& copies_at_yield) {
  Counted c(longs);
  Counted::copies = 0;
  co_yield c;  // one copy into the awaitable
  copies_at_yield = Counted::copies;
  co_yield Counted("prvalue");
  const Counted cc(longs);
  co_yield std::move(cc);  // const rvalue: also the copying overload
}

struct ThrowCopy {
  bool armed;
  explicit ThrowCopy(bool a) : armed(a) {}
  ThrowCopy(const ThrowCopy& o) : armed(o.armed) {
    if (armed) throw std::runtime_error("copy");
  }
  ThrowCopy(ThrowCopy&& o) noexcept : armed(false) { (void)o; }
};

std::generator<ThrowCopy&&> throwing_copy(bool& caught) {
  ThrowCopy t(true);
  try {
    co_yield t;  // the stored copy throws inside the coroutine body
  } catch (const std::runtime_error&) {
    caught = true;
  }
  co_yield ThrowCopy(false);
}

std::generator<std::string_view, std::string> views_of() {
  co_yield "abc";               // converts to a temporary string_view
  std::string s = "def";
  co_yield s;
}

int main() {
  {
    int seen = 0;
    auto g = lref(seen);
    auto it = g.begin();
    static_assert(std::is_same_v<decltype(*it), int&>);
    *it += 10;
    ++it;
    CHECK(seen == 11 && *it == 11);
  }
  {
    const std::string* addr = nullptr;
    bool same = false;
    auto g = cref(addr, same);
    auto it = g.begin();
    static_assert(std::is_same_v<decltype(*it), const std::string&>);
    CHECK(&*it == addr && *it == longs);
    ++it;
    CHECK(same && *it == "temporary, long enough to need the heap 0123456789");
  }
  {
    int after_l = 0, after_x = 0;
    auto g = rref(after_l, after_x);
    auto it = g.begin();
    static_assert(std::is_same_v<decltype(*it), Box&&>);
    Box taken = *it;  // moves from the yielded copy
    CHECK(taken.v == 1);
    ++it;
    CHECK(after_l == 1);  // the coroutine's lvalue was not moved from
    Box taken2 = *it;  // moves from b itself
    CHECK(taken2.v == 2);
    ++it;
    CHECK(after_x == -1);  // b was moved from by the consumer
    CHECK(it == std::default_sentinel);
  }
  {
    int copies = -1;
    auto g = by_value(copies);
    auto it = g.begin();
    Counted got = *it;
    CHECK(got.s == longs);
    ++it;
    CHECK(copies == 1);
    CHECK((*it).s == "prvalue");
    int before = Counted::copies;
    ++it;
    CHECK(Counted::copies == before + 1 && (*it).s == longs);
  }
  {
    bool caught = false;
    int n = 0;
    for (ThrowCopy&& t : throwing_copy(caught)) {
      CHECK(!t.armed);
      ++n;
    }
    CHECK(caught && n == 1);
  }
  {
    auto g = views_of();
    auto it = g.begin();
    static_assert(std::is_same_v<decltype(*it), std::string_view>);
    static_assert(std::is_same_v<std::ranges::range_value_t<decltype(g)>, std::string>);
    CHECK(*it == "abc");
    ++it;
    CHECK(*it == "def");
  }
  return 0;
}
