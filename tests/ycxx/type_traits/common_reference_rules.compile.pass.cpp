// [meta.trans.other]/3: CREF, XREF, COPYCV, COND-RES, COMMON-REF (3.5)-(3.9).
// [meta.trans.other]/6: common_reference.
//   (6.1) no types: no member; (6.2) one type T0: type is T0;
//   (6.3.1) R = COMMON-REF(T1, T2): if T1 and T2 are references, R is well-formed and
//           is_convertible_v<add_pointer_t<T1>, add_pointer_t<R>> && is_convertible_v<add_pointer_t<T2>,
//           add_pointer_t<R>>, type is R (P2655R3);
//   (6.3.2) else basic_common_reference<remove_cvref_t<T1>, remove_cvref_t<T2>, XREF(T1), XREF(T2)>::type;
//   (6.3.3) else COND-RES(T1, T2); (6.3.4) else common_type_t<T1, T2>; (6.3.5) else none.
//   (6.4) more than two: fold from the left.
// /7: programs may partially specialize basic_common_reference for decayed types.
#include <type_traits>

template <class A, class B> constexpr bool same = std::is_same_v<A, B>;
template <class... T> concept HasCR = requires { typename std::common_reference<T...>::type; };

struct B {};
struct D : B {};
struct D2 : B {};
struct Other {};
struct Incomplete;

// (6.1), (6.2)
static_assert(!HasCR<>);
static_assert(same<std::common_reference_t<int>, int>);
static_assert(same<std::common_reference_t<const int&>, const int&>);
static_assert(same<std::common_reference_t<int&&>, int&&>);
static_assert(same<std::common_reference_t<int[3]>, int[3]>);          // not decayed
static_assert(same<std::common_reference_t<void() const>, void() const>);
static_assert(same<std::common_reference_t<void>, void>);
static_assert(same<std::common_reference_t<const volatile void>, const volatile void>);
static_assert(same<std::common_reference_t<Incomplete&>, Incomplete&>);

// (3.5) both lvalue references.
static_assert(same<std::common_reference_t<int&, int&>, int&>);
static_assert(same<std::common_reference_t<int&, const int&>, const int&>);
static_assert(same<std::common_reference_t<const int&, volatile int&>, const volatile int&>);
static_assert(same<std::common_reference_t<D&, B&>, B&>);
static_assert(same<std::common_reference_t<const D&, B&>, const B&>);
static_assert(same<std::common_reference_t<int (&)[3], int (&)[3]>, int (&)[3]>);
static_assert(same<std::common_reference_t<int (&)[3], const int (&)[3]>, const int (&)[3]>);
static_assert(same<std::common_reference_t<int* &, const int* &>, const int*>);   // COMMON-REF: prvalue, not a reference
static_assert(same<std::common_reference_t<void (&)(), void (&)()>, void (&)()>);
// (3.6) both rvalue references.
static_assert(same<std::common_reference_t<int&&, int&&>, int&&>);
static_assert(same<std::common_reference_t<int&&, const int&&>, const int&&>);
static_assert(same<std::common_reference_t<D&&, B&&>, B&&>);
static_assert(same<std::common_reference_t<const D&&, volatile B&&>, const volatile B&&>);
static_assert(same<std::common_reference_t<int (&&)[3], const int (&&)[3]>, const int (&&)[3]>);
// (3.7)/(3.8) mixed.
static_assert(same<std::common_reference_t<int&&, int&>, const int&>);
static_assert(same<std::common_reference_t<int&, int&&>, const int&>);
static_assert(same<std::common_reference_t<int&&, const int&>, const int&>);
static_assert(same<std::common_reference_t<const int&, int&&>, const int&>);
static_assert(same<std::common_reference_t<D&&, B&>, const B&>);
static_assert(same<std::common_reference_t<B&, D&&>, const B&>);
static_assert(same<std::common_reference_t<D&&, const B&>, const B&>);
// volatile: D = COMMON-REF(const volatile int&, volatile int&) = const volatile int&, and
// volatile int&& is not convertible to it (an rvalue cannot bind to a volatile lvalue reference),
// so COMMON-REF fails; COND-RES(volatile int&&, volatile int&) is the prvalue int.
static_assert(same<std::common_reference_t<volatile int&&, volatile int&>, int>);
static_assert(same<std::common_reference_t<volatile int&, volatile int&&>, int>);
static_assert(same<std::common_reference_t<const volatile int&&, int&>, int>);
// Non-reference operands: COND-RES.
static_assert(same<std::common_reference_t<int, int>, int>);
static_assert(same<std::common_reference_t<int&, int>, int>);
static_assert(same<std::common_reference_t<int, const int&&>, int>);
static_assert(same<std::common_reference_t<int, long>, long>);
static_assert(same<std::common_reference_t<int&, long&>, long>);
static_assert(same<std::common_reference_t<const int&, long&&>, long>);
static_assert(same<std::common_reference_t<B, D>, B>);
static_assert(same<std::common_reference_t<D&, B>, B>);
static_assert(same<std::common_reference_t<void, void>, void>);
static_assert(same<std::common_reference_t<const void, void>, void>);
static_assert(!HasCR<void, int>);
static_assert(!HasCR<int&, void>);
static_assert(!HasCR<B&, Other&>);
static_assert(!HasCR<int*, long*>);
static_assert(same<std::common_reference_t<int*, const int*>, const int*>);
static_assert(same<std::common_reference_t<int (&)[3], int*>, int*>);
static_assert(!HasCR<D&, D2&>);   // no conversion between sibling classes
// (6.3.4) common_type is reached only when COND-RES fails.
struct CA {};
struct CB {};
struct CC { CC(CA); CC(CB); };
template <> struct std::common_type<CA, CB> { using type = CC; };
template <> struct std::common_type<CB, CA> { using type = CC; };
static_assert(same<std::common_reference_t<CA, CB>, CC>);
static_assert(same<std::common_reference_t<CA&, const CB&&>, CC>);
static_assert(same<std::common_reference_t<CB&, CA&>, CC>);

