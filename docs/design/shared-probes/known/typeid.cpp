// typeid needs std::type_info; a failed typeid of a null polymorphic glvalue throws std::bad_typeid
// (__cxa_bad_typeid); a failed dynamic_cast to a reference throws std::bad_cast (__cxa_bad_cast).
#include <typeinfo>
#include <cstdio>
struct B { virtual ~B() = default; };
struct D : B {};
struct E : B {};
int main() {
  int r = 0;
  B* volatile null = nullptr;
  E e;
  B& b = e;
  r |= typeid(int) == typeid(int) && typeid(b) == typeid(E) ? 1 : 0;
  try { (void)typeid(*null); } catch (const std::bad_typeid&) { r |= 2; } catch (...) {}
  try { (void)dynamic_cast<D&>(b); } catch (const std::bad_cast&) { r |= 4; } catch (...) {}
  r |= typeid(typeid(int)) != typeid(int) ? 8 : 0;
  std::puts(r == 15 ? "ok" : "FAIL");
  return r == 15 ? 0 : 1;
}
