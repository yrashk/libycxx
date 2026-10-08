// [exec.getcomplsigs]/1, /3.1: a sender's get_completion_signatures that exits with an exception
// other than dependent_sender_error (here, without an environment) gives no signatures, and the
// sender is not dependent ([exec.snd.concepts]: is-dependent-sender-helper catches only
// dependent_sender_error), so it is neither sender_in nor dependent_sender. [exec.snd.general]/6:
// such an unspecified-exception is matched by a handler of type exception but not by one of
// type dependent_sender_error.
// XFAIL: clang Clang 23.1 cannot throw during constant evaluation (P3068): a throwing get_completion_signatures is only not a constant expression, so its exception cannot be told from dependent_sender_error nor caught (STATUS.md, known compiler gaps)
// REQUIRES: exceptions
#include <execution>
#include <stdexcept>

namespace ex = std::execution;

struct invalid_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    if constexpr (sizeof...(Env) == 0)
      throw std::logic_error("never valid");
    return ex::completion_signatures<ex::set_value_t()>();
  }
};
static_assert(ex::sender<invalid_sender>);
static_assert(!ex::sender_in<invalid_sender>);
static_assert(!ex::dependent_sender<invalid_sender>);
static_assert(ex::sender_in<invalid_sender, ex::env<>>);

// A library adaptor whose check-types throws: not dependent either.
struct dep_two_values {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    if constexpr (sizeof...(Env) == 0)
      throw ex::dependent_sender_error();
    return ex::completion_signatures<ex::set_value_t(int), ex::set_value_t(long)>();
  }
};
static_assert(ex::dependent_sender<dep_two_values>);

// The unspecified-exception of a library check-types: an exception, not dependent_sender_error.
consteval bool library_error_is_exception_not_dependent() {
  try {
    (void)ex::get_completion_signatures<decltype(ex::when_all(dep_two_values())), ex::env<>>();
  } catch (const ex::dependent_sender_error&) {
    return false;
  } catch (const std::exception&) {
    return true;
  }
  return false;
}
static_assert(library_error_is_exception_not_dependent());
