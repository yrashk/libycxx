// [exec.snd.expos]/37, /54-55: default-impls::get-state is allocator-aware-forward(data, rcvr):
// when get_allocator(get_env(rcvr)) is valid, the operation's data is a new object made with
// that allocator by uses-allocator construction (make_obj_using_allocator), element by element
// for a product-type (/55.2); otherwise the data is forwarded (/55.1). just's data is
// product-type{ts...} ([exec.just]/2) and its start sends the stored datums (moved), so
// just(pmr::string) connected to a receiver whose environment has a polymorphic_allocator sends a
// string that uses that allocator's resource; without an allocator in the environment, the
// string keeps its own.
#include <execution>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <string>
#include <utility>
#include "check.hpp"

namespace ex = std::execution;

struct counting_resource : std::pmr::memory_resource {
  int allocations = 0;
  void* do_allocate(std::size_t n, std::size_t a) override {
    ++allocations;
    return std::pmr::new_delete_resource()->allocate(n, a);
  }
  void do_deallocate(void* p, std::size_t n, std::size_t a) override { std::pmr::new_delete_resource()->deallocate(p, n, a); }
  bool do_is_equal(const std::pmr::memory_resource& o) const noexcept override { return this == &o; }
};

template <class Env>
struct string_receiver {
  using receiver_concept = ex::receiver_tag;
  std::pmr::memory_resource** seen;
  std::pmr::string* out;
  Env env;
  void set_value(std::pmr::string&& s) && noexcept {
    *seen = s.get_allocator().resource();
    *out = std::move(s);
  }
  const Env& get_env() const noexcept { return env; }
};

int main() {
  const char* text = "a string long enough to need an allocation of its own";
  counting_resource own, from_env;
  // With an allocator in the receiver's environment.
  {
    std::pmr::string s(text, &own);
    using Env = ex::env<ex::prop<std::get_allocator_t, std::pmr::polymorphic_allocator<>>>;
    std::pmr::memory_resource* seen = nullptr;
    std::pmr::string out;
    auto snd = ex::just(std::move(s));
    const int before = from_env.allocations;
    {
      auto op = ex::connect(std::move(snd), string_receiver<Env>{&seen, &out, Env{ex::prop(std::get_allocator, std::pmr::polymorphic_allocator<>(&from_env))}});
      ex::start(op);
    }
    CHECK(seen == &from_env);
    CHECK(from_env.allocations > before);
    CHECK(out == text);
  }
  // Without one: forwarded, the string keeps its resource.
  {
    std::pmr::string s(text, &own);
    std::pmr::memory_resource* seen = nullptr;
    std::pmr::string out;
    {
      auto op = ex::connect(ex::just(std::move(s)), string_receiver<ex::env<>>{&seen, &out, {}});
      ex::start(op);
    }
    CHECK(seen == &own);
    CHECK(out == text);
  }
  return 0;
}
