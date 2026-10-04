// A private temporary directory for filesystem tests, created and removed with POSIX calls
// only (mkdtemp / nftw), so that setting up and cleaning up does not depend on the
// <filesystem> implementation under test. It is removed on destruction; a failing CHECK aborts
// and leaves it under $TMPDIR (or /tmp) with the prefix "ycxx-fs-".
#pragma once

#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <stdio.h>
#include <string>

class TmpDir {
public:
  TmpDir() {
    const char* base = getenv("TMPDIR");
    std::string tmpl = std::string(base && *base ? base : "/tmp") + "/ycxx-fs-XXXXXX";
    char buf[4096];
    strncpy(buf, tmpl.c_str(), sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';
    if (!mkdtemp(buf)) {
      perror("mkdtemp");
      abort();
    }
    path_ = buf;
  }
  ~TmpDir() { nftw(path_.c_str(), &TmpDir::rm, 16, FTW_DEPTH | FTW_PHYS); }
  TmpDir(const TmpDir&) = delete;
  TmpDir& operator=(const TmpDir&) = delete;

  const std::string& str() const { return path_; }
  std::string operator/(const std::string& rel) const { return path_ + "/" + rel; }

private:
  static int rm(const char* p, const struct stat*, int, struct FTW*) {
    ::remove(p);
    return 0;
  }
  std::string path_;
};

// Write a small file with POSIX stdio (independent of <fstream> and <filesystem>).
inline void write_file(const std::string& p, const std::string& content) {
  FILE* f = fopen(p.c_str(), "wb");
  if (!f) {
    perror(p.c_str());
    abort();
  }
  fwrite(content.data(), 1, content.size(), f);
  fclose(f);
}

inline std::string read_file(const std::string& p) {
  std::string r;
  FILE* f = fopen(p.c_str(), "rb");
  if (!f) return "<missing>";
  char buf[256];
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0) r.append(buf, n);
  fclose(f);
  return r;
}
