// [list.syn]: namespace pmr { template<class T> using list = std::list<T,
// polymorphic_allocator<T>>; } is declared by <list> itself.
#include <list>
#include <type_traits>

static_assert(std::is_same_v<std::pmr::list<int>, std::list<int, std::pmr::polymorphic_allocator<int>>>);
static_assert(std::is_same_v<std::pmr::list<int>::allocator_type, std::pmr::polymorphic_allocator<int>>);

int main() { return 0; }
