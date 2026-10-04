// [syserr.errcode.constructors]/3: error_code's converting constructor template is constrained
// on is_error_code_enum_v<ErrorCodeEnum>; is_error_code_enum<errc> is false ([system.error.syn]
// specializes only is_error_condition_enum<errc>), so errc does not convert to error_code.
#include <system_error>

std::error_condition ok = std::errc::invalid_argument;  // control: condition enum
std::error_code ok2 = std::make_error_code(std::errc::invalid_argument);
std::error_code bad = std::errc::invalid_argument;

int main() { return ok.value() + ok2.value() + bad.value(); }
