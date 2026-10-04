// [expected.object.assign]/1: reinit-expected(newval, oldval, args...) is specified exactly:
//   if is_nothrow_constructible_v<T, Args...>: destroy oldval, construct newval;
//   else if is_nothrow_move_constructible_v<T>: T tmp(args...); destroy oldval;
//     construct newval from std::move(tmp);
//   else: U tmp(std::move(oldval)); destroy oldval; try { construct newval } catch (...)
//     { construct oldval from std::move(tmp); throw; }
// and /2, /7, /12 use it for copy, move and converting assignment that switch between value
// and error ("Then, if no exception was thrown, ... has_val = rhs.has_value()"), so the
// sequence of constructions/destructions and the state after an exception are observable.
// [expected.object.swap]/2: the value <-> error case is spelled out the same way (Table 72).
// [expected.void.assign]/1.2, /12.1: for expected<void, E>, construct_at(addressof(unex), ...)
// happens before has_val = false, so a throwing E construction leaves *this holding a value.
// [expected.void.swap]: "construct_at(addressof(unex), std::move(rhs.unex)); destroy_at(...);
// has_val = false; rhs.has_val = true".
#include <expected>
#include <string>
#include <utility>
#include "check.hpp"

static std::string events;
static int throw_at = 0;  // throw from the throw_at-th throwing-eligible operation (0: never)
static int counter = 0;
static void maybe_throw() {
  if (throw_at && ++counter == throw_at) throw 99;
}
static void arm(int n) { counter = 0; throw_at = n; }

// Tag 't' for the value type, 'e' for the error type. c: from int, C: copy, M: move, ~: dtor.
template <char Tag, bool NothrowMove>
struct L {
  int v;
  L(int x) : v(x) { events += Tag; events += 'c'; maybe_throw(); }
  L(const L& o) : v(o.v) { events += Tag; events += 'C'; maybe_throw(); }
  L(L&& o) noexcept(NothrowMove) : v(o.v) {
    events += Tag; events += 'M';
    if (!NothrowMove) maybe_throw();
  }
  L& operator=(const L& o) { v = o.v; events += Tag; events += '='; return *this; }
  L& operator=(L&& o) noexcept(NothrowMove) { v = o.v; events += Tag; events += 'm'; return *this; }
  ~L() { events += Tag; events += '~'; }
};
using TN = L<'t', true>;   // value type, nothrow move
using TT = L<'t', false>;  // value type, potentially-throwing move
using EN = L<'e', true>;
using ET = L<'e', false>;

static void value_to_error_third_branch() {
  // reinit-expected(unex, val, rhs.error()): E is neither nothrow copy- nor nothrow
  // move-constructible -> third branch with a T temporary.
  std::expected<TN, ET> a(std::in_place, 1), b(std::unexpect, 2);
  events.clear();
  a = b;
  CHECK(events == "tMt~eCt~");  // tmp(move(val)), destroy val, construct unex, destroy tmp
  CHECK(!a.has_value() && a.error().v == 2);

  std::expected<TN, ET> c(std::in_place, 1);
  events.clear();
  arm(1);  // the copy of the error throws
  bool caught = false;
  try { c = b; } catch (int) { caught = true; }
  arm(0);
  CHECK(caught);
  CHECK(events == "tMt~eCtMt~");  // ... val restored from tmp, then tmp destroyed
  CHECK(c.has_value() && c->v == 1);
}

static void value_to_error_second_branch() {
  // E nothrow move-constructible, copy may throw: E tmp(rhs.error()); destroy val; move in.
  std::expected<TT, EN> a(std::in_place, 1), b(std::unexpect, 2);
  events.clear();
  a = b;
  CHECK(events == "eCt~eMe~");
  CHECK(!a.has_value() && a.error().v == 2);

  std::expected<TT, EN> c(std::in_place, 1);
  events.clear();
  arm(1);  // constructing tmp throws: the value was never touched
  bool caught = false;
  try { c = b; } catch (int) { caught = true; }
  arm(0);
  CHECK(caught && events == "eC");
  CHECK(c.has_value() && c->v == 1);
}

