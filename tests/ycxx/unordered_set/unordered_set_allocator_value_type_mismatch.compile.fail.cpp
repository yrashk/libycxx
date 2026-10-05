// [container.alloc.reqmts]/5: typename X::allocator_type: "Mandates:
// allocator_type::value_type is the same as X::value_type." A unordered_set whose allocator's
// value_type differs from the unordered_set's value_type is ill-formed.
#include <unordered_set>
#include <memory>
#include <functional>
// COUNTERPART: libstdcxx:23_containers/unordered_set/requirements/explicit_instantiation/3.cc

std::unordered_set<int, std::hash<int>, std::equal_to<int>, std::allocator<long>> c;

int main() {}
