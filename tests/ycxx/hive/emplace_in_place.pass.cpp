// [hive.modifiers]/1-2: emplace and emplace_hint insert "an object of type T constructed with
// std::forward<Args>(args)..." (Preconditions: T is Cpp17EmplaceConstructible into hive from
// args; the hint is ignored). So no copy or move of the new element is made, a type that can be
// neither copied nor moved works, and the arguments keep their value category.
#include <hive>
#include <utility>
#include "inplace_probe.hpp"
#include "check.hpp"

using probe::Arg;
using probe::counts;
using probe::Pinned;
using probe::Probe;

void hive_cases() {
  std::hive<Probe> h;
  Arg a{};
  probe::reset();
  auto it1 = h.emplace(1, a);
  auto it2 = h.emplace_hint(it1, 2, std::move(a));
  CHECK(it1->cat == probe::lref && it2->cat == probe::rref);
  CHECK(counts.made == 2 && counts.extra() == 0 && counts.destroyed == 0);

  std::hive<Pinned> p;
  for (int i = 0; i < 300; ++i) p.emplace(i, Arg{});
  p.emplace_hint(p.begin(), 300);
  CHECK(p.size() == 301);
}

int main() {
  hive_cases();
  return 0;
}
