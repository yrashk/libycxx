// [pair.astuple]/5: template<class T1, class T2> constexpr T1& get(pair<T1, T2>& p) noexcept;
// "Mandates: T1 and T2 are distinct types."
#include <utility>

int f(std::pair<int, int>& p) { return std::get<int>(p); }
