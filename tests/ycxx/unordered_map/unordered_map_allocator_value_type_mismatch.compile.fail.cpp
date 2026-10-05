// [container.alloc.reqmts]/5: typename X::allocator_type: "Mandates:
// allocator_type::value_type is the same as X::value_type." A unordered_map whose allocator's
// value_type differs from the unordered_map's value_type is ill-formed.
#include <unordered_map>
#include <memory>
#include <functional>
#include <utility>
// COUNTERPART: libstdcxx:23_containers/unordered_map/requirements/explicit_instantiation/3.cc

std::unordered_map<int, double, std::hash<int>, std::equal_to<int>, std::allocator<std::pair<int, double>>> c;

int main() {}
