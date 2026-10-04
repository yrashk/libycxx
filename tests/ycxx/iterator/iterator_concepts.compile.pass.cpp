// [iterator.concepts]: indirectly_readable, indirectly_writable, weakly_incrementable (allows
// move-only types; C++20 P1207), incrementable, input_or_output_iterator, sentinel_for,
// sized_sentinel_for (with disable_sized_sentinel_for), input_iterator (requires ITER_CONCEPT
// derived from input_iterator_tag), output_iterator, forward_iterator, bidirectional_iterator,
// random_access_iterator, contiguous_iterator (requires lvalue reference and to_address).
#include <iterator>
#include <cstddef>
#include <type_traits>

struct MoveOnlyIn {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  MoveOnlyIn(MoveOnlyIn&&) = default;
  MoveOnlyIn& operator=(MoveOnlyIn&&) = default;
  int operator*() const;
  MoveOnlyIn& operator++();
  void operator++(int);
};
struct NoDiff {
  using value_type = int;
  int operator*() const;
  NoDiff& operator++();
  void operator++(int);
};
struct OutOnly {
  using difference_type = std::ptrdiff_t;
  OutOnly& operator*();
  OutOnly& operator=(int);
  OutOnly& operator++();
  OutOnly& operator++(int);
};
// claims to be an output iterator via iterator_concept: not an input_iterator
struct TaggedOutput {
  using iterator_concept = std::output_iterator_tag;
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int& operator*() const;
  TaggedOutput& operator++();
  TaggedOutput operator++(int);
  bool operator==(const TaggedOutput&) const;
};
// random access by operations, but with a prvalue reference: not contiguous
struct ProxyRA {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  using iterator_concept = std::contiguous_iterator_tag;
  int* p;
  int operator*() const;
  int operator[](std::ptrdiff_t) const;
  ProxyRA& operator++();
  ProxyRA operator++(int);
  ProxyRA& operator--();
  ProxyRA operator--(int);
  ProxyRA& operator+=(std::ptrdiff_t);
  ProxyRA& operator-=(std::ptrdiff_t);
  friend ProxyRA operator+(ProxyRA, std::ptrdiff_t);
  friend ProxyRA operator+(std::ptrdiff_t, ProxyRA);
  friend ProxyRA operator-(ProxyRA, std::ptrdiff_t);
  friend std::ptrdiff_t operator-(ProxyRA, ProxyRA);
  friend auto operator<=>(const ProxyRA&, const ProxyRA&) = default;
};
struct NotSized {
  friend bool operator==(const NotSized&, int*);
};

static_assert(std::weakly_incrementable<MoveOnlyIn>);
static_assert(!std::incrementable<MoveOnlyIn>);
static_assert(std::input_iterator<MoveOnlyIn>);
static_assert(!std::forward_iterator<MoveOnlyIn>);
static_assert(!std::weakly_incrementable<NoDiff>);
static_assert(!std::input_or_output_iterator<NoDiff>);
static_assert(std::output_iterator<OutOnly, int>);
static_assert(!std::input_iterator<OutOnly>);
static_assert(std::indirectly_writable<OutOnly, int>);
static_assert(!std::input_iterator<TaggedOutput>);
static_assert(std::input_or_output_iterator<TaggedOutput>);
static_assert(std::random_access_iterator<ProxyRA>);
static_assert(!std::contiguous_iterator<ProxyRA>);
static_assert(std::contiguous_iterator<int*>);
static_assert(std::contiguous_iterator<const int*>);
static_assert(!std::contiguous_iterator<void*>);
static_assert(std::indirectly_readable<const int*>);
static_assert(std::indirectly_readable<int* const>);
static_assert(!std::indirectly_writable<const int*, int>);
static_assert(std::indirectly_writable<int*, long>);
static_assert(std::sentinel_for<int*, int*>);
static_assert(std::sentinel_for<NotSized, int*>);
static_assert(!std::sized_sentinel_for<NotSized, int*>);
static_assert(std::sized_sentinel_for<int*, int*>);
static_assert(std::sized_sentinel_for<const int*, int*>);
static_assert(std::incrementable<int>);
static_assert(!std::input_or_output_iterator<int>);
