// [syncstream.syncbuf.members]/1-2: emit() transfers the output "so that it appears in the
// output stream as a contiguous sequence of characters", and "All emit() calls transferring
// characters to the same stream buffer object appear to execute in a total order". Lines
// written by several threads through their own osyncstreams are never interleaved.
// FLAGS: -pthread
#include <syncstream>
#include <sstream>
#include <thread>
#include <vector>
#include <string>
#include "check.hpp"

int main() {
  std::ostringstream target;
  std::vector<std::thread> ts;
  for (int t = 0; t < 4; ++t)
    ts.emplace_back([&target, t] {
      for (int i = 0; i < 200; ++i) {
        std::osyncstream out(target);
        out << "thread" << t << ':';
        for (int k = 0; k < 10; ++k) out << static_cast<char>('a' + t);
        out << '\n';
      }
    });
  for (auto& th : ts) th.join();
  std::istringstream lines(target.str());
  std::string line;
  int count = 0;
  while (std::getline(lines, line)) {
    ++count;
    CHECK(line.size() == 18);
    int t = line[6] - '0';
    CHECK(line.compare(0, 6, "thread") == 0 && line[7] == ':');
    CHECK(line.substr(8) == std::string(10, static_cast<char>('a' + t)));
  }
  CHECK(count == 800);
  return 0;
}
