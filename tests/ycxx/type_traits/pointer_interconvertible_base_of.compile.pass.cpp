// [meta.rel] Table 56: is_pointer_interconvertible_base_of<Base, Derived>: "Derived is
// unambiguously derived from Base without regard to cv-qualifiers, and each object of type
// Derived is pointer-interconvertible ([basic.compound]) with its Base subobject, or Base and
// Derived are not unions and name the same class type without regard to cv-qualifiers."
// [basic.compound]/5: a standard-layout class object is pointer-interconvertible with its base
// class subobjects (and first member); a non-standard-layout class object need not be.
#include <type_traits>

struct A { int a; };
struct B { int b; };
struct C : public A, public B {};   // not standard-layout
struct SL { int x; char y; double z; };
union U { int i; double d; };
struct Empty {};
struct DerivedEmptyBase : Empty { int v; };
struct Virt : virtual A {};
struct Priv : private A {};
struct Amb1 : A {};
struct Amb2 : A {};
struct Amb : Amb1, Amb2 {};
struct Second : Empty, SL {};
struct Incomplete;

// is_pointer_interconvertible_base_of
template <class Bs, class D> constexpr bool pib = std::is_pointer_interconvertible_base_of_v<Bs, D> &&
    std::is_base_of_v<std::true_type, std::is_pointer_interconvertible_base_of<Bs, D>>;
template <class Bs, class D> constexpr bool npib = !std::is_pointer_interconvertible_base_of_v<Bs, D> &&
    std::is_base_of_v<std::false_type, std::is_pointer_interconvertible_base_of<Bs, D>>;
static_assert(pib<Empty, DerivedEmptyBase> && pib<const Empty, volatile DerivedEmptyBase>);
static_assert(pib<SL, Second> && pib<Empty, Second>);
static_assert(pib<A, A> && pib<const A, A> && pib<Incomplete, Incomplete> && pib<Incomplete, const Incomplete>);
static_assert(pib<C, C>);   // same class type, even though not standard-layout
static_assert(npib<A, C> && npib<B, C>);   // C is not standard-layout
static_assert(npib<A, Virt> && npib<A, Amb>);
static_assert(npib<U, U>);   // unions are excluded from the same-type case
static_assert(npib<int, int> && npib<A&, A&> && npib<A*, A*>);
static_assert(npib<DerivedEmptyBase, Empty>);
static_assert(pib<A, Priv> == std::is_standard_layout_v<Priv>);   // private base: still a base

int main() {}
