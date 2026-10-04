// [facet.numpunct], [facet.numpunct.virtuals]: the required numpunct<char> and numpunct<wchar_t>
// facets provide classic "C" formats: decimal_point '.', thousands_sep ',', grouping "" (no
// grouping), truename "true" and falsename "false". A derived facet can override the virtuals,
// and num_put/num_get use them (grouping, decimal point, boolalpha names).
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"

struct Grouped : std::numpunct<char> {
  char do_thousands_sep() const override { return '\''; }
  std::string do_grouping() const override { return "\3"; }
  char do_decimal_point() const override { return ','; }
  std::string do_truename() const override { return "yes"; }
  std::string do_falsename() const override { return "no"; }
};

int main() {
  const std::locale& c = std::locale::classic();
  const auto& np = std::use_facet<std::numpunct<char>>(c);
  CHECK(np.decimal_point() == '.' && np.thousands_sep() == ',' && np.grouping().empty());
  CHECK(np.truename() == "true" && np.falsename() == "false");
  const auto& wnp = std::use_facet<std::numpunct<wchar_t>>(c);
  CHECK(wnp.decimal_point() == L'.' && wnp.thousands_sep() == L',' && wnp.grouping().empty());
  CHECK(wnp.truename() == L"true" && wnp.falsename() == L"false");

  std::locale g(c, new Grouped);
  std::ostringstream os;
  os.imbue(g);
  os << 1234567 << ' ' << 2.5 << ' ' << std::boolalpha << true << ' ' << false;
  CHECK(os.str() == "1'234'567 2,5 yes no");
  std::istringstream is("7'654'321 3,25 yes");
  is.imbue(g);
  long v = 0;
  double d = 0;
  bool b = false;
  is >> v >> d >> std::boolalpha >> b;
  CHECK(!is.fail() && v == 7654321 && d == 3.25 && b);

  // Classic locale: no grouping in output.
  std::ostringstream plain;
  plain.imbue(c);
  plain << 1234567;
  CHECK(plain.str() == "1234567");
  return 0;
}
