// [pair.astuple]/5-8: get<T>(pair<T1, T2>&) is declared only for T = T1 (first overload set)
// and T = T2 (second); there is no overload for any other type.
#include <utility>

int f(std::pair<int, long>& p) { return std::get<char>(p); }
