// [streambuf.virt.get]/7-8, [streambuf.virt.put]/1-2: basic_filebuf's xsgetn and xsputn have the
// effect of repeated sbumpc and sputc. An unbuffered filebuf (setbuf(0, 0), [filebuf.virtuals]/12)
// and a buffered one given a run longer than its buffer move the run with whole reads and writes,
// not one overflow or uflow (each one file read or write) per character: the counts of the virtual
// calls of a derived filebuf show it. Also checked: the contents, the positions (tellg/tellp),
// the putback position after a direct read, switching between reading and writing in an
// in|out file, short reads at the end of the file, and the converting (wide) unbuffered filebuf.
#include <fstream>
#include <string>
#include "fs_tmpdir.hpp"
#include "check.hpp"

template <class CharT>
struct counting : std::basic_filebuf<CharT> {
  using base = std::basic_filebuf<CharT>;
  using int_type = typename base::int_type;
  int overflows = 0, uflows = 0, underflows = 0;
  int_type overflow(int_type c) override {
    ++overflows;
    return base::overflow(c);
  }
  int_type uflow() override {
    ++uflows;
    return base::uflow();
  }
  int_type underflow() override {
    ++underflows;
    return base::underflow();
  }
  void reset() { overflows = uflows = underflows = 0; }
};

static std::string pattern(std::size_t n) {
  std::string s(n, '\0');
  for (std::size_t i = 0; i < n; ++i)
    s[i] = static_cast<char>('a' + i % 26);
  return s;
}

static std::string read_back(const std::string& name) {
  std::FILE* f = std::fopen(name.c_str(), "rb");
  CHECK(f != nullptr);
  std::string r;
  char buf[4096];
  for (std::size_t n; (n = std::fread(buf, 1, sizeof buf, f)) != 0;)
    r.append(buf, n);
  std::fclose(f);
  return r;
}

static void unbuffered_narrow(const TmpDir& dir) {
  const std::string name = dir / "unbuf";
  const std::string data = pattern(100000);
  {
    counting<char> fb;
    fb.pubsetbuf(nullptr, 0);
    CHECK(fb.open(name, std::ios_base::out) != nullptr);
    CHECK(fb.sputc('<') == '<');
    fb.reset();
    CHECK(fb.sputn(data.data(), static_cast<std::streamsize>(data.size())) == static_cast<std::streamsize>(data.size()));
    CHECK(fb.overflows == 0); // one per character before
    CHECK(fb.pubseekoff(0, std::ios_base::cur, std::ios_base::out) == std::streamoff(1 + data.size()));
    CHECK(fb.sputc('>') == '>');
    CHECK(fb.close() != nullptr);
  }
  CHECK(read_back(name) == "<" + data + ">");
  {
    counting<char> fb;
    fb.pubsetbuf(nullptr, 0);
    CHECK(fb.open(name, std::ios_base::in) != nullptr);
    CHECK(fb.sbumpc() == '<');
    fb.reset();
    std::string got(data.size(), '\0');
    CHECK(fb.sgetn(got.data(), static_cast<std::streamsize>(got.size())) == static_cast<std::streamsize>(got.size()));
    CHECK(fb.uflows == 0 && fb.underflows == 0);
    CHECK(got == data);
    CHECK(fb.pubseekoff(0, std::ios_base::cur, std::ios_base::in) == std::streamoff(1 + data.size()));
    // the last character read is the putback position
    CHECK(fb.sungetc() == static_cast<unsigned char>(data.back()));
    CHECK(fb.sbumpc() == static_cast<unsigned char>(data.back()));
    // a request past the end: what is there, then nothing
    char tail[10];
    CHECK(fb.sgetn(tail, 10) == 1 && tail[0] == '>');
    CHECK(fb.sgetn(tail, 10) == 0);
    CHECK(fb.sgetc() == EOF);
  }
}

