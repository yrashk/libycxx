// [format.string.std] Table 111: pointers (void*, const void*, nullptr_t) format as
// to_chars(reinterpret_cast<uintptr_t>(value), 16) with a 0x prefix (P: uppercase digits
// and 0X); right aligned by default (Table 104); the 0 option pads after the prefix (/8).
#include <cstdint>
#include <cstdio>
#include <format>
#include <string>
#include "check.hpp"

int main() {
  CHECK(std::format("{}", nullptr) == "0x0");
  CHECK(std::format("{:p}", nullptr) == "0x0");
  CHECK(std::format("{:P}", nullptr) == "0X0");
  CHECK(std::format("{:6}", nullptr) == "   0x0");
  CHECK(std::format("{:<6}|", nullptr) == "0x0   |");
  CHECK(std::format("{:06}", nullptr) == "0x0000");
  void* p = reinterpret_cast<void*>(static_cast<std::uintptr_t>(0xabc123));
  const void* cp = p;
  CHECK(std::format("{}", p) == "0xabc123");
  CHECK(std::format("{:p}", cp) == "0xabc123");
  CHECK(std::format("{:P}", p) == "0XABC123");
  CHECK(std::format("{:012}", p) == "0x0000abc123");
  CHECK(std::format("{:*^12}", cp) == "**0xabc123**");
  CHECK(std::format(L"{}", p) == L"0xabc123");
  int x = 0;
  char expect[32];
  std::snprintf(expect, sizeof expect, "0x%llx",
                static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(static_cast<void*>(&x))));
  CHECK(std::format("{}", static_cast<void*>(&x)) == expect);
}
