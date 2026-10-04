// [numeric.ops.gcd]/1: "Mandates: M and N both are integer types other than cv bool."
#include <numeric>

int main() {
  return std::gcd(true, 4);
}
