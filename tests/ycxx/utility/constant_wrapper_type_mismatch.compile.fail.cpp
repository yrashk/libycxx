// [const.wrap.class]/2: "If a specialization of constant_wrapper is instantiated with a type T
// such that is_same_v<T, value_type> is false, the program is ill-formed."
#include <utility>

std::constant_wrapper<1, long> c;
