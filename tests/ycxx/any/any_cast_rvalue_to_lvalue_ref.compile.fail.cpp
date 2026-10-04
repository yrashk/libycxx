// [any.nonmembers]/5: "For the third overload [any&&], is_constructible_v<T, U> is true."
// is_constructible_v<int&, int> is false, so any_cast<int&>(std::move(a)) is ill-formed.
#include <any>
#include <utility>

void f(std::any& a) { (void)std::any_cast<int&>(std::move(a)); }
