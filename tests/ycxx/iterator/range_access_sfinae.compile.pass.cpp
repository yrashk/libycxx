// [iterator.range]: the container forms are declared with trailing return types
// (-> decltype(c.begin()) etc.), so they drop out of overload resolution when the member is
// missing, and their exception specifications are noexcept(noexcept(c.member())). The array
// forms are noexcept.
#include <iterator>
#include <cstddef>

struct NoMembers {};
struct ThrowingBegin {
  int* begin();  // potentially throwing
  int* end() noexcept;
  std::size_t size() const;
  bool empty() const noexcept;
  int* data() noexcept;
};

template <class T> concept has_begin = requires(T& t) { std::begin(t); };
template <class T> concept has_end = requires(T& t) { std::end(t); };
template <class T> concept has_cbegin = requires(T& t) { std::cbegin(t); };
template <class T> concept has_rbegin = requires(T& t) { std::rbegin(t); };
template <class T> concept has_size = requires(T& t) { std::size(t); };
template <class T> concept has_ssize = requires(T& t) { std::ssize(t); };
template <class T> concept has_empty = requires(T& t) { std::empty(t); };
template <class T> concept has_data = requires(T& t) { std::data(t); };

static_assert(!has_begin<NoMembers> && !has_end<NoMembers> && !has_cbegin<NoMembers>);
static_assert(!has_rbegin<NoMembers> && !has_size<NoMembers> && !has_ssize<NoMembers>);
static_assert(!has_empty<NoMembers> && !has_data<NoMembers>);
static_assert(!has_begin<int> && !has_size<int>);
static_assert(has_begin<int[2]> && has_size<int[2]> && has_ssize<int[2]> && has_data<int[2]>);
static_assert(has_empty<int[2]> && has_rbegin<int[2]>);
static_assert(!has_rbegin<ThrowingBegin>);  // no rbegin member and not an array

extern ThrowingBegin& tbr;
static_assert(!noexcept(std::begin(tbr)));
static_assert(noexcept(std::end(tbr)));
static_assert(!noexcept(std::size(tbr)));
static_assert(!noexcept(std::ssize(tbr)));
static_assert(noexcept(std::empty(tbr)));
static_assert(noexcept(std::data(tbr)));

extern int (&ar)[3];
static_assert(noexcept(std::begin(ar)) && noexcept(std::end(ar)));
static_assert(noexcept(std::cbegin(ar)) && noexcept(std::cend(ar)));
static_assert(noexcept(std::rbegin(ar)) && noexcept(std::rend(ar)));
static_assert(noexcept(std::size(ar)) && noexcept(std::ssize(ar)));
static_assert(noexcept(std::empty(ar)) && noexcept(std::data(ar)));
