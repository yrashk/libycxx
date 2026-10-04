// [numeric.sat.func]/9-10: saturating_div: "Preconditions: y != 0 is true." "Remarks: A
// function call expression that violates the precondition in the Preconditions element is
// not a core constant expression." So it cannot initialize a constexpr variable.
#include <numeric>

constexpr int r = std::saturating_div(1, 0);

int main() {
  return r;
}
