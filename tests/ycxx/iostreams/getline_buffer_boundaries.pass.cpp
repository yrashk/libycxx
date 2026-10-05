// getline (both forms), get(s, n, delim) and ignore(n, delim) through stream buffers that hand
// out their characters in get areas of 1 to 9 characters, or with no get area at all (only
// underflow/uflow), so that delimiters and line ends fall on every refill boundary.
// [string.io]/getline(is, str, delim): characters are appended until end-of-file (eofbit), the
// delimiter ("extracted but not appended") or str.max_size(); failbit if nothing was extracted.
// [istream.unformatted]/18-21 getline(s, n, delim): stops at end-of-file, then at the delimiter
// (extracted, not stored), then when n - 1 characters are stored (failbit) -- "tested in the
// order shown", so a line of exactly n - 1 characters followed by the delimiter does not fail;
// failbit if nothing was extracted; a null character is stored; gcount() counts the delimiter.
// get(s, n, delim) leaves the delimiter in the stream. ignore(n, delim) extracts up to n
// characters, stopping after the delimiter.
#include <istream>
#include <streambuf>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include "check.hpp"

// serves `data` in get areas of `chunk` characters (0: no get area, underflow/uflow only)
class ChunkBuf : public std::streambuf {
 public:
  ChunkBuf(std::string d, std::size_t chunk) : data_(std::move(d)), chunk_(chunk) {}

 protected:
  int_type underflow() override {
    if (chunk_ == 0) return pos_ < data_.size() ? traits_type::to_int_type(data_[pos_]) : traits_type::eof();
    if (gptr() < egptr()) return traits_type::to_int_type(*gptr());
    if (pos_ >= data_.size()) return traits_type::eof();
    const std::size_t n = std::min(chunk_, data_.size() - pos_);
    std::memcpy(buf_, data_.data() + pos_, n);
    pos_ += n;
    setg(buf_, buf_, buf_ + n);
    return traits_type::to_int_type(*gptr());
  }
  int_type uflow() override {
    if (chunk_ != 0) return std::streambuf::uflow();
    if (pos_ >= data_.size()) return traits_type::eof();
    return traits_type::to_int_type(data_[pos_++]);
  }

 private:
  std::string data_;
  std::size_t chunk_, pos_ = 0;
  char buf_[16];
};

static std::string make_text(unsigned seed, std::vector<std::string>& lines, char delim, bool final_delim) {
  std::string text;
  unsigned x = seed;
  const int count = 1 + static_cast<int>(seed % 7);
  for (int i = 0; i < count; ++i) {
    x = x * 1103515245u + 12345u;
    const std::size_t len = (x >> 16) % 23;
    std::string line;
    for (std::size_t k = 0; k < len; ++k) line += static_cast<char>('a' + (k + i) % 20);  // never the delimiter 'z'
    lines.push_back(line);
    text += line;
    if (i + 1 < count || final_delim) text += delim;
  }
  return text;
}

int main() {
  for (unsigned seed = 1; seed < 60; ++seed) {
    for (char delim : {'\n', 'z', '\0'}) {
      for (bool final_delim : {false, true}) {
        std::vector<std::string> lines;
        const std::string text = make_text(seed, lines, delim, final_delim);
        for (std::size_t chunk = 0; chunk <= 9; ++chunk) {
          // std::getline
          {
            ChunkBuf sb(text, chunk);
            std::istream is(&sb);
            std::string s;
            std::vector<std::string> got;
            while (std::getline(is, s, delim)) got.push_back(s);
            CHECK(got == lines || (lines.back().empty() && !final_delim &&
                                   std::vector<std::string>(lines.begin(), lines.end() - 1) == got));
            CHECK(is.eof() && is.fail());
          }
          // member getline with every buffer size around the line lengths
          for (std::streamsize n = 1; n <= 25; n += (n < 6 ? 1 : 4)) {
            ChunkBuf sb(text, chunk);
            std::istream is(&sb);
            for (std::size_t li = 0; li < lines.size(); ++li) {
              const std::string& line = lines[li];
              const bool has_delim = li + 1 < lines.size() || final_delim;
              char out[32];
              std::memset(out, '#', sizeof out);
              is.getline(out, n, delim);
              const auto stored = std::min<std::size_t>(line.size(), static_cast<std::size_t>(n - 1));
              CHECK(std::memcmp(out, line.data(), stored) == 0 && out[stored] == '\0');
              if (line.size() > static_cast<std::size_t>(n - 1)) {
                // too long: n - 1 stored, failbit; the rest stays (skip it to resynchronise)
                CHECK(is.fail() && is.gcount() == n - 1);
                is.clear();
                is.ignore(1000, delim);
              } else if (has_delim) {
                CHECK(!is.fail() && is.gcount() == static_cast<std::streamsize>(line.size()) + 1);
              } else {
                CHECK(is.gcount() == static_cast<std::streamsize>(line.size()));
                CHECK(is.fail() == line.empty());  // nothing extracted at the very end
              }
              if (is.fail()) break;
            }
          }
          // get(s, n, delim) + ignore(1) walks the same lines
          {
            ChunkBuf sb(text, chunk);
            std::istream is(&sb);
            std::vector<std::string> got;
            char out[64];
            for (std::size_t li = 0; li < lines.size(); ++li) {
              is.get(out, sizeof out, delim);
              got.push_back(out);
              CHECK(is.gcount() == static_cast<std::streamsize>(lines[li].size()));
              is.clear();
              if (li + 1 < lines.size() || final_delim) CHECK(is.get() == static_cast<unsigned char>(delim));
            }
            CHECK(got == lines);
          }
          // ignore(n, delim) counts and stops after the delimiter
          {
            ChunkBuf sb(text, chunk);
            std::istream is(&sb);
            is.ignore(static_cast<std::streamsize>(lines[0].size()) + 5, delim);
            const bool first_has_delim = lines.size() > 1 || final_delim;
            CHECK(is.gcount() == static_cast<std::streamsize>(lines[0].size()) + (first_has_delim ? 1 : 0));
          }
        }
      }
    }
  }
  return 0;
}
