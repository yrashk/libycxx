// [syserr.errcat.objects]/1, /3: generic_category() and system_category() return references to
// objects of types derived from error_category; "All calls to this function shall return
// references to the same object."
// [syserr.errcat.objects]/2: generic_category()'s name() returns "generic", and its
// default_error_condition and equivalent behave as specified for error_category
// ([syserr.errcat.virtuals]/2-4).
// [syserr.errcat.objects]/4: system_category()'s name() returns "system"; its
// default_error_condition(ev) returns error_condition(0, generic_category()) for ev == 0,
// error_condition(pxv, generic_category()) if ev corresponds to POSIX errno value pxv, and
// otherwise error_condition(ev, system_category()); its equivalent functions behave as
// specified for error_category.
// [syserr.general]/2: "Components described in [syserr] do not change the value of errno."
// On a POSIX system (this test's platform), a system error value equal to a POSIX errno value
// denotes that errno value, so ENOENT etc. correspond to themselves.
#include <system_error>
#include <cerrno>
#include <cstring>
#include <string>
#include <type_traits>
#include "check.hpp"

static_assert(noexcept(std::generic_category()));
static_assert(noexcept(std::system_category()));
static_assert(std::is_same_v<decltype(std::generic_category()), const std::error_category&>);
static_assert(std::is_same_v<decltype(std::system_category()), const std::error_category&>);

int main() {
  const std::error_category& g = std::generic_category();
  const std::error_category& s = std::system_category();
  CHECK(&g == &std::generic_category());
  CHECK(&s == &std::system_category());
  CHECK(&g != &s);
  CHECK(g == std::generic_category());
  CHECK(g != s);
  CHECK((g <=> s) != 0);

  CHECK(std::strcmp(g.name(), "generic") == 0);
  CHECK(std::strcmp(s.name(), "system") == 0);

  // generic: default_error_condition(ev) == error_condition(ev, generic_category())
  for (int ev : {0, EINVAL, ENOENT, EDOM, 12345, -1}) {
    std::error_condition c = g.default_error_condition(ev);
    CHECK(c.value() == ev);
    CHECK(&c.category() == &g);
    CHECK(g.equivalent(ev, std::error_condition(ev, g)));
    CHECK(!g.equivalent(ev, std::error_condition(ev, s)));
    CHECK(g.equivalent(std::error_code(ev, g), ev));
    CHECK(!g.equivalent(std::error_code(ev, s), ev));
  }
  CHECK(!g.equivalent(EINVAL, std::error_condition(ENOENT, g)));

  // system: 0 maps to (0, generic); POSIX errno values map to (pxv, generic)
  {
    std::error_condition c = s.default_error_condition(0);
    CHECK(c.value() == 0);
    CHECK(&c.category() == &g);
  }
  for (int ev : {EPERM, ENOENT, EINTR, EIO, EACCES, EEXIST, EINVAL, ENOSPC, EPIPE, EDOM, ERANGE,
                 EAGAIN, ENOMEM, ENOTDIR, EISDIR, ETIMEDOUT, ECONNREFUSED, EOVERFLOW}) {
    std::error_condition c = s.default_error_condition(ev);
    CHECK(c.value() == ev);
    CHECK(&c.category() == &g);
    // equivalent(int, cond) is default_error_condition(code) == cond ([syserr.errcat.virtuals]/3)
    CHECK(s.equivalent(ev, std::error_condition(ev, g)));
    CHECK(!s.equivalent(ev, std::error_condition(ev, s)));
    // equivalent(code, int): same category and value ([syserr.errcat.virtuals]/4)
    CHECK(s.equivalent(std::error_code(ev, s), ev));
    CHECK(!s.equivalent(std::error_code(ev, g), ev));
    // and through the comparison operators ([syserr.compare]/2)
    CHECK(std::error_code(ev, s) == std::error_condition(ev, g));
    CHECK(std::error_code(ev, s).default_error_condition() == std::error_condition(ev, g));
  }
  CHECK(std::error_code(ENOENT, s) == std::errc::no_such_file_or_directory);
  CHECK(std::error_code(EINVAL, s) == std::errc::invalid_argument);
  CHECK(std::error_code(EINVAL, s) != std::errc::no_such_file_or_directory);
  CHECK(std::error_code(0, s) == std::error_condition());
  CHECK(std::error_code() == std::error_condition());

  // a value that is no errno value: either it corresponds to some errno value (unspecified),
  // or it maps to (ev, system_category())
  {
    int ev = 987654;
    std::error_condition c = s.default_error_condition(ev);
    CHECK(&c.category() == &g || &c.category() == &s);
    if (&c.category() == &s) CHECK(c.value() == ev);
  }

  // messages are strings describing the condition; neither the categories' functions nor the
  // error_code/condition observers change errno ([syserr.general]/2)
  errno = 4242;
  std::string m1 = g.message(ENOENT);
  std::string m2 = s.message(ENOENT);
  std::string m3 = g.message(987654);
  std::string m4 = s.message(-3);
  std::string m5 = std::error_code(EINVAL, s).message();
  std::string m6 = std::make_error_condition(std::errc::io_error).message();
  (void)s.default_error_condition(EIO);
  (void)s.default_error_condition(987654);
  CHECK(errno == 4242);
  CHECK(!m1.empty() && !m2.empty() && !m5.empty() && !m6.empty());
  (void)m3;
  (void)m4;
  CHECK(std::error_code(EINVAL, g).message() == g.message(EINVAL));
  CHECK(std::error_condition(EINVAL, s).message() == s.message(EINVAL));
  return 0;
}
