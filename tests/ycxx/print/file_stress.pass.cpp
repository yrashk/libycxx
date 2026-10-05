// print / println / vprint_unicode / vprint_nonunicode (and the _buffered forms) to a FILE*
// that is not a terminal, interleaved with C stdio output, under every buffering mode of
// setvbuf and with outputs from 0 to 200000 characters; then several threads printing lines to
// one FILE*.
// [print.fun]/10.2: vprint_unicode "writes out to stream unchanged" when stream is not a
// terminal; /16: vprint_nonunicode writes the formatted characters to stream; /14 and the
// unicode counterpart: the _buffered forms format into a string first and print that. /10, /16:
// both hold the lock on stream while writing ("Locks stream ... Unconditionally unlocks stream
// on function exit"), so the output of one call is contiguous: lines printed concurrently from
// several threads are never interleaved (C stdio functions lock the stream too, ISO C 7.23.2).
// print(stream, fmt, args...) and println append to what C stdio wrote before them, in order.
// FLAGS: -pthread
#include <print>
#include <cstdio>
#include <format>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include "check.hpp"

std::string read_all(std::FILE* f) {
  std::fflush(f);
  std::rewind(f);
  std::string s;
  char buf[4096];
  std::size_t n;
  while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) s.append(buf, n);
  return s;
}

std::string pattern(std::size_t n, unsigned seed) {
  std::string s(n, ' ');
  for (std::size_t i = 0; i < n; ++i) {
    seed = seed * 1103515245u + 12345u;
    s[i] = static_cast<char>('!' + (seed >> 16) % 90);  // printable, may include '{' and '}'
  }
  return s;
}

void single_thread(int mode, std::size_t bufsize) {
  std::FILE* f = std::tmpfile();
  CHECK(f != nullptr);
  std::vector<char> vbuf(bufsize + 1);
  CHECK(std::setvbuf(f, mode == _IONBF ? nullptr : vbuf.data(), mode, mode == _IONBF ? 0 : bufsize) == 0);
  std::string want;
  unsigned seed = 1;
  for (std::size_t n : {0u, 1u, 7u, 63u, 64u, 65u, 511u, 4095u, 4096u, 4097u, 65536u, 200000u}) {
    std::string s = pattern(n, seed++);
    // C stdio first
    std::fputs("<", f);
    want += "<";
    switch (seed % 6) {
      case 0: std::print(f, "{}|{:>5}|", s, n); break;
      case 1: std::println(f, "{}|{:>5}|", s, n); break;
      case 2: std::vprint_unicode(f, "{}|{:>5}|", std::make_format_args(s, n)); break;
      case 3: std::vprint_nonunicode(f, "{}|{:>5}|", std::make_format_args(s, n)); break;
      case 4: std::vprint_unicode_buffered(f, "{}|{:>5}|", std::make_format_args(s, n)); break;
      default: std::vprint_nonunicode_buffered(f, "{}|{:>5}|", std::make_format_args(s, n)); break;
    }
    want += s + "|" + std::format("{:>5}", n) + "|";
    if (seed % 6 == 1) want += "\n";
    std::fprintf(f, "%zu>", n);
    want += std::to_string(n) + ">";
    std::println(f);
    want += "\n";
  }
  // a replacement field producing a long run, and many arguments
  std::print(f, "{:*^60001}", 'x');
  want += std::string(30000, '*') + "x" + std::string(30000, '*');
  std::print(f, "{}{}{}{}{}{}{}{}{}{}{}{}", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, "ten", 11.5);
  want += "0123456789ten11.5";
  CHECK(read_all(f) == want);
  std::fclose(f);
}

void threads() {
  std::FILE* f = std::tmpfile();
  CHECK(f != nullptr);
  constexpr int T = 4, L = 300;
  std::vector<std::thread> ts;
  for (int t = 0; t < T; ++t) {
    ts.emplace_back([f, t] {
      for (int i = 0; i < L; ++i) {
        // a long line, so that the stream's buffer fills in the middle of it
        std::string body(static_cast<std::size_t>(500 + (i * 37) % 3000), static_cast<char>('a' + t));
        switch (i % 3) {
          case 0: std::println(f, "{} {} {}", t, i, body); break;
          case 1: std::vprint_unicode(f, "{} {} {}\n", std::make_format_args(t, i, body)); break;
          default: std::vprint_nonunicode(f, "{} {} {}\n", std::make_format_args(t, i, body)); break;
        }
      }
    });
  }
  for (auto& th : ts) th.join();
  std::string all = read_all(f);
  std::fclose(f);
  std::vector<int> next(T, 0);
  std::size_t pos = 0, lines = 0;
  while (pos < all.size()) {
    std::size_t nl = all.find('\n', pos);
    CHECK(nl != std::string::npos);
    std::string_view line(all.data() + pos, nl - pos);
    int t = line[0] - '0';
    CHECK(t >= 0 && t < T && line[1] == ' ');
    std::size_t sp = line.find(' ', 2);
    int i = std::stoi(std::string(line.substr(2, sp - 2)));
    CHECK(i == next[static_cast<std::size_t>(t)]++);  // each thread's lines in its own order
    std::string_view body = line.substr(sp + 1);
    CHECK(body.size() == static_cast<std::size_t>(500 + (i * 37) % 3000));
    CHECK(body.find_first_not_of(static_cast<char>('a' + t)) == std::string_view::npos);
    pos = nl + 1;
    ++lines;
  }
  CHECK(lines == static_cast<std::size_t>(T * L));
}

int main() {
  single_thread(_IONBF, 0);
  for (std::size_t b : {1u, 16u, 4096u}) {
    single_thread(_IOLBF, b);
    single_thread(_IOFBF, b);
  }
  threads();
}
