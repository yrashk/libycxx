// [futures.errors]: future_category().name() is "future"; make_error_code / make_error_condition
// use future_category(); is_error_code_enum<future_errc> is true, so a future_errc converts to
// error_code. [futures.future.error]: future_error derives from logic_error, code() returns
// make_error_code(e), what() is an NTBS. [future.syn]: future_status enumerators; launch is a
// bitmask type ([bitmask.types]) with distinct async and deferred.
#include <future>
#include <cstring>
#include <stdexcept>
#include <system_error>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_error_code_enum_v<std::future_errc>);
static_assert(!std::is_error_condition_enum_v<std::future_errc>);
static_assert(std::is_base_of_v<std::logic_error, std::future_error>);
static_assert(!std::is_convertible_v<std::future_errc, std::future_error>);  // explicit
static_assert(noexcept(std::future_category()));
static_assert(noexcept(std::make_error_code(std::future_errc::no_state)));
static_assert(noexcept(std::declval<const std::future_error&>().code()));
static_assert(std::is_same_v<decltype(std::declval<const std::future_error&>().code()), const std::error_code&>);
static_assert(static_cast<int>(std::future_status::ready) == 0);
static_assert(static_cast<int>(std::future_status::timeout) == 1);
static_assert(static_cast<int>(std::future_status::deferred) == 2);

int main() {
  const std::error_category& cat = std::future_category();
  CHECK(std::strcmp(cat.name(), "future") == 0);
  CHECK(&cat == &std::future_category());

  const std::future_errc all[] = {std::future_errc::broken_promise, std::future_errc::future_already_retrieved,
                                  std::future_errc::promise_already_satisfied, std::future_errc::no_state};
  for (auto e : all) {
    std::error_code ec = std::make_error_code(e);
    CHECK(ec.value() == static_cast<int>(e));
    CHECK(&ec.category() == &cat);
    std::error_condition cond = std::make_error_condition(e);
    CHECK(cond.value() == static_cast<int>(e) && &cond.category() == &cat);
    std::error_code implicit = e;
    CHECK(implicit == ec);
    CHECK(ec == e);  // via make_error_condition / equivalence
    std::future_error fe(e);
    CHECK(fe.code() == ec);
    CHECK(fe.what() != nullptr);
    CHECK(!cat.message(static_cast<int>(e)).empty());
  }
  for (auto a : all)
    for (auto b : all)
      CHECK((a == b) == (std::make_error_code(a) == std::make_error_code(b)));

  using L = std::launch;
  CHECK((L::async & L::deferred) == L{});
  CHECK((L::async | L::deferred) != L::async);
  L l = L::async;
  l |= L::deferred;
  CHECK((l & L::deferred) == L::deferred);
  l &= ~L::async;
  CHECK(l == L::deferred);
  l ^= L::async;
  CHECK((l & L::async) == L::async);
  return 0;
}
