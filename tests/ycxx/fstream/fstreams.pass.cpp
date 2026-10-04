// [ifstream.cons]/2-5, [ofstream.cons], [fstream.cons]: the constructors from const char*,
// string and filesystem::path open with mode | in (ifstream), mode | out (ofstream) or mode
// (fstream; default in | out); "If that function returns a null pointer, calls
// setstate(failbit)". [ifstream.members]/1: rdbuf() returns the address of the contained
// filebuf (also from a const stream); /3 is_open(); /4: open(s, mode) calls rdbuf()->open(s,
// mode | in) and "If that function does not return a null pointer calls clear(), otherwise
// calls setstate(failbit)"; /6: close() calls rdbuf()->close() and sets failbit on a null
// result. [ofstream.members], [fstream.members] likewise. [ifstream.cons]/6: the move
// constructor installs the contained filebuf with set_rdbuf; [ifstream.swap]: swap exchanges
// the streams and the buffers.
#include <fstream>
#include <filesystem>
#include <string>
#include <utility>
#include <type_traits>
#include "fs_tmpdir.hpp"
#include "check.hpp"

using std::ios_base;

int main() {
  TmpDir dir;
  const std::string p = dir / "data.txt";
  {
    std::ofstream o(p);
    CHECK(o.is_open() && o.good());
    o << "line one\n" << 42 << ' ' << 3.5 << '\n';
    CHECK(o.rdbuf()->is_open());
  }  // destructor closes and flushes
  CHECK(read_file(p) == "line one\n42 3.5\n");
  {
    std::ifstream i(p.c_str());
    std::string w;
    std::getline(i, w);
    int n = 0;
    double d = 0;
    i >> n >> d;
    CHECK(w == "line one" && n == 42 && d == 3.5);
    const std::ifstream& ci = i;
    static_assert(std::is_same_v<decltype(ci.rdbuf()), std::filebuf*>);
    CHECK(ci.rdbuf() == i.rdbuf());
    CHECK(static_cast<std::istream&>(i).rdbuf() == i.rdbuf());
  }
  {
    // filesystem::path constructors and open
    std::filesystem::path fp(p);
    std::ifstream i(fp);
    CHECK(i.is_open());
    std::ofstream o(fp, ios_base::app);  // out | app
    o << "more";
    o.close();
    CHECK(!o.is_open() && o.good());
    CHECK(read_file(p) == "line one\n42 3.5\nmore");
    std::fstream f;
    f.open(fp, ios_base::in);
    CHECK(f.is_open() && f.get() == 'l');
  }
  {
    // ifstream adds in, ofstream adds out
    std::ifstream i(p, ios_base::binary);
    CHECK(i.is_open() && i.get() == 'l');
    const std::string q = dir / "o2";
    std::ofstream o(q, ios_base::trunc);  // out | trunc
    CHECK(o.is_open());
    o << "x";
    o.close();
    CHECK(read_file(q) == "x");
    std::ofstream o2(q, ios_base::app);
    o2 << "y";
    o2.close();
    CHECK(read_file(q) == "xy");
    // ofstream with in: in | out ("r+"), does not truncate
    std::ofstream o3(q, ios_base::in);
    CHECK(o3.is_open());
    o3 << "Z";
    o3.close();
    CHECK(read_file(q) == "Zy");
  }
  {
    // failures: failbit; a successful open clears the state
    const std::string missing = dir / "missing";
    std::ifstream i(missing);
    CHECK(!i.is_open() && i.fail() && !i.bad());
    i.open(missing);
    CHECK(i.fail());
    i.open(p);
    CHECK(i.is_open() && i.good());
    i.setstate(ios_base::eofbit);
    i.open(p);  // already open: rdbuf()->open fails
    CHECK(i.fail() && i.is_open());
    i.close();
    CHECK(!i.is_open());
    i.clear();
    i.close();  // not open: failbit
    CHECK(i.fail());
    i.open(p);
    CHECK(i.good());  // clear() after success
    std::fstream f(missing);  // in | out: "r+" does not create
    CHECK(f.fail() && !f.is_open());
    std::fstream g(missing, ios_base::in | ios_base::out | ios_base::trunc);
    CHECK(g.is_open() && g.good());
  }
  {
    // exceptions(): the failure of open throws
    std::ifstream i;
    i.exceptions(ios_base::failbit);
    bool thrown = false;
    try {
      i.open(dir / "missing2");
    } catch (const ios_base::failure&) {
      thrown = true;
    }
    CHECK(thrown);
  }
  {
    // move construction / assignment / swap
    std::ifstream a(p);
    CHECK(a.get() == 'l');
    std::ifstream b(std::move(a));
    CHECK(b.is_open() && !a.is_open());
    CHECK(b.rdbuf() != a.rdbuf());
    CHECK(static_cast<std::istream&>(b).rdbuf() == b.rdbuf());  // set_rdbuf installed it
    CHECK(b.get() == 'i');
    std::ifstream c;
    c = std::move(b);
    CHECK(c.is_open() && !b.is_open() && c.get() == 'n');
    const std::string q = dir / "other";
    write_file(q, "QRS");
    std::ifstream d(q);
    c.swap(d);
    CHECK(c.get() == 'Q' && d.get() == 'e');
    std::swap(c, d);
    CHECK(d.get() == 'R' && c.get() == ' ');
    CHECK(static_cast<std::istream&>(c).rdbuf() == c.rdbuf());
    CHECK(static_cast<std::istream&>(d).rdbuf() == d.rdbuf());
  }
  {
    // fstream reads and writes the same file
    const std::string q = dir / "rw";
    std::fstream f(q, ios_base::in | ios_base::out | ios_base::trunc);
    f << "hello world";
    f.seekg(6);
    std::string w;
    f >> w;
    CHECK(w == "world");
    f.clear();
    f.seekp(0);
    f << "J";
    f.seekg(0);
    std::getline(f, w);
    CHECK(w == "Jello world");
    f.close();
    CHECK(read_file(q) == "Jello world");
  }
  return 0;
}