static void error_to_value() {
  // reinit-expected(val, unex, *rhs): T's copy and move may throw -> third branch, E tmp.
  std::expected<TT, EN> d(std::unexpect, 3), e(std::in_place, 4);
  events.clear();
  d = e;
  CHECK(events == "eMe~tCe~");
  CHECK(d.has_value() && d->v == 4);

  std::expected<TT, EN> f(std::unexpect, 3);
  events.clear();
  arm(1);
  bool caught = false;
  try { f = e; } catch (int) { caught = true; }
  arm(0);
  CHECK(caught && events == "eMe~tCeMe~");
  CHECK(!f.has_value() && f.error().v == 3);

  // (12.2) converting assignment: reinit-expected(val, unex, std::forward<U>(v)), U = int.
  events.clear();
  f = 7;
  CHECK(events == "eMe~tce~");
  CHECK(f.has_value() && f->v == 7);

  // (7.3) move assignment: reinit-expected(val, unex, std::move(*rhs)).
  std::expected<TT, EN> g(std::unexpect, 5), h(std::in_place, 6);
  events.clear();
  arm(1);
  caught = false;
  try { g = std::move(h); } catch (int) { caught = true; }
  arm(0);
  CHECK(caught && events == "eMe~tMeMe~");
  CHECK(!g.has_value() && g.error().v == 5);
}

static void swap_value_error() {
  // Table 72, E nothrow move: E tmp(move(rhs.unex)); destroy rhs.unex; construct rhs.val from
  // move(val); destroy val; construct unex from move(tmp); (tmp destroyed).
  std::expected<TT, EN> a(std::in_place, 1), b(std::unexpect, 2);
  events.clear();
  a.swap(b);
  CHECK(events == "eMe~tMt~eMe~");
  CHECK(!a.has_value() && a.error().v == 2 && b.has_value() && b->v == 1);

  std::expected<TT, EN> c(std::in_place, 1), d(std::unexpect, 2);
  events.clear();
  arm(1);  // moving the value into rhs throws: rhs.unex restored from tmp
  bool caught = false;
  try { c.swap(d); } catch (int) { caught = true; }
  arm(0);
  CHECK(caught && events == "eMe~tMeMe~");
  CHECK(c.has_value() && c->v == 1 && !d.has_value() && d.error().v == 2);

  // "!this->has_value() && rhs.has_value(): calls rhs.swap(*this)" -- the same sequence.
  events.clear();
  d.swap(c);
  CHECK(events == "eMe~tMt~eMe~");
  CHECK(!c.has_value() && c.error().v == 2 && d.has_value() && d->v == 1);

  // E's move may throw, T's may not: T tmp(move(val)); destroy val; construct unex from
  // move(rhs.unex); destroy rhs.unex; construct rhs.val from move(tmp).
  std::expected<TN, ET> e(std::in_place, 1), f(std::unexpect, 2);
  events.clear();
  e.swap(f);
  CHECK(events == "tMt~eMe~tMt~");
  CHECK(!e.has_value() && e.error().v == 2 && f.has_value() && f->v == 1);

  std::expected<TN, ET> g(std::in_place, 1), h(std::unexpect, 2);
  events.clear();
  arm(1);  // moving the error throws: val restored from tmp
  caught = false;
  try { g.swap(h); } catch (int) { caught = true; }
  arm(0);
  CHECK(caught && events == "tMt~eMtMt~");
  CHECK(g.has_value() && g->v == 1 && !h.has_value() && h.error().v == 2);
}

static void void_specialization() {
  std::expected<void, ET> a, b(std::unexpect, 2);
  arm(1);
  bool caught = false;
  try { a = b; } catch (int) { caught = true; }  // (1.2) construct_at throws first
  arm(0);
  CHECK(caught && a.has_value());
  arm(1);
  caught = false;
  try { a = std::move(b); } catch (int) { caught = true; }  // (6.2)
  arm(0);
  CHECK(caught && a.has_value());
  arm(1);
  caught = false;
  try { a = std::unexpected<ET>(std::in_place, 5); } catch (int) { caught = true; }  // (12.1)
  arm(0);
  CHECK(caught && a.has_value());

  a = b;
  CHECK(!a.has_value() && a.error().v == 2);
  std::expected<void, ET> c;
  events.clear();
  a = c;  // (1.3) destroys unex
  CHECK(events == "e~" && a.has_value());
  events.clear();
  a = c;  // (1.1) no effects
  CHECK(events.empty());

  std::expected<void, EN> d, e(std::unexpect, 4);
  events.clear();
  d.swap(e);
  CHECK(events == "eMe~");
  CHECK(!d.has_value() && d.error().v == 4 && e.has_value());

  std::expected<void, ET> f, g(std::unexpect, 4);
  arm(1);
  caught = false;
  try { f.swap(g); } catch (int) { caught = true; }
  arm(0);
  CHECK(caught && f.has_value() && !g.has_value() && g.error().v == 4);
}

int main() {
  value_to_error_third_branch();
  value_to_error_second_branch();
  error_to_value();
  swap_value_error();
  void_specialization();
  return 0;
}
