// [exec.prop]: prop<QueryTag, ValueType> has the members QueryTag query_ and ValueType value_
// and is initialized as an aggregate: prop(q, v) (the deduction guide makes a reference_wrapper
// a reference member and copies anything else), prop<Q, V>{q, v}, and prop<Q, V>()
// (value-initialized members) for any value type, also one whose default constructor is not
// trivial; /4: it is not assignable; copies keep the value. [exec.env]: env<Envs...> holds one
// member per queryable (so env<...>() value-initializes them); /2: not assignable; /3: it can be
// initialized with a parenthesized single env.
#include <execution>
#include <functional>
#include <stop_token>
#include <string>
#include <type_traits>
#include "check.hpp"

namespace ex = std::execution;

struct name_q_t {
  template <class Env>
  constexpr auto operator()(const Env& e) const noexcept -> decltype(e.query(*this)) {
    return e.query(*this);
  }
};
inline constexpr name_q_t name_q{};

using StopProp = ex::prop<std::get_stop_token_t, std::inplace_stop_token>;
using NameProp = ex::prop<name_q_t, std::string>;

static_assert(!std::is_copy_assignable_v<StopProp> && !std::is_move_assignable_v<StopProp>);
static_assert(std::is_copy_constructible_v<NameProp> && std::is_move_constructible_v<NameProp>);
static_assert(!std::is_copy_assignable_v<ex::env<NameProp>> && !std::is_move_assignable_v<ex::env<NameProp>>);

int main() {
  // Value-initialization, with value types whose default constructors are not trivial.
  {
    auto p = StopProp();
    CHECK(!std::get_stop_token(p).stop_possible());
    auto n = NameProp();
    CHECK(name_q(n).empty());
    NameProp n2{};
    CHECK(name_q(n2).empty());
    auto e = ex::env<StopProp, NameProp>();
    CHECK(name_q(e).empty() && !std::get_stop_token(e).stop_possible());
    ex::env<NameProp> e2{};
    CHECK(name_q(e2).empty());
  }
  // Aggregate initialization and CTAD.
  {
    NameProp n{name_q, "abc"};
    CHECK(name_q(n) == "abc");
    std::string s = "ref";
    auto r = ex::prop(name_q, std::ref(s));
    static_assert(std::is_same_v<decltype(r), ex::prop<name_q_t, std::string&>>); // a reference
    s = "changed";
    CHECK(name_q(r) == "changed" && &name_q(r) == &s);
    auto v = ex::prop(name_q, s); // a copy
    static_assert(std::is_same_v<decltype(v), NameProp>);
    s = "again";
    CHECK(name_q(v) == "changed");
    auto copy = v;
    CHECK(name_q(copy) == "changed");
    static_assert(std::is_same_v<decltype(name_q(copy)), const std::string&>);
  }
  // [exec.env]/3
  {
    ex::env<NameProp> e{NameProp{name_q, "x"}};
    ex::env<NameProp> e3(e);
    const ex::env<NameProp> ce{NameProp{name_q, "y"}};
    ex::env<NameProp> e4(ce);
    CHECK(name_q(e3) == "x" && name_q(e4) == "y");
  }
  return 0;
}
