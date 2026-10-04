// Second translation unit of linkage/static_destruction_iostreams.pass.cpp. It includes
// <iostream>, so ([iostream.objects.overview]/5) it behaves as if it defined its own
// ios_base::Init object, constructed before c2 below.
#include <iostream>
#include <format>
#include <string>
#include "child_process.hpp"

namespace {
struct Loud2 {
  std::string name;
  explicit Loud2(std::string n) : name(std::move(n)) {
    if (child_mode() && !child_mode_is("unsync")) std::cout << "ctor " << name << '\n';
  }
  ~Loud2() {
    if (!child_mode()) return;
    bool ok = std::cout.good() && std::cout.rdbuf() != nullptr && std::cin.tie() == &std::cout;
    std::cout << std::format("dtor {} {}", name, ok ? "ok" : "BAD") << '\n';
    std::cerr << "err " << name << '\n';
  }
};
Loud2 c2("c2");
}  // namespace

int tu2_touch() {
  if (child_mode()) std::cout << "touch2\n";
  return static_cast<int>(c2.name.size());
}
