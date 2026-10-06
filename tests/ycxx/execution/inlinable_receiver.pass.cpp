// [exec.recv.concepts]/4: inlinable_receiver<Rcvr, ChildOp> holds for a receiver with a static
// make_receiver_for(ChildOp*) that is noexcept and returns remove_cvref_t<Rcvr>; ChildOp may be
// incomplete. /7, /9: whether connecting a library sender inlines such a receiver is
// implementation-defined, but when it does, every use of the receiver is a receiver that
// make_receiver_for(addressof(op)) recreates, which is equal to the one connected: so the
// completions reach the same place either way.
#include <execution>
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

struct incomplete_op;

// An inlinable receiver: make_receiver_for recreates it from the operation's address. The test
// connects one operation at a time, recorded in `current`.
struct inl_rcvr;
record<int, int>* current = nullptr;
int remade = 0;
struct inl_rcvr {
  using receiver_concept = ex::receiver_tag;
  record<int, int>* rec;
  template <class Op>
  static inl_rcvr make_receiver_for(Op*) noexcept {
    ++remade;
    return {current};
  }
  void set_value(int v) && noexcept {
    rec->how = done::value;
    ++rec->calls;
    rec->values.emplace(v);
  }
  void set_error(int e) && noexcept {
    rec->how = done::error;
    ++rec->calls;
    rec->error = e;
  }
  void set_error(std::exception_ptr) && noexcept {
    rec->how = done::error;
    ++rec->calls;
  }
  void set_stopped() && noexcept {
    rec->how = done::stopped;
    ++rec->calls;
  }
  ex::env<> get_env() const noexcept { return {}; }
};

struct not_noexcept {
  using receiver_concept = ex::receiver_tag;
  static not_noexcept make_receiver_for(void*) { return {}; }
};
struct wrong_type {
  using receiver_concept = ex::receiver_tag;
  static inl_rcvr make_receiver_for(void*) noexcept { return {}; }
};
struct non_static {
  using receiver_concept = ex::receiver_tag;
  non_static make_receiver_for(void*) noexcept { return {}; }
};
struct only_some_ops {
  using receiver_concept = ex::receiver_tag;
  static only_some_ops make_receiver_for(int*) noexcept { return {}; }
};

static_assert(ex::inlinable_receiver<inl_rcvr, incomplete_op>);
static_assert(ex::inlinable_receiver<inl_rcvr&&, incomplete_op>);
static_assert(ex::inlinable_receiver<const inl_rcvr&, int>);
static_assert(!ex::inlinable_receiver<not_noexcept, incomplete_op>);
static_assert(!ex::inlinable_receiver<wrong_type, incomplete_op>);
static_assert(!ex::inlinable_receiver<non_static, incomplete_op>);
static_assert(ex::inlinable_receiver<only_some_ops, int>);
static_assert(!ex::inlinable_receiver<only_some_ops, incomplete_op>);
static_assert(!ex::inlinable_receiver<int, incomplete_op>);
// A receiver is required.
struct not_a_receiver {
  static not_a_receiver make_receiver_for(void*) noexcept { return {}; }
};
static_assert(!ex::inlinable_receiver<not_a_receiver, incomplete_op>);

template <class S>
void check(S&& s, done how, int v = 0) {
  record<int, int> rec;
  current = &rec;
  {
    auto op = ex::connect(std::forward<S>(s), inl_rcvr{&rec});
    ex::start(op);
  }
  current = nullptr;
  CHECK(rec.how == how && rec.calls == 1);
  if (how == done::value)
    CHECK(std::get<0>(*rec.values) == v);
  if (how == done::error && rec.error)
    CHECK(*rec.error == v);
}

int main() {
  check(ex::just(1), done::value, 1);
  check(ex::just_error(2), done::error, 2);
  check(ex::just_stopped(), done::stopped);
  check(ex::just(3) | ex::then([](int x) { return x + 1; }), done::value, 4);
  check(ex::just(5) | ex::let_value([](int x) { return ex::just(x * 2); }), done::value, 10);
  check(ex::when_all(ex::just(6)), done::value, 6);
  check(ex::just(7) | ex::continues_on(ex::inline_scheduler()), done::value, 7);
  check(ex::just_error(8) | ex::upon_error([](int e) { return e + 1; }), done::value, 9);
  check(ex::starts_on(ex::inline_scheduler(), ex::just(11)), done::value, 11);
  (void)remade; // implementation-defined how often (/9)
  return 0;
}
