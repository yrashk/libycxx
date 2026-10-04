// [vector.syn]: namespace pmr { template<class T> using vector = std::vector<T,
// polymorphic_allocator<T>>; }. <vector> alone must make the alias nameable, also for bool.
#include <vector>
#include <type_traits>

static_assert(std::is_same_v<std::pmr::vector<int>, std::vector<int, std::pmr::polymorphic_allocator<int>>>);
static_assert(std::is_same_v<std::pmr::vector<bool>, std::vector<bool, std::pmr::polymorphic_allocator<bool>>>);
static_assert(std::is_same_v<std::pmr::vector<int>::allocator_type, std::pmr::polymorphic_allocator<int>>);

int main() {}
