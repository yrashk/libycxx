// Whole-program integration: a log analyser.
//   <filesystem>: create_directories, recursive_directory_iterator (visits every entry once, in
//     unspecified order: [fs.class.rec.dir.itr]; the test sorts), path::extension,
//     lexically_relative, generic_string ([fs.path.gen], [fs.path.generic.obs]), file_size.
//   <fstream>/<string>: getline over each file.
//   <regex>: ECMAScript regex_match with captures ([re.alg.match]), regex_iterator over
//     key=value pairs ([re.regiter]).
//   <chrono>: parse("%Y-%m-%dT%H:%M:%SZ", sys_seconds) through operator>> ([time.parse]/2;
//     an invalid date such as month 13 sets failbit: [time.parse]/17 "If ... fails to decode
//     a valid ..."); floor<days>; formatting "{:%F %T}", "{:%a}" (C locale names,
//     [time.format] Table 133), durations "{}" ([time.duration.io]/1: "86402s", "1d").
//   <locale>/<format>: format(loc, "{:L}", n) uses loc's numpunct grouping and thousands
//     separator ([format.string.std]/17).
//   <map>, <algorithm>: grouping and a stable, deterministic ordering.
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <locale>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include "fs_tmpdir.hpp"
#include "check.hpp"

namespace fs = std::filesystem;
namespace chr = std::chrono;

struct Event {
  chr::sys_seconds t;
  std::string level, file, msg;
  std::map<std::string, std::string> kv;
};

struct Grouping : std::numpunct<char> {
  char do_thousands_sep() const override { return ','; }
  std::string do_grouping() const override { return "\3"; }
};

static void write(const fs::path& p, const std::string& s) {
  std::ofstream o(p, std::ios::binary);
  CHECK(o.is_open());
  o << s;
}

int main() {
  TmpDir tmp;
  const fs::path root = fs::path(tmp.str()) / "logs";
  CHECK(fs::create_directories(root / "sub"));
  const std::string f1 =
      "2026-10-04T12:00:00Z INFO  started service=web\n"
      "2026-10-04T12:00:05Z WARN  slow request ms=1500\n"
      "garbage line\n"
      "2026-10-04T12:01:00Z ERROR failed code=500 retry=3\n";
  write(root / "app-1.log", f1);
  write(root / "app-2.log",
        "2026-10-04T11:59:58Z INFO  boot version=26\n"
        "2026-10-04T12:00:05Z INFO  tick n=1\n"
        "2026-13-01T00:00:00Z INFO  bad month\n"
        "2026-10-05T00:00:01Z WARN  rollover day=2");  // no final newline
  write(root / "sub" / "app-3.log", "2026-10-03T23:59:59Z ERROR late code=404\n");
  write(root / "notes.txt", "2026-10-04T12:00:00Z INFO  ignored\n");

  std::vector<std::string> files;
  for (const fs::directory_entry& e : fs::recursive_directory_iterator(root))
    if (e.is_regular_file() && e.path().extension() == ".log") files.push_back(e.path().lexically_relative(root).generic_string());
  std::ranges::sort(files);
  CHECK((files == std::vector<std::string>{"app-1.log", "app-2.log", "sub/app-3.log"}));
  CHECK(fs::file_size(root / "app-1.log") == f1.size());

  const std::regex line_re(R"(^(\S+) (INFO|WARN|ERROR) +(.*)$)");
  const std::regex kv_re(R"((\w+)=(\w+))");
  std::vector<Event> events;
  int unparsed = 0;
  for (const std::string& rel : files) {
    std::ifstream in(root / rel);
    std::string line;
    while (std::getline(in, line)) {
      std::smatch m;
      if (!std::regex_match(line, m, line_re)) {
        ++unparsed;
        continue;
      }
      Event ev;
      std::istringstream ts(m[1].str());
      ts >> chr::parse("%Y-%m-%dT%H:%M:%SZ", ev.t);
      if (!ts) {
        ++unparsed;
        continue;
      }
      ev.level = m[2];
      ev.file = rel;
      std::string rest = m[3];
      for (std::sregex_iterator it(rest.begin(), rest.end(), kv_re), end; it != end; ++it)
        ev.kv[(*it)[1]] = (*it)[2];
      auto first_kv = rest.find('=');
      ev.msg = rest.substr(0, first_kv == std::string::npos ? rest.size() : rest.rfind(' ', first_kv));
      events.push_back(std::move(ev));
    }
  }
  CHECK(unparsed == 2);
  CHECK(events.size() == 7);
  std::ranges::stable_sort(events, {}, [](const Event& e) { return std::tie(e.t, e.file); });

  std::string report;
  for (const Event& e : events)
    report += std::format("{:%F %T} {:%a} {:<5} {:<13} {}\n", e.t, e.t, e.level, e.file, e.msg);
  CHECK(report ==
        "2026-10-03 23:59:59 Sat ERROR sub/app-3.log late\n"
        "2026-10-04 11:59:58 Sun INFO  app-2.log     boot\n"
        "2026-10-04 12:00:00 Sun INFO  app-1.log     started\n"
        "2026-10-04 12:00:05 Sun WARN  app-1.log     slow request\n"
        "2026-10-04 12:00:05 Sun INFO  app-2.log     tick\n"
        "2026-10-04 12:01:00 Sun ERROR app-1.log     failed\n"
        "2026-10-05 00:00:01 Mon WARN  app-2.log     rollover\n");

  auto span = events.back().t - events.front().t;
  CHECK(std::format("{} {}", span, chr::floor<chr::days>(span)) == "86402s 1d");

  std::map<std::string, int> per_level;
  std::map<chr::sys_days, int> per_day;
  long code_sum = 0;
  for (const Event& e : events) {
    ++per_level[e.level];
    ++per_day[chr::floor<chr::days>(e.t)];
    if (auto it = e.kv.find("code"); it != e.kv.end()) code_sum += std::stol(it->second);
  }
  CHECK(std::format("{}", per_level) == R"({"ERROR": 2, "INFO": 3, "WARN": 2})");
  std::string days;
  for (auto [d, n] : per_day) days += std::format("{:%F}={} ", d, n);
  CHECK(days == "2026-10-03=1 2026-10-04=5 2026-10-05=1 ");
  CHECK(code_sum == 904);
  CHECK(chr::year_month_day(per_day.rbegin()->first) == chr::year(2026) / 10 / 5);

  std::locale grouped(std::locale::classic(), new Grouping);
  long ms = std::stol(events[3].kv.at("ms"));
  CHECK(std::format(grouped, "{:L} {:L} {}", ms, 1234567, ms) == "1,500 1,234,567 1500");
  CHECK(std::format(std::locale::classic(), "{:L}", ms) == "1500");
  return 0;
}
