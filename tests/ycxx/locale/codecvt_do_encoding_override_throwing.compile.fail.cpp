// [locale.codecvt.general]: codecvt's virtual do_encoding() is declared noexcept, and
// [except.spec]/8: if a virtual function has a non-throwing exception specification, every
// declaration of an overrider shall have one too. An override of do_encoding without noexcept
// is therefore ill-formed.
// EXPECT-ERROR: looser exception specification|exception specification of overriding function is more lax
#include <locale>

struct bad : std::codecvt<wchar_t, char, std::mbstate_t> {
protected:
  int do_encoding() const override { return 1; }
};

int main() { return 0; }
