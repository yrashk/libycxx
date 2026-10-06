// libycxx core: std::random_device ([rand.device]).
//
// Declared in core, like the C-library parts of <string>: its members are defined in the hosted
// runtime (src/hosted/random.cpp), which reads the operating system's random source through the
// PAL (ycxx_pal_random_open/_read/_close), so a freestanding program can name the class but gets
// a link error if it uses it. Tokens: "default" (the default constructor's; the system generator,
// getrandom), "getrandom", "/dev/urandom" and "/dev/random"; any other token throws system_error.
#pragma once

#include <ycxx/core/basic_string.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/pal.h>

namespace [[__gnu__::__visibility__("hidden")]] std {

class random_device {
public:
  using result_type = unsigned int;

  static constexpr result_type min() { return numeric_limits<result_type>::min(); }
  static constexpr result_type max() { return numeric_limits<result_type>::max(); }

  random_device();
  explicit random_device(const string& token);
  ~random_device();

  result_type operator()();

  double entropy() const noexcept;

  random_device(const random_device&) = delete;
  void operator=(const random_device&) = delete;

private:
  static constexpr unsigned __buffer_size = 16;
  __ycxx_pal_handle __handle_;
  unsigned __avail_ = 0; // values left in buffer_, consumed from the end
  result_type __buffer_[__buffer_size];
};

} // namespace std
