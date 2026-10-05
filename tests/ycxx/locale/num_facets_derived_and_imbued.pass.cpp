// Numeric formatting and parsing always go through the facets of the locale in effect:
// [ostream.inserters.arithmetic]/1: the inserters call use_facet<num_put<charT,
// ostreambuf_iterator<charT, traits>>>(getloc()).put(*this, *this, fill(), v) with v of type
// long for short and int (unsigned short/unsigned int converted first when basefield is oct or
// hex), unsigned long for unsigned short and unsigned int, double for float, and the type
// itself otherwise; [istream.formatted.arithmetic]/1-3: the extractors call
// use_facet<num_get<...>>(loc).get(*this, 0, *this, state, val), via a long for short and int.
// [facet.num.put.virtuals]/1: "loc names a local variable initialized as locale loc =
// str.getloc()" -- the ios_base argument's locale decides, not the locale the facet came from;
// Stage 2: thousands_sep inserted per do_grouping, '.' replaced by decimal_point(); /6:
// boolalpha output uses truename()/falsename(). [facet.num.get.virtuals] likewise. [locale.
// members] imbue: a stream imbued mid-way formats with the new locale from then on; [ios.base.
// cons]/[locale.statics]: a stream constructed after locale::global(L) uses L.
#include <locale>
#include <iterator>
#include <sstream>
#include <string>
#include "check.hpp"

// ---- a num_put that records the overload called and marks its output ----
static std::string put_log;
struct RecPut : std::num_put<char> {
  using It = std::ostreambuf_iterator<char>;
  It tag(It out, const char* t) const {
    put_log += t;
    *out++ = '[';
    return out;
  }
  It do_put(It out, std::ios_base& s, char f, bool v) const override {
    return std::num_put<char>::do_put(tag(out, "b"), s, f, v);
  }
  It do_put(It out, std::ios_base& s, char f, long v) const override {
    return std::num_put<char>::do_put(tag(out, "l"), s, f, v);
  }
  It do_put(It out, std::ios_base& s, char f, long long v) const override {
    return std::num_put<char>::do_put(tag(out, "L"), s, f, v);
  }
  It do_put(It out, std::ios_base& s, char f, unsigned long v) const override {
    return std::num_put<char>::do_put(tag(out, "u"), s, f, v);
  }
  It do_put(It out, std::ios_base& s, char f, unsigned long long v) const override {
    return std::num_put<char>::do_put(tag(out, "U"), s, f, v);
  }
  It do_put(It out, std::ios_base& s, char f, double v) const override {
    return std::num_put<char>::do_put(tag(out, "d"), s, f, v);
  }
  It do_put(It out, std::ios_base& s, char f, long double v) const override {
    return std::num_put<char>::do_put(tag(out, "D"), s, f, v);
  }
  It do_put(It out, std::ios_base& s, char f, const void* v) const override {
    return std::num_put<char>::do_put(tag(out, "p"), s, f, v);
  }
};

// ---- a num_get that records the overload called and offsets integer results ----
static std::string get_log;
struct RecGet : std::num_get<char> {
  using It = std::istreambuf_iterator<char>;
  using St = std::ios_base::iostate;
  template <class T>
  It fwd(It in, It end, std::ios_base& s, St& e, T& v, const char* t) const {
    get_log += t;
    return std::num_get<char>::do_get(in, end, s, e, v);
  }
  It do_get(It i, It e, std::ios_base& s, St& st, bool& v) const override { return fwd(i, e, s, st, v, "b"); }
  It do_get(It i, It e, std::ios_base& s, St& st, long& v) const override {
    It r = fwd(i, e, s, st, v, "l");
    v += 1000;
    return r;
  }
  It do_get(It i, It e, std::ios_base& s, St& st, long long& v) const override { return fwd(i, e, s, st, v, "L"); }
  It do_get(It i, It e, std::ios_base& s, St& st, unsigned short& v) const override { return fwd(i, e, s, st, v, "s"); }
  It do_get(It i, It e, std::ios_base& s, St& st, unsigned int& v) const override { return fwd(i, e, s, st, v, "i"); }
  It do_get(It i, It e, std::ios_base& s, St& st, unsigned long& v) const override { return fwd(i, e, s, st, v, "u"); }
  It do_get(It i, It e, std::ios_base& s, St& st, unsigned long long& v) const override { return fwd(i, e, s, st, v, "U"); }
  It do_get(It i, It e, std::ios_base& s, St& st, float& v) const override { return fwd(i, e, s, st, v, "f"); }
  It do_get(It i, It e, std::ios_base& s, St& st, double& v) const override { return fwd(i, e, s, st, v, "d"); }
  It do_get(It i, It e, std::ios_base& s, St& st, long double& v) const override { return fwd(i, e, s, st, v, "D"); }
  It do_get(It i, It e, std::ios_base& s, St& st, void*& v) const override { return fwd(i, e, s, st, v, "p"); }
};

