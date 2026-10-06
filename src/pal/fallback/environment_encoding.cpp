// libycxx, YCXX_PAL=none without the 'environment' hosted layer (DECISIONS §18): nothing names the
// environment's character encoding, so text_encoding::environment() is unknown
// ([text.encoding.members]/14 allows that: the result is then id::unknown). Built only when the
// layer is absent; with it the provider defines ycxx_pal_environment_encoding.
#include <ycxx/pal.h>

extern "C" int ycxx_pal_environment_encoding(char* buf, ycxx_pal_size n) noexcept {
  if (n != 0)
    buf[0] = '\0';
  return 0;
}
