// libycxx core: the error-number macros of <cerrno> ([cerrno.syn]; freestanding).
//
// Freestanding <cerrno> has no C library to take them from: the values are std::errc's (the
// target's errno numbers, ycxx/core/errc.hpp), which the hosted <system_error> checks against the
// C library's <errno.h>. They must be usable in #if, so they are literals here, one list per C
// library family (_YCXX_TARGET_DARWIN, config.hpp), checked against errc below. errno itself is
// not freestanding.
// Hosted <cerrno> includes this after <errno.h> too: each macro is defined only if the C library
// did not define it, which it may not do under a strict feature-test macro ([cerrno.syn] defines
// them all; Darwin's <errno.h> hides EOWNERDEAD and ENOTRECOVERABLE under _XOPEN_SOURCE 600,
// libstdc++'s PR 93151 test).
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/errc.hpp>

#if _YCXX_TARGET_DARWIN
#  ifndef E2BIG
#    define E2BIG 7
#  endif
#  ifndef EACCES
#    define EACCES 13
#  endif
#  ifndef EADDRINUSE
#    define EADDRINUSE 48
#  endif
#  ifndef EADDRNOTAVAIL
#    define EADDRNOTAVAIL 49
#  endif
#  ifndef EAFNOSUPPORT
#    define EAFNOSUPPORT 47
#  endif
#  ifndef EAGAIN
#    define EAGAIN 35
#  endif
#  ifndef EALREADY
#    define EALREADY 37
#  endif
#  ifndef EBADF
#    define EBADF 9
#  endif
#  ifndef EBADMSG
#    define EBADMSG 94
#  endif
#  ifndef EBUSY
#    define EBUSY 16
#  endif
#  ifndef ECANCELED
#    define ECANCELED 89
#  endif
#  ifndef ECHILD
#    define ECHILD 10
#  endif
#  ifndef ECONNABORTED
#    define ECONNABORTED 53
#  endif
#  ifndef ECONNREFUSED
#    define ECONNREFUSED 61
#  endif
#  ifndef ECONNRESET
#    define ECONNRESET 54
#  endif
#  ifndef EDEADLK
#    define EDEADLK 11
#  endif
#  ifndef EDESTADDRREQ
#    define EDESTADDRREQ 39
#  endif
#  ifndef EDOM
#    define EDOM 33
#  endif
#  ifndef EEXIST
#    define EEXIST 17
#  endif
#  ifndef EFAULT
#    define EFAULT 14
#  endif
#  ifndef EFBIG
#    define EFBIG 27
#  endif
#  ifndef EHOSTUNREACH
#    define EHOSTUNREACH 65
#  endif
#  ifndef EIDRM
#    define EIDRM 90
#  endif
#  ifndef EILSEQ
#    define EILSEQ 92
#  endif
#  ifndef EINPROGRESS
#    define EINPROGRESS 36
#  endif
#  ifndef EINTR
#    define EINTR 4
#  endif
#  ifndef EINVAL
#    define EINVAL 22
#  endif
#  ifndef EIO
#    define EIO 5
#  endif
#  ifndef EISCONN
#    define EISCONN 56
#  endif
#  ifndef EISDIR
#    define EISDIR 21
#  endif
#  ifndef ELOOP
#    define ELOOP 62
#  endif
#  ifndef EMFILE
#    define EMFILE 24
#  endif
#  ifndef EMLINK
#    define EMLINK 31
#  endif
#  ifndef EMSGSIZE
#    define EMSGSIZE 40
#  endif
#  ifndef ENAMETOOLONG
#    define ENAMETOOLONG 63
#  endif
#  ifndef ENETDOWN
#    define ENETDOWN 50
#  endif
#  ifndef ENETRESET
#    define ENETRESET 52
#  endif
#  ifndef ENETUNREACH
#    define ENETUNREACH 51
#  endif
#  ifndef ENFILE
#    define ENFILE 23
#  endif
#  ifndef ENOBUFS
#    define ENOBUFS 55
#  endif
#  define ENODATA 96 // [depr.cerrno]
#  ifndef ENODEV
#    define ENODEV 19
#  endif
#  ifndef ENOENT
#    define ENOENT 2
#  endif
#  ifndef ENOEXEC
#    define ENOEXEC 8
#  endif
#  ifndef ENOLCK
#    define ENOLCK 77
#  endif
#  ifndef ENOLINK
#    define ENOLINK 97
#  endif
#  ifndef ENOMEM
#    define ENOMEM 12
#  endif
#  ifndef ENOMSG
#    define ENOMSG 91
#  endif
#  ifndef ENOPROTOOPT
#    define ENOPROTOOPT 42
#  endif
#  ifndef ENOSPC
#    define ENOSPC 28
#  endif
#  define ENOSR 98 // [depr.cerrno]
#  define ENOSTR 99 // [depr.cerrno]
#  ifndef ENOSYS
#    define ENOSYS 78
#  endif
#  ifndef ENOTCONN
#    define ENOTCONN 57
#  endif
#  ifndef ENOTDIR
#    define ENOTDIR 20
#  endif
#  ifndef ENOTEMPTY
#    define ENOTEMPTY 66
#  endif
#  ifndef ENOTRECOVERABLE
#    define ENOTRECOVERABLE 104
#  endif
#  ifndef ENOTSOCK
#    define ENOTSOCK 38
#  endif
#  ifndef ENOTSUP
#    define ENOTSUP 45
#  endif
#  ifndef ENOTTY
#    define ENOTTY 25
#  endif
#  ifndef ENXIO
#    define ENXIO 6
#  endif
#  ifndef EOPNOTSUPP
#    define EOPNOTSUPP 102
#  endif
#  ifndef EOVERFLOW
#    define EOVERFLOW 84
#  endif
#  ifndef EOWNERDEAD
#    define EOWNERDEAD 105
#  endif
#  ifndef EPERM
#    define EPERM 1
#  endif
#  ifndef EPIPE
#    define EPIPE 32
#  endif
#  ifndef EPROTO
#    define EPROTO 100
#  endif
#  ifndef EPROTONOSUPPORT
#    define EPROTONOSUPPORT 43
#  endif
#  ifndef EPROTOTYPE
#    define EPROTOTYPE 41
#  endif
#  ifndef ERANGE
#    define ERANGE 34
#  endif
#  ifndef EROFS
#    define EROFS 30
#  endif
#  ifndef ESPIPE
#    define ESPIPE 29
#  endif
#  ifndef ESRCH
#    define ESRCH 3
#  endif
#  define ETIME 101 // [depr.cerrno]
#  ifndef ETIMEDOUT
#    define ETIMEDOUT 60
#  endif
#  ifndef ETXTBSY
#    define ETXTBSY 26
#  endif
#  ifndef EWOULDBLOCK
#    define EWOULDBLOCK 35
#  endif
#  ifndef EXDEV
#    define EXDEV 18
#  endif
#else
#  ifndef E2BIG
#    define E2BIG 7
#  endif
#  ifndef EACCES
#    define EACCES 13
#  endif
#  ifndef EADDRINUSE
#    define EADDRINUSE 98
#  endif
#  ifndef EADDRNOTAVAIL
#    define EADDRNOTAVAIL 99
#  endif
#  ifndef EAFNOSUPPORT
#    define EAFNOSUPPORT 97
#  endif
#  ifndef EAGAIN
#    define EAGAIN 11
#  endif
#  ifndef EALREADY
#    define EALREADY 114
#  endif
#  ifndef EBADF
#    define EBADF 9
#  endif
#  ifndef EBADMSG
#    define EBADMSG 74
#  endif
#  ifndef EBUSY
#    define EBUSY 16
#  endif
#  ifndef ECANCELED
#    define ECANCELED 125
#  endif
#  ifndef ECHILD
#    define ECHILD 10
#  endif
#  ifndef ECONNABORTED
#    define ECONNABORTED 103
#  endif
#  ifndef ECONNREFUSED
#    define ECONNREFUSED 111
#  endif
#  ifndef ECONNRESET
#    define ECONNRESET 104
#  endif
#  ifndef EDEADLK
#    define EDEADLK 35
#  endif
#  ifndef EDESTADDRREQ
#    define EDESTADDRREQ 89
#  endif
#  ifndef EDOM
#    define EDOM 33
#  endif
#  ifndef EEXIST
#    define EEXIST 17
#  endif
#  ifndef EFAULT
#    define EFAULT 14
#  endif
#  ifndef EFBIG
#    define EFBIG 27
#  endif
#  ifndef EHOSTUNREACH
#    define EHOSTUNREACH 113
#  endif
#  ifndef EIDRM
#    define EIDRM 43
#  endif
#  ifndef EILSEQ
#    define EILSEQ 84
#  endif
#  ifndef EINPROGRESS
#    define EINPROGRESS 115
#  endif
#  ifndef EINTR
#    define EINTR 4
#  endif
#  ifndef EINVAL
#    define EINVAL 22
#  endif
#  ifndef EIO
#    define EIO 5
#  endif
#  ifndef EISCONN
#    define EISCONN 106
#  endif
#  ifndef EISDIR
#    define EISDIR 21
#  endif
#  ifndef ELOOP
#    define ELOOP 40
#  endif
#  ifndef EMFILE
#    define EMFILE 24
#  endif
#  ifndef EMLINK
#    define EMLINK 31
#  endif
#  ifndef EMSGSIZE
#    define EMSGSIZE 90
#  endif
#  ifndef ENAMETOOLONG
#    define ENAMETOOLONG 36
#  endif
#  ifndef ENETDOWN
#    define ENETDOWN 100
#  endif
#  ifndef ENETRESET
#    define ENETRESET 102
#  endif
#  ifndef ENETUNREACH
#    define ENETUNREACH 101
#  endif
#  ifndef ENFILE
#    define ENFILE 23
#  endif
#  ifndef ENOBUFS
#    define ENOBUFS 105
#  endif
#  define ENODATA 61 // [depr.cerrno]
#  ifndef ENODEV
#    define ENODEV 19
#  endif
#  ifndef ENOENT
#    define ENOENT 2
#  endif
#  ifndef ENOEXEC
#    define ENOEXEC 8
#  endif
#  ifndef ENOLCK
#    define ENOLCK 37
#  endif
#  ifndef ENOLINK
#    define ENOLINK 67
#  endif
#  ifndef ENOMEM
#    define ENOMEM 12
#  endif
#  ifndef ENOMSG
#    define ENOMSG 42
#  endif
#  ifndef ENOPROTOOPT
#    define ENOPROTOOPT 92
#  endif
#  ifndef ENOSPC
#    define ENOSPC 28
#  endif
#  define ENOSR 63 // [depr.cerrno]
#  define ENOSTR 60 // [depr.cerrno]
#  ifndef ENOSYS
#    define ENOSYS 38
#  endif
#  ifndef ENOTCONN
#    define ENOTCONN 107
#  endif
#  ifndef ENOTDIR
#    define ENOTDIR 20
#  endif
#  ifndef ENOTEMPTY
#    define ENOTEMPTY 39
#  endif
#  ifndef ENOTRECOVERABLE
#    define ENOTRECOVERABLE 131
#  endif
#  ifndef ENOTSOCK
#    define ENOTSOCK 88
#  endif
#  ifndef ENOTSUP
#    define ENOTSUP 95
#  endif
#  ifndef ENOTTY
#    define ENOTTY 25
#  endif
#  ifndef ENXIO
#    define ENXIO 6
#  endif
#  ifndef EOPNOTSUPP
#    define EOPNOTSUPP 95
#  endif
#  ifndef EOVERFLOW
#    define EOVERFLOW 75
#  endif
#  ifndef EOWNERDEAD
#    define EOWNERDEAD 130
#  endif
#  ifndef EPERM
#    define EPERM 1
#  endif
#  ifndef EPIPE
#    define EPIPE 32
#  endif
#  ifndef EPROTO
#    define EPROTO 71
#  endif
#  ifndef EPROTONOSUPPORT
#    define EPROTONOSUPPORT 93
#  endif
#  ifndef EPROTOTYPE
#    define EPROTOTYPE 91
#  endif
#  ifndef ERANGE
#    define ERANGE 34
#  endif
#  ifndef EROFS
#    define EROFS 30
#  endif
#  ifndef ESPIPE
#    define ESPIPE 29
#  endif
#  ifndef ESRCH
#    define ESRCH 3
#  endif
#  define ETIME 62 // [depr.cerrno]
#  ifndef ETIMEDOUT
#    define ETIMEDOUT 110
#  endif
#  ifndef ETXTBSY
#    define ETXTBSY 26
#  endif
#  ifndef EWOULDBLOCK
#    define EWOULDBLOCK 11
#  endif
#  ifndef EXDEV
#    define EXDEV 18
#  endif
#endif

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
consteval bool __errno_macros_match_errc() {
  using std::errc;
  const struct {
    errc e;
    int __v;
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
  for (const auto& __entry : table)
    if (static_cast<int>(__entry.e) != __entry.__v)
      return false;
  return ENODATA == __errno_enodata && ENOSR == __errno_enosr && ENOSTR == __errno_enostr && ETIME == __errno_etime;
}
static_assert(__ycxx::__detail::__errno_macros_match_errc(), "libycxx: the freestanding <cerrno> macros differ from std::errc");
}} // namespace __ycxx::__detail
