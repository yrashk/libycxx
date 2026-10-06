// Example C: a 'files' hosted layer of the program's own. libycxx is built with YCXX_PAL=none
// and the layers clib (the host's C library), memory and files: std::ofstream, std::ifstream and
// std::fstream run on ramdisk.c, a RAM disk on the program's heap, instead of the host's file
// system (examples/hosted-layers/README.md). With the C library present, iostreams, locales and
// std::print to stdout work as usual.
#include <cstdio>
#include <fstream>
#include <iostream>
#include <print>
#include <sstream>
#include <string>
#include <vector>

extern "C" long ramdisk_file_size(const char* name);

int main() {
  int failures = 0;
  auto check = [&](bool ok, const char* what) {
    if (!ok) {
      ++failures;
      std::println("  FAILED: {}", what);
    }
  };
  const char* notes = "hosted-layers-ramdisk-notes.txt";

  {
    std::ofstream out(notes);
    check(out.is_open(), "ofstream opens a new file");
    out << "line one\n" << 42 << ' ' << 3.5 << '\n';
  }
  {
    std::ofstream out(notes, std::ios::app);
    out << "appended\n";
  }
  std::vector<std::string> lines;
  {
    std::ifstream in(notes);
    for (std::string line; std::getline(in, line);)
      lines.push_back(line);
  }
  check(lines == std::vector<std::string>{"line one", "42 3.5", "appended"}, "ifstream reads back what was written");
  std::println("{} on the RAM disk: {} bytes, lines {}", notes, ramdisk_file_size(notes), lines);

  {
    std::fstream io("data.bin", std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
    io.write("abcdef", 6);
    io.seekg(2);
    char c = 0;
    io.get(c);
    check(c == 'c', "fstream seeks and reads");
    io.seekp(0, std::ios::end);
    io << "gh";
  }
  check(ramdisk_file_size("data.bin") == 8, "fstream appends after seeking to the end");

  std::ifstream missing("no-such-file.txt");
  check(!missing.is_open(), "a missing file does not open");
  // The files are on the RAM disk, not on the host's file system (the C library's fopen).
  std::FILE* host = std::fopen(notes, "r");
  check(host == nullptr, "the host's file system does not have the file");
  if (host != nullptr)
    std::fclose(host);

  std::ostringstream os;
  os << "iostreams and locales work as usual, through the C library: " << 1.25;
  std::cout << os.str() << '\n';

  if (failures == 0)
    std::println("hosted-layers files demo: ok");
  return failures == 0 ? 0 : 1;
}
