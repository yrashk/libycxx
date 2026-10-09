// EXPECT-ERROR: error: static assertion failed[^\n]*discrete_distribution: IntType must be a standard integer type \(\[rand\.req\.genl\]/1\.6\)
// [rand.req.genl]/1.6: discrete_distribution<IntType> with a floating-point IntType is ill-formed.
#include <random>

int main() {
  std::mt19937 g;
  std::discrete_distribution<double> d{1.0, 2.0};
  return int(d(g));
}
