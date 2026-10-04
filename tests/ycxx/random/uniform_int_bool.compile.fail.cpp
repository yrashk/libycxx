// [rand.req.genl]/1.6: IntType must be a standard signed or unsigned integer type (or an extended
// or implementation-defined integer type); bool is none of these, so the program is ill-formed.
#include <random>

int main() {
  std::mt19937 g;
  std::uniform_int_distribution<bool> d(false, true);
  return d(g);
}
