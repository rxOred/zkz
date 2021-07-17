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

        inline void SetDwarf(void){

            b_dwarf_state = false;
        }

        inline void SetSym(void){

            b_sym_state = false;
        }

        inline void SetSystrace(void){

            m_arguments.is_systrace = true;
        }

        inline void SetInforeg(void){

            m_arguments.b_is_inforeg = true;
        }

        inline void SetPid(pid_t pid){

            this->m_arguments.m_dbg.pid = pid;
        }

        inline void SetPathname(char **pathname){

            this->m_arguments.m_dbg.pathname = pathname;
        }

        inline void SetCount(int count){

            this->m_arguments.m_dbg.count = count;
        }

        inline void SetProgramState(bool state){

            b_is_running = state;
        }

        inline void SetSyscallState(bool state){

            b_is_sys_stopped = state;
        }

        inline bool GetDwarfState(void) const {

            return b_dwarf_state;
        }

        inline bool GetSymState(void) const {

            return b_sym_state;
        }

        inline bool GetSystrace(void) const{

            return m_arguments.is_systrace;
        }

        inline bool GetInforeg(void) const{

            return m_arguments.b_is_inforeg;
        }

        inline pid_t GetPid(void) const{

            return m_arguments.m_dbg.pid;
        }

        inline char **GetPathname(void) const{

            return m_arguments.m_dbg.pathname;
        }

        inline int GetCount(void) const{

            return m_arguments.m_dbg.count;
        }

        inline bool GetProgramState(void) const{

            return b_is_running;
        }

        inline bool GetSyscallState(void) const{

            return b_is_sys_stopped;
        }
};

#endif /* DEBUG_H */
