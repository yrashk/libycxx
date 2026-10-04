// [rand.req.genl]/1.1: the program is ill-formed if the template argument for RealType is
// cv-qualified.
#include <random>

int main() {
  std::mt19937 g;
  std::normal_distribution<const double> d;
  return int(d(g));
}
