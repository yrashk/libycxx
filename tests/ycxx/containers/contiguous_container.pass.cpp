// [container.reqmts]/68: "A contiguous container is a container whose member types iterator
// and const_iterator meet the Cpp17RandomAccessIterator requirements and model
// contiguous_iterator." [iterator.concept.contiguous]/2: for dereferenceable a and b with b
// reachable from a, to_address(b) == to_address(a) + iter_difference_t<I>(b - a), and
// to_address(a) == addressof(*a). vector<T> (T other than bool, [vector.overview]/1) and
// basic_string ([basic.string.general]/2) are contiguous containers; their data() is
// to_address(begin()) for a non-empty container ([vector.data]/1, [string.accessors]/1).
#include <vector>
#include <string>
#include <iterator>
#include <memory>
#include <type_traits>
#include "container_values.hpp"
#include "check.hpp"

template <class X>
constexpr bool test() {
  static_assert(std::contiguous_iterator<typename X::iterator>);
  static_assert(std::contiguous_iterator<typename X::const_iterator>);
  static_assert(std::random_access_iterator<typename X::iterator>);
  static_assert(std::derived_from<typename std::iterator_traits<typename X::iterator>::iterator_category,
                                  std::random_access_iterator_tag>);
  static_assert(std::derived_from<typename std::iterator_traits<typename X::const_iterator>::iterator_category,
                                  std::random_access_iterator_tag>);
  X a = make<X>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25});
  const X& ca = a;
  auto b = a.begin();
  auto cb = ca.begin();
  for (int k = 0; k < 25; ++k) {
    auto it = b + k;
    if (std::to_address(it) != std::to_address(b) + k) return false;
    if (std::to_address(it) != std::addressof(*it)) return false;
    if (std::to_address(cb + k) != std::to_address(it)) return false;
    if (a.data() + k != std::addressof(a[static_cast<typename X::size_type>(k)])) return false;
  }
  if (a.data() != std::to_address(a.begin()) || ca.data() != std::to_address(ca.cbegin())) return false;
  if (std::to_address(a.end()) != a.data() + 25) return false;  // past-the-end
  return true;
}

static_assert(test<std::vector<int>>());
static_assert(test<std::vector<Elem>>());
static_assert(test<std::string>());

int main() {
  CHECK(test<std::vector<int>>());
  CHECK(test<std::vector<Elem>>());
  CHECK(test<std::vector<std::string>>());
  CHECK(test<std::string>());
  CHECK(test<std::wstring>());
  CHECK(test<std::u32string>());
  return 0;
}
