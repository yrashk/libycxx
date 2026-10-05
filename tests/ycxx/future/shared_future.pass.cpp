// [futures.shared.future]: shared_future is copyable; copies refer to the same shared state;
// get() does not release the state (valid() stays true) and returns const R& (R& for R&, void
// for void); several threads may call get on their own copies. [futures.unique.future]/13-14:
// share() returns shared_future<R>(std::move(*this)) and leaves valid() == false.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <future>
#include <thread>
#include <vector>
#include <stdexcept>
#include <string>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_copy_constructible_v<std::shared_future<int>>);
static_assert(std::is_nothrow_copy_constructible_v<std::shared_future<int>>);
static_assert(std::is_same_v<decltype(std::declval<const std::shared_future<int>&>().get()), const int&>);
static_assert(std::is_same_v<decltype(std::declval<const std::shared_future<int&>&>().get()), int&>);
static_assert(std::is_same_v<decltype(std::declval<const std::shared_future<void>&>().get()), void>);
static_assert(std::is_convertible_v<std::future<int>&&, std::shared_future<int>>);

int main() {
  std::shared_future<int> none;
  CHECK(!none.valid());

  std::promise<std::string> p;
  std::future<std::string> f = p.get_future();
  std::shared_future<std::string> sf = f.share();
  CHECK(!f.valid() && sf.valid());
  std::shared_future<std::string> copy = sf;
  std::vector<std::thread> ts;
  std::vector<const std::string*> addrs(3, nullptr);
  for (int i = 0; i < 3; ++i)
    ts.emplace_back([&addrs, i, mine = sf] { addrs[i] = &mine.get(); });
  p.set_value("shared");
  for (auto& t : ts) t.join();
  CHECK(sf.get() == "shared");
  CHECK(sf.valid());  // get does not release
  CHECK(&sf.get() == &copy.get());  // same stored object
  for (auto a : addrs) CHECK(a == &sf.get());

  std::promise<void> pv;
  std::shared_future<void> sv(pv.get_future());
  pv.set_exception(std::make_exception_ptr(std::logic_error("x")));
  for (int i = 0; i < 2; ++i) {
    bool caught = false;
    try { sv.get(); } catch (const std::logic_error&) { caught = true; }
    CHECK(caught && sv.valid());
  }

  int x = 0;
  std::promise<int&> pr;
  std::shared_future<int&> sr = pr.get_future();
  pr.set_value(x);
  sr.get() = 5;
  CHECK(x == 5);

  std::shared_future<int> moved;
  {
    std::promise<int> pm;
    std::shared_future<int> s1 = pm.get_future().share();
    moved = std::move(s1);
    CHECK(!s1.valid());
    pm.set_value(1);
  }
  CHECK(moved.get() == 1);
  return 0;
}
