// EXPECT-ERROR: error: static assertion failed[^\n]*counting_semaphore: LeastMaxValue must not be negative
// [thread.sema.cnt]/2: "least_max_value shall be non-negative; otherwise the program is
// ill-formed."
#include <semaphore>

std::counting_semaphore<-1> s(0);
