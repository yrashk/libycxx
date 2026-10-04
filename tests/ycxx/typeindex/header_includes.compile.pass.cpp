// [type.index.synopsis]: "#include <compare> // see [compare.syn]" and "#include <typeinfo>
// // see [typeinfo.syn]" -- so including <typeindex> alone makes the comparison category
// types and functions and the <typeinfo> classes available.
#include <typeindex>

static_assert(std::is_lt(std::strong_ordering::less));
static_assert(std::partial_ordering::unordered != 0);
static_assert(std::weak_ordering::equivalent == 0);
using C = std::common_comparison_category_t<std::strong_ordering, std::weak_ordering>;
std::compare_three_way cmp3;
const std::type_info& ti = typeid(int);
std::bad_cast* bc = nullptr;
std::bad_typeid* bt = nullptr;
std::type_index idx = typeid(long);
std::hash<std::type_index> h;