// ---- a numpunct that differs from "C" in every member ----
template <class C>
struct Punct : std::numpunct<C> {
  C do_decimal_point() const override { return C(','); }
  C do_thousands_sep() const override { return C('.'); }
  std::string do_grouping() const override { return "\2\3"; }
  std::basic_string<C> do_truename() const override { return {C('o'), C('u'), C('i')}; }
  std::basic_string<C> do_falsename() const override { return {C('n'), C('o'), C('n')}; }
};

static void inserters_use_num_put() {
  std::ostringstream os;
  os.imbue(std::locale(std::locale::classic(), new RecPut));
  put_log.clear();
  short s = -5;
  unsigned short us = 6;
  os << s << ' ' << -7 << ' ' << us << ' ' << 8u << ' ' << 9L << ' ' << 10UL << ' ' << 11LL << ' ' << 12ULL << ' '
     << 1.5f << ' ' << 2.5 << ' ' << 3.5L << ' ' << std::boolalpha << true << std::noboolalpha << ' '
     << static_cast<const void*>(nullptr);
  CHECK(put_log == "lluuluLUddDbp");
  std::string out = os.str();
  CHECK(out.rfind("[-5 [-7 [6 [8 [9 [10 [11 [12 [1.5 [2.5 [3.5 [true [", 0) == 0);
  // short/int in hex: converted through the unsigned type, still put as long
  put_log.clear();
  std::ostringstream hx;
  hx.imbue(std::locale(std::locale::classic(), new RecPut));
  hx << std::hex << static_cast<short>(-1) << ' ' << -1;
  CHECK(put_log == "ll");
  CHECK(hx.str() == "[ffff [ffffffff");
  // char is not numeric output
  put_log.clear();
  os << 'c' << "str";
  CHECK(put_log.empty());
}

static void extractors_use_num_get() {
  std::istringstream is("1 2 3 4 5 6 7 8 9.5 10.5 11.5 true");
  is.imbue(std::locale(std::locale::classic(), new RecGet));
  get_log.clear();
  short s = 0;
  int i = 0;
  unsigned short us = 0;
  unsigned u = 0;
  long l = 0;
  unsigned long ul = 0;
  long long ll = 0;
  unsigned long long ull = 0;
  float f = 0;
  double d = 0;
  long double ld = 0;
  bool b = false;
  is >> s >> i >> us >> u >> l >> ul >> ll >> ull >> f >> d >> ld >> std::boolalpha >> b;
  CHECK(is);
  CHECK(get_log == "llsiluLUfdDb");
  CHECK(s == 1001 && i == 1002 && us == 3 && u == 4 && l == 1005 && ul == 6 && ll == 7 && ull == 8);
  CHECK(f == 9.5f && d == 10.5 && ld == 11.5L && b);
  // short: the facet's long result is range-checked ([istream.formatted.arithmetic]/2)
  std::istringstream big("32000");
  big.imbue(std::locale(std::locale::classic(), new RecGet));
  short s2 = 0;
  big >> s2;  // 32000 + 1000 > SHRT_MAX
  CHECK(big.fail());
  CHECK(s2 == 32767);
}

static void numpunct_formatting() {
  const std::locale P(std::locale::classic(), new Punct<char>);
  std::ostringstream os;
  os.imbue(P);
  os << 1234567 << ' ' << -1234567 << ' ' << 12 << ' ' << 123 << ' ' << 1234.5 << ' ';
  os << std::fixed;
  os.precision(2);
  os << 1234567.891 << ' ' << std::boolalpha << true << ' ' << false << ' ' << std::noboolalpha << true;
  CHECK(os.str() == "12.345.67 -12.345.67 12 1.23 12.34,5 12.345.67,89 oui non 1");
  // padding counts the inserted separators ([facet.num.put.virtuals] Stage 3)
  std::ostringstream w;
  w.imbue(P);
  w.width(12);
  w.fill('*');
  w << std::internal << -1234567;
  CHECK(w.str() == "-**12.345.67");
}

