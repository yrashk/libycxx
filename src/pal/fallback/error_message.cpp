// libycxx, YCXX_PAL=none without the 'environment' hosted layer (DECISIONS §18): nothing describes
// the error numbers, so the message of error number N (generic_category().message(N),
// system_category().message(N), and system_error's what()) is "error N". Built only when the
// layer is absent; with it the provider defines ycxx_pal_error_message.
#include <ycxx/pal.h>

extern "C" int ycxx_pal_error_message(int __ev, char* __buf, ycxx_pal_size n) noexcept {
  if (n == 0)
    return 0;
  char __text[32] = "error ";
  ycxx_pal_size __len = 6;
  // The digits of |ev|, computed in the unsigned type so that INT_MIN works too.
  unsigned __u = __ev < 0 ? 0u - static_cast<unsigned>(__ev) : static_cast<unsigned>(__ev);
  char digits[12];
  int __nd = 0;
  do {
    digits[__nd++] = static_cast<char>('0' + __u % 10);
    __u /= 10;
  } while (__u != 0);
  if (__ev < 0)
    __text[__len++] = '-';
  while (__nd != 0)
    __text[__len++] = digits[--__nd];
  const ycxx_pal_size m = __len < n - 1 ? __len : n - 1;
  for (ycxx_pal_size i = 0; i < m; ++i)
    __buf[i] = __text[i];
  __buf[m] = '\0';
  return 0;
}
