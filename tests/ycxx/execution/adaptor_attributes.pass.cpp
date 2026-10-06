// Attributes of adaptors:
//   [exec.snd.general]/4: a sender that can determine that all its completions with tag T run on
//     sch's agents reports sch as get_completion_scheduler<T>; Example 2: then(sndr, f) has
//     sndr's set_value completion scheduler. continues_on(sndr, sch) runs every completion on sch
//     ([exec.continues.on]/12; a run_loop's schedule sender has no error completion), and the
//     schedule sender of a run_loop reports its scheduler for set_value and set_stopped
//     ([exec.run.loop.types]/5);
//   [exec.snd.general]/3, [exec.get.compl.domain]/2.3: their completion domain follows;
//   [exec.adapt.general]/3.2: a single-child adaptor's attributes are FWD-ENV of the child's
//     (forwarding queries only); /3.3: an adaptor with several children has env<> attributes
//     apart from the completion queries;
//   [exec.get.compl.sched]/6, [exec.get.compl.domain]/3: a sender without completions of tag T
//     reports none for T (a program asking otherwise is ill-formed; here, the query is absent).
#include <execution>
#include <type_traits>
#include "check.hpp"

namespace ex = std::execution;

struct fwd_q_t : std::forwarding_query_t {
  template <class Env>
  constexpr auto operator()(const Env& e) const noexcept -> decltype(e.query(*this)) {
    return e.query(*this);
  }
};
inline constexpr fwd_q_t fwd_q{};
struct local_q_t {
  template <class Env>
  constexpr auto operator()(const Env& e) const noexcept -> decltype(e.query(*this)) {
    return e.query(*this);
  }
};
inline constexpr local_q_t local_q{};

// A sender with attributes answering both queries.
struct attr_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>();
  }
  struct attrs {
    int query(fwd_q_t) const noexcept { return 1; }
    int query(local_q_t) const noexcept { return 2; }
  };
  attrs get_env() const noexcept { return {}; }
};

template <class Q, class E>
concept answers = requires(const E& e) { Q()(e); };

int main() {
  ex::run_loop loop;
  auto sch = loop.get_scheduler();
  using Sch = decltype(sch);
  // then over a schedule sender: set_value on the loop.
  {
    auto s = ex::schedule(sch) | ex::then([]() noexcept { return 1; });
    CHECK(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s), ex::env<>()) == sch);
    static_assert(std::is_same_v<decltype(ex::get_completion_domain<ex::set_value_t>(ex::get_env(s), ex::env<>())),
                                 decltype(ex::get_completion_domain<ex::set_value_t>(sch, ex::env<>()))>);
  }
  // continues_on: values on sch.
  {
    auto s = ex::continues_on(ex::just(1), sch);
    auto vs = ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s), ex::env<>());
    static_assert(std::is_same_v<decltype(vs), Sch>);
    CHECK(vs == sch);
    auto s2 = ex::just(1) | ex::continues_on(sch) | ex::then([](int x) noexcept { return x; });
    CHECK(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s2), ex::env<>()) == sch);
  }
  // [exec.adapt.general]/3.2: then forwards the child's forwarding attributes only.
  {
    auto s = attr_sender() | ex::then([](int x) noexcept { return x; });
    static_assert(answers<fwd_q_t, decltype(ex::get_env(s))>);
    static_assert(!answers<local_q_t, decltype(ex::get_env(s))>);
    CHECK(fwd_q(ex::get_env(s)) == 1);
    auto s2 = ex::continues_on(attr_sender(), sch);
    static_assert(!answers<local_q_t, decltype(ex::get_env(s2))>);
  }
  // /3.3: when_all has none of its children's attributes.
  {
    auto w = ex::when_all(attr_sender(), attr_sender());
    static_assert(!answers<fwd_q_t, decltype(ex::get_env(w))>);
    static_assert(!answers<local_q_t, decltype(ex::get_env(w))>);
  }
  return 0;
}
