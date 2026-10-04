// [atomics.types.generic.general]/1: "The program is ill-formed if any of ...
// is_same_v<T, remove_cv_t<T>>, is false." (atomic<volatile S> also fails the copy checks;
// cf. cv_qualified_const for const int, which has no integral specialization.)
#include <atomic>

struct S { int v; };
std::atomic<volatile S> a;
