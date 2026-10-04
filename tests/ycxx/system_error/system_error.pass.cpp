// [syserr.syserr.overview]: system_error derives publicly from runtime_error and has six
// constructors, code() const noexcept and what() const noexcept.
// [syserr.syserr.members]/1-6: the postconditions code() == ec (or error_code(ev, ecat)) and,
// for the constructors taking what_arg, string_view(what()).find(what_arg) != npos.
// [syserr.syserr.members]/7: code() returns ec or error_code(ev, ecat).
// [syserr.syserr.members]/8: what() returns an ntbs incorporating the constructor arguments.
// [exception]/2: copy constructor and copy assignment are noexcept, and a copy has the same
// what() (strcmp == 0).
// [syserr.general]/2: [syserr] components do not change errno.
#include <system_error>
#include <cerrno>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include "check.hpp"

struct Cat : std::error_category {
  const char* name() const noexcept override { return "mycat"; }
  std::string message(int ev) const override { return "mycat message " + std::to_string(ev); }
};
const Cat cat;

static_assert(std::is_base_of_v<std::runtime_error, std::system_error>);
static_assert(std::is_convertible_v<std::system_error*, std::runtime_error*>);  // public
static_assert(std::is_convertible_v<std::system_error*, std::exception*>);
static_assert(std::is_nothrow_copy_constructible_v<std::system_error>);
static_assert(std::is_nothrow_copy_assignable_v<std::system_error>);
static_assert(std::is_constructible_v<std::system_error, std::error_code, const std::string&>);
static_assert(std::is_constructible_v<std::system_error, std::error_code, const char*>);
static_assert(std::is_constructible_v<std::system_error, std::error_code>);
static_assert(std::is_constructible_v<std::system_error, int, const std::error_category&, const std::string&>);
static_assert(std::is_constructible_v<std::system_error, int, const std::error_category&, const char*>);
static_assert(std::is_constructible_v<std::system_error, int, const std::error_category&>);
static_assert(std::is_same_v<decltype(std::declval<const std::system_error&>().code()), const std::error_code&>);
static_assert(noexcept(std::declval<const std::system_error&>().code()));
static_assert(noexcept(std::declval<const std::system_error&>().what()));
static_assert(std::is_same_v<decltype(std::declval<const std::system_error&>().what()), const char*>);

bool contains(const char* what, std::string_view needle) {
  return what != nullptr && std::string_view(what).find(needle) != std::string_view::npos;
}

int main() {
  const std::error_code ec(5, cat);
  const std::string arg = "while frobnicating the widget";

  {  // (ec, const string&)
    std::system_error e(ec, arg);
    CHECK(e.code() == ec);
    CHECK(&e.code().category() == &cat && e.code().value() == 5);
    CHECK(contains(e.what(), arg));
  }
  {  // (ec, const char*)
    std::system_error e(ec, "opening /etc/frob");
    CHECK(e.code() == ec);
    CHECK(contains(e.what(), "opening /etc/frob"));
  }
  {  // (ec)
    std::system_error e(ec);
    CHECK(e.code() == ec);
    CHECK(e.what() != nullptr);
  }
  {  // (ev, ecat, const string&)
    std::system_error e(EACCES, std::generic_category(), arg);
    CHECK(e.code() == std::error_code(EACCES, std::generic_category()));
    CHECK(e.code() == std::errc::permission_denied);
    CHECK(contains(e.what(), arg));
  }
  {  // (ev, ecat, const char*)
    std::system_error e(ENOENT, std::system_category(), "stat");
    CHECK(e.code() == std::error_code(ENOENT, std::system_category()));
    CHECK(contains(e.what(), "stat"));
  }
  {  // (ev, ecat)
    std::system_error e(9, cat);
    CHECK(e.code().value() == 9 && &e.code().category() == &cat);
    CHECK(e.what() != nullptr);
  }
  {  // an empty what_arg is "contained" trivially; a zero code is allowed
    std::system_error e(std::error_code(), "");
    CHECK(e.code() == std::error_code());
    CHECK(e.what() != nullptr);
  }
  {  // a long what_arg (beyond any small buffer)
    std::string longarg(300, 'x');
    longarg += "END";
    std::system_error e(ec, longarg);
    CHECK(contains(e.what(), longarg));
  }
  {  // copies keep code() and what() ([exception]/2)
    std::system_error a(ec, arg);
    std::system_error b = a;
    CHECK(b.code() == ec);
    CHECK(std::strcmp(a.what(), b.what()) == 0);
    std::system_error c(std::error_code(1, std::generic_category()), "other");
    c = a;
    CHECK(c.code() == ec);
    CHECK(std::strcmp(a.what(), c.what()) == 0);
  }
  {  // [syserr.general]/2: components in [syserr] do not change errno
    errno = 31337;
    std::system_error a(std::error_code(ENOENT, std::system_category()), "x");
    std::system_error b(EINVAL, std::generic_category());
    std::system_error c(987654, std::system_category(), std::string("y"));
    (void)a.what();
    (void)b.what();
    (void)c.what();
    CHECK(errno == 31337);
  }
  {  // a copy's what() outlives the original ([exception]/6: valid until *its* object dies)
    std::system_error* a = new std::system_error(ec, arg);
    std::system_error b(*a);
    delete a;
    CHECK(contains(b.what(), arg));
    CHECK(b.code() == ec);
  }
  {  // thrown and caught through its bases; what() is virtual
    bool caught = false;
    try {
      throw std::system_error(EIO, std::generic_category(), "write");
    } catch (const std::runtime_error& e) {
      CHECK(contains(e.what(), "write"));
      const std::system_error* p = dynamic_cast<const std::system_error*>(&e);
      CHECK(p != nullptr);
      CHECK(p->code() == std::errc::io_error);
      caught = true;
    }
    CHECK(caught);
    try {
      throw std::system_error(std::make_error_code(std::errc::timed_out), arg);
    } catch (const std::exception& e) {
      CHECK(contains(e.what(), arg));
    }
  }
  return 0;
}
