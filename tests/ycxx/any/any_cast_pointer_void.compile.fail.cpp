// EXPECT-ERROR: error: static assertion failed[^\n]*std::any_cast: T must not be void
// [any.nonmembers]/9: pointer forms of any_cast: "Mandates: is_void_v<T> is false."
#include <any>

void f(std::any& a) { (void)std::any_cast<void>(&a); }
