// [range.utility.conv.to]/1: "Mandates: C is a cv-unqualified class type."
#include <ranges>

struct C {
  C() = default;
  template <class R>
  C(R&&) {}
};

int main() {
  int a[2] = {1, 2};
  auto c = std::ranges::to<const C>(a);
  (void)c;
}
