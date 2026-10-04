// [charconv.syn]: "to_chars_result to_chars(char* first, char* last, bool value, int base =
// 10) = delete;"
#include <charconv>

int main() {
  char buf[8];
  auto r = std::to_chars(buf, buf + 8, true);
  return r.ptr == buf;
}
