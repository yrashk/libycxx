// libycxx hosted runtime: std::random_device ([rand.device]), on the PAL's random sources.
#include <random>
#include <system_error>
#include <ycxx/pal.h>

namespace [[__gnu__::__visibility__("hidden")]] std {

namespace {
[[noreturn]] void random_device_error(int e, const char* what) {
  throw system_error(e, system_category(), what);
}
} // namespace

random_device::random_device() {
  if (int e = ::__ycxx_pal_random_open("default", 7, &__handle_))
    random_device_error(e, "std::random_device: cannot open the system random source");
}

random_device::random_device(const string& token) {
  if (int e = ::__ycxx_pal_random_open(token.data(), token.size(), &__handle_))
    random_device_error(e, "std::random_device: unsupported or unavailable token");
}

random_device::~random_device() { ::__ycxx_pal_random_close(__handle_); }

random_device::result_type random_device::operator()() {
  if (__avail_ == 0) {
    if (int e = ::__ycxx_pal_random_read(__handle_, __buffer_, sizeof __buffer_))
      random_device_error(e, "std::random_device: cannot read the random source");
    __avail_ = __buffer_size;
  }
  return __buffer_[--__avail_];
}

// Every source is the operating system's cryptographic generator: full entropy per bit.
double random_device::entropy() const noexcept { return numeric_limits<result_type>::digits; }

} // namespace std
