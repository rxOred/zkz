#include "utils.h"
#include "log.h"
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include "debug.h"

void PrintHelp(void){

    log.Print("[zkz] help\nQuick navigation help for zkz debugger\n\t \
            [r]un / [c]ontinue\tstart debugging\n\t             \
            list\tlist line information\n\t                     \
            [b]reak\tbreakpoint at memory address\n\t           \
            [b]reak[l]\tbreakpoint at line number\n\t           \
            select\tselect a compilation unit\n\t               \
            [[i]nfo] [[r]egisters]\tregister information\n\t    \
            [[d]el]ete\tdelete a breakpoint\n\t                 \
            set\tset register values\n\t                        \
            [dis]able\t disable breakpoints\n\t                 \
            exit\texit zkz");
}

void PrintUsage(void){

    log.Print("[zkz] Usage\nQuick help to start zkz debugger\n\t \
            -f - Start executing given file as the debugee\n\t  \
            -p - Attach to given process ID\n\t                 \
            -s - Enable system call tracing\n\t                 \
            -i - Print register information on each step\n\t    \
            ");
}
