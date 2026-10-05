// [string.view.access]/1: operator[](pos): "Hardened preconditions: pos < size() is true." (unlike
// basic_string, pos == size() is a violation).
// Death test (support/violation.hpp): run only in hardened mode.
// REQUIRES: hardened
// EXPECT-TERMINATE: about to violate
// COUNTERPART: libstdcxx:21_strings/basic_string_view/element_access/(char|wchar_t)/2.cc
#include <string_view>
#include "violation.hpp"

int main() {
  std::string_view sv = "abc";
  about_to_violate("string_view_subscript");
  keep(sv[3]);
  never_reached();
}
