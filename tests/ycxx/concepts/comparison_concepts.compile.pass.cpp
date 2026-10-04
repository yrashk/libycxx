// [concept.booleantestable], [concept.comparisoncommontype] (C++23, P2404),
// [concept.equalitycomparable], [concept.totallyordered]:
// equality_comparable_with / totally_ordered_with use comparison-common-type-with, which
// accepts move-only types (convertible_to<T, const C&>), so e.g. a move-only handle is
// equality_comparable_with<nullptr_t>.
#include <concepts>
#include <compare>
#include <cstddef>

struct EqOnly {
  friend bool operator==(EqOnly, EqOnly);
};
struct NotBool {};
struct WeirdBool {
  operator bool() const;
  NotBool operator!() const;  // ! does not yield something boolean-testable
};
struct EqWeird {
  friend WeirdBool operator==(EqWeird, EqWeird);
  friend WeirdBool operator!=(EqWeird, EqWeird);
};
struct ExplicitBool {
  explicit operator bool() const;
};
struct EqExplicit {
  friend ExplicitBool operator==(EqExplicit, EqExplicit);
};
struct Ordered {
  friend auto operator<=>(Ordered, Ordered) = default;
};
struct LessOnly {
  friend bool operator<(LessOnly, LessOnly);
  friend bool operator==(LessOnly, LessOnly);
};

// A move-only handle comparable with nullptr_t (like unique_ptr).
struct Handle {
  Handle(std::nullptr_t);
  Handle(Handle&&);
  Handle(const Handle&) = delete;
  friend bool operator==(const Handle&, const Handle&);
  friend bool operator==(const Handle&, std::nullptr_t);
};

static_assert(std::equality_comparable<int>);
static_assert(std::equality_comparable<int*>);
static_assert(std::equality_comparable<EqOnly>);  // != is rewritten from ==
static_assert(!std::equality_comparable<NotBool>);
static_assert(!std::equality_comparable<EqWeird>);
static_assert(!std::equality_comparable<EqExplicit>);
static_assert(std::equality_comparable<int&>);
static_assert(std::equality_comparable_with<int, long>);
static_assert(std::equality_comparable_with<int*, const int*>);
static_assert(std::equality_comparable_with<int*, std::nullptr_t>);
static_assert(!std::equality_comparable_with<int, int*>);
static_assert(std::equality_comparable_with<Handle, std::nullptr_t>);

static_assert(std::totally_ordered<int>);
static_assert(std::totally_ordered<double>);
static_assert(std::totally_ordered<Ordered>);
static_assert(!std::totally_ordered<LessOnly>);
static_assert(!std::totally_ordered<EqOnly>);
static_assert(std::totally_ordered_with<int, double>);
static_assert(std::totally_ordered_with<int*, const int*>);
static_assert(!std::totally_ordered_with<int, Ordered>);
