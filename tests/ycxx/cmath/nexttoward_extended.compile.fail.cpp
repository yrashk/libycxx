// EXPECT-ERROR: error: static assertion failed[^\n]*nexttoward is ill-formed for an argument of extended floating-point type
// [cmath.syn]/4: nexttoward rejects an extended floating-point first argument.
// REQUIRES: extended-float32
// The harness checks the advertised type before running this diagnostic test.
#include <cmath>
#include <stdfloat>

auto r = std::nexttoward(std::float32_t(1), 2.0L);
