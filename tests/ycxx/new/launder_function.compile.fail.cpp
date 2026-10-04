// [ptr.launder]/1: launder: "Mandates: !is_function_v<T> && !is_void_v<T> is true."
#include <new>

void f();
auto p = std::launder(&f);
