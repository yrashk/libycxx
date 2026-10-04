// [util.smartptr.enab]: shared_from_this() returns shared_ptr<T>(weak_this);
// weak_from_this() returns weak_this. The shared_ptr constructors that create a unique
// owner of a pointer to an object with an unambiguous and accessible enable_shared_from_this
// base initialize weak_this ([util.smartptr.shared.const]/1), as do make_shared and
// unique_ptr conversion. Without such an owner, weak_from_this() is empty and
// shared_from_this() throws bad_weak_ptr (it constructs shared_ptr from an expired
// weak_ptr). Copying the object does not copy weak_this.
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Node : std::enable_shared_from_this<Node> {
  int v = 1;
};
struct Derived : Node {};

static_assert(std::is_same_v<decltype(std::declval<Node&>().shared_from_this()), std::shared_ptr<Node>>);
static_assert(std::is_same_v<decltype(std::declval<const Node&>().shared_from_this()), std::shared_ptr<const Node>>);
static_assert(std::is_same_v<decltype(std::declval<Node&>().weak_from_this()), std::weak_ptr<Node>>);
static_assert(std::is_same_v<decltype(std::declval<const Node&>().weak_from_this()), std::weak_ptr<const Node>>);
static_assert(noexcept(std::declval<Node&>().weak_from_this()));

int main() {
  {
    auto p = std::make_shared<Node>();
    auto q = p->shared_from_this();
    CHECK(q.get() == p.get() && p.use_count() == 2);
    CHECK(!p->weak_from_this().expired());
    const Node& cr = *p;
    std::shared_ptr<const Node> c = cr.shared_from_this();
    CHECK(c.get() == p.get());
  }
  {
    std::shared_ptr<Node> p(new Node);
    CHECK(p->shared_from_this() == p);
    std::shared_ptr<Node> d(new Derived);  // base is accessible through Derived
    CHECK(d->shared_from_this() == d);
    std::shared_ptr<Node> u(std::make_unique<Node>());
    CHECK(u->shared_from_this() == u);
  }
  {
    Node stack;
    CHECK(stack.weak_from_this().expired());
    bool threw = false;
    try {
      (void)stack.shared_from_this();
    } catch (const std::bad_weak_ptr&) {
      threw = true;
    }
    CHECK(threw);
  }
  {
    auto p = std::make_shared<Node>();
    Node copy = *p;  // weak_this is not copied
    CHECK(copy.weak_from_this().expired());
    Node other;
    *p = other;  // assignment leaves weak_this unchanged
    CHECK(p->shared_from_this() == p);
    std::weak_ptr<Node> w = p->weak_from_this();
    p.reset();
    CHECK(w.expired());
  }
  return 0;
}
