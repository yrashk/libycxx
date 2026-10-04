// Exception-injection sweep over the shared states of promise, packaged_task and async: the
// value type's copy/move constructors, a promise's allocator and operator new throw at their
// k-th call, for every k.
//   [futures.promise]/17-18: set_value(const R&) throws "any exception thrown by the
//     constructor selected to copy an object of R" (move for set_value(R&&)); it reports
//     promise_already_satisfied only "if its shared state already has a stored value or
//     exception". A copy that throws stores nothing: the state is not ready, a later set_value
//     or set_exception succeeds, and the future then yields that value.
//   [futures.promise]/3 (promise(allocator_arg_t, const Allocator&)): the shared state is
//     allocated with the allocator; a failure leaks nothing.
//   [futures.task.members], [futures.async]: the decay-copies of the callable and arguments
//     happen in the calling thread; an exception there leaks nothing and creates no state.
// After every run every element object is destroyed exactly once and every block freed.
#include <future>
#include "exc_new.hpp"

using namespace exh;

static const T value_g(42);

template <class F>
void sw(const char* name, std::initializer_list<Kind> ks, F f) {
  for (Kind k : ks) {
    if (k == gnew)
      sweep_new(name, f);
    else
      sweep(name, k, new_balanced(f));
  }
}

int main() {
  sw("promise<T>::set_value(const T&) throws, then succeeds", {copy_ctor}, [] {
    std::promise<T> p;
    std::future<T> f = p.get_future();
    bool threw = attempt([&] { p.set_value(value_g); });
    if (threw) {
      EXH_EXPECT(f.wait_for(std::chrono::seconds(0)) == std::future_status::timeout,
                 "the shared state became ready although set_value threw");
      bool ok = true;
      try {
        p.set_value(T(7));
      } catch (const std::future_error&) {
        ok = false;
      }
      EXH_EXPECT(ok, "[futures.promise]/18.1: promise_already_satisfied after a set_value that threw");
      if (ok) EXH_EXPECT(f.get().v == 7, "wrong value after the retry");
    } else {
      EXH_EXPECT(f.get().v == 42, "wrong value");
    }
    return threw;
  });
  sw("promise<T>::set_value(T&&) throws, then set_exception succeeds", {move_ctor}, [] {
    std::promise<T> p;
    std::future<T> f = p.get_future();
    T x(9);
    bool threw = attempt([&] { p.set_value(std::move(x)); });
    if (threw) {
      bool ok = true;
      try {
        p.set_exception(std::make_exception_ptr(17));
      } catch (const std::future_error&) {
        ok = false;
      }
      EXH_EXPECT(ok, "[futures.promise]: set_exception failed after a set_value that threw");
      bool got = false;
      try {
        f.get();
      } catch (int e) {
        got = e == 17;
      }
      EXH_EXPECT(got, "the stored exception was not delivered");
    }
    return threw;
  });
  sw("promise<T>(allocator_arg, alloc) and set_value", {allocation, copy_ctor}, [] {
    return attempt([] {
      std::promise<T> p(std::allocator_arg, alloc<int>());
      p.set_value(value_g);
      std::future<T> f = p.get_future();
      CHECK(f.get().v == 42);
    });
  });
  // (Not swept over operator new: ~promise must store a future_error (broken_promise) from a
  // noexcept destructor; how an allocation failure there is handled is not specified.)
  sw("promise<T> destroyed without a value (broken_promise)", {copy_ctor}, [] {
    return attempt([] {
      std::future<T> f;
      {
        std::promise<T> p;
        f = p.get_future();
      }
      bool broken = false;
      try {
        f.get();
      } catch (const std::future_error& e) {
        broken = e.code() == std::future_errc::broken_promise;
      }
      CHECK(broken);
    });
  });
  sw("packaged_task<T(int)> from a closure owning a T", {copy_ctor, move_ctor, gnew}, [] {
    return attempt([] {
      std::packaged_task<T(int)> task([t = value_g](int i) { return T(t.v + i); });
      std::future<T> f = task.get_future();
      task(1);
      CHECK(f.get().v == 43);
    });
  });
  sw("async(deferred, f, T)", {copy_ctor, move_ctor, gnew}, [] {
    return attempt([] {
      auto f = std::async(std::launch::deferred, [](T t) { return T(t.v + 1); }, value_g);
      CHECK(f.get().v == 43);
    });
  });
  sw("shared_future<T>::get after set_value", {copy_ctor, gnew}, [] {
    return attempt([] {
      std::promise<T> p;
      std::shared_future<T> f = p.get_future().share();
      p.set_value(value_g);
      std::shared_future<T> g = f;
      CHECK(g.get().v == 42 && f.get().v == 42);
    });
  });
  return finish();
}
