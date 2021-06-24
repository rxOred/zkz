#pragma once

#include <bits/stdint-uintn.h>
#include <functional>
#include <vector>

class Breakpoint{

    private:
        bool b_is_enabled;

    public:
        uint64_t m_address;
        uint64_t m_origdata;
        int m_breakpoint_number;

        Breakpoint();
        Breakpoint(uint64_t address, uint64_t origdata, int breakpoint_number, bool is_enabled);
        bool GetState(void) const;
        void DisableBreakpoint(void);
        void EnableBreakpoint(void);
};

class BreakpointList{

    private:
        std::vector<Breakpoint*> B_List;    // dynamic array of pointers to Breakpoint

    public:
        ~BreakpointList();

        int GetNoOfBreakpoints() const;
        void AppendElement(uint64_t address, uint64_t origdata);
        void RemoveElement(int breakpoint_number);
        Breakpoint *GetElementByIndex(int index);
        Breakpoint *GetElementByBreakpointNumber(int breakpoint_number);
        Breakpoint *GetElementByAddress(uint64_t addr);
};
