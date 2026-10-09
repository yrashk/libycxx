// EXPECT-ERROR-GCC: error: call to non-'constexpr' function [^\n]*__format_string_argument_index_out_of_range\(\)
// EXPECT-ERROR-CLANG: error: call to consteval function [^\n]*std::basic_format_string[^\n]*is not a constant expression
// EXPECT-ERROR-CLANG: note: non-constexpr function '__format_string_argument_index_out_of_range' cannot be used in a constant expression
// [format.parse.ctx]/10: next_arg_id: "Call expressions where cur-arg-id >= num_args_ is true
// are not core constant expressions"; /15.1: check_dynamic_spec is a core constant expression
// only if id < num_args_. A formatter consuming an extra argument id when no such argument
// exists makes the compile-time checked format string ill-formed ([format.fmt.string]/3).
// The control passes the extra argument.
#include <format>

struct W {};
template <>
struct std::formatter<W> {
  constexpr auto parse(std::format_parse_context& pc) {
    std::size_t id = pc.next_arg_id();
    pc.check_dynamic_spec<int>(id);
    return pc.begin();
  }
  auto format(W, std::format_context& fc) const { return fc.out(); }
};

int main() {
  (void)std::format("{}", W{}, 1);  // control
#ifndef YCXX_CONTROL
  (void)std::format("{}", W{});
#endif
}