static void buffered_narrow(const TmpDir& dir) {
  const std::string name = dir / "buf";
  const std::string data = pattern(50000);
  {
    counting<char> fb;
    CHECK(fb.open(name, std::ios_base::out) != nullptr);
    CHECK(fb.sputn("head", 4) == 4);
    fb.reset();
    CHECK(fb.sputn(data.data(), static_cast<std::streamsize>(data.size())) == static_cast<std::streamsize>(data.size()));
    CHECK(fb.overflows == 0); // about one per buffer before
    CHECK(fb.sputn("tail", 4) == 4); // short: into the buffer
    CHECK(fb.pubseekoff(0, std::ios_base::cur, std::ios_base::out) == std::streamoff(8 + data.size()));
    CHECK(fb.close() != nullptr);
  }
  CHECK(read_back(name) == "head" + data + "tail");
  {
    counting<char> fb;
    CHECK(fb.open(name, std::ios_base::in) != nullptr);
    char head[4];
    CHECK(fb.sgetn(head, 4) == 4 && std::string(head, 4) == "head");
    fb.reset();
    std::string got(data.size(), '\0');
    CHECK(fb.sgetn(got.data(), static_cast<std::streamsize>(got.size())) == static_cast<std::streamsize>(got.size()));
    CHECK(fb.uflows + fb.underflows <= 1); // about one per buffer before
    CHECK(got == data);
    CHECK(fb.pubseekoff(0, std::ios_base::cur, std::ios_base::in) == std::streamoff(4 + data.size()));
    CHECK(fb.sungetc() == static_cast<unsigned char>(data.back()));
    CHECK(fb.sbumpc() == static_cast<unsigned char>(data.back()));
    char tail[8];
    CHECK(fb.sgetn(tail, 8) == 4 && std::string(tail, 4) == "tail");
  }
}

// in|out: a bulk write after reading goes to the logical position; a bulk read after writing
// sees what was written.
static void read_write_switch(const TmpDir& dir) {
  const std::string name = dir / "rw";
  const std::string data = pattern(10000);
  for (bool unbuffered : {false, true}) {
    write_file(name, data);
    std::filebuf fb;
    if (unbuffered)
      fb.pubsetbuf(nullptr, 0);
    CHECK(fb.open(name, std::ios_base::in | std::ios_base::out) != nullptr);
    CHECK(fb.sbumpc() == 'a' && fb.sbumpc() == 'b');
    const std::string up(5000, 'X');
    CHECK(fb.sputn(up.data(), 5000) == 5000);
    std::string rest(data.size() - 5002, '\0');
    CHECK(fb.sgetn(rest.data(), static_cast<std::streamsize>(rest.size())) == static_cast<std::streamsize>(rest.size()));
    CHECK(rest == data.substr(5002));
    CHECK(fb.pubseekpos(0) == std::streampos(0));
    std::string all(data.size(), '\0');
    CHECK(fb.sgetn(all.data(), static_cast<std::streamsize>(all.size())) == static_cast<std::streamsize>(all.size()));
    CHECK(all == "ab" + up + data.substr(5002));
    CHECK(fb.close() != nullptr);
  }
}

static void unbuffered_wide(const TmpDir& dir) {
  const std::string name = dir / "wide";
  std::wstring data;
  for (int i = 0; i < 3000; ++i)
    data += i % 3 == 0 ? L'é' : i % 3 == 1 ? L'€' : L'z';
  {
    counting<wchar_t> fb;
    fb.pubsetbuf(nullptr, 0);
    CHECK(fb.open(name, std::ios_base::out) != nullptr);
    CHECK(fb.sputn(data.data(), static_cast<std::streamsize>(data.size())) == static_cast<std::streamsize>(data.size()));
    CHECK(fb.overflows == 0);
    CHECK(fb.close() != nullptr);
  }
  std::string utf8;
  for (int i = 0; i < 3000; ++i)
    utf8 += i % 3 == 0 ? "\xc3\xa9" : i % 3 == 1 ? "\xe2\x82\xac" : "z";
  CHECK(read_back(name) == utf8);
  {
    std::wfilebuf fb;
    fb.pubsetbuf(nullptr, 0);
    CHECK(fb.open(name, std::ios_base::in) != nullptr);
    std::wstring got(data.size() + 5, L'\0');
    CHECK(fb.sgetn(got.data(), static_cast<std::streamsize>(got.size())) == static_cast<std::streamsize>(data.size()));
    got.resize(data.size());
    CHECK(got == data);
  }
}

int main() {
  TmpDir dir;
  unbuffered_narrow(dir);
  buffered_narrow(dir);
  read_write_switch(dir);
  unbuffered_wide(dir);
}
