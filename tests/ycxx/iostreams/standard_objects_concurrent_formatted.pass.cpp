// [iostream.objects.overview]/7: "Concurrent access to a synchronized ([ios.members.static])
// standard iostream object's formatted and unformatted input ([istream]) and output
// ([ostream]) functions or a standard C stream by multiple threads does not result in a data
// race" (characters may interleave, Note 2). The formatted output functions read the stream's
// formatting state (flags, width, precision, fill, locale) and reset the width
// ([ostream.formatted.reqmts], [facet.num.put.virtuals]: width(0) after a numeric conversion;
// [ostream.inserters.character]: width(0)), so their bookkeeping must not race when several
// threads use the same standard object without changing its state.
// A child process runs threads that all write through every formatted inserter of [ostream]
// (arithmetic types, bool, nullptr, characters, character arrays, strings, string
// views, print/println, the endl/ends/flush manipulators) and the unformatted put/write/flush,
// to cout (mode "out"), to cerr and clog (mode "err"), to wcout (mode "wout") and to wcerr and
// wclog (mode "werr"). The parent checks that the captured text holds exactly the characters
// that the threads' texts hold (each thread's text formatted once by an ostringstream; the
// interleaving is not checked). Run under TSan this also checks for data races.
// FLAGS: -pthread
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <latch>
#include <map>
#include <ostream>
#include <print>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include "child_process.hpp"
#include "check.hpp"

constexpr int K = 4;
constexpr int R = 400;

// What thread k writes in one round; the same function writes to the standard object and,
// in the parent, to an ostringstream whose text is the expectation.
template <class CharT, class Os>
void round_out(Os& os, int k, int r) {
  const int i = k * 1000 + r;
  os << i << ' ' << static_cast<long long>(i) * 1000003 << ' ' << static_cast<unsigned>(i) << ' '
     << static_cast<short>(-i % 30000) << ' ' << static_cast<unsigned long>(i) * 7 << ' ';
  os << (i * 0.25) << ' ' << static_cast<float>(i) / 8 << ' ' << static_cast<long double>(i) / 3 << ' ';
  os << (r % 2 == 0) << ' ' << nullptr << ' ';
  if constexpr (sizeof(CharT) == 1) {
    os << 'c' << 's' << 'u' << "lit"
       << std::string("str") << std::string_view("sv") << ' ';
  } else {
    os << L'c' << 'n' << L"lit" << "nar" << std::wstring(L"str") << std::wstring_view(L"sv") << ' ';
  }
  os.put(static_cast<CharT>('p'));
  const CharT w[3] = {static_cast<CharT>('w'), static_cast<CharT>('r'), static_cast<CharT>('t')};
  os.write(w, 3);
  if (r % 50 == 0) os << std::flush;
  if (r % 7 == 0) os << std::endl;
  else os << static_cast<CharT>('\n');
}

template <class CharT>
std::basic_string<CharT> expected_text() {
  std::basic_ostringstream<CharT> o;
  for (int k = 0; k < K; ++k)
    for (int r = 0; r < R; ++r) round_out<CharT>(o, k, r);
  return o.str();
}

// print/println to cout: thread k writes "<k>:<r>;" per round
static std::string print_expected() {
  std::string s;
  for (int k = 0; k < K; ++k)
    for (int r = 0; r < R; ++r) s += std::to_string(k) + ':' + std::to_string(r) + ";\n";
  return s;
}

template <class CharT, class Os>
void run_threads(Os& a, Os& b, bool print) {
  std::latch go(K);
  std::vector<std::thread> ts;
  for (int k = 0; k < K; ++k)
    ts.emplace_back([&, k] {
      Os& os = (k % 2 == 0) ? a : b;
      go.arrive_and_wait();
      for (int r = 0; r < R; ++r) {
        round_out<CharT>(os, k, r);
        if constexpr (sizeof(CharT) == 1) {
          if (print) {
            if (r % 2 == 0) std::println(os, "{}:{};", k, r);
            else std::print(os, "{}:{};\n", k, r);
          }
        }
        if (r % 3 == 0) os.flush();
      }
    });
  for (auto& t : ts) t.join();
  a.flush();
  b.flush();
}

static int child(const std::string& mode) {
  if (mode == "out") run_threads<char>(std::cout, std::cout, true);
  else if (mode == "err") run_threads<char>(std::cerr, std::clog, true);
  else if (mode == "wout") run_threads<wchar_t>(std::wcout, std::wcout, false);
  else if (mode == "werr") run_threads<wchar_t>(std::wcerr, std::wclog, false);
  else return 3;
  return 0;
}

static std::map<char, long> histogram(const std::string& s) {
  std::map<char, long> h;
  for (char c : s) ++h[c];
  return h;
}

static void verify(const ChildResult& r, bool on_err, std::string want) {
  if (r.status != 0) dprintf(2, "child status %d, stderr:\n%s\n", r.status, r.err.c_str());
  CHECK(r.status == 0);
  const std::string& got = on_err ? r.err : r.out;
  CHECK((on_err ? r.out : r.err).empty());
  if (got.size() != want.size()) dprintf(2, "got %zu characters, want %zu\n", got.size(), want.size());
  CHECK(histogram(got) == histogram(want));
}

static std::string narrow(const std::wstring& w) {
  std::string s;
  for (wchar_t c : w) s += static_cast<char>(c);  // ASCII only
  return s;
}

int main(int argc, char** argv) {
  if (child_mode()) return child(argv[1]);
  (void)argc;
  const std::string text = expected_text<char>();
  verify(run_self("out"), false, text + print_expected());
  verify(run_self("err"), true, text + print_expected());
  const std::string wtext = narrow(expected_text<wchar_t>());
  CHECK(wtext.size() > 1000);
  verify(run_self("wout"), false, wtext);
  verify(run_self("werr"), true, wtext);
}
