// [func.not.fn]/5: "Throws: Any exception thrown by the initialization of fd."
// [func.bind.partial]/5: "Throws: Any exception thrown by the initialization of the state
// entities of g"; /10 (constant target): "Any exception thrown by the initialization of
// bound_args." [func.bind.bind]/5: "Throws: Any exception thrown by the initialization of the
// state entities of g." [func.require]/7: copying a wrapper copies its state entities, so a
// throwing copy constructor propagates. An exception from the target propagates out of the call
// (the call is expression-equivalent to the call pattern, [func.require]/5).
#include <functional>
#include <utility>
#include "check.hpp"

struct Bomb {
  static inline bool armed = false;
  Bomb() = default;
  Bomb(const Bomb&) {
    if (armed) throw 7;
  }
  Bomb(Bomb&&) noexcept {}
  bool operator()(...) const { return true; }
};
bool take(const Bomb&) { return true; }
bool boom(int) { throw 'x'; }

template <class F>
bool throws_int(F f) {
  try {
    f();
  } catch (int v) {
    return v == 7;
  }
  return false;
}

int main() {
  Bomb b;
  Bomb::armed = true;
  CHECK(throws_int([&] { (void)std::not_fn(b); }));
  CHECK(throws_int([&] { (void)std::bind_front(b, 1); }));
  CHECK(throws_int([&] { (void)std::bind_back(b, 1); }));
  CHECK(throws_int([&] { (void)std::bind(b, 1); }));
  CHECK(throws_int([&] { (void)std::bind<bool>(b, 1); }));
  CHECK(throws_int([&] { (void)std::bind_front(take, b); }));
  CHECK(throws_int([&] { (void)std::bind_back(take, b); }));
  CHECK(throws_int([&] { (void)std::bind(take, b); }));
  CHECK(throws_int([&] { (void)std::bind_front<take>(b); }));
  CHECK(throws_int([&] { (void)std::bind_back<take>(b); }));

  // copying a wrapper copies its state entities
  Bomb::armed = false;
  auto nf = std::not_fn(b);
  auto bf = std::bind_front(take, b);
  auto bb = std::bind_back<take>(b);
  auto bd = std::bind(take, b);
  Bomb::armed = true;
  CHECK(throws_int([&] { auto c = nf; (void)c; }));
  CHECK(throws_int([&] { auto c = bf; (void)c; }));
  CHECK(throws_int([&] { auto c = bb; (void)c; }));
  CHECK(throws_int([&] { auto c = bd; (void)c; }));
  // moving uses the non-throwing move constructor
  auto moved = std::move(bf);
  CHECK(moved());
  Bomb::armed = false;

  // exceptions from the target propagate through the call
  bool caught = false;
  try {
    std::not_fn(boom)(1);
  } catch (char) {
    caught = true;
  }
  CHECK(caught);
  caught = false;
  try {
    std::bind(boom, 1)();
  } catch (char) {
    caught = true;
  }
  CHECK(caught);
  caught = false;
  try {
    std::bind_back<boom>()(1);
  } catch (char) {
    caught = true;
  }
  CHECK(caught);
  return 0;
}
