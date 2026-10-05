// Facts about the formatters as this translation unit sees them (internal linkage: one copy per
// translation unit). Include after the headers under test; Ref... is vector<bool>::reference
// when <vector> is available.
#pragma once
#include "decl.hpp"

template <class... Ref>
static Facts facts_here() {
  Facts f{sizeof(std::formatter<int>),
          sizeof(std::formatter<double>),
          sizeof(std::formatter<const char*>),
          sizeof(std::formatter<std::string>),
          sizeof(std::formatter<char>),
          sizeof(std::formatter<wchar_t, wchar_t>),
          sizeof(std::formatter<const void*>),
          0,
          &typeid(std::formatter<int>),
          &typeid(std::formatter<std::string>),
          &typeid(std::formatter<wchar_t, wchar_t>),
          nullptr};
  if constexpr (sizeof...(Ref) == 1) {
    f.size_ref = (sizeof(std::formatter<Ref>) + ...);
    f.ti_ref = (&typeid(std::formatter<Ref>), ...);
  }
  return f;
}
