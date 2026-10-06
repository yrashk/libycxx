// The program both hosted-layers examples run (demo.cpp): containers, strings, formatting and
// printing, exceptions, smart pointers and a clock, on libycxx built with YCXX_PAL=none and the
// layers abort, memory, console and clock, whose primitives each example provides.
#pragma once

// Runs the demonstration, printing to standard output (the console layer); `where` names the
// environment. Returns the number of failed checks: 0 means "hosted-layers demo: ok" was printed.
int hosted_layers_demo(const char* where);
