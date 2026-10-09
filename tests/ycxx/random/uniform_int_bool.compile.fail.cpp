// [rand.req.genl]/1.6 permits an implementation-defined additional subset of integer
// types. Bool is outside the mandatory IntType set; libycxx chooses an empty additional
// subset (DECISIONS.md section 10), so this specialization is rejected.
// This is a libycxx supported-type policy test, not a universal draft rejection oracle.
#include <random>

int main() {
  std::mt19937 g;
  std::uniform_int_distribution<bool> d(false, true);
  return d(g);
}
