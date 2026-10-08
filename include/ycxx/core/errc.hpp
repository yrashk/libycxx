// libycxx core: std::errc ([system.error.syn]; freestanding).
//
// [system.error.syn]/1: each enumerator has the value of the <cerrno> macro of the same meaning,
// the C library's errno number. Core cannot include <errno.h>, so the numbers are spelled out for
// each C library family libycxx supports, selected by cfg::darwin (config.hpp): the Linux numbers
// (asm-generic, which glibc and musl use on x86-64, AArch64 and RISC-V; also the default for
// bare-metal targets, which have no errno and only need distinct values) and the Darwin numbers
// (BSD, from Apple's <sys/errno.h>). The hosted <system_error> checks every value against the C
// library's macros; the freestanding <cerrno> (cerrno_macros.hpp) checks its macros against these.
#pragma once

#include <ycxx/config.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// The errno number of one meaning: Linux's or Darwin's.
consteval int __errno_number(int __linux_value, int __y_darwin_value) noexcept {
  return __cfg::__darwin ? __y_darwin_value : __linux_value;
}
// The values of the [depr.cerrno] enumerators, by name (naming the deprecated enumerators in the
// library's own checks would warn).
inline constexpr int __errno_enodata = __errno_number(61, 96); // ENODATA
inline constexpr int __errno_enosr = __errno_number(63, 98); // ENOSR
inline constexpr int __errno_enostr = __errno_number(60, 99); // ENOSTR
inline constexpr int __errno_etime = __errno_number(62, 101); // ETIME
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
// Each value is errno_number(Linux, Darwin); the comment names the <cerrno> macro.
enum class errc {
  address_family_not_supported = __ycxx::__detail::__errno_number(97, 47), // EAFNOSUPPORT
  address_in_use = __ycxx::__detail::__errno_number(98, 48), // EADDRINUSE
  address_not_available = __ycxx::__detail::__errno_number(99, 49), // EADDRNOTAVAIL
  already_connected = __ycxx::__detail::__errno_number(106, 56), // EISCONN
  argument_list_too_long = __ycxx::__detail::__errno_number(7, 7), // E2BIG
  argument_out_of_domain = __ycxx::__detail::__errno_number(33, 33), // EDOM
  bad_address = __ycxx::__detail::__errno_number(14, 14), // EFAULT
  bad_file_descriptor = __ycxx::__detail::__errno_number(9, 9), // EBADF
  bad_message = __ycxx::__detail::__errno_number(74, 94), // EBADMSG
  broken_pipe = __ycxx::__detail::__errno_number(32, 32), // EPIPE
  connection_aborted = __ycxx::__detail::__errno_number(103, 53), // ECONNABORTED
  connection_already_in_progress = __ycxx::__detail::__errno_number(114, 37), // EALREADY
  connection_refused = __ycxx::__detail::__errno_number(111, 61), // ECONNREFUSED
  connection_reset = __ycxx::__detail::__errno_number(104, 54), // ECONNRESET
  cross_device_link = __ycxx::__detail::__errno_number(18, 18), // EXDEV
  destination_address_required = __ycxx::__detail::__errno_number(89, 39), // EDESTADDRREQ
  device_or_resource_busy = __ycxx::__detail::__errno_number(16, 16), // EBUSY
  directory_not_empty = __ycxx::__detail::__errno_number(39, 66), // ENOTEMPTY
  executable_format_error = __ycxx::__detail::__errno_number(8, 8), // ENOEXEC
  file_exists = __ycxx::__detail::__errno_number(17, 17), // EEXIST
  file_too_large = __ycxx::__detail::__errno_number(27, 27), // EFBIG
  filename_too_long = __ycxx::__detail::__errno_number(36, 63), // ENAMETOOLONG
  function_not_supported = __ycxx::__detail::__errno_number(38, 78), // ENOSYS
  host_unreachable = __ycxx::__detail::__errno_number(113, 65), // EHOSTUNREACH
  identifier_removed = __ycxx::__detail::__errno_number(43, 90), // EIDRM
  illegal_byte_sequence = __ycxx::__detail::__errno_number(84, 92), // EILSEQ
  inappropriate_io_control_operation = __ycxx::__detail::__errno_number(25, 25), // ENOTTY
  interrupted = __ycxx::__detail::__errno_number(4, 4), // EINTR
  invalid_argument = __ycxx::__detail::__errno_number(22, 22), // EINVAL
  invalid_seek = __ycxx::__detail::__errno_number(29, 29), // ESPIPE
  io_error = __ycxx::__detail::__errno_number(5, 5), // EIO
  is_a_directory = __ycxx::__detail::__errno_number(21, 21), // EISDIR
  message_size = __ycxx::__detail::__errno_number(90, 40), // EMSGSIZE
  network_down = __ycxx::__detail::__errno_number(100, 50), // ENETDOWN
  network_reset = __ycxx::__detail::__errno_number(102, 52), // ENETRESET
  network_unreachable = __ycxx::__detail::__errno_number(101, 51), // ENETUNREACH
  no_buffer_space = __ycxx::__detail::__errno_number(105, 55), // ENOBUFS
  no_child_process = __ycxx::__detail::__errno_number(10, 10), // ECHILD
  no_link = __ycxx::__detail::__errno_number(67, 97), // ENOLINK
  no_lock_available = __ycxx::__detail::__errno_number(37, 77), // ENOLCK
  no_message = __ycxx::__detail::__errno_number(42, 91), // ENOMSG
  no_protocol_option = __ycxx::__detail::__errno_number(92, 42), // ENOPROTOOPT
  no_space_on_device = __ycxx::__detail::__errno_number(28, 28), // ENOSPC
  no_such_device_or_address = __ycxx::__detail::__errno_number(6, 6), // ENXIO
  no_such_device = __ycxx::__detail::__errno_number(19, 19), // ENODEV
  no_such_file_or_directory = __ycxx::__detail::__errno_number(2, 2), // ENOENT
  no_such_process = __ycxx::__detail::__errno_number(3, 3), // ESRCH
  not_a_directory = __ycxx::__detail::__errno_number(20, 20), // ENOTDIR
  not_a_socket = __ycxx::__detail::__errno_number(88, 38), // ENOTSOCK
  not_connected = __ycxx::__detail::__errno_number(107, 57), // ENOTCONN
  not_enough_memory = __ycxx::__detail::__errno_number(12, 12), // ENOMEM
  not_supported = __ycxx::__detail::__errno_number(95, 45), // ENOTSUP
  operation_canceled = __ycxx::__detail::__errno_number(125, 89), // ECANCELED
  operation_in_progress = __ycxx::__detail::__errno_number(115, 36), // EINPROGRESS
  operation_not_permitted = __ycxx::__detail::__errno_number(1, 1), // EPERM
  operation_not_supported = __ycxx::__detail::__errno_number(95, 102), // EOPNOTSUPP
  operation_would_block = __ycxx::__detail::__errno_number(11, 35), // EWOULDBLOCK
  owner_dead = __ycxx::__detail::__errno_number(130, 105), // EOWNERDEAD
  permission_denied = __ycxx::__detail::__errno_number(13, 13), // EACCES
  protocol_error = __ycxx::__detail::__errno_number(71, 100), // EPROTO
  protocol_not_supported = __ycxx::__detail::__errno_number(93, 43), // EPROTONOSUPPORT
  read_only_file_system = __ycxx::__detail::__errno_number(30, 30), // EROFS
  resource_deadlock_would_occur = __ycxx::__detail::__errno_number(35, 11), // EDEADLK
  resource_unavailable_try_again = __ycxx::__detail::__errno_number(11, 35), // EAGAIN
  result_out_of_range = __ycxx::__detail::__errno_number(34, 34), // ERANGE
  state_not_recoverable = __ycxx::__detail::__errno_number(131, 104), // ENOTRECOVERABLE
  text_file_busy = __ycxx::__detail::__errno_number(26, 26), // ETXTBSY
  timed_out = __ycxx::__detail::__errno_number(110, 60), // ETIMEDOUT
  too_many_files_open_in_system = __ycxx::__detail::__errno_number(23, 23), // ENFILE
  too_many_files_open = __ycxx::__detail::__errno_number(24, 24), // EMFILE
  too_many_links = __ycxx::__detail::__errno_number(31, 31), // EMLINK
  too_many_symbolic_link_levels = __ycxx::__detail::__errno_number(40, 62), // ELOOP
  value_too_large = __ycxx::__detail::__errno_number(75, 84), // EOVERFLOW
  wrong_protocol_type = __ycxx::__detail::__errno_number(91, 41), // EPROTOTYPE
  // [depr.cerrno] (Annex D)
  no_message_available [[deprecated("errc::no_message_available (ENODATA) is deprecated ([depr.cerrno])")]] = __ycxx::__detail::__errno_enodata,
  no_stream_resources [[deprecated("errc::no_stream_resources (ENOSR) is deprecated ([depr.cerrno])")]] = __ycxx::__detail::__errno_enosr,
  not_a_stream [[deprecated("errc::not_a_stream (ENOSTR) is deprecated ([depr.cerrno])")]] = __ycxx::__detail::__errno_enostr,
  stream_timeout [[deprecated("errc::stream_timeout (ETIME) is deprecated ([depr.cerrno])")]] = __ycxx::__detail::__errno_etime,
};
}} // namespace std
