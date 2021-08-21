#include "utils.h"
#include "log.h"
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include "debug.h"

char **ParseString(char *s)
{
    int count = 0;
    char *str = nullptr;
    char **pathname = nullptr;

    str = strtok(s, " ");
    while(str != nullptr){
        count++;
        pathname = (char **) realloc(pathname, sizeof(char *) * 
                (count + 1));
        if(pathname == nullptr){
            goto m_failed;
        }
        pathname[count - 1] = strdup(str);
        if(pathname[count - 1] == nullptr){
            log.PError("Memory allocation failed");
            goto m_failed;
        }

        str = strtok(nullptr, " ");
    }
    pathname[count] = nullptr;
    return pathname;

m_failed:
    if(count >= 1){
        for(int i = 0; i < count; i++)
            if(pathname[i])
                free(pathname[i]);
    }
    if(pathname)
        free(pathname);

failed:
    return nullptr;
}

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
