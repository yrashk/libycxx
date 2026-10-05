// [container.alloc.reqmts]/5: typename X::allocator_type: "Mandates:
// allocator_type::value_type is the same as X::value_type." A map whose allocator's
// value_type differs from the map's value_type is ill-formed.
#include <map>
#include <memory>
#include <functional>
// COUNTERPART: libstdcxx:23_containers/map/requirements/explicit_instantiation/3.cc

std::map<int, double, std::less<int>, std::allocator<char>> c;

int main() {}
