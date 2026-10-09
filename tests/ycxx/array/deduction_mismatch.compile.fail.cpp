// EXPECT-ERROR-GCC: error: no matching function for call to 'array\(int, long int\)'
// EXPECT-ERROR-CLANG: error: no viable constructor or deduction guide.*'std::array'
// [array.cons]/2: deduction guide array(T, U...): "Mandates: (is_same_v<T, U> && ...) is true."
#include <array>

std::array a{1, 2L};
