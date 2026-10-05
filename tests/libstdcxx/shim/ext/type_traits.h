// Test-harness shim (see bits/c++config.h), included by testsuite_tr1.h. Its check_ret_type
// returns `__gnu_cxx::__enable_if<std::__are_same<R, T>::__value, bool>::__type`. Harness-side
// equivalents of those two libstdc++-internal templates, in terms of std::enable_if and
// std::is_same ([meta.trans.other], [meta.rel]).
#pragma once
#include <type_traits>

namespace __gnu_cxx {
template <bool B, class T>
struct __enable_if {};
template <class T>
struct __enable_if<true, T> {
  using __type = T;
};
} // namespace __gnu_cxx

namespace std {
template <class T, class U>
struct __are_same {
  static constexpr bool __value = is_same_v<T, U>;
};
} // namespace std
