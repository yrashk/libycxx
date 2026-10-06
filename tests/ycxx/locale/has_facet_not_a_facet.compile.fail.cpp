// [locale.facet]/1-2: a class is a facet if it is publicly derived from another facet, or derived
// from locale::facet with a publicly accessible "static ::std::locale::id id;"; passing a type
// that is not a facet to a locale function expecting a facet is ill-formed
// ([locale.global.templates]/1: use_facet Mandates that Facet is such a class).
// A class derived from locale::facet without an id is not a facet.
// EXPECT-ERROR: facet|id
#include <locale>

struct no_id : std::locale::facet {};

void f(const std::locale& l) { (void)std::has_facet<no_id>(l); }

int main() { return 0; }
