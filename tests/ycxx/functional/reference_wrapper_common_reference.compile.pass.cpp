// [refwrap.common.ref] (C++23, P2655): basic_common_reference specializations make
// common_reference_t<reference_wrapper<T>, T&> be common_reference_t<T&, T&> = T&, in
// either order, and similarly with differently-qualified T.
#include <functional>
#include <concepts>
#include <type_traits>

using RI = std::reference_wrapper<int>;
using RCI = std::reference_wrapper<const int>;

static_assert(std::is_same_v<std::common_reference_t<RI, int&>, int&>);
static_assert(std::is_same_v<std::common_reference_t<int&, RI>, int&>);
static_assert(std::is_same_v<std::common_reference_t<RI&, int&>, int&>);
static_assert(std::is_same_v<std::common_reference_t<const RI&, int&>, int&>);
static_assert(std::is_same_v<std::common_reference_t<RI, const int&>, const int&>);
static_assert(std::is_same_v<std::common_reference_t<RCI, int&>, const int&>);
static_assert(std::is_same_v<std::common_reference_t<RI, int>, int>);
static_assert(std::common_reference_with<RI, int&>);
static_assert(std::common_reference_with<int&, RI&>);

// R::type& is Der&; common_reference_t<Der&, B&> is B&, and reference_wrapper<Der> converts to B&.
struct B {};
struct Der : B {};
static_assert(std::is_same_v<std::common_reference_t<std::reference_wrapper<Der>, B&>, B&>);
