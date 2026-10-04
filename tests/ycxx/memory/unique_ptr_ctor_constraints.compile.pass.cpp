// [unique.ptr.single.ctor]/1,5: the default, nullptr_t and single-pointer constructors are
// constrained on "is_pointer_v<deleter_type> is false and is_default_constructible_v
// <deleter_type> is true". /9: the (p, d) constructors are constrained on
// is_constructible_v<D, decltype(d)>, and /12 for a reference D the rvalue-deleter one is
// deleted. /19: unique_ptr(unique_ptr<U, E>&&) needs pointer convertibility, U not an
// array, and E == D for a reference D (else E implicitly convertible to D). /15: the move
// constructor needs is_move_constructible_v<D>.
#include <memory>
#include <cstddef>
#include <type_traits>

struct D {
  void operator()(int*) const {}
};
struct NoDefault {
  NoDefault(int) {}
  void operator()(int*) const {}
};
struct MoveOnlyDel {
  MoveOnlyDel() = default;
  MoveOnlyDel(MoveOnlyDel&&) = default;
  MoveOnlyDel& operator=(MoveOnlyDel&&) = default;
  void operator()(int*) const {}
};
using FP = void (*)(int*);

// Function pointer deleter: no default / nullptr / single-pointer construction.
static_assert(!std::is_default_constructible_v<std::unique_ptr<int, FP>>);
static_assert(!std::is_constructible_v<std::unique_ptr<int, FP>, std::nullptr_t>);
static_assert(!std::is_constructible_v<std::unique_ptr<int, FP>, int*>);
static_assert(std::is_constructible_v<std::unique_ptr<int, FP>, int*, FP>);
// Non-default-constructible deleter, and reference deleters.
static_assert(!std::is_default_constructible_v<std::unique_ptr<int, NoDefault>>);
static_assert(!std::is_constructible_v<std::unique_ptr<int, NoDefault>, int*>);
static_assert(std::is_constructible_v<std::unique_ptr<int, NoDefault>, int*, NoDefault>);
static_assert(!std::is_default_constructible_v<std::unique_ptr<int, D&>>);
static_assert(!std::is_constructible_v<std::unique_ptr<int, D&>, int*>);
// (p, d) with reference deleters: lvalue ok, rvalue deleted.
static_assert(std::is_constructible_v<std::unique_ptr<int, D&>, int*, D&>);
static_assert(!std::is_constructible_v<std::unique_ptr<int, D&>, int*, D&&>);
static_assert(!std::is_constructible_v<std::unique_ptr<int, D&>, int*, const D&>);
static_assert(std::is_constructible_v<std::unique_ptr<int, const D&>, int*, D&>);
static_assert(std::is_constructible_v<std::unique_ptr<int, const D&>, int*, const D&>);
static_assert(!std::is_constructible_v<std::unique_ptr<int, const D&>, int*, D&&>);
static_assert(!std::is_constructible_v<std::unique_ptr<int, const D&>, int*, const D&&>);
// Move-only deleter: copy from an lvalue is not constructible, from an rvalue is.
static_assert(!std::is_constructible_v<std::unique_ptr<int, MoveOnlyDel>, int*, MoveOnlyDel&>);
static_assert(std::is_constructible_v<std::unique_ptr<int, MoveOnlyDel>, int*, MoveOnlyDel&&>);

// Converting construction.
struct B {
  virtual ~B() = default;
};
struct Dv : B {};
static_assert(std::is_constructible_v<std::unique_ptr<B>, std::unique_ptr<Dv>&&>);
static_assert(std::is_convertible_v<std::unique_ptr<Dv>, std::unique_ptr<B>>);
static_assert(!std::is_constructible_v<std::unique_ptr<Dv>, std::unique_ptr<B>&&>);
static_assert(!std::is_constructible_v<std::unique_ptr<B>, std::unique_ptr<Dv>&>);
static_assert(!std::is_constructible_v<std::unique_ptr<B>, std::unique_ptr<Dv[]>&&>);
static_assert(!std::is_constructible_v<std::unique_ptr<int>, std::unique_ptr<int[]>&&>);
static_assert(std::is_constructible_v<std::unique_ptr<const int>, std::unique_ptr<int>&&>);
static_assert(!std::is_constructible_v<std::unique_ptr<int>, std::unique_ptr<const int>&&>);
// reference deleter: E must be the same type as D
static_assert(std::is_constructible_v<std::unique_ptr<const int, D&>, std::unique_ptr<int, D&>&&>);
static_assert(!std::is_constructible_v<std::unique_ptr<const int, D&>, std::unique_ptr<int, D>&&>);
static_assert(!std::is_constructible_v<std::unique_ptr<const int, const D&>, std::unique_ptr<int, D&>&&>);
// value deleter: E implicitly convertible to D (a reference E included)
static_assert(std::is_constructible_v<std::unique_ptr<const int, D>, std::unique_ptr<int, D&>&&>);
static_assert(!std::is_constructible_v<std::unique_ptr<int, D>, std::unique_ptr<int, NoDefault>&&>);
// move constructor needs a move-constructible deleter
struct Immovable {
  Immovable() = default;
  Immovable(const Immovable&) = delete;
  void operator()(int*) const {}
};
static_assert(!std::is_move_constructible_v<std::unique_ptr<int, Immovable>>);
static_assert(std::is_move_constructible_v<std::unique_ptr<int, Immovable&>>);

int main() {}
