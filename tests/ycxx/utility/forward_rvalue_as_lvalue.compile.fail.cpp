// EXPECT-ERROR: error: static assertion failed[^\n]*std::forward: cannot forward an rvalue as an lvalue
// [forward]/2: template<class T> constexpr T&& forward(remove_reference_t<T>&& t) noexcept;
// "Mandates: For the second overload, is_lvalue_reference_v<T> is false."
#include <utility>

void test() { (void)std::forward<int&>(42); }
