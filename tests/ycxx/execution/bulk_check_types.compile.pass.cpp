// [exec.bulk]/6, /8: check-types of bulk_chunked and bulk_unchunked: for every value completion
// set_value_t(Ts...) of the child, f must be invocable with (Shape, Shape, Ts&...) for
// bulk_chunked and with (Shape, Ts&...) for bulk_unchunked (the datums as lvalues), else the
// sender has no completion signatures (not sender_in). Other completions are not checked.
// /5: the exception_ptr error is a completion only when that call of f is potentially
// throwing. The children are dependent senders, so that the ill-formed cases are not ruled out
// by make-sender's Mandates ([exec.snd.expos]/24.4) already.
// /2: bulk-algo(sndr, policy, shape, f) is ill-formed unless sndr is a sender, policy an
// execution policy (after remove_cvref), shape integral and f copy_constructible.
#include <exception>
#include <execution>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include "exec_support.hpp"

namespace ex = std::execution;
using exec_test::same_sigs;

template <class... Sigs>
struct dep_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    if constexpr (sizeof...(Env) == 0)
      return (throw ex::dependent_sender_error(), ex::completion_signatures<Sigs...>());
    else
      return ex::completion_signatures<Sigs...>();
  }
};

using E = ex::env<>;
using S = dep_sender<ex::set_value_t(int), ex::set_value_t(std::string&&), ex::set_error_t(long), ex::set_stopped_t()>;

struct chunked_ok {
  void operator()(int, int, int&) const noexcept {}
  void operator()(int, int, std::string&) const noexcept {}
};
struct chunked_no_string {
  void operator()(int, int, int&) const noexcept {}
};
struct chunked_rvalue_string {
  void operator()(int, int, int&) const noexcept {}
  void operator()(int, int, std::string&&) const noexcept {}
};
struct unchunked_ok {
  void operator()(int, const int&) const {}
  void operator()(int, std::string&) const {}
};
struct unchunked_chunked_arity {
  void operator()(int, int, int&) const noexcept {}
  void operator()(int, int, std::string&) const noexcept {}
};

using C1 = decltype(ex::bulk_chunked(S(), ex::seq, 4, chunked_ok()));
static_assert(ex::dependent_sender<C1> && ex::sender_in<C1, E>);
static_assert(same_sigs<ex::completion_signatures_of_t<C1, E>,
                        ex::completion_signatures<ex::set_value_t(int), ex::set_value_t(std::string&&), ex::set_error_t(long), ex::set_stopped_t()>>);
static_assert(!ex::sender_in<decltype(ex::bulk_chunked(S(), ex::seq, 4, chunked_no_string())), E>);
static_assert(!ex::sender_in<decltype(ex::bulk_chunked(S(), ex::seq, 4, chunked_rvalue_string())), E>);
// The unchunked arity is not enough for bulk_chunked, and the reverse.
static_assert(!ex::sender_in<decltype(ex::bulk_chunked(S(), ex::seq, 4, unchunked_ok())), E>);
static_assert(!ex::sender_in<decltype(ex::bulk_unchunked(S(), ex::seq, 4, unchunked_chunked_arity())), E>);

using U1 = decltype(ex::bulk_unchunked(S(), ex::par, 4, unchunked_ok()));
static_assert(ex::sender_in<U1, E>);
// f can throw: exception_ptr is added.
static_assert(same_sigs<ex::completion_signatures_of_t<U1, E>,
                        ex::completion_signatures<ex::set_value_t(int), ex::set_value_t(std::string&&), ex::set_error_t(long), ex::set_stopped_t(),
                                                  ex::set_error_t(std::exception_ptr)>>);

// Only the value completions are checked: f need not accept the error.
using OnlyError = dep_sender<ex::set_error_t(std::string), ex::set_stopped_t()>;
static_assert(ex::sender_in<decltype(ex::bulk_chunked(OnlyError(), ex::seq, 2, [](int, int) noexcept {})), E>);
static_assert(ex::sender_in<decltype(ex::bulk_unchunked(OnlyError(), ex::seq, 2, [](int) noexcept {})), E>);

// bulk_chunked: a nothrow f adds no exception_ptr error; a throwing one does.
using JC = decltype(ex::just(1) | ex::bulk_chunked(ex::seq, 3, [](int, int, int&) noexcept {}));
static_assert(std::is_same_v<ex::completion_signatures_of_t<JC>, ex::completion_signatures<ex::set_value_t(int)>>);
using JT = decltype(ex::just(1) | ex::bulk_chunked(ex::seq, 3, [](int, int, int&) {}));
static_assert(same_sigs<ex::completion_signatures_of_t<JT>, ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr)>>);

// /2
auto f1 = [](int) {};
using Just = decltype(ex::just());
static_assert(std::is_invocable_v<ex::bulk_unchunked_t, Just, ex::parallel_unsequenced_policy, int, decltype(f1)>);
static_assert(std::is_invocable_v<ex::bulk_unchunked_t, Just, const ex::unsequenced_policy&, long, decltype(f1)>);
static_assert(!std::is_invocable_v<ex::bulk_unchunked_t, Just, int, int, decltype(f1)>);
static_assert(!std::is_invocable_v<ex::bulk_unchunked_t, Just, const ex::sequenced_policy&, float, decltype(f1)>);
static_assert(!std::is_invocable_v<ex::bulk_unchunked_t, int, const ex::sequenced_policy&, int, decltype(f1)>);
auto move_only = [p = std::unique_ptr<int>()](int) {};
static_assert(!std::is_invocable_v<ex::bulk_unchunked_t, Just, const ex::sequenced_policy&, int, decltype(move_only)>);
static_assert(!std::is_invocable_v<ex::bulk_chunked_t, Just, const ex::sequenced_policy&, int, decltype(move_only)>);
static_assert(!std::is_invocable_v<ex::bulk_t, Just, const ex::sequenced_policy&, int, decltype(move_only)>);
static_assert(!std::is_invocable_v<ex::bulk_chunked_t, Just, const ex::sequenced_policy&, double, decltype(f1)>);
