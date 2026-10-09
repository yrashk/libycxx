// EXPECT-ERROR-GCC: error: call to non-'constexpr' function [^\n]*__format_string_dynamic_argument_has_wrong_type\(\)
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: non-constexpr function '__format_string_dynamic_argument_has_wrong_type' cannot be used in a constant expression
// [format.parse.ctx]/15-16: "A call to this function is a core constant expression only if
// (15.1) id < num_args_ is true and (15.2) the type of the corresponding format argument
// (after conversion to basic_format_arg<Context>) is one of the types in Ts..."; check_dynamic_
// spec_integral is check_dynamic_spec<int, unsigned, long long, unsigned long long>.
// [format.fmt.string]/3: format_string's constructor is consteval and the format string is
// checked by the formatters' parse, so a string argument for an integral dynamic spec makes the
// program ill-formed. The control (an int for the spec) is well-formed.
#include <format>
#include <string>

struct W {};
template <>
struct std::formatter<W> {
  std::size_t id = 0;
  constexpr auto parse(std::format_parse_context& pc) {
    id = pc.next_arg_id();
    pc.check_dynamic_spec_integral(id);
    return pc.begin();
  }
  auto format(W, std::format_context& fc) const { return fc.out(); }
};

int main() {
  (void)std::format("{}", W{}, 3);  // control: well-formed
#ifndef YCXX_CONTROL
  (void)std::format("{}", W{}, "three");
#endif
}
