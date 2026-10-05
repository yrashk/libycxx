// The stream inserters that other headers define, writing to a stream buffer whose output
// functions throw. Each is specified as (or as equivalent to a sequence of) formatted output
// functions, so [ostream.formatted.reqmts]/1 applies: "If an exception is thrown during output,
// then ios_base::badbit is set in *this's error state. If (exceptions() & badbit) != 0 then the
// exception is rethrown." - the stream buffer's own exception (not ios_base::failure), and with
// exceptions() == goodbit no exception escapes.
//   <complex> [complex.ops]: "o << s.str()"; <bitset> [bitset.operators]: "os << x.to_string(...)";
//   <system_error> [syserr.errcode.nonmembers]: "os << ec.category().name() << ':' << ec.value()";
//   <filesystem> [fs.path.io]: "os << quoted(p.string<charT, traits>())", and quoted
//   [quoted.manip]/2-3 behaves as a formatted output function; <chrono> [time.duration.io],
//   [time.clock.system.nonmembers], [time.cal.ymd.nonmembers], [time.hms.nonmembers]:
//   "os << format(...)"/"os << s.str()"; <memory> [util.smartptr.shared.io]: "os << p.get()";
//   <iomanip> [ext.manip]/7: put_money behaves as a formatted output function; <string_view>
//   [string.view.io]: behaves as a formatted output function; <print>
//   [ostream.formatted.print]/4: behaves as a formatted output function (only an exception from
//   vformat is exempt); <ostream> [ostream.inserters.arithmetic].
// REQUIRES: exceptions
#include <bitset>
#include <chrono>
#include <complex>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <ostream>
#include <print>
#include <streambuf>
#include <string>
#include <string_view>
#include <system_error>
#include "check.hpp"

struct Boom {};
struct Thrower : std::streambuf {
  int_type overflow(int_type) override { throw Boom{}; }
  std::streamsize xsputn(const char*, std::streamsize) override { throw Boom{}; }
};

template <class F>
void check(F insert, int line) {
  Thrower buf;
  std::ostream os(&buf);
  // exceptions() == goodbit: swallowed, badbit set
  bool escaped = false;
  try {
    insert(os);
  } catch (...) {
    escaped = true;
  }
  if (escaped || !os.bad()) dprintf(2, "line %d: exceptions() == goodbit: escaped %d, bad %d\n", line, escaped, os.bad());
  CHECK(!escaped && os.bad());
  // exceptions() == badbit: the stream buffer's exception is rethrown
  os.clear();
  os.exceptions(std::ios_base::badbit);
  int kind = 0;
  try {
    insert(os);
  } catch (const Boom&) {
    kind = 1;
  } catch (const std::ios_base::failure&) {
    kind = 2;
  } catch (...) {
    kind = 3;
  }
  if (kind != 1 || !os.bad()) dprintf(2, "line %d: exceptions() == badbit: caught kind %d, bad %d\n", line, kind, os.bad());
  CHECK(kind == 1 && os.bad());
}

int main() {
  using namespace std::chrono;
  check([](std::ostream& os) { os << 42; }, __LINE__);
  check([](std::ostream& os) { os << std::string("str"); }, __LINE__);
  check([](std::ostream& os) { os << std::string_view("sv"); }, __LINE__);
  check([](std::ostream& os) { os << std::complex<double>(1, 2); }, __LINE__);
  check([](std::ostream& os) { os << std::bitset<4>(5); }, __LINE__);
  check([](std::ostream& os) { os << std::make_error_code(std::errc::io_error); }, __LINE__);
  check([](std::ostream& os) { os << std::filesystem::path("a/b"); }, __LINE__);
  check([](std::ostream& os) { os << std::quoted("q"); }, __LINE__);
  check([](std::ostream& os) { os << seconds(5); }, __LINE__);
  check([](std::ostream& os) { os << sys_days(year(2026) / 10 / 4); }, __LINE__);
  check([](std::ostream& os) { os << year(2026) / 10 / 4; }, __LINE__);
  check([](std::ostream& os) { os << hh_mm_ss(seconds(3661)); }, __LINE__);
  check([](std::ostream& os) { os << std::make_shared<int>(1); }, __LINE__);
  check([](std::ostream& os) { os << std::put_money(123.0L); }, __LINE__);
  check([](std::ostream& os) { std::print(os, "{}", 1); }, __LINE__);
  check([](std::ostream& os) { std::println(os, "{}", 1); }, __LINE__);
  check([](std::ostream& os) { std::println(os); }, __LINE__);
  return 0;
}
