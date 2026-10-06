# The default replaceable allocation functions

One function per file, in each runtime archive (`src/runtime/new`: the forwarding forms shared by
the hosted and freestanding archives; `src/hosted/new`, `src/freestanding/new`: the forms that
allocate), so that a program can replace any subset ([replacement.functions]): a replaced
function is linked from the program, and the archive member holding the default is not.

The defaults are hidden (`hidden.hpp`), and the images of a process that link libycxx share them
through the allocation table (`allocation_table.hpp`, DECISIONS §2): each default first forwards
to the process's table entry when that entry is another image's.
