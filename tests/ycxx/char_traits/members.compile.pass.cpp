// [char.traits.specializations.char] (and the four other specializations, which have the
// same member list): signatures, constexpr-ness and noexcept-ness of the members.
//   static constexpr void assign(char_type& c1, const char_type& c2) noexcept;
//   static constexpr bool eq(char_type c1, char_type c2) noexcept;
//   static constexpr bool lt(char_type c1, char_type c2) noexcept;
//   static constexpr int compare(const char_type* s1, const char_type* s2, size_t n);
//   static constexpr size_t length(const char_type* s);
//   static constexpr const char_type* find(const char_type* s, size_t n, const char_type& a);
//   static constexpr char_type* move(char_type* s1, const char_type* s2, size_t n);
//   static constexpr char_type* copy(char_type* s1, const char_type* s2, size_t n);
//   static constexpr char_type* assign(char_type* s, size_t n, char_type a);
//   static constexpr int_type not_eof(int_type c) noexcept;
//   static constexpr char_type to_char_type(int_type c) noexcept;
//   static constexpr int_type to_int_type(char_type c) noexcept;
//   static constexpr bool eq_int_type(int_type c1, int_type c2) noexcept;
//   static constexpr int_type eof() noexcept;
#include <string_view>
#include <cstddef>
#include <type_traits>
#include <utility>

template <class C>
constexpr bool check() {
  using T = std::char_traits<C>;
  using I = typename T::int_type;
  C c{};
  C* s = nullptr;
  const C* p = nullptr;
  I e{};
  // [member.functions]/2 lets the implementation declare different signatures, so the
  // checks below use call expressions only.
  static_assert(std::is_same_v<decltype(T::assign(c, c)), void>);
  static_assert(std::is_same_v<decltype(T::eq(c, c)), bool>);
  static_assert(std::is_same_v<decltype(T::lt(c, c)), bool>);
  static_assert(std::is_same_v<decltype(T::compare(p, p, 0)), int>);
  static_assert(std::is_same_v<decltype(T::length(p)), std::size_t>);
  static_assert(std::is_same_v<decltype(T::copy(s, p, 0)), C*>);
  static_assert(std::is_same_v<decltype(T::not_eof(e)), I>);
  static_assert(std::is_same_v<decltype(T::to_char_type(e)), C>);
  static_assert(std::is_same_v<decltype(T::to_int_type(c)), I>);
  static_assert(std::is_same_v<decltype(T::eq_int_type(e, e)), bool>);
  static_assert(std::is_same_v<decltype(T::eof()), I>);
  static_assert(std::is_same_v<decltype(T::find(p, 0, c)), const C*>);
  static_assert(std::is_same_v<decltype(T::move(s, p, 0)), C*>);
  static_assert(std::is_same_v<decltype(T::assign(s, 0, c)), C*>);
  static_assert(noexcept(T::eq(c, c)) && noexcept(T::lt(c, c)) && noexcept(T::assign(c, c)));
  static_assert(noexcept(T::not_eof(e)) && noexcept(T::to_char_type(e)));
  static_assert(noexcept(T::to_int_type(c)) && noexcept(T::eq_int_type(e, e)) && noexcept(T::eof()));
  return true;
}

static_assert(check<char>());
static_assert(check<wchar_t>());
static_assert(check<char8_t>());
static_assert(check<char16_t>());
static_assert(check<char32_t>());
