// EXPECT-ERROR-GCC: error: use of deleted function [^\n]*basic_string_view\(nullptr_t\)
// EXPECT-ERROR-CLANG: error: conversion function from 'std::nullptr_t' to 'std::string_view'[^\n]*invokes a deleted function
// [string.view.template.general]: "basic_string_view(nullptr_t) = delete;" (P2166).
#include <string_view>

std::string_view s = nullptr;
