// [ptr.align]: assume_aligned: "Mandates: N is a power of two."
#include <memory>

int* f(int* p) { return std::assume_aligned<12>(p); }
