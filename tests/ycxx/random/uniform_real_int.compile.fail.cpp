// [rand.req.genl]/1.5: "If a template argument corresponding to a template parameter named
// RealType is neither a standard floating-point type nor a member of an implementation-defined
// subset of extended floating-point types, the program is ill-formed."
#include <random>

int main() {
  std::mt19937 g;
  std::uniform_real_distribution<int> d(0, 9);
  return d(g);
}
