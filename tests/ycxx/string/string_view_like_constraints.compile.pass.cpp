// The "string-view-like" overloads of basic_string take a template parameter T constrained
// on is_convertible_v<const T&, basic_string_view<charT, traits>> && !is_convertible_v<const
// T&, const charT*> ([string.cons]/11,32, [string.op.append]/2, [string.append]/3,5,
// [string.assign]/4,6, [string.insert]/3,5, [string.replace]/3,5,16, [string.find]/2,
// [string.compare]/1,4,6). A type convertible to both therefore resolves to the const charT*
// overloads, and a type convertible to neither is rejected.
#include <string>
#include <string_view>
#include <type_traits>

struct ViewOnly {
  operator std::string_view() const;
};
struct Both {
  operator std::string_view() const;
  operator const char*() const;
};
struct Neither {};

template <class T>
concept all_ops = requires(std::string& s, const std::string& cs, const T& t) {
  s = t;
  s += t;
  s.append(t);
  s.append(t, 0, 1);
  s.assign(t);
  s.assign(t, 0, 1);
  s.insert(0, t);
  s.insert(0, t, 0, 1);
  s.replace(0, 1, t);
  s.replace(0, 1, t, 0, 1);
  s.replace(s.cbegin(), s.cend(), t);
  cs.find(t);
  cs.rfind(t);
  cs.find_first_of(t);
  cs.find_last_of(t);
  cs.find_first_not_of(t);
  cs.find_last_not_of(t);
  cs.compare(t);
  cs.compare(0, 1, t);
  cs.compare(0, 1, t, 0, 1);
};

static_assert(all_ops<ViewOnly>);
static_assert(all_ops<std::string_view>);
static_assert(all_ops<std::string>);
static_assert(all_ops<const char*>);
static_assert(!all_ops<Neither>);

// For Both, every one of these expressions is still valid, through the const char*
// overloads (or the string_view parameter of starts_with etc.).
static_assert(requires(std::string& s, const Both& b) {
  s = b;
  s += b;
  s.append(b);
  s.assign(b);
  s.insert(0, b);
  s.replace(0, 1, b);
  s.find(b);
  s.compare(b);
});

// The two-argument (t, pos, n) forms whose only non-template competitors take a
// basic_string are reached for Both through the template only if it is viable; for the
// explicit constructor the template is excluded and the const charT* constructor is used.
static_assert(std::is_constructible_v<std::string, const Both&>);
// (Both -> const char* -> string would need two user-defined conversions, so no implicit
// conversion exists.)
static_assert(!std::is_convertible_v<const Both&, std::string>);

// Return types.
static_assert(std::is_same_v<decltype(std::declval<std::string&>().append(std::declval<ViewOnly>())),
                             std::string&>);
static_assert(std::is_same_v<decltype(std::declval<const std::string&>().find(std::declval<ViewOnly>())),
                             std::string::size_type>);
static_assert(std::is_same_v<decltype(std::declval<const std::string&>().compare(std::declval<ViewOnly>())),
                             int>);
