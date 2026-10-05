// User facets whose virtual functions call back into streams, format and locale while the
// library is in the middle of a call that invoked them.
//   [facet.num.put.virtuals] Stage 2 and [facet.num.get.virtuals] Stage 2 obtain grouping() and
//     thousands_sep() from the numpunct facet of the stream's locale; [locale.numpunct.virtuals]
//     do_grouping is a virtual the program may override, and nothing in the draft forbids it
//     from using the library (here: formatting and parsing with streams imbued with the same
//     locale, std::format with that locale, locale::global and locale()). The outer operation
//     must still produce its own result: an implementation that keeps the conversion state of
//     num_put/num_get or of a stream in storage shared between calls, or that holds a lock while
//     calling the facet, fails or deadlocks. [reentrancy]/1 makes it implementation-defined
//     which library functions "may be recursively reentered", so the inner operations use the
//     other character type (narrow inside a wide operation and the reverse): they are other
//     functions than the ones active, and no function is reentered.
//   [locale.operators]/2-3: locale::operator() compares with collate<charT>::compare; /4 the
//     locale is a comparator for std::sort ([alg.sort]); here do_compare formats with a stream.
//   [locale.statics]/1-2: locale::global sets the global locale and returns the previous one;
//     do_truename calls it (and locale()) during boolalpha output.
//   [locale.facet]/3: a refs == 0 facet is deleted when the last locale containing it is
//     destroyed; its destructor uses locale(), locale::global and a stream.
// A watchdog ends the test if any of this deadlocks.
// FLAGS: -pthread
#include <algorithm>
#include <format>
#include <locale>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

static std::locale shared_loc;  // the locale every reentrant call uses
static int depth = 0;
static int inner_failures = 0;
static int inner_runs = 0;

// Formats and parses with the same locale from inside the facet, with the other character
// type: the inner operations are distinct functions (other specializations) from the outer
// ones, so no library function is recursively reentered ([reentrancy]/1).
template<class C> static void reenter();
template<> void reenter<char>() {  // called from numpunct<char>: uses the wide functions
  ++depth;
  ++inner_runs;
  std::wostringstream os;
  os.imbue(shared_loc);
  os << 42424242 << L'|' << std::fixed;
  os.precision(2);
  os << 1234.5;
  if (os.str() != L"42.424.242|1.234,50") ++inner_failures;
  std::wistringstream is(L"9.876 x");
  is.imbue(shared_loc);
  int v = 0;
  is >> v;
  if (v != 9876 || !is) ++inner_failures;
  if (std::format(shared_loc, L"{:L}", 31415926) != L"31.415.926") ++inner_failures;
  --depth;
}
template<> void reenter<wchar_t>() {  // called from numpunct<wchar_t>: uses the narrow functions
  ++depth;
  ++inner_runs;
  std::ostringstream os;
  os.imbue(shared_loc);
  os << 42424242 << '|' << std::fixed;
  os.precision(2);
  os << 1234.5;
  if (os.str() != "42,424,242|1,234.50") ++inner_failures;
  std::istringstream is("9,876 x");
  is.imbue(shared_loc);
  int v = 0;
  is >> v;
  if (v != 9876 || !is) ++inner_failures;
  if (std::format(shared_loc, "{:L}", 31415926) != "31,415,926") ++inner_failures;
  --depth;
}

