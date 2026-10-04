// [rand.req.genl]/1.5: generate_canonical<RealType, digits, URBG> with a RealType that is not a
// floating-point type is ill-formed.
#include <random>

int main() {
  std::mt19937 g;
  return std::generate_canonical<int, 10>(g);
}
