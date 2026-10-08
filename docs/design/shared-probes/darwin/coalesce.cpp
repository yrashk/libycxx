// Weak-definition coalescing against Apple's libc++abi (DECISIONS §2: dyld coalesces each exported
// weak definition with a definition of the same name in any loaded image, a non-weak one winning).
// Built with -nostdinc++ -nostdlib++ and linked with -lc++ (so libc++abi is loaded and bound).
//   VARIANT 1  plain std, default visibility    the hazard: expected "libc++abi"  (control)
//   VARIANT 2  std::__y1, default visibility    the design's shared mode: expected "own"
//   VARIANT 3  plain std, hidden                the design's plain-std entities: expected "own"
// Prints "current_exception: own|other <image>" and "type_info: own|other <image>".
#include <dlfcn.h>
extern "C" int printf(const char*, ...);
extern "C" int strcmp(const char*, const char*);
#if VARIANT == 1
#  define OPEN namespace std {
#  define CLOSE }
#  define VIS [[gnu::visibility("default")]]
#elif VARIANT == 2
#  define OPEN namespace std { inline namespace __y1 {
#  define CLOSE }}
#  define VIS [[gnu::visibility("default")]]
#else
#  define OPEN namespace std {
#  define CLOSE }
#  define VIS [[gnu::visibility("hidden")]]
#endif
// std::type_info as libc++abi lays it out, for typeid (its vtables are libc++abi's).
namespace std { class type_info { public: virtual ~type_info(); const char* n; }; }
OPEN
struct VIS exception_ptr { void* p; };
// Inline, address taken: emitted as a weak definition with the visibility above.
VIS inline exception_ptr current_exception() noexcept { return {reinterpret_cast<void*>(0x1234)}; }
class VIS exception {
public:
  virtual ~exception() {}
  virtual const char* what() const noexcept { return "mine"; }
};
CLOSE
static const char* image(const void* p) {
  Dl_info i;
  return dladdr(p, &i) && i.dli_fname ? i.dli_fname : "?";
}
static const char* self;
int main(int, char** argv) {
  Dl_info me;
  dladdr(reinterpret_cast<void*>(&main), &me);
  self = me.dli_fname;
  std::exception_ptr (*volatile f)() noexcept = &std::current_exception;
  const char* fi = image(reinterpret_cast<const void*>(f));
  printf("current_exception: %s %s\n", strcmp(fi, self) == 0 ? "own" : "other", fi);
  const std::type_info& t = typeid(std::exception);
  const char* ti = image(&t);
  printf("type_info: %s %s\n", strcmp(ti, self) == 0 ? "own" : "other", ti);
  (void)argv;
  return 0;
}
