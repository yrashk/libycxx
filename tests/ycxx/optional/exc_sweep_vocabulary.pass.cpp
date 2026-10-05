// Exception-injection sweep over optional, variant, expected and any: the contained type's
// constructors and assignments (and operator new for any) throw at their k-th call, for every
// k. After every run all element objects are destroyed exactly once and all operator new blocks
// are freed; the state each operation must leave is checked:
//   optional ([optional.assign]/7, /13, /18, /23, /28): "If any exception is thrown, the result
//     of the expression this->has_value() remains unchanged. If an exception is thrown during
//     the call to T's copy constructor, no effect. If an exception is thrown during the call to
//     T's copy assignment, the state of its contained value is as defined by the exception
//     safety guarantee of T's copy assignment" (exh::T's assignments throw before changing
//     anything, so the value is unchanged too).
//     emplace ([optional.assign]/30, /34): "Calls *this = nullopt. Then direct-non-list-
//     initializes val"; "If an exception is thrown during the call to T's constructor, *this
//     does not contain a value, and the previous *val (if any) has been destroyed."
//     swap ([optional.swap]/6): "the results of the expressions this->has_value() and
//     rhs.has_value() remain unchanged."
//   variant ([variant.assign]/10.1): move assignment: "If an exception is thrown during the
//     call to Tj's move construction (with j being rhs.index()), the variant will hold no
//     value." /10.2: if Tj's move assignment throws, "index() will be j". /16.1: converting
//     assignment to the same alternative: "valueless_by_exception() will be false".
//     [variant.mod]/11: emplace "is permitted to not hold a value".
//   expected ([expected.object.assign]/2 with reinit-expected): when the new value's
//     construction throws, the old value is restored (or never destroyed): has_value() and
//     the old value are unchanged.
//   any ([any.assign]/1, /10): copy assignment and assignment from a value: "No effects if an
//     exception is thrown." [any.modifiers]/8: emplace: "If an exception is thrown during the
//     call to VT's constructor, *this does not contain a value, and any previously contained
//     value has been destroyed."
// REQUIRES: exceptions
#include <any>
#include <expected>
#include <optional>
#include <variant>
#include "exc_new.hpp"

using namespace exh;

static const T two_g(2); // created before any sweep

template <class F>
void each(const char* name, std::initializer_list<Kind> ks, F f) {
  for (Kind k : ks) {
    if (k == gnew)
      sweep_new(name, f);
    else
      sweep(name, k, new_balanced(f));
  }
}

