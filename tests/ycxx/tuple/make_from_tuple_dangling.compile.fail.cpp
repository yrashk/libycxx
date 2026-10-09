// EXPECT-ERROR: error: static assertion failed[^\n]*std::make_from_tuple: would bind a reference to a temporary
// [tuple.apply]/2: make_from_tuple "Mandates: If tuple_size_v<remove_reference_t<Tuple>> is
// 1, then reference_constructs_from_temporary_v<T, decltype(get<0>(declval<Tuple>()))> is
// false." Here T = const int& and the element is long&&, which would bind to a temporary.
#include <tuple>

void f() {
  const int& r = std::make_from_tuple<const int&>(std::tuple<long>(1L));
  (void)r;
}
