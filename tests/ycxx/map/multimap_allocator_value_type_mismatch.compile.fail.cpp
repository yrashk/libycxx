// [container.alloc.reqmts]/5: typename X::allocator_type: "Mandates:
// allocator_type::value_type is the same as X::value_type." A multimap whose allocator's
// value_type differs from the multimap's value_type is ill-formed.
#include <map>
#include <memory>
#include <functional>
#include <utility>
// COUNTERPART: libstdcxx:23_containers/multimap/requirements/explicit_instantiation/3.cc

std::multimap<int, double, std::less<int>, std::allocator<std::pair<int, double>>> c;

int main() {}
