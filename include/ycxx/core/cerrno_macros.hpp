// libycxx core: the error-number macros of <cerrno> ([cerrno.syn]; freestanding).
//
// Freestanding <cerrno> has no C library to take them from: the values are std::errc's (the
// target's errno numbers, ycxx/core/errc.hpp), which the hosted <system_error> checks against the
// C library's <errno.h>. They must be usable in #if, so they are literals here, one list per C
// library family (YCXX_TARGET_DARWIN, config.hpp), checked against errc below. errno itself is
// not freestanding.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/errc.hpp>

#if YCXX_TARGET_DARWIN
#  define E2BIG 7
#  define EACCES 13
#  define EADDRINUSE 48
#  define EADDRNOTAVAIL 49
#  define EAFNOSUPPORT 47
#  define EAGAIN 35
#  define EALREADY 37
#  define EBADF 9
#  define EBADMSG 94
#  define EBUSY 16
#  define ECANCELED 89
#  define ECHILD 10
#  define ECONNABORTED 53
#  define ECONNREFUSED 61
#  define ECONNRESET 54
#  define EDEADLK 11
#  define EDESTADDRREQ 39
#  define EDOM 33
#  define EEXIST 17
#  define EFAULT 14
#  define EFBIG 27
#  define EHOSTUNREACH 65
#  define EIDRM 90
#  define EILSEQ 92
#  define EINPROGRESS 36
#  define EINTR 4
#  define EINVAL 22
#  define EIO 5
#  define EISCONN 56
#  define EISDIR 21
#  define ELOOP 62
#  define EMFILE 24
#  define EMLINK 31
#  define EMSGSIZE 40
#  define ENAMETOOLONG 63
#  define ENETDOWN 50
#  define ENETRESET 52
#  define ENETUNREACH 51
#  define ENFILE 23
#  define ENOBUFS 55
#  define ENODATA 96 // [depr.cerrno]
#  define ENODEV 19
#  define ENOENT 2
#  define ENOEXEC 8
#  define ENOLCK 77
#  define ENOLINK 97
#  define ENOMEM 12
#  define ENOMSG 91
#  define ENOPROTOOPT 42
#  define ENOSPC 28
#  define ENOSR 98 // [depr.cerrno]
#  define ENOSTR 99 // [depr.cerrno]
#  define ENOSYS 78
#  define ENOTCONN 57
#  define ENOTDIR 20
#  define ENOTEMPTY 66
#  define ENOTRECOVERABLE 104
#  define ENOTSOCK 38
#  define ENOTSUP 45
#  define ENOTTY 25
#  define ENXIO 6
#  define EOPNOTSUPP 102
#  define EOVERFLOW 84
#  define EOWNERDEAD 105
#  define EPERM 1
#  define EPIPE 32
#  define EPROTO 100
#  define EPROTONOSUPPORT 43
#  define EPROTOTYPE 41
#  define ERANGE 34
#  define EROFS 30
#  define ESPIPE 29
#  define ESRCH 3
#  define ETIME 101 // [depr.cerrno]
#  define ETIMEDOUT 60
#  define ETXTBSY 26
#  define EWOULDBLOCK 35
#  define EXDEV 18
#else
#  define E2BIG 7
#  define EACCES 13
#  define EADDRINUSE 98
#  define EADDRNOTAVAIL 99
#  define EAFNOSUPPORT 97
#  define EAGAIN 11
#  define EALREADY 114
#  define EBADF 9
#  define EBADMSG 74
#  define EBUSY 16
#  define ECANCELED 125
#  define ECHILD 10
#  define ECONNABORTED 103
#  define ECONNREFUSED 111
#  define ECONNRESET 104
#  define EDEADLK 35
#  define EDESTADDRREQ 89
#  define EDOM 33
#  define EEXIST 17
#  define EFAULT 14
#  define EFBIG 27
#  define EHOSTUNREACH 113
#  define EIDRM 43
#  define EILSEQ 84
#  define EINPROGRESS 115
#  define EINTR 4
#  define EINVAL 22
#  define EIO 5
#  define EISCONN 106
#  define EISDIR 21
#  define ELOOP 40
#  define EMFILE 24
#  define EMLINK 31
#  define EMSGSIZE 90
#  define ENAMETOOLONG 36
#  define ENETDOWN 100
#  define ENETRESET 102
#  define ENETUNREACH 101
#  define ENFILE 23
#  define ENOBUFS 105
#  define ENODATA 61 // [depr.cerrno]
#  define ENODEV 19
#  define ENOENT 2
#  define ENOEXEC 8
#  define ENOLCK 37
#  define ENOLINK 67
#  define ENOMEM 12
#  define ENOMSG 42
#  define ENOPROTOOPT 92
#  define ENOSPC 28
#  define ENOSR 63 // [depr.cerrno]
#  define ENOSTR 60 // [depr.cerrno]
#  define ENOSYS 38
#  define ENOTCONN 107
#  define ENOTDIR 20
#  define ENOTEMPTY 39
#  define ENOTRECOVERABLE 131
#  define ENOTSOCK 88
#  define ENOTSUP 95
#  define ENOTTY 25
#  define ENXIO 6
#  define EOPNOTSUPP 95
#  define EOVERFLOW 75
#  define EOWNERDEAD 130
#  define EPERM 1
#  define EPIPE 32
#  define EPROTO 71
#  define EPROTONOSUPPORT 93
#  define EPROTOTYPE 91
#  define ERANGE 34
#  define EROFS 30
#  define ESPIPE 29
#  define ESRCH 3
#  define ETIME 62 // [depr.cerrno]
#  define ETIMEDOUT 110
#  define ETXTBSY 26
#  define EWOULDBLOCK 11
#  define EXDEV 18
#endif

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
consteval bool errno_macros_match_errc() {
  using std::errc;
  const struct {
    errc e;
    int v;
  } table[] = {
      {errc::address_family_not_supported, EAFNOSUPPORT},
      {errc::address_in_use, EADDRINUSE},
      {errc::address_not_available, EADDRNOTAVAIL},
      {errc::already_connected, EISCONN},
      {errc::argument_list_too_long, E2BIG},
      {errc::argument_out_of_domain, EDOM},
      {errc::bad_address, EFAULT},
      {errc::bad_file_descriptor, EBADF},
      {errc::bad_message, EBADMSG},
      {errc::broken_pipe, EPIPE},
      {errc::connection_aborted, ECONNABORTED},
      {errc::connection_already_in_progress, EALREADY},
      {errc::connection_refused, ECONNREFUSED},
      {errc::connection_reset, ECONNRESET},
      {errc::cross_device_link, EXDEV},
      {errc::destination_address_required, EDESTADDRREQ},
      {errc::device_or_resource_busy, EBUSY},
      {errc::directory_not_empty, ENOTEMPTY},
      {errc::executable_format_error, ENOEXEC},
      {errc::file_exists, EEXIST},
      {errc::file_too_large, EFBIG},
      {errc::filename_too_long, ENAMETOOLONG},
      {errc::function_not_supported, ENOSYS},
      {errc::host_unreachable, EHOSTUNREACH},
      {errc::identifier_removed, EIDRM},
      {errc::illegal_byte_sequence, EILSEQ},
      {errc::inappropriate_io_control_operation, ENOTTY},
      {errc::interrupted, EINTR},
      {errc::invalid_argument, EINVAL},
      {errc::invalid_seek, ESPIPE},
      {errc::io_error, EIO},
      {errc::is_a_directory, EISDIR},
      {errc::message_size, EMSGSIZE},
      {errc::network_down, ENETDOWN},
      {errc::network_reset, ENETRESET},
      {errc::network_unreachable, ENETUNREACH},
      {errc::no_buffer_space, ENOBUFS},
      {errc::no_child_process, ECHILD},
      {errc::no_link, ENOLINK},
      {errc::no_lock_available, ENOLCK},
      {errc::no_message, ENOMSG},
      {errc::no_protocol_option, ENOPROTOOPT},
      {errc::no_space_on_device, ENOSPC},
      {errc::no_such_device_or_address, ENXIO},
      {errc::no_such_device, ENODEV},
      {errc::no_such_file_or_directory, ENOENT},
      {errc::no_such_process, ESRCH},
      {errc::not_a_directory, ENOTDIR},
      {errc::not_a_socket, ENOTSOCK},
      {errc::not_connected, ENOTCONN},
      {errc::not_enough_memory, ENOMEM},
      {errc::not_supported, ENOTSUP},
      {errc::operation_canceled, ECANCELED},
      {errc::operation_in_progress, EINPROGRESS},
      {errc::operation_not_permitted, EPERM},
      {errc::operation_not_supported, EOPNOTSUPP},
      {errc::operation_would_block, EWOULDBLOCK},
      {errc::owner_dead, EOWNERDEAD},
      {errc::permission_denied, EACCES},
      {errc::protocol_error, EPROTO},
      {errc::protocol_not_supported, EPROTONOSUPPORT},
      {errc::read_only_file_system, EROFS},
      {errc::resource_deadlock_would_occur, EDEADLK},
      {errc::resource_unavailable_try_again, EAGAIN},
      {errc::result_out_of_range, ERANGE},
      {errc::state_not_recoverable, ENOTRECOVERABLE},
      {errc::text_file_busy, ETXTBSY},
      {errc::timed_out, ETIMEDOUT},
      {errc::too_many_files_open_in_system, ENFILE},
      {errc::too_many_files_open, EMFILE},
      {errc::too_many_links, EMLINK},
      {errc::too_many_symbolic_link_levels, ELOOP},
      {errc::value_too_large, EOVERFLOW},
      {errc::wrong_protocol_type, EPROTOTYPE},
  };
  for (const auto& entry : table)
    if (static_cast<int>(entry.e) != entry.v)
      return false;
  return ENODATA == errno_enodata && ENOSR == errno_enosr && ENOSTR == errno_enostr && ETIME == errno_etime;
}
static_assert(ycxx::detail::errno_macros_match_errc(), "libycxx: the freestanding <cerrno> macros differ from std::errc");
}} // namespace ycxx::detail
