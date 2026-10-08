// libycxx hosted runtime: the error category objects and system_error's members ([syserr]).
#include <system_error>
#include <ycxx/pal.h>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

error_category::~error_category() {}

}} // namespace std

namespace {

std::string error_message(int __ev) {
  char __buf[256];
  ::ycxx_pal_error_message(__ev, __buf, sizeof __buf);
  return std::string(__buf);
}

// [syserr.errcat.objects]/4: the system error values that correspond to a POSIX errno value
// are those of errc (on POSIX systems the system category's values are errno values).
bool is_posix_errno(int __ev) noexcept {
  using std::errc;
  constexpr errc posix[] = {
      errc::address_family_not_supported, errc::address_in_use, errc::address_not_available,
      errc::already_connected, errc::argument_list_too_long, errc::argument_out_of_domain,
      errc::bad_address, errc::bad_file_descriptor, errc::bad_message, errc::broken_pipe,
      errc::connection_aborted, errc::connection_already_in_progress, errc::connection_refused,
      errc::connection_reset, errc::cross_device_link, errc::destination_address_required,
      errc::device_or_resource_busy, errc::directory_not_empty, errc::executable_format_error,
      errc::file_exists, errc::file_too_large, errc::filename_too_long, errc::function_not_supported,
      errc::host_unreachable, errc::identifier_removed, errc::illegal_byte_sequence,
      errc::inappropriate_io_control_operation, errc::interrupted, errc::invalid_argument,
      errc::invalid_seek, errc::io_error, errc::is_a_directory, errc::message_size,
      errc::network_down, errc::network_reset, errc::network_unreachable, errc::no_buffer_space,
      errc::no_child_process, errc::no_link, errc::no_lock_available, errc::no_message,
      errc::no_protocol_option, errc::no_space_on_device, errc::no_such_device_or_address,
      errc::no_such_device, errc::no_such_file_or_directory, errc::no_such_process,
      errc::not_a_directory, errc::not_a_socket, errc::not_connected, errc::not_enough_memory,
      errc::not_supported, errc::operation_canceled, errc::operation_in_progress,
      errc::operation_not_permitted, errc::operation_not_supported, errc::operation_would_block,
      errc::owner_dead, errc::permission_denied, errc::protocol_error,
      errc::protocol_not_supported, errc::read_only_file_system,
      errc::resource_deadlock_would_occur, errc::resource_unavailable_try_again,
      errc::result_out_of_range, errc::state_not_recoverable, errc::text_file_busy,
      errc::timed_out, errc::too_many_files_open_in_system, errc::too_many_files_open,
      errc::too_many_links, errc::too_many_symbolic_link_levels, errc::value_too_large,
      errc::wrong_protocol_type,
      // [depr.cerrno]: no_message_available, no_stream_resources, not_a_stream, stream_timeout
      // (deprecated enumerators, named by their errno values: core's, so that this also builds
      // without the C library's <errno.h>, DECISIONS §18).
      errc(__ycxx::__detail::__errno_enodata), errc(__ycxx::__detail::__errno_enosr), errc(__ycxx::__detail::__errno_enostr),
      errc(__ycxx::__detail::__errno_etime),
  };
  for (errc e : posix)
    if (static_cast<int>(e) == __ev)
      return true;
  return false;
}

class generic_error_category final : public std::error_category {
public:
  constexpr generic_error_category() noexcept {}
  const char* name() const noexcept override { return "generic"; }
  std::string message(int __ev) const override { return error_message(__ev); }
};

class system_error_category final : public std::error_category {
public:
  constexpr system_error_category() noexcept {}
  const char* name() const noexcept override { return "system"; }
  std::string message(int __ev) const override { return error_message(__ev); }
  std::error_condition default_error_condition(int __ev) const noexcept override {
    if (__ev == 0 || is_posix_errno(__ev))
      return std::error_condition(__ev, std::generic_category());
    return std::error_condition(__ev, *this);
  }
};

// Constant-initialized, so usable from any other static initializer, and never destroyed, so
// usable from any static destructor too ([syserr.errcat.objects]: every call returns the same
// object).
template <class _Tp>
union immortal {
  _Tp __object;
  constexpr immortal() noexcept : __object() {}
  ~immortal() {}
};
constinit immortal<generic_error_category> generic_object;
constinit immortal<system_error_category> system_object;

std::string compose(const char* __what_arg, std::size_t n, const std::error_code& ec) {
  std::string s(__what_arg, n);
  if (n != 0)
    s += ": ";
  s += ec.message();
  return s;
}

} // namespace

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

const error_category& generic_category() noexcept { return generic_object.__object; }
const error_category& system_category() noexcept { return system_object.__object; }

system_error::system_error(error_code ec, const string& __what_arg)
    : runtime_error(compose(__what_arg.data(), __what_arg.size(), ec)), __code_(ec) {}
system_error::system_error(error_code ec, const char* __what_arg)
    : runtime_error(compose(__what_arg, __builtin_strlen(__what_arg), ec)), __code_(ec) {}
system_error::system_error(error_code ec) : runtime_error(ec.message()), __code_(ec) {}
system_error::system_error(int __ev, const error_category& __ecat, const string& __what_arg)
    : system_error(error_code(__ev, __ecat), __what_arg) {}
system_error::system_error(int __ev, const error_category& __ecat, const char* __what_arg)
    : system_error(error_code(__ev, __ecat), __what_arg) {}
system_error::system_error(int __ev, const error_category& __ecat) : system_error(error_code(__ev, __ecat)) {}
system_error::~system_error() {}

}} // namespace std
