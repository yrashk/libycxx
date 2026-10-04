// [basic.string.general]: the basic_string and charT forms of the search functions are
// noexcept; [string.find]/4: for the template<class T> forms "The exception specification is
// equivalent to is_nothrow_convertible_v<const T&, basic_string_view<charT, traits>>".
// [string.compare]/3: likewise for compare(const T&). compare(const basic_string&) is
// noexcept; starts_with / ends_with / contains with a string_view or charT are noexcept.
#include <string>
#include <string_view>
#include <utility>

struct ThrowingView {
  operator std::string_view() const noexcept(false);
};
struct NothrowView {
  operator std::string_view() const noexcept;
};

extern const std::string& s;

static_assert(noexcept(s.find(s)));
static_assert(noexcept(s.find('c')));
static_assert(noexcept(s.rfind(s)));
static_assert(noexcept(s.rfind('c')));
static_assert(noexcept(s.find_first_of(s)));
static_assert(noexcept(s.find_first_of('c')));
static_assert(noexcept(s.find_last_of(s)));
static_assert(noexcept(s.find_last_of('c')));
static_assert(noexcept(s.find_first_not_of(s)));
static_assert(noexcept(s.find_first_not_of('c')));
static_assert(noexcept(s.find_last_not_of(s)));
static_assert(noexcept(s.find_last_not_of('c')));
static_assert(noexcept(s.compare(s)));

static_assert(noexcept(s.find(std::declval<std::string_view>())));
static_assert(noexcept(s.find(std::declval<NothrowView>())));
static_assert(!noexcept(s.find(std::declval<ThrowingView>())));
static_assert(!noexcept(s.rfind(std::declval<ThrowingView>())));
static_assert(!noexcept(s.find_first_of(std::declval<ThrowingView>())));
static_assert(!noexcept(s.find_last_of(std::declval<ThrowingView>())));
static_assert(!noexcept(s.find_first_not_of(std::declval<ThrowingView>())));
static_assert(!noexcept(s.find_last_not_of(std::declval<ThrowingView>())));
static_assert(noexcept(s.rfind(std::declval<NothrowView>())));
static_assert(noexcept(s.find_last_not_of(std::declval<NothrowView>())));
static_assert(noexcept(s.compare(std::declval<std::string_view>())));
static_assert(noexcept(s.compare(std::declval<NothrowView>())));
static_assert(!noexcept(s.compare(std::declval<ThrowingView>())));

static_assert(noexcept(s.starts_with(std::string_view())));
static_assert(noexcept(s.starts_with('c')));
static_assert(noexcept(s.ends_with(std::string_view())));
static_assert(noexcept(s.ends_with('c')));
static_assert(noexcept(s.contains(std::string_view())));
static_assert(noexcept(s.contains('c')));
