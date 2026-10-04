// libycxx core: the <stdexcept> constructors taking `const string&` ([std.exceptions]).
//
// ycxx/core/stdexcept.hpp only declares them, so that ycxx/core/error.hpp can include it without
// <string>; basic_string.hpp includes this file once std::string is complete.
#pragma once

#include <ycxx/core/basic_string.hpp>
#include <ycxx/core/stdexcept.hpp>

namespace std {

constexpr logic_error::logic_error(const string& what_arg) : msg_(what_arg.c_str(), what_arg.size()) {}
constexpr runtime_error::runtime_error(const string& what_arg) : msg_(what_arg.c_str(), what_arg.size()) {}
constexpr domain_error::domain_error(const string& what_arg) : logic_error(what_arg) {}
constexpr invalid_argument::invalid_argument(const string& what_arg) : logic_error(what_arg) {}
constexpr length_error::length_error(const string& what_arg) : logic_error(what_arg) {}
constexpr out_of_range::out_of_range(const string& what_arg) : logic_error(what_arg) {}
constexpr range_error::range_error(const string& what_arg) : runtime_error(what_arg) {}
constexpr overflow_error::overflow_error(const string& what_arg) : runtime_error(what_arg) {}
constexpr underflow_error::underflow_error(const string& what_arg) : runtime_error(what_arg) {}

} // namespace std
