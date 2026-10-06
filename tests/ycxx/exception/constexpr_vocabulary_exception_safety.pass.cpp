// The exception-safety guarantees of optional and expected hold during constant evaluation, when
// a constructor of the contained type throws (P3068; the members are constexpr):
// [optional.assign]/30, /34: emplace first destroys the contained value (*this = nullopt); if T's
//   constructor then throws, *this does not contain a value and the previous value has been
//   destroyed; /7: if copy assignment throws in T's copy constructor, has_value() is unchanged
//   and there is no effect;
// [expected.object.assign]/1 (reinit-expected), /16: assigning an unexpected<G> to an expected
//   holding a value, with E not nothrow-constructible from G: if E is nothrow move
//   constructible, E is built in a temporary first, so a throw leaves the value untouched;
//   otherwise (/15.3: T is then nothrow move constructible) the value is moved into a temporary
//   and moved back when E's constructor throws. Either way has_value() stays true with the same
//   value (has_val changes only "if no exception was thrown", /2).
// XFAIL: clang Clang 23 cannot throw during constant evaluation (P3068's core-language part)
// REQUIRES: exceptions
#include <expected>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include "check.hpp"

struct counters {
  int alive = 0;
  int destroyed = 0;
};

// Constructible from (int, counters*); throws for a negative int; copy throws when told to.
struct widget {
  int v;
  counters* c;
  bool throw_on_copy = false;
  constexpr widget(int x, counters* p) : v(x), c(p) {
    if (x < 0) throw std::invalid_argument("negative");
    ++c->alive;
  }
  constexpr widget(const widget& o) : v(o.v), c(o.c), throw_on_copy(o.throw_on_copy) {
    if (o.throw_on_copy) throw std::runtime_error("copy");
    ++c->alive;
  }
  constexpr widget& operator=(const widget&) = default;
  constexpr ~widget() {
    --c->alive;
    ++c->destroyed;
  }
};

constexpr bool optional_emplace() {
  counters c;
  std::optional<widget> o(std::in_place, 1, &c);
  try {
    o.emplace(-1, &c);
    return false;
  } catch (const std::invalid_argument&) {
  }
  return !o.has_value() && c.destroyed == 1 && c.alive == 0;
}
static_assert(optional_emplace());

constexpr bool optional_copy_assign() {
  counters c;
  std::optional<widget> empty;
  std::optional<widget> full(std::in_place, 2, &c);
  full->throw_on_copy = true;
  try {
    empty = full; // T's copy constructor throws: no effect
    return false;
  } catch (const std::runtime_error&) {
  }
  return !empty.has_value() && full.has_value() && full->v == 2 && c.alive == 1;
}
static_assert(optional_copy_assign());

// E: constructible from int, throws for a negative one; its move may or may not be noexcept.
template <bool NothrowMove>
struct error {
  int code;
  constexpr explicit error(int x) : code(x) {
    if (x < 0) throw std::domain_error("bad code");
  }
  constexpr error(error&& o) noexcept(NothrowMove) : code(o.code) {}
  constexpr error(const error& o) : code(o.code) {}
  constexpr error& operator=(const error&) = default;
  constexpr error& operator=(int x) {
    code = x;
    return *this;
  }
};

// T recording its moves; its move constructor is noexcept(NothrowMove).
template <bool NothrowMove>
struct value {
  int v;
  int* moves;
  constexpr value(int x, int* m) : v(x), moves(m) {}
  constexpr value(value&& o) noexcept(NothrowMove) : v(o.v), moves(o.moves) { ++*moves; }
  constexpr value(const value&) = default;
  constexpr value& operator=(const value&) = default;
};

// E nothrow move constructible: E is built in a temporary first, the value is not touched.
constexpr bool expected_error_temporary() {
  using E = error<true>;
  using T = value<false>;
  static_assert(!std::is_nothrow_constructible_v<E, int> && !std::is_nothrow_move_constructible_v<T>);
  int moves = 0;
  std::expected<T, E> x(std::in_place, 42, &moves);
  try {
    x = std::unexpected(-1);
    return false;
  } catch (const std::domain_error&) {
  }
  if (!x.has_value() || x->v != 42 || moves != 0) return false;
  x = std::unexpected(7); // a successful one switches
  return !x.has_value() && x.error().code == 7;
}
static_assert(expected_error_temporary());

// E neither nothrow constructible from G nor nothrow move constructible (T is nothrow move
// constructible): the value is moved into a temporary and moved back.
constexpr bool expected_restore_value() {
  using E = error<false>;
  using T = value<true>;
  static_assert(!std::is_nothrow_constructible_v<E, int> && !std::is_nothrow_move_constructible_v<E>);
  int moves = 0;
  std::expected<T, E> x(std::in_place, 5, &moves);
  try {
    x = std::unexpected(-3);
    return false;
  } catch (const std::domain_error&) {
  }
  return x.has_value() && x->v == 5 && moves == 2;
}
static_assert(expected_restore_value());

int main() {
  CHECK(optional_emplace());
  CHECK(optional_copy_assign());
  CHECK(expected_error_temporary());
  CHECK(expected_restore_value());
}
