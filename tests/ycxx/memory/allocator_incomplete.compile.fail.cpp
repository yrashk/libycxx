// EXPECT-ERROR-GCC: error: invalid application of 'sizeof' to incomplete type 'Incomplete'
// EXPECT-ERROR-CLANG: error: invalid application of 'sizeof' to an incomplete type 'Incomplete'
// [allocator.members]/2: allocate(n): "Mandates: T is not an incomplete type."
#include <memory>

struct Incomplete;

void test(std::allocator<Incomplete>& a) { (void)a.allocate(1); }
