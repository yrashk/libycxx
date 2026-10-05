// [container.alloc.reqmts]/5: typename X::allocator_type: "Mandates:
// allocator_type::value_type is the same as X::value_type." A set whose allocator's
// value_type differs from the set's value_type is ill-formed.
#include <set>
#include <memory>
#include <functional>
// COUNTERPART: libstdcxx:23_containers/set/requirements/explicit_instantiation/3.cc

std::set<int, std::less<int>, std::allocator<long>> c;

int main() {}
