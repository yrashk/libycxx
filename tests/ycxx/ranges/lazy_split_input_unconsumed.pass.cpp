// [range.lazy.split.outer]: over an input range, outer-iterator::operator++ marks a trailing
// empty piece only when the pattern found ends the range; the number of pieces does not depend
// on how much of each piece was read. ranges::distance reads none of them.
#include <ranges>
#include <sstream>
#include "check.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

int main() {
  {
    std::istringstream is("a;b;c");
    CHECK(rg::distance(vw::istream<char>(is) | vw::lazy_split(';')) == 3);
  }
  {
    std::istringstream is("a;b;");
    CHECK(rg::distance(vw::istream<char>(is) | vw::lazy_split(';')) == 3); // "a" "b" ""
  }
  {
    std::istringstream is("abc");
    CHECK(rg::distance(vw::istream<char>(is) | vw::lazy_split(';')) == 1);
  }
  {
    // Only the first element of each piece read.
    std::istringstream is("ab;cd");
    char firsts[4] = {};
    int n = 0;
    for (auto piece : vw::istream<char>(is) | vw::lazy_split(';')) {
      CHECK(n < 3);
      auto it = piece.begin();
      firsts[n++] = it != piece.end() ? *it : '-';
    }
    CHECK(n == 2 && firsts[0] == 'a' && firsts[1] == 'c');
  }
  {
    // Every piece read in full, with the trailing empty piece.
    std::istringstream is("x;yz;");
    int n = 0;
    long sizes[4] = {};
    for (auto piece : vw::istream<char>(is) | vw::lazy_split(';')) {
      CHECK(n < 3);
      for (auto it = piece.begin(); it != piece.end(); ++it)
        ++sizes[n];
      ++n;
    }
    CHECK(n == 3 && sizes[0] == 1 && sizes[1] == 2 && sizes[2] == 0);
  }
}
