#include "utils.h"
#include "log.h"
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include "debug.h"

void PrintHelp(void){

    log.Print("[zkz] help\nQuick navigation help for zkz debugger\n\t \
            [r]un / [c]ontinue\tstart debugging\n             \
            list\tlist line information\n             \
            [b]reak\tbreakpoint at memory address\n           \
            [b]reak[l]\tbreakpoint at line number\n           \
            select\tselect a compilation unit\n               \
            [[i]nfo] [[r]egisters]\tregister information\n    \
            [[d]el]ete\tdelete a breakpoint\n                 \
            set\tset register values\n                        \
            [dis]able\t disable breakpoints\n                 \
            exit\texit zkz");
}

void PrintUsage(void){

    log.Print("[zkz] Usage\nQuick help to start zkz debugger\n \
            -f - Start executing given file as the debugee\n  \
            -p - Attach to given process ID\n               \
            -s - Enable system call tracing\n                      \
            -i - Print register information on each step\n    \
            ");
}