// Narrow: ',' groups and '.' decimal point; wide: '.' groups and ',' decimal point.
template<class C> struct Punct : std::numpunct<C> {
  C do_thousands_sep() const override { return std::is_same_v<C, char> ? C(',') : C('.'); }
  C do_decimal_point() const override { return std::is_same_v<C, char> ? C('.') : C(','); }
  std::string do_grouping() const override {
    if (depth == 0) reenter<C>();
    return "\3";
  }
  std::basic_string<C> do_truename() const override {
    // Switches the global locale twice and reads it.
    std::locale prev = std::locale::global(std::locale::classic());
    const bool classic_now = std::locale() == std::locale::classic();
    std::locale::global(prev);
    if (!classic_now || !(std::locale() == prev)) ++inner_failures;
    return std::is_same_v<C, char> ? std::basic_string<C>(1, C('y')) : std::basic_string<C>(1, C('Y'));
  }
};
struct Collate : std::collate<char> {
  int do_compare(const char* a1, const char* a2, const char* b1, const char* b2) const override {
    // Orders by numeric value, parsed with a stream (and formats again for good measure).
    std::istringstream sa(std::string(a1, a2)), sb(std::string(b1, b2));
    sa.imbue(shared_loc);
    sb.imbue(shared_loc);
    long x = 0, y = 0;
    sa >> x;
    sb >> y;
    std::ostringstream os;
    os.imbue(shared_loc);
    os << x;
    if (os.str().empty()) ++inner_failures;
    return x < y ? -1 : y < x ? 1 : 0;
  }
};

static int dtor_ok = -1;
struct DtorUser : std::locale::facet {
  static std::locale::id id;
  ~DtorUser() override {
    std::locale g = std::locale();
    std::locale prev = std::locale::global(std::locale::classic());
    std::locale::global(prev);
    std::ostringstream os;
    os.imbue(g);
    os << 5;
    dtor_ok = os.str() == "5";
  }
};
std::locale::id DtorUser::id;

int main() {
  watchdog(5);
  shared_loc = std::locale(std::locale(std::locale(std::locale::classic(), new Punct<char>), new Punct<wchar_t>), new Collate);

  {
    std::ostringstream os;
    os.imbue(shared_loc);
    os << 1234567 << ' ' << std::fixed;
    os.precision(1);
    os << 7654321.5 << ' ' << std::boolalpha << true;
    CHECK(os.str() == "1,234,567 7,654,321.5 y");
  }
  {
    std::istringstream is("1,234,567 2,000.25");
    is.imbue(shared_loc);
    int i = 0;
    double d = 0;
    is >> i >> d;
    CHECK(is);
    CHECK(i == 1234567);
    CHECK(d == 2000.25);
  }
  CHECK(std::format(shared_loc, "{:L} {:L}", 1000000, 2500000.5) == "1,000,000 2,500,000.5");
  {
    std::wostringstream ws;
    ws.imbue(shared_loc);
    ws << 1234567 << L' ' << std::fixed;
    ws.precision(1);
    ws << 7654321.5 << L' ' << std::boolalpha << true;
    CHECK(ws.str() == L"1.234.567 7.654.321,5 Y");
    std::wistringstream is(L"1.234.567 2.000,25");
    is.imbue(shared_loc);
    int i = 0;
    double d = 0;
    is >> i >> d;
    CHECK(is);
    CHECK(i == 1234567);
    CHECK(d == 2000.25);
  }
  CHECK(std::format(shared_loc, L"{:L} {:L}", 1000000, 2500000.5) == L"1.000.000 2.500.000,5");
  CHECK(inner_runs > 0);
  CHECK(inner_failures == 0);

  // A locale as a comparator whose collate facet uses streams.
  std::vector<std::string> v = {"300", "20", "1000", "5", "41", "7", "100000", "64"};
  std::sort(v.begin(), v.end(), shared_loc);
  CHECK((v == std::vector<std::string>{"5", "7", "20", "41", "64", "300", "1000", "100000"}));
  CHECK(shared_loc(std::string("9"), std::string("10")));
  CHECK(inner_failures == 0);

  // The global locale holds the only reference to a facet whose destructor uses the library.
  {
    std::locale::global(std::locale(std::locale::classic(), new DtorUser));
    CHECK(std::has_facet<DtorUser>(std::locale()));
  }
  std::locale::global(std::locale::classic());
  CHECK(dtor_ok == 1);
  CHECK(!std::has_facet<DtorUser>(std::locale()));

  shared_loc = std::locale::classic();
  return 0;
}
