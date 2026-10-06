// [locale.facet]/2: "A program that passes ... a type that refers to a volatile-qualified facet,
// as an (explicit or deduced) template parameter to a locale function expecting a facet, is
// ill-formed."
// EXPECT-ERROR: volatile
#include <locale>

void f(const std::locale& l) { (void)std::use_facet<volatile std::ctype<char>>(l); }

int main() { return 0; }
