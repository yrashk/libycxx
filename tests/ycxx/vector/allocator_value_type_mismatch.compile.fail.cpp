// [container.alloc.reqmts]/5: typename X::allocator_type: "Mandates:
// allocator_type::value_type is the same as X::value_type."
// COUNTERPART: libstdcxx:23_containers/vector/requirements/explicit_instantiation/3.cc
#include <vector>
#include <memory>

std::vector<int, std::allocator<long>> v;
