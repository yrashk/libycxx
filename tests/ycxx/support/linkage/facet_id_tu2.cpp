// Second translation unit of linkage/facet_id_across_tus.pass.cpp: installs facets whose
// template specializations (and so their static id members) are instantiated here as well.
#include <locale>

std::locale tu2_locale() {
  std::locale l(std::locale::classic(), new std::num_put<char, char*>);
  l = std::locale(l, new std::num_get<char, const char*>);
  l = std::locale(l, new std::money_put<wchar_t, wchar_t*>);
  return std::locale(l, new std::time_get<char, const char*>);
}

const void* tu2_id_addresses(int which) {
  switch (which) {
    case 0: return &std::num_put<char, char*>::id;
    case 1: return &std::ctype<char>::id;
    case 2: return &std::numpunct<wchar_t>::id;
    case 3: return &std::codecvt<char32_t, char8_t, std::mbstate_t>::id;
  }
  return nullptr;
}

bool tu2_has(const std::locale& l) { return std::has_facet<std::num_put<char, char*>>(l); }
