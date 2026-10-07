// [version.syn]: __cpp_lib_view_interface is 202606L, also in <ranges> ([version.syn]/2). It
// names view_interface::at ([view.interface.members]/5-6; ranges/view_interface_at.pass.cpp).
// Only <ranges> is included.
#include <ranges>

#if !defined(__cpp_lib_view_interface)
#  error "__cpp_lib_view_interface is not defined by <ranges>"
#elif __cpp_lib_view_interface != 202606L
#  error "__cpp_lib_view_interface != 202606L"
#endif

struct V : std::ranges::view_interface<V> {
  int a[2] = {1, 2};
  constexpr const int* begin() const { return a; }
  constexpr const int* end() const { return a + 2; }
};
static_assert(V().at(1) == 2);
