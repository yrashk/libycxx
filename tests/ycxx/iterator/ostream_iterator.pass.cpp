// [ostream.iterator.general]: member types (output_iterator_tag, value_type void,
// difference_type ptrdiff_t, pointer void, reference void, char_type, traits_type,
// ostream_type); there is no default constructor. [ostream.iterator.cons.des]: the
// constructors store addressof(s) and the delimiter (or nullptr). [ostream.iterator.ops]/1:
// it = value is "*out_stream << value; if (delim) *out_stream << delim; return *this;" /2-3:
// *it, ++it and it++ return *this. The iterator models output_iterator<It, const T&>
// ([iterator.concept.output]) and works with the algorithms.
#include <iterator>
#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <type_traits>
#include "check.hpp"

using It = std::ostream_iterator<int>;
static_assert(std::is_same_v<It::iterator_category, std::output_iterator_tag>);
static_assert(std::is_same_v<It::value_type, void>);
static_assert(std::is_same_v<It::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<It::pointer, void>);
static_assert(std::is_same_v<It::reference, void>);
static_assert(std::is_same_v<It::char_type, char>);
static_assert(std::is_same_v<It::traits_type, std::char_traits<char>>);
static_assert(std::is_same_v<It::ostream_type, std::ostream>);
static_assert(std::is_same_v<std::ostream_iterator<int, wchar_t>::ostream_type, std::wostream>);
static_assert(!std::is_default_constructible_v<It>);
static_assert(std::is_copy_constructible_v<It> && std::is_copy_assignable_v<It>);
static_assert(std::output_iterator<It, const int&>);
static_assert(std::is_same_v<decltype(*std::declval<It&>()), It&>);
static_assert(std::is_same_v<decltype(++std::declval<It&>()), It&>);
static_assert(std::is_same_v<decltype(std::declval<It&>()++), It&>);
static_assert(std::is_same_v<decltype(std::declval<It&>() = 1), It&>);

struct Pt {
  int x, y;
};
std::ostream& operator<<(std::ostream& os, const Pt& p) { return os << '(' << p.x << ',' << p.y << ')'; }

int main() {
  {
    std::ostringstream os;
    It it(os);
    CHECK(&*it == &it && &++it == &it && &it++ == &it);
    CHECK(&(it = 5) == &it);
    it = 12;
    CHECK(os.str() == "512");  // no delimiter
  }
  {
    std::ostringstream os;
    It it(os, ", ");
    *it++ = 1;
    *it++ = 2;
    it = 3;
    CHECK(os.str() == "1, 2, 3, ");  // the delimiter after every element
  }
  {
    // the delimiter is written with operator<< (as a C string), also when empty
    std::ostringstream os;
    It it(os, "");
    it = 7;
    it = 8;
    CHECK(os.str() == "78");
  }
  {
    // copies write to the same stream
    std::ostringstream os;
    It a(os, "|");
    It b = a;
    a = 1;
    b = 2;
    It c(os);
    c = a;  // copy assignment: now with the delimiter
    c = 3;
    CHECK(os.str() == "1|2|3|");
  }
  {
    // algorithms; formatting state of the stream applies
    std::ostringstream os;
    os << std::hex;
    int v[] = {10, 11, 255};
    std::copy(v, v + 3, It(os, " "));
    CHECK(os.str() == "a b ff ");
    std::ostringstream os2;
    Pt pts[] = {{1, 2}, {3, 4}};
    std::ranges::copy(pts, std::ostream_iterator<Pt>(os2, ";"));
    CHECK(os2.str() == "(1,2);(3,4);");
    std::ostringstream os3;
    std::ranges::copy(std::string("abc"), std::ostream_iterator<char>(os3, "-"));
    CHECK(os3.str() == "a-b-c-");
  }
  {
    std::wostringstream os;
    std::ostream_iterator<double, wchar_t> it(os, L" ");
    it = 1.5;
    it = -2;
    CHECK(os.str() == L"1.5 -2 ");
  }
  return 0;
}
