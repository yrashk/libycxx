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

#include "reqs/contiguous_container.hpp"

using namespace reqs::contiguous_container;

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
