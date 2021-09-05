#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <elf.h>
#include <libelfin/dwarf/dwarf++.hh>
#include <sys/types.h>
#include <sys/user.h>

#include "breakpoint.h"
#include "debug.h"
#include "dwarf.h"
#include "parsesymbols.h"

class Main{

    /* TODO use unique_ptr */
    private:
        /* Debugee information */
        Debug m_debug;

        /* dwarf information */
        DebugLineInfo *m_line_info_ptr;

        /* Elf and symbol information */
        Symbol *m_symbols_ptr;

        /* registers */
        struct user_regs_struct m_regs;

        int m_base_addr;

    public:

        Main()
            :m_line_info_ptr(nullptr), m_symbols_ptr(nullptr) 
        {}

        ~Main();
        int ParseArguments(int argc, char *argv[]);
        int Debugger(void);

        /*
         * process
         */ 
        int AttachProcess(void);
        int StartProcess(void);
        int WaitForProcess(void);
        int ContinueExecution(BreakpointList& li);
        void KillProcess(pid_t pid) const;
        uint64_t GetBaseAddress(pid_t pid) const;

        /*
         * debug information
         */
        void InitDebugLines(void);
        void ParseLines(const dwarf::line_table& lt, int unit_number) const;

        /*
         * mainloop
         */
        int MainLoop(void);

        /*
         * breakpoints and registers
         */
        int RestoreBreakpoint(Breakpoint *b, uint64_t address);
        int PlaceBreakpoint(BreakpointList& li, uint64_t address) const;
        int DisableBreakpoint(BreakpointList& li, int breakpoint_number) const;
        int RemoveAllBreakpoints(BreakpointList& li) const;

        /*
         * stepping and stopping 
         */
        int StepAuto(BreakpointList& li);
        int StepX(BreakpointList& li, int number_of_steps);
};

#endif /* DEBUGGER_H */
