// [cmath.syn]/4: "An invocation of nexttoward is ill-formed if the argument corresponding to
// the floating-point-type parameter has extended floating-point type."
// (Needs an extended floating-point type: without one, nothing can be checked, and the test
// fails to compile trivially.)
#include <cmath>
#include <stdfloat>

#if defined(__STDCPP_FLOAT32_T__)
auto r = std::nexttoward(std::float32_t(1), 2.0L);
#else
static_assert(false, "no extended floating-point type: [cmath.syn]/4 cannot be exercised");
#endif
