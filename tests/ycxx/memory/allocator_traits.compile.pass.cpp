// [allocator.traits.types]: defaults of allocator_traits member types; rebind_alloc is
// Alloc::rebind<T>::other if valid, otherwise Alloc<T, Args> for Alloc = SomeAlloc<U, Args>;
// is_always_equal defaults to is_empty<Alloc>::type; propagate_* default to false_type.
// [allocator.traits.members]: max_size defaults to numeric_limits<size_type>::max() /
// sizeof(value_type); select_on_container_copy_construction returns rhs when the allocator
// has no such member.
#include <memory>
#include <cstddef>
#include <limits>
#include <type_traits>

template <class T, class Extra = int>
struct Minimal {
  using value_type = T;
  Minimal() = default;
  template <class U>
  Minimal(const Minimal<U, Extra>&) {}
  T* allocate(std::size_t);
  void deallocate(T*, std::size_t);
};
template <class T>
struct Stateful {
  using value_type = T;
  int id = 0;
  T* allocate(std::size_t);
  void deallocate(T*, std::size_t);
};
template <class T, class Tag = int>
struct CustomT {
  using value_type = T;
  using size_type = unsigned short;
  using difference_type = short;
  using propagate_on_container_swap = std::true_type;
  using is_always_equal = std::false_type;
  // a member rebind that differs from the SomeAlloc<U, Args...> default for U != T
  template <class U>
  struct rebind {
    using other = std::conditional_t<std::is_same_v<U, T>, CustomT, CustomT<U, char>>;
  };
  CustomT() = default;
  template <class U, class G>
  CustomT(const CustomT<U, G>&) {}
  CustomT select_on_container_copy_construction() const;
  size_type max_size() const;
  T* allocate(std::size_t);
  void deallocate(T*, std::size_t);
};
using Custom = CustomT<int>;

using TM = std::allocator_traits<Minimal<int>>;
static_assert(std::is_same_v<TM::allocator_type, Minimal<int>>);
static_assert(std::is_same_v<TM::value_type, int>);
static_assert(std::is_same_v<TM::pointer, int*>);
static_assert(std::is_same_v<TM::const_pointer, const int*>);
static_assert(std::is_same_v<TM::void_pointer, void*>);
static_assert(std::is_same_v<TM::const_void_pointer, const void*>);
static_assert(std::is_same_v<TM::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<TM::size_type, std::size_t>);
static_assert(std::is_same_v<TM::propagate_on_container_copy_assignment, std::false_type>);
static_assert(std::is_same_v<TM::propagate_on_container_move_assignment, std::false_type>);
static_assert(std::is_same_v<TM::propagate_on_container_swap, std::false_type>);
static_assert(std::is_same_v<TM::is_always_equal, std::true_type>);  // is_empty
static_assert(std::is_same_v<TM::rebind_alloc<long>, Minimal<long, int>>);
static_assert(std::is_same_v<TM::rebind_traits<long>, std::allocator_traits<Minimal<long, int>>>);
static_assert(std::is_same_v<std::allocator_traits<Stateful<int>>::is_always_equal, std::false_type>);

using TC = std::allocator_traits<Custom>;
static_assert(std::is_same_v<TC::pointer, int*>);
static_assert(std::is_same_v<TC::size_type, unsigned short>);
static_assert(std::is_same_v<TC::difference_type, short>);
static_assert(std::is_same_v<TC::propagate_on_container_swap, std::true_type>);
static_assert(std::is_same_v<TC::is_always_equal, std::false_type>);
static_assert(std::is_same_v<TC::rebind_alloc<double>, CustomT<double, char>>);
static_assert(std::is_same_v<TC::rebind_alloc<int>, Custom>);
static_assert(std::is_same_v<decltype(TC::select_on_container_copy_construction(std::declval<const Custom&>())), Custom>);

static_assert(std::is_same_v<decltype(TM::select_on_container_copy_construction(std::declval<const Minimal<int>&>())),
                             Minimal<int>>);
static_assert(std::is_same_v<decltype(TM::max_size(std::declval<const Minimal<int>&>())), std::size_t>);
static_assert(noexcept(TM::max_size(std::declval<const Minimal<int>&>())));

using TA = std::allocator_traits<std::allocator<int>>;
static_assert(std::is_same_v<TA::is_always_equal, std::true_type>);
static_assert(std::is_same_v<TA::propagate_on_container_move_assignment, std::true_type>);
static_assert(std::is_same_v<TA::rebind_alloc<char>, std::allocator<char>>);
static_assert(std::is_same_v<TA::size_type, std::size_t>);

// max_size default value (constexpr through a constant-evaluated call)
constexpr bool max_size_default() {
  std::allocator<int> a;
  return TA::max_size(a) == std::numeric_limits<std::size_t>::max() / sizeof(int);
}
static_assert(max_size_default());
