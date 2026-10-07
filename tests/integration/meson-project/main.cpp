#include <cstdio>
#include <string>
#include <thread>

std::string mgreet(const std::string& who);
extern "C" int c_twice(int);

int main() {
  std::string s;
  std::thread t([&] { s = mgreet("ok"); });
  t.join();
  if (s != "meson ok" || c_twice(2) != 4) return 1;
  std::printf("%s\n", s.c_str());
}
