# The default replaceable allocation functions

One function per file, in each runtime archive (`src/runtime/new`: the forwarding forms shared by
the hosted and freestanding archives; `src/hosted/new`, `src/freestanding/new`: the forms that
allocate), so that a program can replace any subset ([replacement.functions]): a replaced
function is linked from the program, and the archive member holding the default is not.

The defaults are hidden (`hidden.hpp`), and the images of a process that link libycxx share them
through the allocation table (`allocation_table.hpp`, DECISIONS §2): each default first forwards
to the process's table entry when that entry is another image's.

In a hosted build with exceptions disabled, the nothrow defaults follow the selected ordinary
allocation functions. The default ordinary array forms forward to the ordinary scalar forms,
so both selections honor replacements. When a hosted scalar default is selected, it retries
the platform allocation through `new_handler` and returns null if no handler is installed.

The configure probe selects the private identity mechanism. ELF uses an optional hidden
same-body alias for each ordinary form. Mach-O places the ordinary defaults within a private
code section and compares the selected address to that image's linker bounds. Both mechanisms
distinguish the winning address when an allocation archive anchor pulls in an unused default.
