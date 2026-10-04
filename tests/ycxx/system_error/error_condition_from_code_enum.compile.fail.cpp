// [syserr.errcondition.constructors]/3: error_condition's converting constructor template is
// constrained on is_error_condition_enum_v<ErrorConditionEnum>; a type registered only with
// is_error_code_enum ([system.error.syn]/2) does not convert to error_condition, even with a
// make_error_condition overload visible.
#include <system_error>
#include <string>

namespace lib {
struct Cat : std::error_category {
  const char* name() const noexcept override { return "lib"; }
  std::string message(int) const override { return ""; }
};
inline const Cat cat{};
enum class E { a = 1 };
inline std::error_code make_error_code(E e) noexcept { return {static_cast<int>(e), cat}; }
inline std::error_condition make_error_condition(E e) noexcept { return {static_cast<int>(e), cat}; }
}  // namespace lib
template <>
struct std::is_error_code_enum<lib::E> : std::true_type {};

std::error_code ok = lib::E::a;  // control: registered as an error code enum
std::error_condition bad = lib::E::a;

int main() { return ok.value() + bad.value(); }
