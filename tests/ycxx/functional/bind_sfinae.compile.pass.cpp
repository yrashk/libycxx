// [func.bind.bind]/4: g(u1, ..., uM) "is expression-equivalent to INVOKE(static_cast<Vfd>(vfd),
// static_cast<V1>(v1), ...)" (INVOKE<R> for bind<R>) -- so the call is valid exactly when that
// expression is: a placeholder _j needs at least j call arguments, surplus call arguments are
// ignored, and an invalid target call makes g(u...) invalid rather than a hard error.
// /8: the target is always an lvalue (cv FD&), so g's value category does not matter but its
// constness does. [func.require]/2: INVOKE<R> converts implicitly. (Every bind call here
// satisfies /3: INVOKE(fd, w1, ..., wN) is valid for some values.)
#include <functional>
#include <type_traits>

using namespace std::placeholders;

int sub(int, int);
int one(int);
struct Explicit {
  explicit Explicit(int);
};
struct LvalueOnly {
  int operator()() & { return 0; }
  int operator()() && = delete;
  int operator()() const& = delete;
};
struct IntOnly {
  int operator()(int) const { return 0; }
};

using B2 = decltype(std::bind(sub, _1, _2));
static_assert(std::is_invocable_v<B2, int, int>);
static_assert(std::is_invocable_v<B2, int, int, int>);
static_assert(!std::is_invocable_v<B2, int>);
static_assert(!std::is_invocable_v<B2>);
static_assert(!std::is_invocable_v<B2, int, int*>);

using BS = decltype(std::bind(one, _3));
static_assert(std::is_invocable_v<BS, void*, void*, int>);
static_assert(!std::is_invocable_v<BS, int, int>);

using B0 = decltype(std::bind(sub, 1, 2));
static_assert(std::is_invocable_v<B0>);
static_assert(std::is_invocable_v<B0, void*>);
static_assert(std::is_invocable_r_v<int, B0>);

using BE = decltype(std::bind<Explicit>(one, 1));
static_assert(!std::is_invocable_v<BE>);
using BI = decltype(std::bind<int>(one, 1));
static_assert(std::is_invocable_r_v<int, BI>);

using BL = decltype(std::bind(LvalueOnly{}));
static_assert(std::is_invocable_v<BL&>);
static_assert(std::is_invocable_v<BL&&>);
static_assert(!std::is_invocable_v<const BL&>);

using BN = decltype(std::bind(IntOnly{}, std::bind(one, _1)));
static_assert(std::is_invocable_v<BN, int>);
static_assert(!std::is_invocable_v<BN, int*>);
static_assert(!std::is_invocable_v<BN>);
