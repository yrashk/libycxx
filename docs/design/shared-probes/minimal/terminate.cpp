// Clang's __clang_call_terminate (the handler of an exception leaving a noexcept function) calls
// std::terminate by its mangled name, _ZSt9terminatev, whatever the program declares: with
// std::__y1::terminate defined and plain std::terminate not, the link fails.
#ifdef PLAIN
namespace std { [[noreturn]] void terminate() noexcept; }
namespace std { [[noreturn]] void terminate() noexcept { __builtin_trap(); } }
#else
namespace std { inline namespace __y1 { [[noreturn]] void terminate() noexcept; } }
namespace std { inline namespace __y1 { [[noreturn]] void terminate() noexcept { __builtin_trap(); } } }
#endif
// What the noexcept function's handler needs besides std::terminate (never called here).
extern "C" [[gnu::weak]] void* __cxa_begin_catch(void*) noexcept { return nullptr; }
extern "C" [[gnu::weak]] void __cxa_call_terminate(void*) noexcept {}
extern "C" [[gnu::weak]] int __gxx_personality_v0() { return 0; }
extern "C" [[gnu::weak]] void _Unwind_Resume(void*) {}
void may_throw();
void wrapper() noexcept { may_throw(); }
void may_throw() {}
int main() { wrapper(); return 0; }
