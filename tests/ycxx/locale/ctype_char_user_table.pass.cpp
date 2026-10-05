// ctype<char> constructed with a program-supplied classification table.
//   [facet.ctype.char.members]/2-3: ctype(const mask* tbl = nullptr, bool del = false, size_t
//     refs = 0); /12: table() "Returns: The first constructor argument, if it was nonzero,
//     otherwise classic_table()"; /4-7: is, is(low, high, vec), scan_is and scan_not consult
//     table()[(unsigned char)c].
//   [facet.ctype.char.dtor]/1: "If the constructor's first argument was nonzero, and its second
//     argument was true, does delete [] table()" -- and otherwise does not.
//   [locale.facet]/? a facet constructed with refs == 0 is deleted when the last locale
//     containing it is destroyed (so its table, with del == true, is deleted then).
//   [classification]/1: isspace(c, loc) is use_facet<ctype<charT>>(loc).is(ctype_base::space, c).
//   [istream.sentry]/2-5: skipping whitespace classifies with the stream locale's ctype facet:
//     with ',' classified as space and ' ' not, "1,2" reads as two numbers.
// REQUIRES: exceptions
#include <istream>
#include <locale>
#include <sstream>
#include "exc_new.hpp"
#include "check.hpp"

using B = std::ctype_base;
using CT = std::ctype<char>;
// ctype's destructor is protected ([facet.ctype.special.general]): a derived facet with a
// public one, for objects the test destroys itself.
struct PubCT : CT {
  using CT::CT;
};

static B::mask* make_table() {
  auto* t = new B::mask[CT::table_size];
  const B::mask* classic = CT::classic_table();
  for (std::size_t i = 0; i < CT::table_size; ++i) t[i] = classic[i];
  t[static_cast<unsigned char>(',')] = B::space;  // ',' is space
  t[static_cast<unsigned char>(' ')] = B::punct;  // ' ' is not
  t[static_cast<unsigned char>('x')] = B::digit | B::xdigit;  // 'x' is a digit
  return t;
}

int main() {
  {
    PubCT plain;
    CHECK(plain.table() == CT::classic_table());
    PubCT null_del(nullptr, true);  // nothing to delete
    CHECK(null_del.table() == CT::classic_table());
  }
  long base = exh::new_live;
  B::mask* t = make_table();
  {
    PubCT kept(t, false, 1);
    CHECK(kept.table() == t);
    CHECK(kept.is(B::space, ',') && !kept.is(B::space, ' ') && kept.is(B::digit, 'x') && !kept.is(B::alpha, 'x'));
    const char s[] = "a, x";
    B::mask vec[4];
    CHECK(kept.is(s, s + 4, vec) == s + 4);
    CHECK(vec[0] == CT::classic_table()['a'] && vec[1] == B::space && vec[2] == B::punct && vec[3] == (B::digit | B::xdigit));
    CHECK(kept.scan_is(B::space, s, s + 4) == s + 1);
    CHECK(kept.scan_not(B::alpha | B::space, s, s + 4) == s + 2);
    CHECK(kept.scan_is(B::digit, s, s + 4) == s + 3);
    CHECK(kept.toupper('q') == 'Q' && kept.widen('z') == 'z' && kept.narrow('z', '?') == 'z');
  }
  CHECK(exh::new_live == base + 1);  // del == false: the table was not deleted
  delete[] t;
  CHECK(exh::new_live == base);

  {
    PubCT* owning = new PubCT(make_table(), true, 1);
    CHECK(exh::new_live == base + 2);
    delete owning;  // deletes its table too
    CHECK(exh::new_live == base);
  }

  {
    std::locale loc(std::locale::classic(), new CT(make_table(), true));  // refs 0: owned by locales
    CHECK(std::isspace(',', loc) && !std::isspace(' ', loc) && std::isdigit('x', loc));
    std::istringstream in("1,2,,3");
    in.imbue(loc);
    int a = 0, b = 0, c = 0;
    in >> a >> b >> c;
    CHECK(in && a == 1 && b == 2 && c == 3);
    std::istringstream sp("4 5");
    sp.imbue(loc);
    int d = 0;
    char e = 0;
    sp >> d >> e;  // ' ' is not skipped: it is read as the character
    CHECK(d == 4 && e == ' ');
  }
  CHECK(exh::new_live == base);  // the locale's facet and its table are gone
  return 0;
}
