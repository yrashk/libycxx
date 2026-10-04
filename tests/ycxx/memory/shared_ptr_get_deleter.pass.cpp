// [util.smartptr.getdeleter]: template<class D, class T> constexpr D* get_deleter(const
// shared_ptr<T>& p) noexcept; "Returns: If p owns a deleter d of type cv-unqualified D, returns
// addressof(d); otherwise returns nullptr. The returned pointer remains valid as long as
// there exists a shared_ptr instance that owns d." Copies and aliases share the same deleter.
#include <memory>
#include <type_traits>
#include "check.hpp"

struct Del {
  int id = 0;
  int* calls = nullptr;
  void operator()(int* p) const {
    if (calls) ++*calls;
    delete p;
  }
};
struct OtherDel {
  void operator()(int* p) const { delete p; }
};
struct Pair {
  int a, b;
};

static void free_fn(int* p) { delete p; }

static_assert(noexcept(std::get_deleter<Del>(std::declval<const std::shared_ptr<int>&>())));
static_assert(std::is_same_v<decltype(std::get_deleter<Del>(std::declval<const std::shared_ptr<int>&>())), Del*>);

int main() {
  int calls = 0;
  {
    std::shared_ptr<int> p(new int(1), Del{7, &calls});
    Del* d = std::get_deleter<Del>(p);
    CHECK(d != nullptr && d->id == 7);
    CHECK(std::get_deleter<OtherDel>(p) == nullptr);
    CHECK(std::get_deleter<const Del>(p) == nullptr || std::get_deleter<const Del>(p) == d);
    std::shared_ptr<int> q = p;
    CHECK(std::get_deleter<Del>(q) == d);
    std::shared_ptr<void> v = p;
    CHECK(std::get_deleter<Del>(v) == d);
    d->id = 9;  // modifications are seen through the owner
    CHECK(std::get_deleter<Del>(q)->id == 9);
  }
  CHECK(calls == 1);

  // aliasing constructor shares the deleter of r
  {
    auto owner = std::shared_ptr<Pair>(new Pair{1, 2}, [](Pair* x) { delete x; });
    std::shared_ptr<int> alias(owner, &owner->b);
    CHECK(std::get_deleter<Del>(alias) == nullptr);
  }

  // function pointer deleter
  {
    std::shared_ptr<int> p(new int(2), free_fn);
    auto* fp = std::get_deleter<void (*)(int*)>(p);
    CHECK(fp != nullptr && *fp == &free_fn);
  }

  // no owned deleter: empty, default-deleted, or created by make_shared
  CHECK(std::get_deleter<Del>(std::shared_ptr<int>()) == nullptr);
  CHECK(std::get_deleter<Del>(std::shared_ptr<int>(new int)) == nullptr);
  CHECK(std::get_deleter<Del>(std::make_shared<int>(3)) == nullptr);

  // nullptr_t with a deleter still owns the deleter
  calls = 0;
  {
    std::shared_ptr<int> n(nullptr, Del{5, &calls});
    CHECK(std::get_deleter<Del>(n) != nullptr && std::get_deleter<Del>(n)->id == 5);
  }
  CHECK(calls == 1);
  return 0;
}
