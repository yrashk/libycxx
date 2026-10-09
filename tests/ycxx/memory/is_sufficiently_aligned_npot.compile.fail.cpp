// EXPECT-ERROR: error: static assertion failed[^\n]*Alignment must be a power of two
// [ptr.align]/10: is_sufficiently_aligned "Mandates: Alignment is a power of two."
#include <memory>

bool f(int* p) { return std::is_sufficiently_aligned<3>(p); }
