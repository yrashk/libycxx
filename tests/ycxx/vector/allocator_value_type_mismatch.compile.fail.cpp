// [container.alloc.reqmts]/5: typename X::allocator_type: "Mandates:
// allocator_type::value_type is the same as X::value_type."
#include <vector>
#include <memory>

std::vector<int, std::allocator<long>> v;
