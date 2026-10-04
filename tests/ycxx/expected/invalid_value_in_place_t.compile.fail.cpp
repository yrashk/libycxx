// [expected.object.general]/2: "A type T is a valid value type for expected, if remove_cv_t<T>
// is void or a complete non-array object type that is not in_place_t, unexpect_t, or a
// specialization of unexpected. A program which instantiates class template expected<T, E> with
// an argument T that is not a valid value type for expected is ill-formed."
// Here T is in_place_t (after remove_cv).
#include <expected>
#include <utility>

int n = sizeof(std::expected<const std::in_place_t, int>);
