// [meta.unary.prop] is_implicit_lifetime: "T is an implicit-lifetime type".
// [basic.types.general]/9: "Scalar types, implicit-lifetime class types, array types, and
// cv-qualified versions of these types are collectively called implicit-lifetime types."
// [class.prop]/8: "A class S is an implicit-lifetime class if it is an aggregate whose
// destructor is not user-provided or it has at least one trivial eligible constructor and a
// trivial, non-deleted destructor."
#include <cstddef>
#include <type_traits>

enum E { e };
enum class SE {};
struct Empty {};
struct Trivial { int x; double y; };
struct UserDtorAgg { int x; ~UserDtorAgg(); };                 // aggregate, user-provided dtor
struct NonTrivialMember { NonTrivialMember(); NonTrivialMember(const NonTrivialMember&); ~NonTrivialMember(); };
struct AggWithNonTrivial { NonTrivialMember m; };              // aggregate, dtor not user-provided
struct UserCtorTrivialCopy {                                   // trivial copy ctor + trivial dtor
  UserCtorTrivialCopy(int);
  int x;
};
struct NoTrivialCtor {                                         // all ctors user-provided
  NoTrivialCtor();
  NoTrivialCtor(const NoTrivialCtor&);
  NoTrivialCtor(NoTrivialCtor&&);
};
struct Virtual { virtual void f(); };                          // not aggregate, copy ctor not trivial
struct DeletedDtor { ~DeletedDtor() = delete; int x; };          // aggregate with deleted (not user-provided) dtor
struct NonAggDeletedDtor {                                     // trivial ctors, deleted dtor, not aggregate
  NonAggDeletedDtor(int);
  ~NonAggDeletedDtor() = delete;
private:
  int x;
};
struct PrivateMembers {                                        // not aggregate; trivial ctors/dtor
private:
  int x;
};
union TrivialUnion { int i; float f; };
union UnionUserDtor { int i; ~UnionUserDtor(); };              // aggregate union with user-provided dtor
struct Incomplete;

static_assert(std::is_base_of_v<std::true_type, std::is_implicit_lifetime<int>>);
static_assert(std::is_base_of_v<std::false_type, std::is_implicit_lifetime<void>>);

// Scalars and cv-qualified scalars.
static_assert(std::is_implicit_lifetime_v<int>);
static_assert(std::is_implicit_lifetime_v<const volatile int>);
static_assert(std::is_implicit_lifetime_v<double>);
static_assert(std::is_implicit_lifetime_v<E>);
static_assert(std::is_implicit_lifetime_v<SE>);
static_assert(std::is_implicit_lifetime_v<int*>);
static_assert(std::is_implicit_lifetime_v<std::nullptr_t>);
static_assert(std::is_implicit_lifetime_v<int Empty::*>);
static_assert(std::is_implicit_lifetime_v<void (Empty::*)()>);
static_assert(std::is_implicit_lifetime_v<void (*)()>);

// Not object types or not implicit-lifetime.
static_assert(!std::is_implicit_lifetime_v<void>);
static_assert(!std::is_implicit_lifetime_v<const void>);
static_assert(!std::is_implicit_lifetime_v<int&>);
static_assert(!std::is_implicit_lifetime_v<int&&>);
static_assert(!std::is_implicit_lifetime_v<void()>);

// Array types are always implicit-lifetime, whatever the element type.
static_assert(std::is_implicit_lifetime_v<int[3]>);
static_assert(std::is_implicit_lifetime_v<int[]>);
static_assert(std::is_implicit_lifetime_v<NoTrivialCtor[2]>);
static_assert(std::is_implicit_lifetime_v<Virtual[]>);
static_assert(std::is_implicit_lifetime_v<Incomplete[]>);
static_assert(std::is_implicit_lifetime_v<const Virtual[2][3]>);

// Classes.
static_assert(std::is_implicit_lifetime_v<Empty>);
static_assert(std::is_implicit_lifetime_v<Trivial>);
static_assert(std::is_implicit_lifetime_v<const Trivial>);
static_assert(!std::is_implicit_lifetime_v<UserDtorAgg>);
static_assert(std::is_implicit_lifetime_v<AggWithNonTrivial>);
static_assert(std::is_implicit_lifetime_v<UserCtorTrivialCopy>);
static_assert(!std::is_implicit_lifetime_v<NoTrivialCtor>);
static_assert(!std::is_implicit_lifetime_v<NonTrivialMember>);
static_assert(!std::is_implicit_lifetime_v<Virtual>);
static_assert(std::is_implicit_lifetime_v<DeletedDtor>);
static_assert(!std::is_implicit_lifetime_v<NonAggDeletedDtor>);
static_assert(std::is_implicit_lifetime_v<PrivateMembers>);
static_assert(std::is_implicit_lifetime_v<TrivialUnion>);
static_assert(!std::is_implicit_lifetime_v<UnionUserDtor>);
