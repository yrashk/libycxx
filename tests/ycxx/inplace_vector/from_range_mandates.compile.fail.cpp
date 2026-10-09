// EXPECT-ERROR: error: static assertion failed[^\n]*std::inplace_vector\(from_range_t, R\&\&\): the range has more than N elements
// [inplace.vector.cons]/9: inplace_vector(from_range_t, R&& rg) "Mandates: If
// ranges::size(rg) is a constant expression, then ranges::size(rg) <= N." An array of 3
// elements into an inplace_vector of capacity 2 is therefore ill-formed.
#include <inplace_vector>
#include <ranges>

int main() {
  int arr[3] = {1, 2, 3};
  std::inplace_vector<int, 2> v(std::from_range, arr);
  return static_cast<int>(v.size());
}
