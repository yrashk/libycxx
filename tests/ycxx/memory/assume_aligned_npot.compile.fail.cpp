// EXPECT-ERROR: error: static assertion failed[^\n]*std::assume_aligned: N must be a power of two
// [ptr.align]: assume_aligned: "Mandates: N is a power of two."
#include <memory>

int* f(int* p) { return std::assume_aligned<12>(p); }
