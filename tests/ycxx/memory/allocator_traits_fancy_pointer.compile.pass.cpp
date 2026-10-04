// [allocator.traits.types]/1-6 for an allocator with a class-type pointer: pointer is
// Alloc::pointer; const_pointer, void_pointer and const_void_pointer default to
// pointer_traits<pointer>::rebind<const value_type>, rebind<void> and rebind<const void>;
// difference_type defaults to pointer_traits<pointer>::difference_type; size_type defaults
// to make_unsigned_t<difference_type>. Members the allocator declares itself win.
// [pointer.traits.types]: rebind of a SomePointer<T, Args...> template without a rebind
// member is SomePointer<U, Args...>; difference_type is Ptr::difference_type when present.
#include <memory>
#include <cstddef>
#include <type_traits>

template <class T, class Tag = void>
struct Fancy {
  using element_type = T;
  using difference_type = int;
  T* p;
};

template <class T>
struct FancyAlloc {
  using value_type = T;
  using pointer = Fancy<T, char>;
  Fancy<T, char> allocate(std::size_t);
  void deallocate(Fancy<T, char>, std::size_t);
};

using TF = std::allocator_traits<FancyAlloc<long>>;
static_assert(std::is_same_v<TF::pointer, Fancy<long, char>>);
static_assert(std::is_same_v<TF::const_pointer, Fancy<const long, char>>);
static_assert(std::is_same_v<TF::void_pointer, Fancy<void, char>>);
static_assert(std::is_same_v<TF::const_void_pointer, Fancy<const void, char>>);
static_assert(std::is_same_v<TF::difference_type, int>);
static_assert(std::is_same_v<TF::size_type, unsigned int>);
static_assert(std::is_same_v<TF::rebind_alloc<short>, FancyAlloc<short>>);
static_assert(std::is_same_v<TF::rebind_traits<short>::pointer, Fancy<short, char>>);

// an allocator that names some of the types itself
template <class T>
struct PartlyNamed {
  using value_type = T;
  using pointer = Fancy<T>;
  using const_pointer = const T*;
  using difference_type = long long;
  Fancy<T> allocate(std::size_t);
  void deallocate(Fancy<T>, std::size_t);
};
using TP = std::allocator_traits<PartlyNamed<int>>;
static_assert(std::is_same_v<TP::const_pointer, const int*>);
static_assert(std::is_same_v<TP::void_pointer, Fancy<void>>);
static_assert(std::is_same_v<TP::difference_type, long long>);
static_assert(std::is_same_v<TP::size_type, unsigned long long>);

// pointer_traits of the fancy pointer
using PT = std::pointer_traits<Fancy<long, char>>;
static_assert(std::is_same_v<PT::element_type, long> && std::is_same_v<PT::difference_type, int>);
static_assert(std::is_same_v<PT::rebind<double>, Fancy<double, char>>);

int main() { return 0; }
