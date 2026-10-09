// EXPECT-ERROR-GCC: error: converting to 'std::any'.*use explicit constructor
// EXPECT-ERROR-CLANG: error: chosen constructor is explicit in copy-initialization
// EXPECT-ERROR-CLANG: \bany\(in_place_type_t
// [any.class.general], [any.cons]/10: "template<class T, class... Args> explicit
// any(in_place_type_t<T>, Args&&...);" is explicit, so copy-list-initialisation is ill-formed.
#include <any>
#include <utility>

std::any a = {std::in_place_type<int>, 1};
