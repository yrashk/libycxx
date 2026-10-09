// EXPECT-ERROR: error: static assertion failed[^\n]*std::any_cast: T must be constructible from const U\&
// [any.nonmembers]/5: "For the first overload [const any&], is_constructible_v<T, const U&>
// is true." and [any.nonmembers]/8: "any_cast<string&>(y); // error: cannot any_cast away const"
#include <any>

void f(const std::any& a) { (void)std::any_cast<int&>(a); }
