// XFAIL-COMPILER: clang  Clang 23.1 has no __builtin_is_corresponding_member / __builtin_is_pointer_interconvertible_with_class (STATUS.md)
// [meta.member]/1-2: is_pointer_interconvertible_with_class(M S::*m): "true if and only if S is a
// standard-layout type, M is an object type, m is not null, and each object s of type S is
// pointer-interconvertible ([basic.compound]) with its subobject s.*m."
// /3-4: is_corresponding_member(M1 S1::*m1, M2 S2::*m2): "true if and only if S1 and S2 are
// standard-layout struct types, M1 and M2 are object types, m1 and m2 are not null, and m1 and m2
// point to corresponding members of the common initial sequence ([class.mem]) of S1 and S2."
// /5 Example 1 (reproduced). Both are constexpr and noexcept.
// (is_pointer_interconvertible_base_of: pointer_interconvertible_base_of.compile.pass.cpp.)
// [meta.rel] Table 56: is_pointer_interconvertible_base_of<Base, Derived>: "Derived is
// unambiguously derived from Base without regard to cv-qualifiers, and each object of type
// Derived is pointer-interconvertible with its Base subobject, or Base and Derived are not unions
// and name the same class type without regard to cv-qualifiers."
// [basic.compound]/5: an object is pointer-interconvertible with its first non-static data member
// in a standard-layout class (or any base class subobject of it), and union members with the
// union.
#include <type_traits>

struct A { int a; };
struct B { int b; };
struct C : public A, public B {};   // not standard-layout
struct SL { int x; char y; double z; };
struct SL2 { int p; char q; float r; };
struct NonSL { int x; private: int y; public: int z() const; };
union U { int i; double d; };
struct Empty {};
struct DerivedEmptyBase : Empty { int v; };
struct Bits1 { int a : 3; int b; };
struct Bits2 { int c : 3; int d; };
struct Bits3 { int e : 4; int f; };
struct EnumHolder1 { enum class E : int {} e; int g; };
struct NoUnique { [[no_unique_address]] Empty e; int i; };
struct Virt : virtual A {};
struct Priv : private A {};
struct Amb1 : A {};
struct Amb2 : A {};
struct Amb : Amb1, Amb2 {};
struct Second : Empty, SL {};   // standard-layout? Empty base is fine; first member SL::x
struct Incomplete;

// /5 Example 1
static_assert(std::is_pointer_interconvertible_with_class(&C::b));
static_assert(!std::is_pointer_interconvertible_with_class<C, int>(&C::b));
static_assert(std::is_corresponding_member(&C::a, &C::b));
static_assert(!std::is_corresponding_member<C, C, int, int>(&C::a, &C::b));

static_assert(noexcept(std::is_pointer_interconvertible_with_class(&SL::x)));
static_assert(noexcept(std::is_corresponding_member(&SL::x, &SL2::p)));
static_assert(std::is_same_v<decltype(std::is_corresponding_member(&SL::x, &SL2::p)), bool>);

static_assert(std::is_pointer_interconvertible_with_class(&SL::x));
static_assert(!std::is_pointer_interconvertible_with_class(&SL::y));
static_assert(!std::is_pointer_interconvertible_with_class(&SL::z));
static_assert(!std::is_pointer_interconvertible_with_class(&NonSL::x));
static_assert(std::is_pointer_interconvertible_with_class(&U::i) && std::is_pointer_interconvertible_with_class(&U::d));
static_assert(std::is_pointer_interconvertible_with_class(&DerivedEmptyBase::v));
static_assert(!std::is_pointer_interconvertible_with_class(static_cast<int SL::*>(nullptr)));
static_assert(std::is_pointer_interconvertible_with_class<Second, int>(&Second::x));

static_assert(std::is_corresponding_member(&SL::x, &SL2::p));
static_assert(std::is_corresponding_member(&SL::y, &SL2::q));
static_assert(!std::is_corresponding_member(&SL::z, &SL2::r));   // double vs float ends the sequence
static_assert(!std::is_corresponding_member(&SL::x, &SL2::q));
static_assert(std::is_corresponding_member(&SL::x, &SL::x));
static_assert(!std::is_corresponding_member(&SL::x, &SL::y));
static_assert(std::is_corresponding_member(&Bits1::b, &Bits2::d));    // same widths
static_assert(!std::is_corresponding_member(&Bits1::b, &Bits3::f));   // widths differ: sequence ends
static_assert(!std::is_corresponding_member(&NonSL::x, &SL::x));
static_assert(!std::is_corresponding_member(&U::i, &U::i));          // unions are not structs
static_assert(!std::is_corresponding_member(static_cast<int SL::*>(nullptr), &SL2::p));

int main() {}
