#include <string>
inline int g(const std::wstring &s) { return s.size(); }
struct P { int a; std::string b; };
static const P v[] = { {g(L"x"), std::string("ab")} };
