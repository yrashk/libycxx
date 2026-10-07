// The second translation unit of string/string_io_declaring_headers_only.pass.cpp: it includes
// only <iosfwd>, <string> and <string_view>, the headers that declare the string inserters,
// extractors and getline ([string.syn], [string.view.synop], [iosfwd.syn]), not the stream
// headers.
#include <iosfwd>
#include <string>
#include <string_view>

void tu2_put(std::ostream& os, const std::string& s, std::string_view v) { os << s << v; }
void tu2_wput(std::wostream& os, const std::wstring& s, std::wstring_view v) { os << s << v; }
void tu2_get(std::istream& is, std::string& word, std::string& rest, std::string& field) {
  is >> word;
  std::getline(is, rest);
  std::getline(is, field, ';');
}
void tu2_wget(std::wistream& is, std::wstring& word, std::wstring& rest, std::wstring& field) {
  is >> word;
  std::getline(is, rest);
  std::getline(is, field, L';');
}
void tu2_get_rvalue(std::istream&& is, std::string& line, std::string& field) {
  std::getline(static_cast<std::istream&&>(is), field, ';');
  std::getline(static_cast<std::istream&&>(is), line);
}
