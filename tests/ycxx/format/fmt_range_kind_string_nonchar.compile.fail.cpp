// [format.range.fmtstr]/1: range-default-formatter<range_format::string, R, charT>
// "Mandates: same_as<remove_cvref_t<range_reference_t<R>>, charT> is true." A program-defined
// range of int whose format_kind is specialized to range_format::string is therefore
// ill-formed when formatted (format/format_kind.pass.cpp formats the same kind of range of
// char).
#include <format>
#include <vector>

struct Ints {
  std::vector<int> v;
  auto begin() const { return v.begin(); }
  auto end() const { return v.end(); }
};
template <>
inline constexpr std::range_format std::format_kind<Ints> = std::range_format::string;

int main() { (void)std::format("{}", Ints{{1, 2}}); }
