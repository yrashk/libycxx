// [move.iterator]/1: iterator_concept is random_access_iterator_tag / bidirectional /
// forward / input according to what Iterator models (never contiguous). /2: iterator_category
// "is declared if and only if the qualified-id iterator_traits<Iterator>::iterator_category is
// valid and denotes a type", and is random_access_iterator_tag when that category derives
// from it. pointer is Iterator; reference is iter_rvalue_reference_t<Iterator>. The default
// constructor "requires default_initializable<Iterator>". Converting construction requires
// !is_same_v<U, Iterator> and const U& models convertible_to<Iterator>.
#include <iterator>
#include <cstddef>
#include <type_traits>

struct InIt {  // C++20 input iterator without iterator_category
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int* p;
  InIt(int*);
  InIt(InIt&&);
  InIt& operator=(InIt&&);
  int& operator*() const;
  InIt& operator++();
  void operator++(int);
};
struct ProxyRef {  // prvalue reference
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::input_iterator_tag;
  int operator*() const;
  ProxyRef& operator++();
  ProxyRef operator++(int);
  bool operator==(const ProxyRef&) const;
};

template <class T>
concept has_category = requires { typename T::iterator_category; };

using M = std::move_iterator<int*>;
static_assert(std::is_same_v<M::iterator_type, int*>);
static_assert(std::is_same_v<M::iterator_concept, std::random_access_iterator_tag>);
static_assert(std::is_same_v<M::iterator_category, std::random_access_iterator_tag>);
static_assert(std::is_same_v<M::value_type, int>);
static_assert(std::is_same_v<M::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<M::pointer, int*>);
static_assert(std::is_same_v<M::reference, int&&>);
static_assert(std::random_access_iterator<M>);
static_assert(!std::contiguous_iterator<M>);

using MI = std::move_iterator<InIt>;
static_assert(std::is_same_v<MI::iterator_concept, std::input_iterator_tag>);
static_assert(!has_category<MI>);
static_assert(std::input_iterator<MI>);
static_assert(!std::is_default_constructible_v<MI>);
static_assert(std::is_default_constructible_v<M>);

using MP = std::move_iterator<ProxyRef>;
static_assert(std::is_same_v<MP::reference, int>);  // iter_rvalue_reference_t of a prvalue
static_assert(std::is_same_v<MP::iterator_category, std::input_iterator_tag>);

static_assert(std::is_convertible_v<std::move_iterator<int*>, std::move_iterator<const int*>>);
static_assert(!std::is_constructible_v<std::move_iterator<int*>, std::move_iterator<const int*>>);
static_assert(!std::is_convertible_v<int*, M>);  // explicit
