// Whole-program integration: a word-frequency counter.
//   ofstream writes a text file; ifstream reads it back line by line (getline,
//   [istream.unformatted]/26 ff.) and word by word (operator>>, [istream.extractors]/7: skips
//   white space, stops before white space); words are folded to lower case with
//   std::tolower(c, locale::classic()) ([conversions.character]) and non-letters dropped
//   (std::erase_if, [string.erasure]); counted in an unordered_map; moved into a vector
//   (ranges::to, [range.utility.conv.to]); sorted by descending count then word (ranges::sort);
//   written with std::print to a FILE* and std::println to an ofstream ([print.fun],
//   [ostream.formatted.print]); read back and compared exactly.
//   Formatting: [format.string.std] fill/align/width; [format.range.formatter] ("[" ", " "]",
//   the n option removes the brackets); [format.tuple]/7 (pair elements formatted with their
//   debug format: strings quoted); [format.range.fmtmap]/1 ("{" "}" and ": " for map-like ranges).
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <format>
#include <locale>
#include <map>
#include <numeric>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include "fs_tmpdir.hpp"
#include "check.hpp"

static std::string slurp(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  CHECK(in.is_open());
  std::string s, line;
  while (std::getline(in, line)) s += line + "\n";
  CHECK(in.eof());
  return s;
}

int main() {
  TmpDir dir;
  const std::string text_path = dir / "text.txt";
  {
    std::ofstream out(text_path);
    CHECK(out.is_open());
    out << "The cat and the hat.\n\tThe CAT sat;  a hat!\n\n" << "and THE end";
  }

  std::unordered_map<std::string, std::size_t> counts;
  std::size_t lines = 0, words_by_line = 0;
  {
    std::ifstream in(text_path);
    std::string line;
    while (std::getline(in, line)) {
      ++lines;
      for (auto w : line | std::views::split(' ') | std::views::filter([](auto r) { return !std::ranges::empty(r); }))
        words_by_line += 1, (void)w;
    }
    CHECK(in.eof() && !in.bad());
  }
  CHECK(lines == 4);
  // "\tThe" is one token when splitting on ' ' only: The cat and the hat. | \tThe CAT sat; a hat! | and THE end
  CHECK(words_by_line == 5 + 5 + 3);
  {
    std::ifstream in(text_path);
    std::string w;
    const std::locale& c = std::locale::classic();
    while (in >> w) {
      std::erase_if(w, [&](char ch) { return !std::isalpha(ch, c); });
      for (char& ch : w) ch = std::tolower(ch, c);
      if (!w.empty()) ++counts[w];
    }
    CHECK(in.eof() && in.fail() && !in.bad());
  }
  CHECK(counts.size() == 7);

  auto v = counts | std::ranges::to<std::vector<std::pair<std::string, std::size_t>>>();
  std::ranges::sort(v, [](const auto& a, const auto& b) {
    return a.second != b.second ? a.second > b.second : a.first < b.first;
  });
  const std::vector<std::pair<std::string, std::size_t>> want = {
      {"the", 4}, {"and", 2}, {"cat", 2}, {"hat", 2}, {"a", 1}, {"end", 1}, {"sat", 1}};
  CHECK(v == want);
  CHECK(std::ranges::fold_left(v | std::views::values, std::size_t{0}, std::plus{}) == 13);

  CHECK(std::format("{}", v) == R"([("the", 4), ("and", 2), ("cat", 2), ("hat", 2), ("a", 1), ("end", 1), ("sat", 1)])");
  CHECK(std::format("{:n}", v | std::views::keys | std::views::take(3)) == R"("the", "and", "cat")");
  std::map<std::string, std::size_t> sorted(counts.begin(), counts.end());
  CHECK(std::format("{}", sorted) == R"({"a": 1, "and": 2, "cat": 2, "end": 1, "hat": 2, "sat": 1, "the": 4})");
  CHECK(std::format("{::*^7}", v | std::views::keys | std::views::drop(4)) == "[***a***, **end**, **sat**]");

  // The report, written twice (FILE* and ostream), must be identical.
  const std::string report_c = dir / "report_c.txt", report_cxx = dir / "report_cxx.txt";
  {
    std::FILE* f = std::fopen(report_c.c_str(), "w");
    CHECK(f != nullptr);
    for (const auto& [word, n] : v) std::print(f, "{:<5}|{:>3}|{}\n", word, n, std::string(n, '#'));
    std::println(f, "{:-^13}", " total ");
    std::println(f, "{:<5}|{:>3}|{:.1f}%", "uniq", v.size(), 100.0 * 7 / 13);
    CHECK(std::fclose(f) == 0);
  }
  {
    std::ofstream o(report_cxx);
    for (const auto& [word, n] : v) std::println(o, "{:<5}|{:>3}|{}", word, n, std::string(n, '#'));
    std::print(o, "{:-^13}\n", " total ");
    std::print(o, "{:<5}|{:>3}|{:.1f}%\n", "uniq", v.size(), 100.0 * 7 / 13);
    CHECK(o.good());
  }
  const std::string want_report =
      "the  |  4|####\n"
      "and  |  2|##\n"
      "cat  |  2|##\n"
      "hat  |  2|##\n"
      "a    |  1|#\n"
      "end  |  1|#\n"
      "sat  |  1|#\n"
      "--- total ---\n"
      "uniq |  7|53.8%\n";
  CHECK(slurp(report_c) == want_report);
  CHECK(slurp(report_cxx) == want_report);
  return 0;
}
