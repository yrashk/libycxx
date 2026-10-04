// [range.utility.conv.adaptors]/1: "Mandates: For the first overload, C is a cv-unqualified
// class type." (here C is int)
#include <ranges>

int main() {
  auto f = std::ranges::to<int>();
  (void)f;
}
