// [print.fun]/1,3,4: print(fmt, args...) and println(...) write to stdout; println() writes a
// newline (C++26). stdout is redirected to a temporary file with freopen-free dup2 so the
// output can be checked.
#include <print>
#include <cstdio>
#include <string>
#include <unistd.h>
#include "check.hpp"

int main() {
  std::FILE* tmp = std::tmpfile();
  CHECK(tmp != nullptr);
  std::fflush(stdout);
  int saved = dup(1);
  CHECK(dup2(fileno(tmp), 1) == 1);
  std::print("x={} ", 1);
  std::println("y={}", 2);
  std::println();
  std::fflush(stdout);
  CHECK(dup2(saved, 1) == 1);
  close(saved);
  std::rewind(tmp);
  std::string r;
  int c;
  while ((c = std::fgetc(tmp)) != EOF) r += static_cast<char>(c);
  std::fclose(tmp);
  CHECK(r == "x=1 y=2\n\n");
  return 0;
}
