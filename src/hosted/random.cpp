// libycxx hosted runtime: std::random_device ([rand.device]), on the PAL's random sources.
#include <random>
#include <system_error>
#include <ycxx/pal.h>

namespace std {

namespace {
[[noreturn]] void random_device_error(int e, const char* what) {
  throw system_error(e, system_category(), what);
}
} // namespace

random_device::random_device() {
  if (int e = ::ycxx_pal_random_open("default", 7, &handle_))
    random_device_error(e, "std::random_device: cannot open the system random source");
}

random_device::random_device(const string& token) {
  if (int e = ::ycxx_pal_random_open(token.data(), token.size(), &handle_))
    random_device_error(e, "std::random_device: unsupported or unavailable token");
}

random_device::~random_device() { ::ycxx_pal_random_close(handle_); }

random_device::result_type random_device::operator()() {
  if (avail_ == 0) {
    if (int e = ::ycxx_pal_random_read(handle_, buffer_, sizeof buffer_))
      random_device_error(e, "std::random_device: cannot read the random source");
    avail_ = buffer_size;
  }
  return buffer_[--avail_];
}

// Every source is the operating system's cryptographic generator: full entropy per bit.
double random_device::entropy() const noexcept { return numeric_limits<result_type>::digits; }

} // namespace std
