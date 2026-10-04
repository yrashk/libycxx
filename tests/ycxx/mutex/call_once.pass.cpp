// [thread.once.callonce]/2: "An execution of call_once that does not call its func is a passive
// execution. ... If such a call to func throws an exception the execution is exceptional,
// otherwise it is returning. An exceptional execution propagates the exception to the caller of
// call_once. Among all executions of call_once for any given once_flag: at most one is a
// returning execution; if there is a returning execution, it is the last active execution; and
// there are passive executions only if there is a returning execution." INVOKE is used (member
// pointers, arguments forwarded). [thread.once.onceflag]: constexpr once_flag() noexcept,
// not copyable.
// FLAGS: -pthread
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <stdexcept>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_nothrow_default_constructible_v<std::once_flag>);
static_assert(!std::is_copy_constructible_v<std::once_flag>);
static_assert(!std::is_copy_assignable_v<std::once_flag>);
constinit std::once_flag global_flag;

struct Info {
  int verified = 0;
  void verify(int by) { verified += by; }
};

int main() {
  std::once_flag f;
  int calls = 0;
  std::call_once(f, [&] { ++calls; });
  std::call_once(f, [&] { ++calls; });  // passive
  CHECK(calls == 1);

  // exceptional executions do not count; the next call is active again
  std::once_flag g;
  int attempts = 0;
  for (int i = 0; i < 3; ++i) {
    try {
      std::call_once(g, [&] {
        ++attempts;
        if (attempts < 3) throw std::runtime_error("again");
      });
    } catch (const std::runtime_error&) {
      CHECK(attempts < 3);
    }
  }
  CHECK(attempts == 3);
  std::call_once(g, [&] { ++attempts; });
  CHECK(attempts == 3);

  // INVOKE with arguments and a member pointer
  Info info;
  std::once_flag h;
  std::call_once(h, &Info::verify, info, 5);  // INVOKE(f, info, 5): info is an lvalue reference
  CHECK(info.verified == 5);
  int out = 0;
  std::call_once(global_flag, [](int& o, int v) { o = v; }, out, 7);
  CHECK(out == 7);

  // concurrent: exactly one returning execution; passive executions observe its effects
  std::once_flag cf;
  std::atomic<int> runs(0);
  int value = 0;
  std::vector<std::thread> ts;
  std::vector<int> seen(6, -1);
  for (int t = 0; t < 6; ++t)
    ts.emplace_back([&, t] {
      std::call_once(cf, [&] { runs.fetch_add(1); value = 99; });
      seen[t] = value;
    });
  for (auto& th : ts) th.join();
  CHECK(runs.load() == 1);
  for (int v : seen) CHECK(v == 99);

  // concurrent with exceptions: the first k active executions throw, one returns
  std::once_flag ef;
  std::atomic<int> active(0), threw(0);
  std::vector<std::thread> ts2;
  for (int t = 0; t < 4; ++t)
    ts2.emplace_back([&] {
      try {
        std::call_once(ef, [&] {
          if (active.fetch_add(1) < 2) throw 1;
        });
      } catch (int) {
        threw.fetch_add(1);
      }
    });
  for (auto& th : ts2) th.join();
  CHECK(active.load() == 3);  // two exceptional, then one returning; the rest passive
  CHECK(threw.load() == 2);
  return 0;
}
