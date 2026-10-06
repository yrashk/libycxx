// [tuple.elem]/5: get<T>: "Mandates: The type T occurs exactly once in Types." Example 1:
// get<double>(t) of a tuple<int, const int, double, double> is an error.
// EXPECT-ERROR: no matching function for call to .*get|static assertion failed
#include <tuple>

const std::tuple<int, const int, double, double> t(1, 2, 3.4, 5.6);
const double& d = std::get<double>(t);
