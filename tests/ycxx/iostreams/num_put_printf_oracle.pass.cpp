// [facet.num.put.virtuals] Stage 1: the characters are those "that would be printed by
// printf ([c.files]) given this conversion specifier for printf(spec, val) assuming that the
// current locale is the "C" locale", with str.precision() as the precision for fixed (%f),
// scientific (%e) and neither (%g; '#' with showpoint), and L for long double (Tables 98-100).
// The C library's snprintf is the oracle, including very large precisions (hundreds or
// thousands of digits) and extreme values (subnormals, DBL_MAX, long double 1e4000).
#include <sstream>
#include <cstdio>
#include <string>
#include <vector>
#include <cfloat>
#include <ios>
#include "check.hpp"
static char ref[20000];
int main(){ int bad = 0;
 std::vector<double> ds = {1e-300, 5e-324, 1.7976931348623157e308, 0.1, 123.456, 1e22, 2.5, 0.5};
 for (double d : ds) for (int prec : {0, 1, 17, 30, 100, 400, 1100, 1500}) for (int f = 0; f < 4; ++f) {
   std::ostringstream os; os.precision(prec);
   const char* spec;
   if (f == 0) { os << std::fixed; spec = "%.*f"; } else if (f == 1) { os << std::scientific; spec = "%.*e"; } else if (f == 2) { spec = "%.*g"; } else { os << std::showpoint; spec = "%#.*g"; }
   os << d; snprintf(ref, sizeof ref, spec, prec, d);
   if (os.str() != ref) { if (bad++ < 5) dprintf(2, "double %g prec %d f %d: got %.60s... want %.60s...\n", d, prec, f, os.str().c_str(), ref); }
 }
 for (long double d : {1e-4000L, 1e4000L, LDBL_MAX, LDBL_TRUE_MIN, 0.1L}) for (int prec : {0, 20, 1100, 5000}) for (int f = 0; f < 3; ++f) {
   std::ostringstream os; os.precision(prec); const char* spec;
   if (f == 0) { os << std::fixed; spec = "%.*Lf"; } else if (f == 1) { os << std::scientific; spec = "%.*Le"; } else spec = "%.*Lg";
   os << d; snprintf(ref, sizeof ref, spec, prec, d);
   if (os.str() != ref) { if (bad++ < 10) dprintf(2, "ld %Lg prec %d f %d: got %zu chars %.40s want %zu %.40s\n", d, prec, f, os.str().size(), os.str().c_str(), std::string(ref).size(), ref); }
 }
 CHECK(bad == 0);
 return 0;
}
