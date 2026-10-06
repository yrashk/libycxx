// libycxx, YCXX_PAL=none without the 'environment' hosted layer (DECISIONS §18): nothing describes
// the error numbers, so the message of error number N (generic_category().message(N),
// system_category().message(N), and system_error's what()) is "error N". Built only when the
// layer is absent; with it the provider defines ycxx_pal_error_message.
#include <ycxx/pal.h>

extern "C" int ycxx_pal_error_message(int ev, char* buf, ycxx_pal_size n) noexcept {
  if (n == 0)
    return 0;
  char text[32] = "error ";
  ycxx_pal_size len = 6;
  // The digits of |ev|, computed in the unsigned type so that INT_MIN works too.
  unsigned u = ev < 0 ? 0u - static_cast<unsigned>(ev) : static_cast<unsigned>(ev);
  char digits[12];
  int nd = 0;
  do {
    digits[nd++] = static_cast<char>('0' + u % 10);
    u /= 10;
  } while (u != 0);
  if (ev < 0)
    text[len++] = '-';
  while (nd != 0)
    text[len++] = digits[--nd];
  const ycxx_pal_size m = len < n - 1 ? len : n - 1;
  for (ycxx_pal_size i = 0; i < m; ++i)
    buf[i] = text[i];
  buf[m] = '\0';
  return 0;
}
