// EXPECT-ERROR-GCC: error: no matching function for call to 'as_writable_bytes\(std::span<const int>&\)'
// EXPECT-ERROR-CLANG: error: no matching function for call to 'as_writable_bytes'
// [span.objectrep]/3: as_writable_bytes "Constraints: is_const_v<ElementType> is false".
#include <span>

void f(std::span<const int> s) { (void)std::as_writable_bytes(s); }
