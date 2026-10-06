// [exec.let]/10: let-state::impl, on the child's set-cpo completion, first decay-copies the datums
// into args, then destroys the child's operation state (ops.emplace<monostate>()), and only
// then invokes f and connects and starts the sender it returns. So f runs after the child's
// operation state is gone, with the copies of the datums; a datum sent by reference to the
// child's own storage is copied before that storage is destroyed. Completions other than
// set-cpo are forwarded unchanged (/16.3).
#include <execution>
#include <string>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

bool child_alive = false;

// Sends a reference to a string its operation state owns, or an error, as told.
struct owning_sender {
  using sender_concept = ex::sender_tag;
  bool fail = false;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(std::string&), ex::set_error_t(int)>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    bool fail;
    std::string owned = "owned by the child";
    op(R rr, bool f) : r(std::move(rr)), fail(f) { child_alive = true; }
    op(op&&) = delete;
    ~op() {
      owned = "destroyed";
      child_alive = false;
    }
    void start() & noexcept {
      if (fail)
        ex::set_error(std::move(r), 3);
      else
        ex::set_value(std::move(r), owned);
    }
  };
  template <class R>
  op<R> connect(R r) const {
    return {std::move(r), fail};
  }
};

int main() {
  {
    bool alive_in_f = true;
    record<int, std::string> rec;
    run(owning_sender{} | ex::let_value([&](std::string& s) {
          alive_in_f = child_alive;
          return ex::just(s + "!");
        }),
        receiver_for(rec));
    CHECK(!alive_in_f);
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == "owned by the child!");
  }
  // let_error: the same for the error; the value path is not taken here.
  {
    bool alive_in_f = true;
    record<int, std::string> rec;
    run(owning_sender{true} | ex::let_error([&](int e) {
          alive_in_f = child_alive;
          return ex::just(std::to_string(e));
        }),
        receiver_for(rec));
    CHECK(!alive_in_f);
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == "3");
  }
  // A completion other than set-cpo passes through; f is not called.
  {
    bool called = false;
    record<int, std::string> rec;
    run(owning_sender{true} | ex::let_value([&](std::string&) {
          called = true;
          return ex::just(std::string());
        }),
        receiver_for(rec));
    CHECK(!called && rec.how == done::error && *rec.error == 3);
  }
  return 0;
}
