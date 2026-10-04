// [ptr.launder]/1: launder: "Mandates: !is_function_v<T> && !is_void_v<T> is true."
#include <new>

void* g(void* p) { return std::launder(p); }
