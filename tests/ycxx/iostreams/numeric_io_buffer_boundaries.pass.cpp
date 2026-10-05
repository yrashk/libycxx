// Arithmetic extraction and insertion through stream buffers whose get/put areas hold 1 to 9
// characters, or that have no get/put area at all (only underflow/uflow and overflow), so
// that every number crosses refill/flush boundaries at every position.
// [istream.formatted.arithmetic]: the extractors use num_get::get with istreambuf_iterators;
// [facet.num.get.virtuals] Stage 2 accumulates characters while they can continue the field,
// Stage 3 converts as strtoll/strtoull/strtod would ("the C library functions"), Stage 4 sets
// eofbit when the end of input is reached ("In any case, if stage 2 processing was terminated
// by the test for in == end then err |= ios_base::eofbit"). The character that ended the field
// stays in the stream. [ostream.inserters.arithmetic], [facet.num.put.virtuals]: the
// characters printf would produce (with width/fill/adjustfield in Stage 3), written by *out++.
// The C library (strtoll, strtod, snprintf) is the oracle.
#include <istream>
#include <ostream>
#include <streambuf>
#include <string>
#include <vector>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include "check.hpp"

// serves `data` in get areas of `chunk` characters (0: no get area)
class InBuf : public std::streambuf {
 public:
  InBuf(std::string d, std::size_t chunk) : data_(std::move(d)), chunk_(chunk) {}

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

// collects output through a put area of `chunk` characters (0: none, overflow per character)
class OutBuf : public std::streambuf {
 public:
  explicit OutBuf(std::size_t chunk) : chunk_(chunk) { reset_area(); }
  std::string str() {
    sync();
    return out_;
  }

 protected:
  int_type overflow(int_type c) override {
    sync();
    if (!traits_type::eq_int_type(c, traits_type::eof())) {
      if (chunk_ == 0) out_ += traits_type::to_char_type(c);
      else {
        *pptr() = traits_type::to_char_type(c);
        pbump(1);
      }
    }
    return traits_type::not_eof(c);
  }
  int sync() override {
    if (chunk_ != 0) {
      out_.append(pbase(), static_cast<std::size_t>(pptr() - pbase()));
      reset_area();
    }
    return 0;
  }

 private:
  void reset_area() {
    if (chunk_ != 0) setp(buf_, buf_ + chunk_);
  }
  std::string out_;
  std::size_t chunk_;
  char buf_[16];
};

static unsigned long long rng = 88172645463325252ull;
static unsigned long long rnd() {
  rng ^= rng << 13;
  rng ^= rng >> 7;
  rng ^= rng << 17;
  return rng;
}

struct Token {
  std::string text;
  bool is_float;
};

static std::vector<Token> make_tokens(int n) {
  std::vector<Token> t;
  char b[80];
  for (int i = 0; i < n; ++i) {
    switch (rnd() % 6) {
      case 0: {  // integer of 1..18 digits, maybe negative
        int digits = 1 + static_cast<int>(rnd() % 18);
        std::string s = rnd() % 2 ? "-" : "";
        s += static_cast<char>('1' + rnd() % 9);
        for (int k = 1; k < digits; ++k) s += static_cast<char>('0' + rnd() % 10);
        t.push_back({s, false});
        break;
      }
      case 1:
        std::snprintf(b, sizeof b, "%lld", static_cast<long long>(rnd()));
        t.push_back({b, false});
        break;
      case 2:
        std::snprintf(b, sizeof b, "%.17g", static_cast<double>(rnd() % 1000000) / 977.0);
        t.push_back({b, true});
        break;
      case 3:
        std::snprintf(b, sizeof b, "%.*e", static_cast<int>(rnd() % 25), static_cast<double>(rnd()) * 1e-200);
        t.push_back({b, true});
        break;
      case 4: {  // long mantissa with leading zeros
        std::string s = "000";
        int digits = 20 + static_cast<int>(rnd() % 20);
        for (int k = 0; k < digits; ++k) s += static_cast<char>('0' + rnd() % 10);
        s += ".";
        s += std::to_string(rnd() % 1000);
        s += "e-" + std::to_string(rnd() % 30);
        t.push_back({s, true});
        break;
      }
      default:
        t.push_back({std::to_string(rnd() % 100), false});
        break;
    }
  }
  return t;
}

static void input(const std::vector<Token>& toks, const char* sep, std::size_t chunk) {
  std::string text;
  for (std::size_t i = 0; i < toks.size(); ++i) {
    if (i) text += sep;
    text += toks[i].text;
  }
  InBuf sb(text, chunk);
  std::istream is(&sb);
  for (std::size_t i = 0; i < toks.size(); ++i) {
    const std::string& s = toks[i].text;
    if (toks[i].is_float) {
      double d = -1;
      is >> d;
      CHECK(!is.fail());
      CHECK(d == std::strtod(s.c_str(), nullptr));
    } else {
      long long v = -1;
      is >> v;
      errno = 0;
      long long want = std::strtoll(s.c_str(), nullptr, 10);
      CHECK(errno == 0);
      CHECK(!is.fail());
      CHECK(v == want);
    }
    const bool last = i + 1 == toks.size();
    CHECK(is.eof() == last);  // eofbit exactly when the field ended at the end of input
    if (!last) CHECK(is.peek() == static_cast<unsigned char>(sep[0]));  // the separator remains
  }
  int extra = 0;
  is.clear();
  is >> extra;
  CHECK(is.fail() && is.eof());
}

static void output(const std::vector<Token>& toks, std::size_t chunk) {
  OutBuf sb(chunk);
  std::ostream os(&sb);
  std::string want;
  char b[200];
  int i = 0;
  for (const Token& t : toks) {
    const int w = static_cast<int>(rnd() % 30);
    os.width(w);
    if (t.is_float) {
      double d = std::strtod(t.text.c_str(), nullptr);
      const int prec = static_cast<int>(rnd() % 20);
      os.precision(prec);
      if (i % 2) {
        os << std::left << d;
        std::snprintf(b, sizeof b, "%-*.*g", w, prec, d);
      } else {
        os << std::right << d;
        std::snprintf(b, sizeof b, "%*.*g", w, prec, d);
      }
    } else {
      long long v = std::strtoll(t.text.c_str(), nullptr, 10);
      os << std::right << v;
      std::snprintf(b, sizeof b, "%*lld", w, v);
    }
    want += b;
    os << '|';
    want += '|';
    ++i;
  }
  CHECK(os.good());
  CHECK(sb.str() == want);
}

int main() {
  for (int round = 0; round < 40; ++round) {
    std::vector<Token> toks = make_tokens(1 + round % 9 * 3);
    for (std::size_t chunk = 0; chunk <= 9; ++chunk) {
      input(toks, " ", chunk);
      input(toks, "\n\t  ", chunk);
      output(toks, chunk);
    }
  }
}
