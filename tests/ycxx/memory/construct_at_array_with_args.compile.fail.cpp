// [specialized.construct]/2: construct_at "Mandates: If is_array_v<T> is true,
// sizeof...(Args) is zero."
#include <memory>

void f(int (*p)[3]) { std::construct_at(p, 1); }
