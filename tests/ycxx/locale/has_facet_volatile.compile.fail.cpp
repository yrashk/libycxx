// [locale.facet]/2: "A program that passes ... a type that refers to a volatile-qualified facet,
// as an (explicit or deduced) template parameter to a locale function expecting a facet, is
// ill-formed." has_facet ([locale.global.templates]/5) is such a function.
// EXPECT-ERROR: volatile
#include <locale>

bool f(const std::locale& l) { return std::has_facet<volatile std::numpunct<char>>(l); }

int main() { return 0; }
