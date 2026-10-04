// [pair.astuple]/3: template<size_t I, class T1, class T2> get(pair<T1, T2>& p) noexcept;
// "Mandates: I<2."
#include <utility>

int f(std::pair<int, int>& p) { return std::get<2>(p); }
