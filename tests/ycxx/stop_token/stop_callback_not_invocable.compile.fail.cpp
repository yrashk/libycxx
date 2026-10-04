// [stopcallback.general]/2: "Mandates: stop_callback is instantiated with an argument for the
// template parameter CallbackFn that satisfies both invocable and destructible."
#include <stop_token>

struct NotInvocable {};
std::stop_callback<NotInvocable>* p;
void f() { (void)sizeof(std::stop_callback<NotInvocable>); }
