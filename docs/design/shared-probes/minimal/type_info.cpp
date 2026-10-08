// Control: typeid works with std::type_info inside std::__y1 (both compilers look it up by name
// and accept the inline namespace's member). The vtable of __class_type_info is defined here.
#ifdef PLAIN
namespace std {
#else
namespace std { inline namespace __y1 {
#endif
class type_info {
public:
  virtual ~type_info();
  const char* name() const noexcept { return n_; }
protected:
  const char* n_;
};
#ifdef PLAIN
}
#else
}}
#endif
std::type_info::~type_info() {}
namespace __cxxabiv1 {
struct __class_type_info : std::type_info { ~__class_type_info() override; };
__class_type_info::~__class_type_info() {}
struct __si_class_type_info : __class_type_info { ~__si_class_type_info() override; const __class_type_info* base; };
__si_class_type_info::~__si_class_type_info() {}
}
void operator delete(void*) noexcept {}
void operator delete(void*, decltype(sizeof 0)) noexcept {}
struct S { virtual ~S() {} };
int main() { S s; return typeid(s).name()[0] == '1' ? 0 : 1; }
