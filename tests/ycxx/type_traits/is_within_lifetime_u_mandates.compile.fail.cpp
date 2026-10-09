// EXPECT-ERROR-GCC: error: no matching function for call to 'is_within_lifetime<long int>\(const int\*\)'
// EXPECT-ERROR-CLANG: error: static assertion failed[^\n]*std::is_within_lifetime: static_cast<const volatile U\*>\(p\) must be well-formed
// [meta.const.eval]/3: is_within_lifetime<U>(p) Mandates: static_cast<const volatile U*>(p) is
// well-formed. int* to const volatile long* is not a valid static_cast.
// Control: is_within_lifetime_u.pass.cpp (is_within_lifetime<const int>(&g) compiles).
#include <type_traits>

constexpr int g = 0;
static_assert(std::is_within_lifetime<long>(&g) || true);

int main() {}
