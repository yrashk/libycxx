// EXPECT-ERROR: error: static assertion failed[^\n]*std::lcm: M and N must be integer types other than bool
// [numeric.ops.lcm]/1: "Mandates: M and N both are integer types other than cv bool."
#include <numeric>

int main() {
  return static_cast<int>(std::lcm(4.0, 6));
}
