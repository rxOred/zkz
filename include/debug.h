#ifndef DEBUG_H
#define DEBUG_H

#include <sys/types.h>
#include <vector>
#include <string>
#include "libelfin/elf/elf++.hh"
#include "libelfin/dwarf/dwarf++.hh"
#include "libelfin/dwarf/data.hh"

class Debug{

    private:
        struct {

            /*
             * process information
             */
            struct {
                pid_t pid;
                char **pathname;    //free this
                int count;
            }m_dbg;

            /*
             * should we stop and wait for user in every syscall invocation of debugee?
             */
            bool is_systrace;

            /*
             * shoudl we print register infomation on every stop of debugging process?
             */
            bool b_is_inforeg;
        } m_arguments;

        /*
         * symbols parsed or not
         */
        bool b_sym_state;

        /*
         * debug lines parsed or not
         */
        bool b_dwarf_state;

        /*
         * is child process running
         */
        bool b_is_running;

        /*
         * for system call ibjection
         */
        bool b_is_sys_stopped;

    public:
        Debug();

        ~Debug();

        void SetDwarf(void);
        void SetSym(void);

        /*
         * set is_systrace (once these are true, cannot be undone
         */
        void SetSystrace(void);

        /*
         * set is_runiing
         */
        void SetInforeg(void);

        /*
         * set process id 
         */
        void SetPid(pid_t pid);


        void SetPathname(char **pathname);
        void SetCount(int count);
        void SetProgramState(bool state);
        void SetSyscallState(bool state);

        bool GetDwarfState(void) const;
        bool GetSymState(void) const;
        bool GetSystrace(void) const;
        bool GetInforeg(void) const;
        pid_t GetPid(void) const;
        char **GetPathname(void) const;
        int GetCount(void) const;
        bool GetProgramState(void) const;
        bool GetSyscallState(void) const;
};

#endif /* DEBUG_H */
