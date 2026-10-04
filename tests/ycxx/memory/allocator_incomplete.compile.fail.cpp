// [allocator.members]/2: allocate(n): "Mandates: T is not an incomplete type."
#include <memory>

struct Incomplete;

void test(std::allocator<Incomplete>& a) { (void)a.allocate(1); }
