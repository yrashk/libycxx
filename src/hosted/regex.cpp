// libycxx hosted runtime: regex_error's members and the name tables of regex_traits ([re.badexp],
// [re.traits]).
#include <regex>

namespace {

struct name_entry {
  const char* name;
  int value;
};

bool __same(const char* a, const char* b, std::size_t n) noexcept {
  for (std::size_t i = 0; i < n; ++i)
    if (a[i] != b[i] || b[i] == '\0')
      return false;
  return b[n] == '\0';
}

// [re.traits] Table 121.
constexpr name_entry class_names[] = {
    {"alnum", std::ctype_base::alnum},   {"alpha", std::ctype_base::alpha}, {"blank", std::ctype_base::blank},
    {"cntrl", std::ctype_base::cntrl},   {"digit", std::ctype_base::digit}, {"d", std::ctype_base::digit},
    {"graph", std::ctype_base::graph},   {"lower", std::ctype_base::lower}, {"print", std::ctype_base::print},
    {"punct", std::ctype_base::punct},   {"space", std::ctype_base::space}, {"s", std::ctype_base::space},
    {"upper", std::ctype_base::upper},   {"xdigit", std::ctype_base::xdigit},
    // alnum plus the underscore; upper and lower are subsets of alnum, included so that the mask
    // also holds them on a C library whose ctype masks give alnum a bit of its own.
    {"w", static_cast<int>(std::ctype_base::alnum | std::ctype_base::upper | std::ctype_base::lower |
                           __ycxx::__detail::__regex_word_bit)},
};

// The collating-symbol names of the POSIX portable character set (Base Definitions, 6.1 and
// the control characters of 6.3) with the characters they stand for.
constexpr name_entry collate_names[] = {
    {"NUL", 0x00}, {"SOH", 0x01}, {"STX", 0x02}, {"ETX", 0x03}, {"EOT", 0x04}, {"ENQ", 0x05}, {"ACK", 0x06},
    {"BEL", 0x07}, {"alert", 0x07}, {"BS", 0x08}, {"backspace", 0x08}, {"HT", 0x09}, {"tab", 0x09},
    {"LF", 0x0A}, {"newline", 0x0A}, {"VT", 0x0B}, {"vertical-tab", 0x0B}, {"FF", 0x0C}, {"form-feed", 0x0C},
    {"CR", 0x0D}, {"carriage-return", 0x0D}, {"SO", 0x0E}, {"SI", 0x0F}, {"DLE", 0x10}, {"DC1", 0x11},
    {"DC2", 0x12}, {"DC3", 0x13}, {"DC4", 0x14}, {"NAK", 0x15}, {"SYN", 0x16}, {"ETB", 0x17}, {"CAN", 0x18},
    {"EM", 0x19}, {"SUB", 0x1A}, {"ESC", 0x1B}, {"IS4", 0x1C}, {"FS", 0x1C}, {"IS3", 0x1D}, {"GS", 0x1D},
    {"IS2", 0x1E}, {"RS", 0x1E}, {"IS1", 0x1F}, {"US", 0x1F}, {"space", 0x20}, {"exclamation-mark", 0x21},
    {"quotation-mark", 0x22}, {"number-sign", 0x23}, {"dollar-sign", 0x24}, {"percent-sign", 0x25},
    {"ampersand", 0x26}, {"apostrophe", 0x27}, {"left-parenthesis", 0x28}, {"right-parenthesis", 0x29},
    {"asterisk", 0x2A}, {"plus-sign", 0x2B}, {"comma", 0x2C}, {"hyphen", 0x2D}, {"hyphen-minus", 0x2D},
    {"period", 0x2E}, {"full-stop", 0x2E}, {"slash", 0x2F}, {"solidus", 0x2F}, {"zero", 0x30}, {"one", 0x31},
    {"two", 0x32}, {"three", 0x33}, {"four", 0x34}, {"five", 0x35}, {"six", 0x36}, {"seven", 0x37},
    {"eight", 0x38}, {"nine", 0x39}, {"colon", 0x3A}, {"semicolon", 0x3B}, {"less-than-sign", 0x3C},
    {"equals-sign", 0x3D}, {"greater-than-sign", 0x3E}, {"question-mark", 0x3F}, {"commercial-at", 0x40},
    {"left-square-bracket", 0x5B}, {"backslash", 0x5C}, {"reverse-solidus", 0x5C},
    {"right-square-bracket", 0x5D}, {"circumflex", 0x5E}, {"circumflex-accent", 0x5E}, {"underscore", 0x5F},
    {"low-line", 0x5F}, {"grave-accent", 0x60}, {"left-brace", 0x7B}, {"left-curly-bracket", 0x7B},
    {"vertical-line", 0x7C}, {"right-brace", 0x7D}, {"right-curly-bracket", 0x7D}, {"tilde", 0x7E},
    {"DEL", 0x7F},
};

} // namespace

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

unsigned __regex_class_by_name(const char* name, std::size_t n, bool icase) noexcept {
  for (const name_entry& e : class_names) {
    if (!__same(name, e.name, n))
      continue;
    unsigned m = static_cast<unsigned>(e.value);
    // [re.traits] footnote 217: without regard to case, [[:lower:]] is the same as [[:alpha:]].
    if (icase && (m == std::ctype_base::lower || m == std::ctype_base::upper))
      m = std::ctype_base::alpha;
    return m;
  }
  return 0;
}

int __regex_collate_by_name(const char* name, std::size_t n) noexcept {
  for (const name_entry& e : collate_names)
    if (__same(name, e.name, n))
      return e.value;
  return -1;
}

const char* __regex_error_message(int code) noexcept {
  switch (code) {
  case 1: return "regex_error: invalid collating element name";
  case 2: return "regex_error: invalid character class name";
  case 3: return "regex_error: invalid escaped character or trailing escape";
  case 4: return "regex_error: invalid back reference";
  case 5: return "regex_error: mismatched [ and ]";
  case 6: return "regex_error: mismatched ( and )";
  case 7: return "regex_error: mismatched { and }";
  case 8: return "regex_error: invalid range in a {} expression";
  case 9: return "regex_error: invalid character range";
  case 10: return "regex_error: insufficient memory to convert the expression into a finite state machine";
  case 11: return "regex_error: one of *?+{ is not preceded by a valid regular expression";
  case 12: return "regex_error: the complexity of the match exceeds the pre-set level";
  case 13: return "regex_error: insufficient memory to determine whether the expression matches";
  default: return "regex_error: unknown error";
  }
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

regex_error::regex_error(regex_constants::error_type __ecode)
    : runtime_error(::__ycxx::__detail::__regex_error_message(__ecode)), __code_(__ecode) {}
regex_error::~regex_error() {}

}} // namespace std
