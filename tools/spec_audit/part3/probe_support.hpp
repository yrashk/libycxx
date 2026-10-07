// Support for the generated spec-audit probes (tools/spec_audit/part3). Only standard C++.
#pragma once
#include <type_traits>
#include <utility>
#include <concepts>
#include <iosfwd>

namespace spec_probe {
  // A type that depends on Z, so that a requires-expression over it is checked when its
  // concept is named (a failed check is then `false`, not a hard error).
  template<class Z, class T> struct dep_ { using type = T; };
  template<class Z, class T> using dep = typename dep_<Z, T>::type;
  template<class T> T&& dv() noexcept;   // declval without <utility>'s static_assert
  template<class A, class B> concept same = std::is_same_v<A, B>;
}
