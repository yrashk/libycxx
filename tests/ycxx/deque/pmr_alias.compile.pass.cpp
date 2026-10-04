// [deque.syn]: namespace pmr { template<class T> using deque = std::deque<T,
// polymorphic_allocator<T>>; } is declared by <deque> itself.
#include <deque>
#include <type_traits>

static_assert(std::is_same_v<std::pmr::deque<int>, std::deque<int, std::pmr::polymorphic_allocator<int>>>);
static_assert(std::is_same_v<std::pmr::deque<int>::allocator_type, std::pmr::polymorphic_allocator<int>>);

int main() { return 0; }
