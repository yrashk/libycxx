// Support for the generated spec-audit probes (tools/spec_audit/part3). Only standard C++.
// Included after the header under test: the sample types of an area are defined when that
// area's header is the probe's subject (the probe defines SPEC_PROBE_<header>).
#pragma once
#include <type_traits>
#include <utility>
#include <concepts>
#include <iosfwd>

namespace spec_probe {
  // A type that depends on Z, so that a requires-expression over it is checked when its
  // concept is named (a failed check is then `false`, not a hard error).
  template<class Z, class T> struct dep_ { using type = T; };
  template<class Z, class T> using dep = typename dep_<Z, T>::type;
  template<class T> T&& dv() noexcept;   // declval without <utility>'s static_assert
  template<class A, class B> concept same = std::is_same_v<A, B>;

  using fn = void (*)();
  using fn_int = int (*)();
  using pred = bool (*)();
  using fn_dd = double (*)(double);
  using fn_ii = int (*)(int);
  struct callback { void operator()() noexcept {} };
  struct completion { void operator()() noexcept {} };
  struct visitor { template<class T> void operator()(T&&) const {} };
}

#if defined(SPEC_PROBE_istream) || defined(SPEC_PROBE_ostream)
namespace spec_probe {
  // only non-template, lvalue-stream operators: an rvalue stream reaches [istream.rvalue] and
  // [ostream.rvalue]
  struct streamable {
    friend std::ostream& operator<<(std::ostream& o, const streamable&) { return o; }
    friend std::istream& operator>>(std::istream& i, streamable&) { return i; }
  };
  using streamable_ref = streamable&;
}
#endif

#if defined(SPEC_PROBE_fstream)
#include <filesystem>   // to spell filesystem::path::value_type
#endif

#if defined(SPEC_PROBE_format)
#include <span>
#endif

#if defined(SPEC_PROBE_rcu)
namespace spec_probe { struct rcu_node : std::rcu_obj_base<rcu_node> {}; }
#endif

#if defined(SPEC_PROBE_hazard_pointer)
namespace spec_probe { struct hp_node : std::hazard_pointer_obj_base<hp_node> {}; }
#endif

#if defined(SPEC_PROBE_linalg)
#include <mdspan>
#include <execution>   // the policies of the ExecutionPolicy overloads
namespace spec_probe {
  using mat = std::mdspan<double, std::dextents<std::size_t, 2>>;
  using vec = std::mdspan<double, std::dextents<std::size_t, 1>>;
}
#endif

#if defined(SPEC_PROBE_simd)
namespace spec_probe {
  struct idxmap { template<class I> constexpr int operator()(I i) const { return 0; } };
  struct generator { template<class I> constexpr float operator()(I) const { return 0; } };
  struct mask_generator { template<class I> constexpr bool operator()(I) const { return false; } };
}
#endif

#if defined(SPEC_PROBE_execution)
namespace spec_probe {
  using sndr = decltype(std::execution::just(1));
  struct rcvr {
    using receiver_concept = std::execution::receiver_tag;
    template<class... A> void set_value(A&&...) && noexcept {}
    template<class E> void set_error(E&&) && noexcept {}
    void set_stopped() && noexcept {}
  };
  struct closure : std::execution::sender_adaptor_closure<closure> {
    template<class S> S operator()(S s) const { return s; }
  };
  using scope_token = decltype(dv<std::execution::counting_scope&>().get_token());
  struct task_env {};
  struct query_env {};
  struct derived_env {};
  struct promise : std::execution::with_awaitable_senders<promise> {};
}
#endif
