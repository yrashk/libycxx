// [span.objectrep]/3: as_writable_bytes "Constraints: is_const_v<ElementType> is false".
#include <span>

void f(std::span<const int> s) { (void)std::as_writable_bytes(s); }
