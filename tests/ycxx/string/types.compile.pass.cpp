// [basic.string.general]: member types of basic_string (size_type / difference_type / pointer
// come from allocator_traits<Allocator>), npos == size_type(-1), reverse iterator aliases;
// /2 a basic_string is a contiguous container, so iterator / const_iterator model
// contiguous_iterator ([container.reqmts]/68) and iterator converts to const_iterator
// ([container.reqmts]/6). [string.syn]: the typedef-names string, u8string, u16string,
// u32string, wstring; default template arguments char_traits<charT> and allocator<charT>.
#include <string>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include "test_allocators.hpp"

template <class S, class C, class Tr, class A>
constexpr bool check() {
  using AT = std::allocator_traits<A>;
  static_assert(std::is_same_v<typename S::traits_type, Tr>);
  static_assert(std::is_same_v<typename S::value_type, C>);
  static_assert(std::is_same_v<typename S::allocator_type, A>);
  static_assert(std::is_same_v<typename S::size_type, typename AT::size_type>);
  static_assert(std::is_same_v<typename S::difference_type, typename AT::difference_type>);
  static_assert(std::is_same_v<typename S::pointer, typename AT::pointer>);
  static_assert(std::is_same_v<typename S::const_pointer, typename AT::const_pointer>);
  static_assert(std::is_same_v<typename S::reference, C&>);
  static_assert(std::is_same_v<typename S::const_reference, const C&>);
  static_assert(std::contiguous_iterator<typename S::iterator>);
  static_assert(std::contiguous_iterator<typename S::const_iterator>);
  static_assert(std::is_same_v<std::iter_value_t<typename S::iterator>, C>);
  static_assert(std::is_same_v<std::iter_reference_t<typename S::iterator>, C&>);
  static_assert(std::is_same_v<std::iter_reference_t<typename S::const_iterator>, const C&>);
  static_assert(std::is_convertible_v<typename S::iterator, typename S::const_iterator>);
  static_assert(!std::is_convertible_v<typename S::const_iterator, typename S::iterator>);
  static_assert(std::is_same_v<typename S::reverse_iterator,
                               std::reverse_iterator<typename S::iterator>>);
  static_assert(std::is_same_v<typename S::const_reverse_iterator,
                               std::reverse_iterator<typename S::const_iterator>>);
  static_assert(std::is_same_v<decltype(S::npos), const typename S::size_type>);
  static_assert(S::npos == static_cast<typename S::size_type>(-1));
  static_assert(std::is_same_v<std::iter_difference_t<typename S::iterator>,
                               typename S::difference_type>);
  return true;
}

static_assert(check<std::string, char, std::char_traits<char>, std::allocator<char>>());
static_assert(check<std::wstring, wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>());
static_assert(check<std::u8string, char8_t, std::char_traits<char8_t>, std::allocator<char8_t>>());
static_assert(check<std::u16string, char16_t, std::char_traits<char16_t>, std::allocator<char16_t>>());
static_assert(check<std::u32string, char32_t, std::char_traits<char32_t>, std::allocator<char32_t>>());
static_assert(check<std::basic_string<char, std::char_traits<char>, MinimalAlloc<char>>, char,
                    std::char_traits<char>, MinimalAlloc<char>>());

static_assert(std::is_same_v<std::string, std::basic_string<char>>);
static_assert(std::is_same_v<std::string,
                             std::basic_string<char, std::char_traits<char>, std::allocator<char>>>);
static_assert(std::is_same_v<std::wstring, std::basic_string<wchar_t>>);
static_assert(std::is_same_v<std::u8string, std::basic_string<char8_t>>);
static_assert(std::is_same_v<std::u16string, std::basic_string<char16_t>>);
static_assert(std::is_same_v<std::u32string, std::basic_string<char32_t>>);

// Allocator whose size_type / difference_type differ from size_t / ptrdiff_t.
template <class T>
struct SmallSizeAlloc {
  using value_type = T;
  using size_type = unsigned short;
  using difference_type = short;
  SmallSizeAlloc() = default;
  template <class U>
  SmallSizeAlloc(const SmallSizeAlloc<U>&) {}
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  friend bool operator==(SmallSizeAlloc, SmallSizeAlloc) { return true; }
};
using SS = std::basic_string<char, std::char_traits<char>, SmallSizeAlloc<char>>;
static_assert(std::is_same_v<SS::size_type, unsigned short>);
static_assert(std::is_same_v<SS::difference_type, short>);
static_assert(SS::npos == 0xFFFF);
