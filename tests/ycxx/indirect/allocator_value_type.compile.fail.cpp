// EXPECT-ERROR: error: static assertion failed[^\n]*std::indirect: allocator_traits<Allocator>::value_type must be T
// [indirect.general]/2: "if the type allocator_traits<Allocator>::value_type is not the same
// type as T, the program is ill-formed."
#include <memory>

std::indirect<int, std::allocator<long>> x;
