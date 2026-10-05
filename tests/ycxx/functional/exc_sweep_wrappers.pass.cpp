// Exception-injection sweep over function, move_only_function and copyable_function: the
// stored callable's copy/move constructors and operator new throw at their k-th call, for
// every k. After every run every callable object is destroyed exactly once and every operator
// new block freed. Assignment guarantees:
//   [func.wrap.func.con]/19: function& operator=(const function& f): "As if by
//     function(f).swap(*this)" (strong: on an exception *this keeps its target); /25 (F&&):
//     "As if by: function(std::forward<F>(f)).swap(*this)".
//   [func.wrap.move.ctor]/26: move_only_function::operator=(F&&): "Equivalent to:
//     move_only_function(std::forward<F>(f)).swap(*this)".
//   [func.wrap.copy.ctor]/24, /30: copyable_function::operator=(const copyable_function&) and
//     operator=(F&&): "Equivalent to: copyable_function(f).swap(*this)" etc.
//   [func.wrap.func.con]/4, /14: constructors throw bad_alloc or what the callable's
//     construction throws.
// Two callables: a small one holding one exh::T (not nothrow-movable: no small-buffer storage
// is permitted for function, [func.wrap.func.con]/note) and a large one.
// REQUIRES: exceptions
#include <functional>
#include "exc_new.hpp"

using namespace exh;

struct Small {
  T t;
  explicit Small(int v) : t(v) {}
  int operator()() const { return t.v; }
};
struct Big {
  T t;
  char pad[200] = {};
  explicit Big(int v) : t(v) {}
  int operator()() const { return t.v; }
};

static const Small small_g(2);
static const Big big_g(3);

template <class F>
void sw(const char* name, F f) {
  for (Kind k : {copy_ctor, move_ctor, gnew}) {
    if (k == gnew)
      sweep_new(name, f);
    else
      sweep(name, k, new_balanced(f));
  }
}

template <class W, class Callable>
void suite(const char* wname, const Callable& c) {
  static char l1[96], l2[96], l3[96], l4[96], l5[96];
  __builtin_snprintf(l1, sizeof l1, "%s(F)", wname);
  __builtin_snprintf(l2, sizeof l2, "%s = F (strong)", wname);
  __builtin_snprintf(l3, sizeof l3, "%s copy construction", wname);
  __builtin_snprintf(l4, sizeof l4, "%s copy assignment (strong)", wname);
  __builtin_snprintf(l5, sizeof l5, "%s move assignment", wname);
  sw(l1, [&] { return attempt([&] { W w(c); }); });
  for (int empty : {0, 1})
    sw(l2, [&, empty] {
      W w;
      if (!empty) w = W(Small(1));
      bool threw = attempt([&] { w = c; });
      if (threw) {
        EXH_EXPECT(bool(w) == !empty, "assignment changed whether *this has a target");
        if (!empty) EXH_EXPECT(w() == 1, "assignment changed the target although it threw");
      }
      return threw;
    });
  if constexpr (std::is_copy_constructible_v<W>) {
    sw(l3, [&] {
      W src(c);
      return attempt([&] { W w(src); });
    });
    for (int empty : {0, 1})
      sw(l4, [&, empty] {
        W src(c);
        W w;
        if (!empty) w = W(Small(1));
        bool threw = attempt([&] { w = src; });
        if (threw) {
          EXH_EXPECT(bool(w) == !empty, "copy assignment changed whether *this has a target");
          if (!empty) EXH_EXPECT(w() == 1, "copy assignment changed the target although it threw");
          EXH_EXPECT(src() == c(), "the source changed");
        }
        return threw;
      });
  }
  sw(l5, [&] {
    W src(c);
    W w(Small(1));
    bool threw = attempt([&] { w = std::move(src); });
    if (threw) EXH_EXPECT(bool(w), "move assignment emptied *this although it threw");
    return threw;
  });
}

int main() {
  suite<std::function<int()>>("function [small]", small_g);
  suite<std::function<int()>>("function [big]", big_g);
  suite<std::move_only_function<int() const>>("move_only_function [small]", small_g);
  suite<std::move_only_function<int() const>>("move_only_function [big]", big_g);
  suite<std::copyable_function<int() const>>("copyable_function [small]", small_g);
  suite<std::copyable_function<int() const>>("copyable_function [big]", big_g);
  return finish();
}
