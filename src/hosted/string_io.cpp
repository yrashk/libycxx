// libycxx hosted runtime: the inserters and extractors of strings and string views ([string.io],
// [string.view.io]) for char and wchar_t with std::char_traits and std::allocator, instantiated
// here.
//
// <string> declares operator<<, operator>> and getline ([string.syn]) and <string_view> declares
// operator<< ([string.view.synop]), but their definitions are in the stream headers
// (ycxx/hosted/ostream.hpp, istream.hpp), so that <string> does not include the streams. A
// translation unit that includes <string> and <iosfwd> only and uses them on a stream it is
// given by reference is valid ([using.headers]: the header that declares a function is the one
// to include) and refers to a specialization it cannot instantiate itself; these explicit
// instantiations define the specializations of std::string, std::wstring and their views for
// such translation units (doctest's, which forward-declares std::ostream instead of including
// <ostream>). A translation unit that includes the stream headers instantiates them itself.
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template basic_ostream<char>& operator<<(basic_ostream<char>&, const string&);
template basic_ostream<wchar_t>& operator<<(basic_ostream<wchar_t>&, const wstring&);
template basic_ostream<char>& operator<<(basic_ostream<char>&, string_view);
template basic_ostream<wchar_t>& operator<<(basic_ostream<wchar_t>&, wstring_view);

template basic_istream<char>& operator>>(basic_istream<char>&, string&);
template basic_istream<wchar_t>& operator>>(basic_istream<wchar_t>&, wstring&);

template basic_istream<char>& getline(basic_istream<char>&, string&, char);
template basic_istream<char>& getline(basic_istream<char>&&, string&, char);
template basic_istream<char>& getline(basic_istream<char>&, string&);
template basic_istream<char>& getline(basic_istream<char>&&, string&);
template basic_istream<wchar_t>& getline(basic_istream<wchar_t>&, wstring&, wchar_t);
template basic_istream<wchar_t>& getline(basic_istream<wchar_t>&&, wstring&, wchar_t);
template basic_istream<wchar_t>& getline(basic_istream<wchar_t>&, wstring&);
template basic_istream<wchar_t>& getline(basic_istream<wchar_t>&&, wstring&);

}} // namespace std
