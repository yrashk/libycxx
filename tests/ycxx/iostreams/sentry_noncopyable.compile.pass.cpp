// [istream.sentry], [ostream.sentry] synopses: "sentry(const sentry&) = delete;
// sentry& operator=(const sentry&) = delete;" and "explicit operator bool() const".
#include <istream>
#include <ostream>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<std::istream::sentry>);
static_assert(!std::is_copy_assignable_v<std::istream::sentry>);
static_assert(!std::is_copy_constructible_v<std::ostream::sentry>);
static_assert(!std::is_copy_assignable_v<std::ostream::sentry>);
static_assert(!std::is_copy_constructible_v<std::wistream::sentry>);
static_assert(!std::is_copy_constructible_v<std::wostream::sentry>);
static_assert(!std::is_convertible_v<std::istream::sentry, bool>);
static_assert(!std::is_convertible_v<std::ostream::sentry, bool>);
static_assert(std::is_constructible_v<bool, std::istream::sentry&>);
static_assert(std::is_constructible_v<bool, const std::ostream::sentry&>);
// the constructors are explicit
static_assert(!std::is_convertible_v<std::istream&, std::istream::sentry>);
static_assert(!std::is_convertible_v<std::ostream&, std::ostream::sentry>);
static_assert(std::is_constructible_v<std::istream::sentry, std::istream&, bool>);

int main() {}
