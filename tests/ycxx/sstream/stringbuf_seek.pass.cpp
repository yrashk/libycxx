// [stringbuf.virtuals]/9-12, Table 144/145: seekoff positions the input sequence (in), the
// output sequence (out) or both (in|out with beg or end); "Otherwise the positioning
// operation fails" (in|out with cur); fails if newoff + off < 0 or refers to an uninitialized
// character (beyond the high mark), returning pos_type(off_type(-1)). seekpos likewise.
// Observed through the stream's tellg/tellp/seekg/seekp.
#include <sstream>
#include <string>
#include "check.hpp"

using pos = std::stringbuf::pos_type;
using off = std::stringbuf::off_type;

int main() {
  std::stringbuf sb("0123456789");
  CHECK(sb.pubseekoff(3, std::ios_base::beg, std::ios_base::in) == pos(3));
  CHECK(sb.sgetc() == '3');
  CHECK(sb.pubseekoff(2, std::ios_base::cur, std::ios_base::in) == pos(5));
  CHECK(sb.sgetc() == '5');
  CHECK(sb.pubseekoff(-1, std::ios_base::end, std::ios_base::in) == pos(9));
  CHECK(sb.sgetc() == '9');
  CHECK(sb.pubseekoff(-1, std::ios_base::beg, std::ios_base::in) == pos(off(-1)));
  CHECK(sb.pubseekoff(1, std::ios_base::end, std::ios_base::in) == pos(off(-1)));  // beyond high mark
  CHECK(sb.pubseekoff(0, std::ios_base::cur, std::ios_base::in | std::ios_base::out) == pos(off(-1)));
  CHECK(sb.sgetc() == '9');  // unchanged by the failures

  CHECK(sb.pubseekoff(4, std::ios_base::beg, std::ios_base::out) == pos(4));
  sb.sputc('x');
  CHECK(sb.str() == "0123x56789");
  CHECK(sb.pubseekoff(0, std::ios_base::end, std::ios_base::in | std::ios_base::out) == pos(10));
  sb.sputc('!');
  CHECK(sb.str() == "0123x56789!");
  CHECK(sb.sgetc() == '!');  // [stringbuf.virtuals]/8: overflow moves egptr() past the new character
  CHECK(sb.pubseekpos(2) == pos(2));
  CHECK(sb.sgetc() == '2');
  sb.sputc('#');
  CHECK(sb.str() == "01#3x56789!");
  CHECK(sb.pubseekpos(12) == pos(off(-1)));

  std::stringstream ss("abcdef");
  ss.seekg(2);
  CHECK(ss.tellg() == pos(2));
  char c;
  ss >> c;
  CHECK(c == 'c');
  ss.seekp(0, std::ios_base::end);
  ss << "gh";
  CHECK(ss.str() == "abcdefgh");
  CHECK(ss.tellp() == pos(8));
  ss.seekg(-2, std::ios_base::end);
  std::string rest;
  ss >> rest;
  CHECK(rest == "gh");
  ss.clear();
  ss.seekg(100);
  CHECK(ss.fail());

  // a seek on an empty buffer: newoff 0 is fine, nonzero fails (null next pointer allowed)
  std::stringbuf empty;
  CHECK(empty.pubseekoff(0, std::ios_base::beg, std::ios_base::in) == pos(0));
  CHECK(empty.pubseekoff(1, std::ios_base::beg, std::ios_base::in) == pos(off(-1)));
  return 0;
}
