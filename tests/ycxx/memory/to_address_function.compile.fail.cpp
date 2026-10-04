// [pointer.conversion]/1: template<class T> constexpr T* to_address(T* p) noexcept;
// "Mandates: T is not a function type."
#include <memory>

void f();
auto p = std::to_address(&f);
