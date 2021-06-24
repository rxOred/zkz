#pragma once

#include <sys/types.h>
#include <vector>
#include <string>
#include "libelfin/elf/elf++.hh"
#include "libelfin/dwarf/dwarf++.hh"
#include "libelfin/dwarf/data.hh"
#include "log.h"

static Log log(DEBUG | PRINT | PROMPT | ERROR | INFO | PERROR);

class Debug{

    private:
        struct {
            struct {
                pid_t pid;
                char **pathname;    //free this
                int count;
            }m_dbg;

            bool is_systrace;   // should we stop and wait for user in every syscall invocation of debugee?
            bool b_is_inforeg;    // shoudl we print register infomation on every stop of debugging process?
        } m_arguments;

        bool b_is_running;
        bool b_is_sys_stopped;    // to implement system call tracing + inspection

    public:
        Debug();

        ~Debug();

        void SetSystrace(void);    // set is_systrace (once these are true, cannot be undone
        void SetInforeg(void);     // set is_inforeg
        void SetPid(pid_t pid);    // set process id to debugee's
        void SetPathname(char **pathname);     // set pathname of debugee if we are forking it
        void SetCount(int count);
        /* int open_elf(const char *filename);
        void set_sourceifo(int file_descriptor); */
        void SetProgramState(bool state);     // set is_runiing
        void SetSyscallState(bool state);     // set is_sys_stopped

        bool GetSystrace(void) const;
        bool GetInforeg(void) const;
        pid_t GetPid(void) const;
        char **GetPathname(void) const;
        int GetCount(void) const;
        bool GetProgramState(void) const;
        bool GetSyscallState(void) const;
};
