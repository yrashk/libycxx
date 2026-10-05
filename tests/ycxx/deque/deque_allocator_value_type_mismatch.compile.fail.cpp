// [container.alloc.reqmts]/5: typename X::allocator_type: "Mandates:
// allocator_type::value_type is the same as X::value_type." A deque whose allocator's
// value_type differs from the deque's value_type is ill-formed.
#include <deque>
#include <memory>
// COUNTERPART: libstdcxx:23_containers/deque/requirements/explicit_instantiation/3.cc

std::deque<int, std::allocator<long>> c;

int main() {}
