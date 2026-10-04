// [any.nonmembers]/9: pointer forms of any_cast: "Mandates: is_void_v<T> is false."
#include <any>

void f(std::any& a) { (void)std::any_cast<void>(&a); }
