// [dcl.init.general]/7-8: default-initializing a const object needs a const-default-constructible
// type, e.g. one whose default constructor is user-provided (/7.3). The library declares these
// default constructors without "= default" (so, as specified, they are user-provided):
//   [func.wrap.func.general]: function() noexcept;
//   [func.wrap.move.class]: move_only_function() noexcept;
//   [func.wrap.copy.class]: copyable_function() noexcept;
//   [any.class]: constexpr any() noexcept;  [optional.optional.general]: constexpr optional() noexcept;
//   [util.smartptr.shared.general]: constexpr shared_ptr() noexcept;
//   [unique.ptr.single.general]: constexpr unique_ptr() noexcept;
// so `const T t;` is well-formed for each. (Contrast [time.duration.general]:
// `constexpr duration() = default;`, which is not checked here.)
#include <any>
#include <functional>
#include <memory>
#include <optional>

const std::function<int(int)> f1;
const std::move_only_function<int(int)> f2;
const std::move_only_function<int(int) const noexcept> f2c;
const std::copyable_function<int(int)> f3;
const std::copyable_function<int(int) &&> f3r;
const std::any a;
const std::optional<int> o;
const std::shared_ptr<int> sp;
const std::unique_ptr<int> up;

void use() {
  const std::function<void()> local;
  const std::move_only_function<void()> local2;
  const std::copyable_function<void()> local3;
  (void)local, (void)local2, (void)local3;
}
