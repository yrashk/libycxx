// EXPECT-ERROR: error: static assertion failed[^\n]*std::any_cast: T must be constructible from U\&
// [any.nonmembers]/5: "For the second overload [any&], is_constructible_v<T, U&> is true."
// is_constructible_v<int&&, int&> is false.
#include <any>

void f(std::any& a) { (void)std::any_cast<int&&>(a); }
