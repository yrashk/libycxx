// EXPECT-ERROR: error: static assertion failed[^\n]*std::forward_list: Allocator::value_type must be T \(\[container\.alloc\.reqmts\]\)
// [container.alloc.reqmts]/5: typename X::allocator_type: "Mandates:
// allocator_type::value_type is the same as X::value_type." A forward_list whose allocator's
// value_type differs from the forward_list's value_type is ill-formed.
#include <forward_list>
#include <memory>
// COUNTERPART: libstdcxx:23_containers/forward_list/requirements/explicit_instantiation/3.cc

std::forward_list<int, std::allocator<long>> c;

int main() {}
