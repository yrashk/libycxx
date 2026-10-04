// [reverse.iterator]/1: iterator_concept is random_access_iterator_tag if Iterator models
// random_access_iterator, otherwise bidirectional_iterator_tag. /2: iterator_category is
// random_access_iterator_tag if iterator_traits<Iterator>::iterator_category models
// derived_from<random_access_iterator_tag>, otherwise that category. pointer is
// iterator_traits<Iterator>::pointer, reference is iter_reference_t<Iterator>.
// [reverse.iter.elem]/2: operator-> requires (is_pointer_v<Iterator> || requires(const
// Iterator i) { i.operator->(); }). [reverse.iter.cons]: the converting constructor and
// assignment require !is_same_v<U, Iterator> and const U& models convertible_to<Iterator>.
// [iterator.synopsis]: disable_sized_sentinel_for<reverse_iterator<I1>, reverse_iterator<I2>>
// is true when !sized_sentinel_for<I1, I2>.
#include <iterator>
#include <cstddef>
#include <type_traits>

// bidirectional iterator with no operator->
struct Bidi {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::bidirectional_iterator_tag;
  int* p;
  int& operator*() const;
  Bidi& operator++();
  Bidi operator++(int);
  Bidi& operator--();
  Bidi operator--(int);
  bool operator==(const Bidi&) const;
};
static_assert(std::bidirectional_iterator<Bidi>);

// bidirectional iterator that has operator- but opts out of sized_sentinel_for
struct BidiDist {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int* p;
  int& operator*() const;
  BidiDist& operator++();
  BidiDist operator++(int);
  BidiDist& operator--();
  BidiDist operator--(int);
  bool operator==(const BidiDist&) const;
  friend std::ptrdiff_t operator-(const BidiDist&, const BidiDist&);
};
template <>
inline constexpr bool std::disable_sized_sentinel_for<BidiDist, BidiDist> = true;
static_assert(std::bidirectional_iterator<BidiDist>);
static_assert(!std::sized_sentinel_for<BidiDist, BidiDist>);

using RP = std::reverse_iterator<int*>;
static_assert(std::is_same_v<RP::iterator_type, int*>);
static_assert(std::is_same_v<RP::iterator_concept, std::random_access_iterator_tag>);  // not contiguous
static_assert(std::is_same_v<RP::iterator_category, std::random_access_iterator_tag>);
static_assert(std::is_same_v<RP::value_type, int>);
static_assert(std::is_same_v<RP::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<RP::pointer, int*>);
static_assert(std::is_same_v<RP::reference, int&>);
static_assert(std::random_access_iterator<RP>);
static_assert(!std::contiguous_iterator<RP>);

using RB = std::reverse_iterator<Bidi>;
static_assert(std::is_same_v<RB::iterator_concept, std::bidirectional_iterator_tag>);
static_assert(std::is_same_v<RB::iterator_category, std::bidirectional_iterator_tag>);
static_assert(std::is_same_v<RB::pointer, void>);
static_assert(std::bidirectional_iterator<RB>);
static_assert(!std::random_access_iterator<RB>);

template <class T>
concept has_arrow = requires(const T& t) { t.operator->(); };
static_assert(has_arrow<RP>);
static_assert(!has_arrow<RB>);

// converting constructor / assignment
static_assert(std::is_convertible_v<std::reverse_iterator<int*>, std::reverse_iterator<const int*>>);
static_assert(!std::is_constructible_v<std::reverse_iterator<int*>, std::reverse_iterator<const int*>>);
static_assert(std::is_assignable_v<std::reverse_iterator<const int*>&, std::reverse_iterator<int*>>);
static_assert(!std::is_assignable_v<std::reverse_iterator<int*>&, std::reverse_iterator<const int*>>);
static_assert(!std::is_convertible_v<int*, RP>);  // explicit
static_assert(std::is_constructible_v<RP, int*>);

// sized sentinels follow the underlying iterators
static_assert(std::sized_sentinel_for<RP, RP>);
static_assert(std::sized_sentinel_for<std::reverse_iterator<const int*>, RP>);
// reverse_iterator<BidiDist> has a well-formed operator-, but the library's
// disable_sized_sentinel_for specialization keeps it from being a sized sentinel
static_assert(requires(std::reverse_iterator<BidiDist> a) { a - a; });
static_assert(!std::sized_sentinel_for<std::reverse_iterator<BidiDist>, std::reverse_iterator<BidiDist>>);
