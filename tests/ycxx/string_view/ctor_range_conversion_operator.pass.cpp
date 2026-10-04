// [string.view.cons]/12: template<class R> constexpr explicit basic_string_view(R&& r);
// "Constraints: ... (12.5) d.operator ::std::basic_string_view<charT, traits>() is not a
// valid expression." where (/11) "Let d be an lvalue of type remove_cvref_t<R>."
// Cases:
//  - an explicit conversion operator still makes d.operator basic_string_view() valid, so
//    the range constructor is excluded and direct-initialisation uses the conversion;
//  - a conversion to basic_string_view with *different traits* does not exclude it;
//  - an rvalue-only (&&-qualified) conversion operator is not callable on the lvalue d, so
//    the range constructor participates, and wins over the conversion for an rvalue R.
#include <string_view>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct OtherTraits : std::char_traits<char> {};

template <int Kind>
struct R {
  char d_[3] = {'a', 'b', 'c'};
  constexpr const char* begin() const { return d_; }
  constexpr const char* end() const { return d_ + 3; }
  constexpr const char* data() const { return d_; }
  constexpr std::size_t size() const { return 3; }
};

struct ExplicitConv : R<0> {
  constexpr explicit operator std::string_view() const { return std::string_view(d_, 1); }
};
struct OtherTraitsConv : R<1> {
  constexpr operator std::basic_string_view<char, OtherTraits>() const {
    return std::basic_string_view<char, OtherTraits>(d_, 1);
  }
};
struct RvalueConv : R<2> {
  constexpr operator std::string_view() && { return std::string_view(d_, 1); }
};

static_assert(std::is_constructible_v<std::string_view, ExplicitConv&>);
static_assert(!std::is_convertible_v<ExplicitConv&, std::string_view>);
static_assert(std::is_constructible_v<std::string_view, OtherTraitsConv&>);
static_assert(!std::is_convertible_v<OtherTraitsConv&, std::string_view>);
static_assert(std::is_constructible_v<std::string_view, RvalueConv&>);   // range constructor
static_assert(!std::is_convertible_v<RvalueConv&, std::string_view>);   // && operator: not on lvalues
static_assert(std::is_convertible_v<RvalueConv, std::string_view>);     // && operator on rvalues

constexpr bool test() {
  ExplicitConv e;
  std::string_view a(e);  // conversion function, not the range constructor
  if (a.size() != 1 || a.data() != e.d_) return false;
  OtherTraitsConv o;
  std::string_view b(o);  // range constructor
  if (b.size() != 3 || b.data() != o.d_) return false;
  RvalueConv r;
  std::string_view c(r);  // range constructor (lvalue)
  if (c.size() != 3) return false;
  std::string_view d(std::move(r));  // range constructor is an exact match: preferred
  if (d.size() != 3 || d.data() != r.d_) return false;
  std::string_view f = RvalueConv{}; // copy-initialisation: only the conversion function
  if (f.size() != 1) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
