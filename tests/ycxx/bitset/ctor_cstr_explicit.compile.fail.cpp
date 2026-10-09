// EXPECT-ERROR-GCC: error: conversion from 'const char \[5\]' to non-scalar type 'std::bitset<4>'
// EXPECT-ERROR-CLANG: error: no viable conversion from 'const char\[5\]' to 'std::bitset<4>'
// [bitset.cons]/8: "template<class charT> constexpr explicit bitset(const charT* str, ...)":
// copy-initialisation from a string literal is ill-formed.
#include <bitset>

std::bitset<4> b = "1010";
