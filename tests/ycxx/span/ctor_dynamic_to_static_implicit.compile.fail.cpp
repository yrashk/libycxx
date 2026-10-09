// EXPECT-ERROR-GCC: error: conversion from 'span[^\n]*to non-scalar type 'span[^\n]*,2>'
// EXPECT-ERROR-CLANG: error: no viable conversion from 'span[^\n]*dynamic_extent[^\n]*to 'span[^\n]*, 2>'
// [span.cons]/26: the converting constructor is explicit when
// "extent != dynamic_extent && OtherExtent == dynamic_extent".
#include <span>

void f(std::span<int> d) { std::span<int, 2> s = d; (void)s; }
