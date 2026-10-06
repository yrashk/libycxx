// FLAGS: -freflection
// XFAIL-COMPILER: clang  Clang 23 has no reflection (P2996)
// [meta.reflection.access.context]/2-4: access_context is a structural, non-aggregate,
// non-default-constructible type with a scope and a designating class; /5 current(): the null
// designating class and the current scope; /8 unprivileged(): the global namespace; /9
// unchecked(): both null; /10 via(cls): the same scope with designating class cls; /11: via
// throws meta::exception unless cls is null or a complete class; /3: two contexts with
// equivalent scope and designating class are template-argument-equivalent.
// [meta.reflection.access.queries]/3: is_accessible (members of a class, a non-member is always
// accessible, unchecked() sees everything, a friend function's context sees private members,
// Example 1), with via() changing the designating class (/2, /3.3); /6-10:
// has_inaccessible_nonstatic_data_members, has_inaccessible_bases, has_inaccessible_subobjects.
// REQUIRES: exceptions
#include <meta>
#include <type_traits>

namespace m = std::meta;
using AC = m::access_context;

static_assert(!std::is_default_constructible_v<AC> && !std::is_aggregate_v<AC>);
static_assert(std::is_structural_v<AC>);
static_assert(noexcept(AC::current()) && noexcept(AC::unprivileged()) && noexcept(AC::unchecked()));

static_assert(AC::unprivileged().scope() == ^^:: && AC::unprivileged().designating_class() == m::info());
static_assert(AC::unchecked().scope() == m::info() && AC::unchecked().designating_class() == m::info());
static_assert(AC::current().scope() == ^^:: && AC::current().designating_class() == m::info());

namespace ns {
static_assert(AC::current().scope() == ^^ns);
consteval AC in_function() { return AC::current(); }
static_assert(in_function().scope() == ^^in_function);
}

// Example 1 of [meta.reflection.access.queries].
consteval AC fn() { return AC::current(); }
class Cls {
  int mem;
  friend consteval AC fn();

public:
  int pub;
  static constexpr auto r = ^^mem;
  static constexpr auto rp = ^^pub;
  static consteval AC inside() { return AC::current(); }
};
static_assert(m::is_accessible(Cls::r, fn()));
static_assert(!m::is_accessible(Cls::r, AC::current()));
static_assert(m::is_accessible(Cls::r, AC::unchecked()));
static_assert(!m::is_accessible(Cls::r, AC::unprivileged()));
static_assert(m::is_accessible(Cls::rp, AC::unprivileged()));
static_assert(m::is_accessible(Cls::r, Cls::inside()) && Cls::inside().scope() == ^^Cls::inside);
static_assert(m::is_accessible(^^Cls, AC::unprivileged()) && m::is_accessible(^^fn, AC::unprivileged())); // /3.2

// via: the designating class.
struct Base {
  int b;
};
struct Derived : private Base {
  int d;
  friend consteval AC friend_of_derived();
};
consteval AC friend_of_derived() { return AC::current(); }
struct Unrelated {};
static_assert(AC::unprivileged().via(^^Derived).designating_class() == ^^Derived);
static_assert(AC::unprivileged().via(^^Derived).scope() == ^^::);
static_assert(AC::unchecked().via(m::info()).designating_class() == m::info());
// Base::b named through Derived (privately derived) is not accessible from outside.
static_assert(m::is_accessible(^^Base::b, AC::unprivileged()));
static_assert(!m::is_accessible(^^Base::b, AC::unprivileged().via(^^Derived)));
static_assert(m::is_accessible(^^Base::b, friend_of_derived().via(^^Derived)));
// /3.3.1: a member that is not a member of the designating class.
static_assert(!m::is_accessible(^^Base::b, AC::unchecked().via(^^Unrelated)));

// has_inaccessible_*
struct AllPublic : Base {
  int x;
};
class Hidden : public Base {
  int secret;
};
static_assert(!m::has_inaccessible_nonstatic_data_members(^^AllPublic, AC::unprivileged()));
static_assert(m::has_inaccessible_nonstatic_data_members(^^Hidden, AC::unprivileged()));
static_assert(!m::has_inaccessible_nonstatic_data_members(^^Hidden, AC::unchecked()));
static_assert(!m::has_inaccessible_bases(^^Hidden, AC::unprivileged()));
static_assert(m::has_inaccessible_bases(^^Derived, AC::unprivileged()));
static_assert(!m::has_inaccessible_bases(^^Derived, friend_of_derived()));
static_assert(m::has_inaccessible_subobjects(^^Derived, AC::unprivileged()));
static_assert(m::has_inaccessible_subobjects(^^Hidden, AC::unprivileged()));
static_assert(!m::has_inaccessible_subobjects(^^AllPublic, AC::unprivileged()));

// /3: equal contexts are interchangeable as template arguments.
template <AC A>
struct Tag {};
static_assert(std::is_same_v<Tag<AC::unprivileged()>, Tag<AC::current()>>); // both: global scope, null class
static_assert(!std::is_same_v<Tag<AC::unprivileged()>, Tag<AC::unchecked()>>);

// /11
struct Incomplete;
template <class F>
consteval bool throws(F f) {
  try {
    f();
  } catch (const m::exception&) {
    return true;
  }
  return false;
}
static_assert(throws([] { (void)AC::unchecked().via(^^int); }));
static_assert(throws([] { (void)AC::unchecked().via(^^Incomplete); }));
static_assert(!throws([] { (void)AC::unchecked().via(^^Cls); }));

int main() {}
