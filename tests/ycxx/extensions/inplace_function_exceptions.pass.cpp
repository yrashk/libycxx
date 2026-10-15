// REQUIRES: exceptions
#include <ycxx/functional.hpp>
#include "check.hpp"

struct Bomb {
  static inline bool armed = false;
  static inline int alive = 0;
  int value;
  explicit Bomb(int n) : value(n) { ++alive; }
  Bomb(const Bomb& other) : value(other.value) { if (armed) throw 42; ++alive; }
  Bomb(Bomb&& other) noexcept : value(other.value) { ++alive; }
  ~Bomb() { --alive; }
  int operator()() { return value; }
};

int main() {
  using W = ycxx::inplace_function<int()>;
  W empty;
  bool caught = false;
  try { empty(); } catch (const std::bad_function_call&) { caught = true; }
  CHECK(caught);
  {
    Bomb bomb(3);
    W source(bomb), destination = [] { return 7; };
    CHECK(Bomb::alive == 2);
    Bomb::armed = true;
    auto fails = [](auto operation) {
      bool threw = false;
      try { operation(); } catch (int n) { CHECK(n == 42); threw = true; }
      CHECK(threw && Bomb::alive == 2);
    };
    fails([&] { W copy(source); });
    fails([&] { destination = source; });
    fails([&] { destination = bomb; });
    CHECK(source() == 3 && destination() == 7);
    Bomb::armed = false;
    destination = source;
    CHECK(destination() == 3 && Bomb::alive == 3);
  }
  CHECK(Bomb::alive == 0);
}
