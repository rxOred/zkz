#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#include <bits/stdint-uintn.h>
#include <functional>
#include <vector>
#include "debug.h"

class Breakpoint{

    private:
        /* 
         * NOTE no stop at breakpoint if this is false, it will restore breakpoint 
         * instruction to its original instruction and continue execution 
         */
        bool b_is_enabled;

    public:

        /*
         * address to place breakpoint
         */
        uint64_t m_address;

        /* 
         * original data at the address
         */
        uint64_t m_origdata;

        int m_breakpoint_number;

        Breakpoint();
        Breakpoint(uint64_t address, uint64_t origdata, int breakpoint_number, bool
                is_enabled);
        inline bool GetState(void) const
        {
            return b_is_enabled;
        }

        inline void DisableBreakpoint(void)
        {
            b_is_enabled = false;
        }

        inline void EnableBreakpoint(void)
        {
            b_is_enabled = true;
        }
};

class BreakpointList{

    private:

        /*
         * vector of Breakpoint pointers
         */
        std::vector<Breakpoint*> B_List;

    public:
        ~BreakpointList();

        inline int GetNoOfBreakpoints() const
        {
            return B_List.size();
        }
        void AppendElement(uint64_t address, uint64_t origdata);
        int RemoveElement(const Debug& debug, int breakpoint_number);
        Breakpoint *GetElementByIndex(int index) const;
        Breakpoint *GetElementByBreakpointNumber(int breakpoint_number) const;
        Breakpoint *GetElementByAddress(uint64_t addr) const;
        void ListBreakpoints(void) const;
};

#endif /* BREAKPOINT_H */
