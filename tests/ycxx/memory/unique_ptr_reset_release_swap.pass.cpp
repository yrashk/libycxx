// [unique.ptr.single.modifiers]: release() returns the stored pointer and leaves get() ==
// nullptr without calling the deleter; reset(p) "assigns p to the stored pointer, and
// then, with the old value of the stored pointer, old_p, evaluates if (old_p)
// get_deleter()(old_p);" — so the deleter observes the new pointer already stored, and a
// null old pointer is not passed to the deleter. swap exchanges pointers and deleters.
// [unique.ptr.special]/1-2: non-member swap calls x.swap(y).
#include <memory>
#include <utility>
#include "check.hpp"

struct Observer {
  const std::unique_ptr<int, Observer&>* owner = nullptr;
  int calls = 0;
  int* seen_in_owner = reinterpret_cast<int*>(1);
  void operator()(int* p) {
    ++calls;
    seen_in_owner = owner->get();  // the stored pointer at the time of deletion
    delete p;
  }
};

struct Tag {
  int id;
  int calls = 0;
  void operator()(int* p) {
    ++calls;
    delete p;
  }
};

int main() {
  {
    Observer o;
    std::unique_ptr<int, Observer&> u(new int(1), o);
    o.owner = &u;
    int* fresh = new int(2);
    u.reset(fresh);
    CHECK(o.calls == 1);
    CHECK(o.seen_in_owner == fresh);  // new pointer stored before deleting the old one
    CHECK(u.get() == fresh);
    u.reset();
    CHECK(o.calls == 2 && o.seen_in_owner == nullptr && u.get() == nullptr);
    u.reset();  // old pointer null: deleter not called
    u.reset(nullptr);
    CHECK(o.calls == 2);
  }
  {
    std::unique_ptr<int> u(new int(5));
    int* raw = u.get();
    int* r = u.release();
    CHECK(r == raw && u.get() == nullptr && !u);
    CHECK(u.release() == nullptr);
    delete r;
  }
  {
    std::unique_ptr<int, Tag> a(new int(1), Tag{1});
    std::unique_ptr<int, Tag> b(new int(2), Tag{2});
    int* ar = a.get();
    int* br = b.get();
    a.swap(b);
    CHECK(a.get() == br && b.get() == ar);
    CHECK(a.get_deleter().id == 2 && b.get_deleter().id == 1);
    swap(a, b);  // found by ADL
    CHECK(a.get() == ar && a.get_deleter().id == 1);
    std::swap(a, b);
    CHECK(a.get() == br && a.get_deleter().id == 2);
    static_assert(noexcept(a.swap(b)));
  }
  {
    // reset on the array form
    std::unique_ptr<int[]> a(new int[3]{1, 2, 3});
    a.reset(new int[2]{4, 5});
    CHECK(a[0] == 4 && a[1] == 5);
    a.reset(nullptr);
    CHECK(!a);
    a.reset(new int[1]{6});
    a.reset();
    CHECK(a.get() == nullptr);
  }
  return 0;
}
