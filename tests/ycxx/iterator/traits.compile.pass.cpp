// [iterator.traits]/5: iterator_traits<T*> for object T has iterator_concept =
// contiguous_iterator_tag, iterator_category = random_access_iterator_tag, value_type =
// remove_cv_t<T>, reference = T&, pointer = T*. [incrementable.traits]: difference_type from
// I::difference_type, or the type of a - b for integral results. [readable.traits]:
// value_type from value_type / element_type members (remove_cv); if both exist and differ
// after remove_cv, there is no value_type; arrays decay. [iterator.assoc.types]:
// iter_value_t, iter_reference_t, iter_difference_t, iter_rvalue_reference_t,
// iter_common_reference_t; iterator_traits for a C++20 iterator.
#include <iterator>
#include <cstddef>
#include <type_traits>

using TP = std::iterator_traits<const int*>;
static_assert(std::is_same_v<TP::iterator_concept, std::contiguous_iterator_tag>);
static_assert(std::is_same_v<TP::iterator_category, std::random_access_iterator_tag>);
static_assert(std::is_same_v<TP::value_type, int>);
static_assert(std::is_same_v<TP::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<TP::pointer, const int*>);
static_assert(std::is_same_v<TP::reference, const int&>);
static_assert(std::is_same_v<std::iterator_traits<volatile int*>::value_type, int>);

struct WithDiff {
  using difference_type = short;
};
struct Subtractable {
  friend long operator-(Subtractable, Subtractable);
};
struct SubtractableNonIntegral {
  friend double operator-(SubtractableNonIntegral, SubtractableNonIntegral);
};
template <class T>
concept has_difference = requires { typename std::incrementable_traits<T>::difference_type; };
static_assert(std::is_same_v<std::incrementable_traits<WithDiff>::difference_type, short>);
static_assert(std::is_same_v<std::incrementable_traits<Subtractable>::difference_type, long>);
static_assert(!has_difference<SubtractableNonIntegral>);
static_assert(std::is_same_v<std::incrementable_traits<int*>::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<std::incrementable_traits<const WithDiff>::difference_type, short>);
static_assert(!has_difference<void*>);  // pointers to non-object types

struct ValueType {
  using value_type = const long;
};
struct ElementType {
  using element_type = volatile char;
};
struct BothSame {
  using value_type = int;
  using element_type = const int;
};
struct BothDiffer {
  using value_type = int;
  using element_type = long;
};
template <class T>
concept has_value = requires { typename std::indirectly_readable_traits<T>::value_type; };
static_assert(std::is_same_v<std::indirectly_readable_traits<ValueType>::value_type, long>);
static_assert(std::is_same_v<std::indirectly_readable_traits<ElementType>::value_type, char>);
static_assert(std::is_same_v<std::indirectly_readable_traits<BothSame>::value_type, int>);
static_assert(!has_value<BothDiffer>);
static_assert(std::is_same_v<std::indirectly_readable_traits<int[3]>::value_type, int>);
static_assert(std::is_same_v<std::indirectly_readable_traits<const int[3]>::value_type, int>);
static_assert(!has_value<void*>);

static_assert(std::is_same_v<std::iter_value_t<int* const>, int>);
static_assert(std::is_same_v<std::iter_reference_t<const int*>, const int&>);
static_assert(std::is_same_v<std::iter_difference_t<long*>, std::ptrdiff_t>);
static_assert(std::is_same_v<std::iter_rvalue_reference_t<int*>, int&&>);
static_assert(std::is_same_v<std::iter_common_reference_t<int*>, int&>);

// a C++20 forward iterator with no iterator_category gets one from iterator_traits
struct Cxx20Fwd {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int* p;
  int& operator*() const;
  Cxx20Fwd& operator++();
  Cxx20Fwd operator++(int);
  bool operator==(const Cxx20Fwd&) const;
};
static_assert(std::forward_iterator<Cxx20Fwd>);
static_assert(std::is_same_v<std::iterator_traits<Cxx20Fwd>::iterator_category, std::forward_iterator_tag>);
static_assert(std::is_same_v<std::iterator_traits<Cxx20Fwd>::reference, int&>);
static_assert(std::is_same_v<std::iterator_traits<Cxx20Fwd>::pointer, void>);
