// [depr.format.arg] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
// COUNTERPART: libcxx:utilities/format/format.arguments/format.arg/visit_format_arg.deprecated.verify.cpp
#include <format>
#include <cstddef>

int main() {
  int i = 1;
  auto store = std::make_format_args(i);
  std::visit_format_arg([](auto) {}, std::format_args(store).get(0));
}
