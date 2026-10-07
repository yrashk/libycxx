// Threads, formatting, a C function and a translation unit of its own.
#include <cstdio>
#include <format>
#include <string>
#include <thread>
#include <vector>

std::string joined(const std::vector<std::string>& parts);
extern "C" int c_twice(int);

int main() {
  int total = 0;
  std::thread t([&] { total = c_twice(21); });
  t.join();
  std::string s = std::format("{}: {}", joined({"make", "ok"}), total);
  if (s != "make ok: 42") return 1;
  std::printf("%s\n", s.c_str());
}