int main() {
  const auto CM = {copy_ctor, move_ctor, copy_assign, move_assign};

  // ---- optional
  for (int lhs = 0; lhs < 2; ++lhs)
    for (int rhs = 0; rhs < 2; ++rhs) {
      each("optional copy assignment", CM, [=] {
        std::optional<T> a, b;
        if (lhs) a.emplace(1);
        if (rhs) b.emplace(2);
        bool threw = attempt([&] { a = b; });
        if (threw) {
          EXH_EXPECT(a.has_value() == bool(lhs), "optional copy assignment changed has_value()");
          if (lhs) EXH_EXPECT(a->v == 1, "optional copy assignment changed the value");
        }
        return threw;
      });
      each("optional move assignment", CM, [=] {
        std::optional<T> a, b;
        if (lhs) a.emplace(1);
        if (rhs) b.emplace(2);
        bool threw = attempt([&] { a = std::move(b); });
        if (threw) {
          EXH_EXPECT(a.has_value() == bool(lhs), "optional move assignment changed has_value()");
          if (lhs) EXH_EXPECT(a->v == 1, "optional move assignment changed the value");
        }
        return threw;
      });
      each("optional swap", CM, [=] {
        std::optional<T> a, b;
        if (lhs) a.emplace(1);
        if (rhs) b.emplace(2);
        bool threw = attempt([&] { a.swap(b); });
        if (threw)
          EXH_EXPECT(a.has_value() == bool(lhs) && b.has_value() == bool(rhs), "optional swap changed has_value()");
        return threw;
      });
      each("optional converting assignment from optional<int>", {value_ctor, copy_assign, move_assign, move_ctor},
           [=] {
             std::optional<T> a;
             std::optional<int> b;
             if (lhs) a.emplace(1);
             if (rhs) b = 2;
             bool threw = attempt([&] { a = b; });
             if (threw) EXH_EXPECT(a.has_value() == bool(lhs), "converting assignment changed has_value()");
             return threw;
           });
    }
  for (int lhs = 0; lhs < 2; ++lhs)
    each("optional::emplace", {value_ctor}, [=] {
      std::optional<T> a;
      if (lhs) a.emplace(1);
      long live0 = st.live;
      bool threw = attempt([&] { a.emplace(5); });
      if (threw) {
        EXH_EXPECT(!a.has_value(), "optional::emplace: *this contains a value after a throwing constructor");
        EXH_EXPECT(st.live == live0 - lhs, "optional::emplace: the previous value was not destroyed");
      }
      return threw;
    });

  // ---- variant<int, T>
  using Var = std::variant<int, T>;
  for (int lhs = 0; lhs < 2; ++lhs) {
    each("variant move assignment from alternative T", CM, [=] {
      Var a = lhs ? Var(std::in_place_index<1>, 1) : Var(7);
      Var b(std::in_place_index<1>, 2);
      bool threw = attempt([&] { a = std::move(b); });
      if (threw) {
        if (st.kind == move_ctor)
          EXH_EXPECT(a.valueless_by_exception(), "[variant.assign]/10.1: Tj's move construction threw but the variant holds a value");
        if (st.kind == move_assign)
          EXH_EXPECT(a.index() == 1, "[variant.assign]/10.2: index() is not j after Tj's move assignment threw");
      }
      return threw;
    });
    each("variant copy assignment from alternative T", CM, [=] {
      Var a = lhs ? Var(std::in_place_index<1>, 1) : Var(7);
      Var b(std::in_place_index<1>, 2);
      bool threw = attempt([&] { a = b; });
      if (threw && lhs && st.kind == copy_assign)
        EXH_EXPECT(a.index() == 1 && std::get<1>(a).v == 1, "same-alternative copy assignment: T's assignment is strong");
      return threw;
    });
    each("variant converting assignment a = T", CM, [=] {
      Var a = lhs ? Var(std::in_place_index<1>, 1) : Var(7);
      bool threw = attempt([&] { a = two_g; });
      if (threw && lhs)
        EXH_EXPECT(!a.valueless_by_exception(), "[variant.assign]/16.1: same alternative: not valueless");
      return threw;
    });
    each("variant::emplace<1>(int)", {value_ctor}, [=] {
      Var a = lhs ? Var(std::in_place_index<1>, 1) : Var(7);
      long live0 = st.live;
      bool threw = attempt([&] { a.emplace<1>(5); });
      if (threw) {
        // permitted to not hold a value; if it holds one, it is a complete object
        if (a.valueless_by_exception())
          EXH_EXPECT(st.live == live0 - lhs, "valueless variant still owns a T");
        else
          EXH_EXPECT(a.index() == 0 || std::get<1>(a).magic == alive_magic, "variant holds a dead object");
      }
      return threw;
    });
    each("variant swap", CM, [=] {
      Var a = lhs ? Var(std::in_place_index<1>, 1) : Var(7);
      Var b(std::in_place_index<1>, 2);
      bool threw = attempt([&] { a.swap(b); });
      return threw;
    });
  }

  // ---- variant<int, NT>: NT's copy may throw but its move does not, so (2.5) and (13.3) make
  // the copy before anything is destroyed: *this is unchanged when it throws.
  {
    using VN = std::variant<int, NT>;
    static const NT two_n(2);
    each("variant<int, NT> copy assignment from the other alternative ([variant.assign]/2.5)", {copy_ctor}, [] {
      VN a(7);
      VN b(std::in_place_index<1>, 2);
      bool threw = attempt([&] { a = b; });
      if (threw)
        EXH_EXPECT(a.index() == 0 && std::get<0>(a) == 7,
                   "[variant.assign]/2.5: operator=(variant(rhs)) leaves *this unchanged when the copy throws");
      return threw;
    });
    each("variant<int, NT> converting assignment from const NT& ([variant.assign]/13.3)", {copy_ctor}, [] {
      VN a(7);
      bool threw = attempt([&] { a = two_n; });
      if (threw)
        EXH_EXPECT(a.index() == 0 && std::get<0>(a) == 7,
                   "[variant.assign]/13.3: emplace<j>(Tj(t)) leaves *this unchanged when Tj(t) throws");
      return threw;
    });
    each("variant<int, T> copy assignment from the other alternative ([variant.assign]/2.4)", {copy_ctor}, [] {
      Var a(7);
      Var b(std::in_place_index<1>, 2);
      long live0 = st.live;
      bool threw = attempt([&] { a = b; });
      if (threw && a.valueless_by_exception()) EXH_EXPECT(st.live == live0, "valueless variant owns an object");
      return threw;
    });
  }
  // ---- any holding a nothrow-movable NT (small-object storage permitted)
  for (int lhs = 0; lhs < 2; ++lhs)
    each("any copy assignment [NT]", {copy_ctor, gnew}, [=] {
      std::any a, b = NT(2);
      if (lhs) a = NT(1);
      bool threw = attempt([&] { a = b; });
      if (threw) {
        EXH_EXPECT(a.has_value() == bool(lhs), "[any.assign]/1: copy assignment had an effect");
        if (lhs) EXH_EXPECT(std::any_cast<NT&>(a).v == 1, "[any.assign]/1: copy assignment changed the value");
      }
      return threw;
    });

  // ---- expected<T, NT> and expected<NT, T> (reinit-expected)
  {
    using E1 = std::expected<T, NT>;
    using E2 = std::expected<NT, T>;
    for (int lhs = 0; lhs < 2; ++lhs)
      for (int rhs = 0; rhs < 2; ++rhs) {
        each("expected<T, NT> copy assignment", CM, [=] {
          E1 a = lhs ? E1(std::in_place, 1) : E1(std::unexpect, 11);
          E1 b = rhs ? E1(std::in_place, 2) : E1(std::unexpect, 12);
          bool threw = attempt([&] { a = b; });
          if (threw) {
            EXH_EXPECT(a.has_value() == bool(lhs), "expected copy assignment changed has_value()");
            if (lhs && rhs == 0) EXH_EXPECT(a->v == 1, "expected: old value not preserved");
            if (!lhs && rhs) EXH_EXPECT(a.error().v == 11, "expected: old error not restored (reinit-expected)");
          }
          return threw;
        });
        each("expected<NT, T> copy assignment", CM, [=] {
          E2 a = lhs ? E2(std::in_place, 1) : E2(std::unexpect, 11);
          E2 b = rhs ? E2(std::in_place, 2) : E2(std::unexpect, 12);
          bool threw = attempt([&] { a = b; });
          if (threw) {
            EXH_EXPECT(a.has_value() == bool(lhs), "expected copy assignment changed has_value()");
            if (lhs && rhs == 0) EXH_EXPECT(a->v == 1, "expected: old value not restored (reinit-expected)");
            if (!lhs && rhs) EXH_EXPECT(a.error().v == 11, "expected: old error not preserved");
          }
          return threw;
        });
        each("expected<T, NT> move assignment", CM, [=] {
          E1 a = lhs ? E1(std::in_place, 1) : E1(std::unexpect, 11);
          E1 b = rhs ? E1(std::in_place, 2) : E1(std::unexpect, 12);
          bool threw = attempt([&] { a = std::move(b); });
          if (threw) {
            EXH_EXPECT(a.has_value() == bool(lhs), "expected move assignment changed has_value()");
            if (!lhs && rhs) EXH_EXPECT(a.error().v == 11, "expected: old error not restored (reinit-expected)");
          }
          return threw;
        });
        each("expected<T, NT> swap", CM, [=] {
          E1 a = lhs ? E1(std::in_place, 1) : E1(std::unexpect, 11);
          E1 b = rhs ? E1(std::in_place, 2) : E1(std::unexpect, 12);
          bool threw = attempt([&] { a.swap(b); });
          if (threw && lhs != rhs) {
            EXH_EXPECT(a.has_value() == bool(lhs) && b.has_value() == bool(rhs), "expected swap changed has_value()");
            // [expected.object.swap] Table 73: the exchange either completes or restores the
            // moved-from error/value
            if (lhs) EXH_EXPECT(a->v == 1 && b.error().v == 12, "expected swap: values not restored");
            if (!lhs) EXH_EXPECT(a.error().v == 11 && b->v == 2, "expected swap: values not restored");
          }
          return threw;
        });
      }
  }

  // ---- any (exh::T is not nothrow-move-constructible, so it is not small-object stored)
  for (int lhs = 0; lhs < 2; ++lhs) {
    each("any copy assignment", {copy_ctor, move_ctor, gnew}, [=] {
      std::any a, b = T(2);
      if (lhs) a = T(1);
      bool threw = attempt([&] { a = b; });
      if (threw) {
        EXH_EXPECT(a.has_value() == bool(lhs), "[any.assign]/1: copy assignment had an effect");
        if (lhs) EXH_EXPECT(std::any_cast<T&>(a).v == 1, "[any.assign]/1: copy assignment changed the value");
      }
      return threw;
    });
    each("any assignment from a T", {copy_ctor, move_ctor, gnew}, [=] {
      std::any a;
      if (lhs) a = T(1);
      bool threw = attempt([&] { a = two_g; });
      if (threw) {
        EXH_EXPECT(a.has_value() == bool(lhs), "[any.assign]/10: assignment had an effect");
        if (lhs) EXH_EXPECT(std::any_cast<T&>(a).v == 1, "[any.assign]/10: assignment changed the value");
      }
      return threw;
    });
    each("any::emplace<T>(int)", {value_ctor}, [=] {
      std::any a;
      if (lhs) a = T(1);
      long live0 = st.live;
      bool threw = attempt([&] { a.emplace<T>(5); });
      if (threw) {
        EXH_EXPECT(!a.has_value(), "[any.modifiers]/8: *this contains a value");
        EXH_EXPECT(st.live == live0 - lhs, "[any.modifiers]/8: previous value not destroyed");
      }
      return threw;
    });
    each("any::emplace<T>(int) allocation", {gnew}, [=] {
      std::any a;
      if (lhs) a = T(1);
      bool threw = attempt([&] { a.emplace<T>(5); });
      return threw;
    });
    each("any copy construction", {copy_ctor, gnew}, [=] {
      std::any b = T(2);
      bool threw = attempt([&] { std::any a(b); });
      return threw;
    });
  }
  return finish();
}
