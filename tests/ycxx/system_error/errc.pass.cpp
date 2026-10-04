// [system.error.syn]: enum class errc with the listed enumerators; /1: "The value of each enum
// errc enumerator is the same as the value of the <cerrno> macro shown in the above synopsis."
// [system.error.syn]: template<> struct is_error_condition_enum<errc> : true_type {};
// is_error_code_enum<errc> is the primary template (false_type).
// [syserr.errcode.nonmembers]/1, [syserr.errcondition.nonmembers]/1: make_error_code and
// make_error_condition use static_cast<int>(e) and generic_category().
#include <system_error>
#include <cerrno>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_enum_v<std::errc>);
static_assert(!std::is_convertible_v<std::errc, int>);  // a scoped enumeration
static_assert(std::is_error_condition_enum_v<std::errc>);
static_assert(!std::is_error_code_enum_v<std::errc>);

#define E(name, macro)                                                     \
  static_assert(static_cast<int>(std::errc::name) == macro, #name);       \
  static_assert(std::is_same_v<decltype(std::errc::name), const std::errc> || \
                std::is_same_v<decltype(std::errc::name), std::errc>);
E(address_family_not_supported, EAFNOSUPPORT)
E(address_in_use, EADDRINUSE)
E(address_not_available, EADDRNOTAVAIL)
E(already_connected, EISCONN)
E(argument_list_too_long, E2BIG)
E(argument_out_of_domain, EDOM)
E(bad_address, EFAULT)
E(bad_file_descriptor, EBADF)
E(bad_message, EBADMSG)
E(broken_pipe, EPIPE)
E(connection_aborted, ECONNABORTED)
E(connection_already_in_progress, EALREADY)
E(connection_refused, ECONNREFUSED)
E(connection_reset, ECONNRESET)
E(cross_device_link, EXDEV)
E(destination_address_required, EDESTADDRREQ)
E(device_or_resource_busy, EBUSY)
E(directory_not_empty, ENOTEMPTY)
E(executable_format_error, ENOEXEC)
E(file_exists, EEXIST)
E(file_too_large, EFBIG)
E(filename_too_long, ENAMETOOLONG)
E(function_not_supported, ENOSYS)
E(host_unreachable, EHOSTUNREACH)
E(identifier_removed, EIDRM)
E(illegal_byte_sequence, EILSEQ)
E(inappropriate_io_control_operation, ENOTTY)
E(interrupted, EINTR)
E(invalid_argument, EINVAL)
E(invalid_seek, ESPIPE)
E(io_error, EIO)
E(is_a_directory, EISDIR)
E(message_size, EMSGSIZE)
E(network_down, ENETDOWN)
E(network_reset, ENETRESET)
E(network_unreachable, ENETUNREACH)
E(no_buffer_space, ENOBUFS)
E(no_child_process, ECHILD)
E(no_link, ENOLINK)
E(no_lock_available, ENOLCK)
E(no_message, ENOMSG)
E(no_protocol_option, ENOPROTOOPT)
E(no_space_on_device, ENOSPC)
E(no_such_device_or_address, ENXIO)
E(no_such_device, ENODEV)
E(no_such_file_or_directory, ENOENT)
E(no_such_process, ESRCH)
E(not_a_directory, ENOTDIR)
E(not_a_socket, ENOTSOCK)
E(not_connected, ENOTCONN)
E(not_enough_memory, ENOMEM)
E(not_supported, ENOTSUP)
E(operation_canceled, ECANCELED)
E(operation_in_progress, EINPROGRESS)
E(operation_not_permitted, EPERM)
E(operation_not_supported, EOPNOTSUPP)
E(operation_would_block, EWOULDBLOCK)
E(owner_dead, EOWNERDEAD)
E(permission_denied, EACCES)
E(protocol_error, EPROTO)
E(protocol_not_supported, EPROTONOSUPPORT)
E(read_only_file_system, EROFS)
E(resource_deadlock_would_occur, EDEADLK)
E(resource_unavailable_try_again, EAGAIN)
E(result_out_of_range, ERANGE)
E(state_not_recoverable, ENOTRECOVERABLE)
E(text_file_busy, ETXTBSY)
E(timed_out, ETIMEDOUT)
E(too_many_files_open_in_system, ENFILE)
E(too_many_files_open, EMFILE)
E(too_many_links, EMLINK)
E(too_many_symbolic_link_levels, ELOOP)
E(value_too_large, EOVERFLOW)
E(wrong_protocol_type, EPROTOTYPE)
#undef E

// On this POSIX platform a system error value equal to an errno value corresponds to it
// ([syserr.errcat.objects]/4), so the system code compares equal to the errc condition.
int main() {
#define E(name, macro)                                                             \
  {                                                                                \
    std::error_condition c = std::errc::name;                                      \
    CHECK(c.value() == macro && &c.category() == &std::generic_category());         \
    std::error_code k = std::make_error_code(std::errc::name);                      \
    CHECK(k.value() == macro && &k.category() == &std::generic_category());         \
    CHECK(k == std::errc::name);                                                    \
    CHECK(std::error_code(macro, std::system_category()) == std::errc::name);       \
  }
  E(address_family_not_supported, EAFNOSUPPORT)
  E(address_in_use, EADDRINUSE)
  E(address_not_available, EADDRNOTAVAIL)
  E(already_connected, EISCONN)
  E(argument_list_too_long, E2BIG)
  E(argument_out_of_domain, EDOM)
  E(bad_address, EFAULT)
  E(bad_file_descriptor, EBADF)
  E(bad_message, EBADMSG)
  E(broken_pipe, EPIPE)
  E(connection_aborted, ECONNABORTED)
  E(connection_already_in_progress, EALREADY)
  E(connection_refused, ECONNREFUSED)
  E(connection_reset, ECONNRESET)
  E(cross_device_link, EXDEV)
  E(destination_address_required, EDESTADDRREQ)
  E(device_or_resource_busy, EBUSY)
  E(directory_not_empty, ENOTEMPTY)
  E(executable_format_error, ENOEXEC)
  E(file_exists, EEXIST)
  E(file_too_large, EFBIG)
  E(filename_too_long, ENAMETOOLONG)
  E(function_not_supported, ENOSYS)
  E(host_unreachable, EHOSTUNREACH)
  E(identifier_removed, EIDRM)
  E(illegal_byte_sequence, EILSEQ)
  E(inappropriate_io_control_operation, ENOTTY)
  E(interrupted, EINTR)
  E(invalid_argument, EINVAL)
  E(invalid_seek, ESPIPE)
  E(io_error, EIO)
  E(is_a_directory, EISDIR)
  E(message_size, EMSGSIZE)
  E(network_down, ENETDOWN)
  E(network_reset, ENETRESET)
  E(network_unreachable, ENETUNREACH)
  E(no_buffer_space, ENOBUFS)
  E(no_child_process, ECHILD)
  E(no_link, ENOLINK)
  E(no_lock_available, ENOLCK)
  E(no_message, ENOMSG)
  E(no_protocol_option, ENOPROTOOPT)
  E(no_space_on_device, ENOSPC)
  E(no_such_device_or_address, ENXIO)
  E(no_such_device, ENODEV)
  E(no_such_file_or_directory, ENOENT)
  E(no_such_process, ESRCH)
  E(not_a_directory, ENOTDIR)
  E(not_a_socket, ENOTSOCK)
  E(not_connected, ENOTCONN)
  E(not_enough_memory, ENOMEM)
  E(not_supported, ENOTSUP)
  E(operation_canceled, ECANCELED)
  E(operation_in_progress, EINPROGRESS)
  E(operation_not_permitted, EPERM)
  E(operation_not_supported, EOPNOTSUPP)
  E(operation_would_block, EWOULDBLOCK)
  E(owner_dead, EOWNERDEAD)
  E(permission_denied, EACCES)
  E(protocol_error, EPROTO)
  E(protocol_not_supported, EPROTONOSUPPORT)
  E(read_only_file_system, EROFS)
  E(resource_deadlock_would_occur, EDEADLK)
  E(resource_unavailable_try_again, EAGAIN)
  E(result_out_of_range, ERANGE)
  E(state_not_recoverable, ENOTRECOVERABLE)
  E(text_file_busy, ETXTBSY)
  E(timed_out, ETIMEDOUT)
  E(too_many_files_open_in_system, ENFILE)
  E(too_many_files_open, EMFILE)
  E(too_many_links, EMLINK)
  E(too_many_symbolic_link_levels, ELOOP)
  E(value_too_large, EOVERFLOW)
  E(wrong_protocol_type, EPROTOTYPE)
#undef E
  return 0;
}
