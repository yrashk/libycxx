// [concept.destructible]: destructible = is_nothrow_destructible_v<T> ("forbids destructors
// that are potentially throwing"). [concept.constructible], [concept.default.init]:
// default_initializable = constructible_from<T> && requires { T{}; } &&
// is-default-initializable<T> ("T t;" is well-formed). [concept.moveconstructible],
// [concept.copyconstructible], [concepts.object]: movable, copyable, semiregular, regular.
#include <concepts>

struct ThrowingDtor {
  ~ThrowingDtor() noexcept(false);
};
struct NoDefault {
  NoDefault(int);
};
struct ExplicitDefault {
  explicit ExplicitDefault() = default;
};
struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(MoveOnly&&) = default;
  MoveOnly& operator=(MoveOnly&&) = default;
};
struct CopyNoAssign {
  CopyNoAssign(const CopyNoAssign&) = default;
  CopyNoAssign& operator=(const CopyNoAssign&) = delete;
};
struct NonConstCopy {
  NonConstCopy() = default;
  NonConstCopy(NonConstCopy&);
};
struct Reg {
  int v;
  bool operator==(const Reg&) const = default;
};
struct ExplicitCopy {
  ExplicitCopy() = default;
  explicit ExplicitCopy(const ExplicitCopy&) = default;
};

static_assert(std::destructible<int>);
static_assert(std::destructible<int&>);
static_assert(std::destructible<int[3]>);
static_assert(!std::destructible<int[]>);
static_assert(!std::destructible<void>);
static_assert(!std::destructible<void()>);
static_assert(!std::destructible<ThrowingDtor>);
static_assert(!std::constructible_from<ThrowingDtor>);

static_assert(std::default_initializable<int>);
static_assert(std::default_initializable<int*>);
static_assert(!std::default_initializable<const int>);  // "const int t;" is ill-formed
static_assert(!std::default_initializable<int&>);
static_assert(!std::default_initializable<NoDefault>);
static_assert(std::default_initializable<int[3]>);
static_assert(std::default_initializable<const Reg> == false);
static_assert(std::default_initializable<ExplicitDefault>);

static_assert(std::move_constructible<MoveOnly>);
static_assert(!std::copy_constructible<MoveOnly>);
static_assert(std::movable<MoveOnly>);
static_assert(!std::copyable<MoveOnly>);
static_assert(std::copy_constructible<CopyNoAssign>);
static_assert(!std::copyable<CopyNoAssign>);
static_assert(!std::copy_constructible<NonConstCopy>);
static_assert(!std::copy_constructible<ExplicitCopy>);  // convertible_to<const T&, T> fails
static_assert(std::move_constructible<int&>);
static_assert(std::copy_constructible<int&>);
static_assert(!std::movable<int&>);  // not an object type
static_assert(!std::movable<const int>);
static_assert(std::semiregular<int>);
static_assert(!std::regular<MoveOnly>);
static_assert(std::semiregular<Reg> && std::regular<Reg>);
static_assert(!std::regular<ExplicitDefault> && std::semiregular<ExplicitDefault>);
