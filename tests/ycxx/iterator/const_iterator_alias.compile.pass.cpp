// [const.iterators.alias]: iter_const_reference_t<It> is
// common_reference_t<const iter_value_t<It>&&, iter_reference_t<It>>; const_iterator<I> is I
// if I models constant-iterator, otherwise basic_const_iterator<I>; const_sentinel<S> is
// const_iterator<S> if S models input_iterator, otherwise S. [iterator.synopsis]:
// make_const_iterator / make_const_sentinel and the common_type specializations.
#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>

// An iterator whose reference is a prvalue (already a constant iterator).
struct Counting {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int n = 0;
  int operator*() const;
  Counting& operator++();
  Counting operator++(int);
  bool operator==(const Counting&) const = default;
};
static_assert(std::forward_iterator<Counting>);

// A zip-like iterator whose reference is a proxy tuple<int&>.
struct ZipLike {
  using value_type = std::tuple<int>;
  using difference_type = std::ptrdiff_t;
  int* p = nullptr;
  std::tuple<int&> operator*() const;
  ZipLike& operator++();
  void operator++(int);
};
static_assert(std::input_iterator<ZipLike>);

static_assert(std::is_same_v<std::iter_const_reference_t<int*>, const int&>);
static_assert(std::is_same_v<std::iter_const_reference_t<const int*>, const int&>);
static_assert(std::is_same_v<std::iter_const_reference_t<Counting>, int>);
static_assert(std::is_same_v<std::iter_const_reference_t<ZipLike>, std::tuple<const int&>>);
static_assert(std::is_same_v<std::iter_const_reference_t<std::basic_const_iterator<int*>>, const int&>);
static_assert(std::is_same_v<std::iter_const_reference_t<std::move_iterator<int*>>, const int&&>);

static_assert(std::is_same_v<std::const_iterator<int*>, std::basic_const_iterator<int*>>);
static_assert(std::is_same_v<std::const_iterator<const int*>, const int*>);
static_assert(std::is_same_v<std::const_iterator<Counting>, Counting>);
static_assert(std::is_same_v<std::const_iterator<ZipLike>, std::basic_const_iterator<ZipLike>>);
static_assert(std::is_same_v<std::const_iterator<std::basic_const_iterator<int*>>, std::basic_const_iterator<int*>>);
static_assert(std::is_same_v<std::const_iterator<std::move_iterator<int*>>, std::basic_const_iterator<std::move_iterator<int*>>>);
static_assert(std::is_same_v<std::const_iterator<std::move_iterator<const int*>>, std::move_iterator<const int*>>);
static_assert(std::is_same_v<std::const_iterator<std::counted_iterator<const int*>>, std::counted_iterator<const int*>>);

static_assert(std::is_same_v<std::const_sentinel<int*>, std::basic_const_iterator<int*>>);
static_assert(std::is_same_v<std::const_sentinel<const int*>, const int*>);
static_assert(std::is_same_v<std::const_sentinel<std::default_sentinel_t>, std::default_sentinel_t>);
static_assert(std::is_same_v<std::const_sentinel<std::unreachable_sentinel_t>, std::unreachable_sentinel_t>);

static_assert(std::is_same_v<decltype(std::make_const_iterator(std::declval<int*>())), std::basic_const_iterator<int*>>);
static_assert(std::is_same_v<decltype(std::make_const_iterator(std::declval<const int*>())), const int*>);
static_assert(std::is_same_v<decltype(std::make_const_sentinel(std::default_sentinel)), std::default_sentinel_t>);
static_assert(std::is_same_v<decltype(std::make_const_sentinel(std::declval<int*>())), std::basic_const_iterator<int*>>);

// common_type specializations.
static_assert(std::is_same_v<std::common_type_t<std::basic_const_iterator<int*>, const int*>,
                             std::basic_const_iterator<const int*>>);
static_assert(std::is_same_v<std::common_type_t<const int*, std::basic_const_iterator<int*>>,
                             std::basic_const_iterator<const int*>>);
static_assert(std::is_same_v<std::common_type_t<std::basic_const_iterator<int*>, std::basic_const_iterator<const int*>>,
                             std::basic_const_iterator<const int*>>);
