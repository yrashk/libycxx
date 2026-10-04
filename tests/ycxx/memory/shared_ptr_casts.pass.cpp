// [util.smartptr.shared.cast]: static_pointer_cast, dynamic_pointer_cast,
// const_pointer_cast and reinterpret_pointer_cast return shared_ptr<T>(r, cast<...>(r.get()))
// for const& r, and shared_ptr<T>(std::move(r), ...) for r&& (r is left empty). dynamic
// cast failure gives an empty shared_ptr (and the rvalue r keeps its value); the result
// shares ownership with r.
#include <memory>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct B {
  virtual ~B() = default;
  int b = 1;
};
struct D : B {
  int d = 2;
};
struct Other : B {};

int main() {
  std::shared_ptr<B> b = std::make_shared<D>();
  auto d = std::static_pointer_cast<D>(b);
  static_assert(std::is_same_v<decltype(d), std::shared_ptr<D>>);
  CHECK(d->d == 2 && b.use_count() == 2);
  auto dd = std::dynamic_pointer_cast<D>(b);
  CHECK(dd.get() == d.get() && b.use_count() == 3);
  auto none = std::dynamic_pointer_cast<Other>(b);
  CHECK(!none && none.use_count() == 0);
  std::shared_ptr<const B> cb = b;
  auto mb = std::const_pointer_cast<B>(cb);
  static_assert(std::is_same_v<decltype(mb), std::shared_ptr<B>>);
  CHECK(mb.get() == b.get());
  auto bytes = std::reinterpret_pointer_cast<unsigned char>(b);
  CHECK(static_cast<void*>(bytes.get()) == static_cast<void*>(b.get()));
  long uc = b.use_count();

  // rvalue overloads move from r
  std::shared_ptr<B> m = b;
  auto md = std::static_pointer_cast<D>(std::move(m));
  CHECK(!m && md.get() == d.get() && b.use_count() == uc + 1);
  std::shared_ptr<B> m2 = b;
  auto mo = std::dynamic_pointer_cast<Other>(std::move(m2));
  CHECK(!mo && m2.get() == b.get());  // failed dynamic cast leaves r untouched
  auto md2 = std::dynamic_pointer_cast<D>(std::move(m2));
  CHECK(!m2 && md2.get() == d.get());
  std::shared_ptr<const B> mc = b;
  auto mcb = std::const_pointer_cast<B>(std::move(mc));
  CHECK(!mc && mcb.get() == b.get());
  std::shared_ptr<B> mr = b;
  auto mrb = std::reinterpret_pointer_cast<char>(std::move(mr));
  CHECK(!mr && static_cast<void*>(mrb.get()) == static_cast<void*>(b.get()));

  // arrays: static_pointer_cast to the array type
  std::shared_ptr<int[]> arr = std::make_shared<int[]>(2);
  auto carr = std::const_pointer_cast<const int[]>(arr);
  static_assert(std::is_same_v<decltype(carr), std::shared_ptr<const int[]>>);
  CHECK(carr.get() == arr.get());
  return 0;
}
