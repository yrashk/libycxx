// [container.alloc.reqmts]/5: typename X::allocator_type: "Mandates:
// allocator_type::value_type is the same as X::value_type." A list whose allocator's
// value_type differs from the list's value_type is ill-formed.
#include <list>
#include <memory>
// COUNTERPART: libstdcxx:23_containers/list/requirements/explicit_instantiation/3.cc

std::list<int, std::allocator<long>> c;

int main() {}