// (6.3.2) basic_common_reference with XREF: TQual/UQual add T1's/T2's cv and ref qualifiers.
// Probe<A, B> records the template arguments; it is constructible from anything, so the
// specialization meets /7 (TQual<T> and UQual<U> are convertible to the result).
template <class A, class Bq> struct Probe { template <class T> Probe(T&&); };
struct X {};
struct Y {};
template <template <class> class XQ, template <class> class YQ>
struct std::basic_common_reference<X, Y, XQ, YQ> { using type = Probe<XQ<int>, YQ<int>>; };
template <template <class> class YQ, template <class> class XQ>
struct std::basic_common_reference<Y, X, YQ, XQ> { using type = Probe<XQ<int>, YQ<int>>; };
static_assert(same<std::common_reference_t<X, Y>, Probe<int, int>>);
static_assert(same<std::common_reference_t<X&, Y>, Probe<int&, int>>);
static_assert(same<std::common_reference_t<const X&, Y&&>, Probe<const int&, int&&>>);
static_assert(same<std::common_reference_t<volatile X&&, const volatile Y&>, Probe<volatile int&&, const volatile int&>>);
static_assert(same<std::common_reference_t<const X, Y>, Probe<const int, int>>);
static_assert(same<std::common_reference_t<Y&, const X&&>, Probe<const int&&, int&>>);
static_assert(same<std::common_reference_t<const volatile Y, X&>, Probe<int&, const volatile int>>);

// (6.3.1) before (6.3.2): when COMMON-REF is a reference that both operands' pointers convert to,
// basic_common_reference is not consulted.
struct P {};
struct Q : P {};
template <template <class> class A, template <class> class Bq>
struct std::basic_common_reference<P, Q, A, Bq> { using type = Probe<P, Q>; };
template <template <class> class A, template <class> class Bq>
struct std::basic_common_reference<Q, P, A, Bq> { using type = Probe<P, Q>; };
static_assert(same<std::common_reference_t<P&, Q&>, P&>);
static_assert(same<std::common_reference_t<Q&&, P&&>, P&&>);
static_assert(same<std::common_reference_t<Q&&, const P&>, const P&>);
static_assert(same<std::common_reference_t<P, Q>, Probe<P, Q>>);    // not both references
static_assert(same<std::common_reference_t<P&, Q>, Probe<P, Q>>);

// (6.3.1) P2655R3: COMMON-REF(Conv&, int&) is int& (Conv converts to int&), but Conv* does not
// convert to int*, so (6.3.2) is used.
struct Conv { operator int&() const; };
template <template <class> class A, template <class> class Bq>
struct std::basic_common_reference<Conv, int, A, Bq> { using type = long; };
template <template <class> class A, template <class> class Bq>
struct std::basic_common_reference<int, Conv, A, Bq> { using type = long; };
static_assert(same<std::common_reference_t<Conv&, int&>, long>);
static_assert(same<std::common_reference_t<int&, Conv&>, long>);
// Without a basic_common_reference specialization, (6.3.3) COND-RES gives int&.
struct Conv2 { operator int&() const; };
static_assert(same<std::common_reference_t<Conv2&, int&>, int&>);
static_assert(same<std::common_reference_t<int&, Conv2&>, int&>);

// A basic_common_reference specialization with no member type falls through to (6.3.3).
struct NoMember {};
template <template <class> class A, template <class> class Bq>
struct std::basic_common_reference<NoMember, NoMember, A, Bq> {};
static_assert(same<std::common_reference_t<NoMember, NoMember>, NoMember>);
static_assert(same<std::common_reference_t<NoMember&, const NoMember&>, const NoMember&>);

// (6.4)
static_assert(same<std::common_reference_t<int&, int&, int&>, int&>);
static_assert(same<std::common_reference_t<int&, const int&, volatile int&>, const volatile int&>);
static_assert(same<std::common_reference_t<int&&, int&, const int&>, const int&>);
static_assert(same<std::common_reference_t<D&, B&, const B&>, const B&>);
static_assert(same<std::common_reference_t<int, long, double>, double>);
static_assert(same<std::common_reference_t<X, Y, Probe<int, int>>, Probe<int, int>>);
static_assert(!HasCR<int&, int&, void>);
static_assert(!HasCR<B&, Other&, B&>);
static_assert(!HasCR<int, int, int*>);

int main() {}
