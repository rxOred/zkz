#pragma once

#include <sys/types.h>
#include <vector>
#include <string>
#include "libelfin/elf/elf++.hh"
#include "libelfin/dwarf/dwarf++.hh"
#include "libelfin/dwarf/data.hh"
#include "log.h"

static Log log;

class Debug{

    private:
        struct {
            struct {
                pid_t pid;
                char **pathname;    //free this
            }u_dbg;

            bool is_systrace;   // should we stop and wait for user in every syscall invocation of debugee?
            bool is_inforeg;    // shoudl we print register infomation on every stop of debugging process?
        } arguments;

        bool is_running;
        bool is_sys_stopped;    // to implement system call tracing + inspection

    public:

        struct{
            elf::elf elf;
            dwarf::dwarf dwarf;
        } sourceifo;


        Debug();

        void set_systrace(void);    // set is_systrace (once these are true, cannot be undone
        void set_inforeg(void);     // set is_inforeg
        void set_pid(pid_t pid);    // set process id to debugee's
        void set_pathname(char **pathname);     // set pathname of debugee if we are forking it
        /* int open_elf(const char *filename);
        void set_sourceifo(int file_descriptor); */
        void set_program_state(bool state);     // set is_runiing
        void set_syscall_state(bool state);     // set is_sys_stopped

        bool get_systrace(void) const;
        bool get_inforeg(void) const;
        pid_t get_pid(void) const;
        char **get_pathname(void) const;
        bool get_program_state(void) const;
        bool get_syscall_state(void) const;
};