static void numpunct_parsing() {
  const std::locale P(std::locale::classic(), new Punct<char>);
  std::istringstream is("12.345.67 12.34,5 oui non 1234 1.234");
  is.imbue(P);
  long a = 0;
  double b = 0;
  bool t = false, f = true;
  long c = 0;
  is >> a >> b >> std::boolalpha >> t >> f >> c;
  CHECK(is);
  CHECK(a == 1234567 && b == 1234.5 && t && !f && c == 1234);
  long bad = 0;
  is >> bad;  // "1.234": groups 1,3 where the grouping asks for 2 at the right
  CHECK(is.fail());
  CHECK(bad == 1234);  // Stage 3 stored the value; Stage 4 set failbit
  std::istringstream nb("yes");
  nb.imbue(P);
  bool x = true;
  nb >> std::boolalpha >> x;
  CHECK(nb.fail() && !x);
}

static void imbue_midway() {
  const std::locale P(std::locale::classic(), new Punct<char>);
  std::ostringstream os;
  os << std::fixed;
  os.precision(1);
  for (int round = 0; round < 3; ++round) {
    os << 1234567.5 << '|';
    os.imbue(P);
    os << 1234567.5 << '|';
    os.imbue(std::locale::classic());
  }
  CHECK(os.str() == "1234567.5|12.345.67,5|1234567.5|12.345.67,5|1234567.5|12.345.67,5|");

  std::istringstream is("1234.5 12.345.67,5 7.25 1.234,5");
  double d1 = 0, d2 = 0, d3 = 0, d4 = 0;
  is >> d1;
  is.imbue(P);
  is >> d2;
  is.imbue(std::locale::classic());
  is >> d3;
  CHECK(is && d1 == 1234.5 && d2 == 1234567.5 && d3 == 7.25);
  is >> d4;  // classic: "1.234" then ',' stops the field
  CHECK(is && d4 == 1.234);

  // a derived num_put installed mid-way, then removed again
  std::ostringstream o2;
  o2 << 5;
  o2.imbue(std::locale(std::locale::classic(), new RecPut));
  o2 << 6;
  o2.imbue(std::locale::classic());
  o2 << 7;
  CHECK(o2.str() == "5[67");
}

static void facet_uses_ios_base_locale() {
  const std::locale P(std::locale::classic(), new Punct<char>);
  // The facet comes from the classic locale; the ios_base argument carries P.
  const auto& np = std::use_facet<std::num_put<char>>(std::locale::classic());
  std::ostringstream fmt;
  fmt.imbue(P);
  std::stringbuf sb;
  np.put(std::ostreambuf_iterator<char>(&sb), fmt, ' ', 1234567L);
  np.put(std::ostreambuf_iterator<char>(&sb), fmt, ' ', 2.5);
  fmt.flags(fmt.flags() | std::ios_base::boolalpha);
  np.put(std::ostreambuf_iterator<char>(&sb), fmt, ' ', false);
  CHECK(sb.str() == "12.345.672,5non");

  const auto& ng = std::use_facet<std::num_get<char>>(std::locale::classic());
  std::istringstream src("12.345.67");
  std::istringstream parse;
  parse.imbue(P);
  std::ios_base::iostate err = std::ios_base::goodbit;
  long v = 0;
  ng.get(std::istreambuf_iterator<char>(src), std::istreambuf_iterator<char>(), parse, err, v);
  CHECK(v == 1234567);
  CHECK(err == std::ios_base::eofbit);
}

static void global_locale() {
  const std::locale P(std::locale::classic(), new Punct<char>);
  std::ostringstream before;
  std::locale old = std::locale::global(P);
  std::ostringstream after;
  before << 1234567;
  after << 1234567;
  CHECK(before.str() == "1234567");
  CHECK(after.str() == "12.345.67");
  std::locale::global(old);
  std::ostringstream restored;
  restored << 1234567;
  CHECK(restored.str() == "1234567");
}

static void wide() {
  const std::locale P(std::locale::classic(), new Punct<wchar_t>);
  std::wostringstream os;
  os << 1234567 << L' ';
  os.imbue(P);
  os << 1234567 << L' ' << 2.5 << L' ' << std::boolalpha << true;
  CHECK(os.str() == L"1234567 12.345.67 2,5 oui");
  std::wistringstream is(L"12.345.67 2,5 non");
  is.imbue(P);
  long a = 0;
  double b = 0;
  bool c = true;
  is >> a >> b >> std::boolalpha >> c;
  CHECK(is && a == 1234567 && b == 2.5 && !c);
}

int main() {
  for (int i = 0; i < 2; ++i) {
    inserters_use_num_put();
    extractors_use_num_get();
    numpunct_formatting();
    numpunct_parsing();
    imbue_midway();
    facet_uses_ios_base_locale();
    global_locale();
    wide();
  }
}
