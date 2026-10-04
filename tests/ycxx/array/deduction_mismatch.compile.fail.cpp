// [array.cons]/2: deduction guide array(T, U...): "Mandates: (is_same_v<T, U> && ...) is true."
#include <array>

std::array a{1, 2L};
