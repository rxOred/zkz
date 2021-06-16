#pragma once

#include <bits/stdint-uintn.h>
#include <functional>
#include <vector>

class Breakpoint{

    private:
        bool is_enabled;

    public:
        uint64_t address;
        uint64_t origdata;
        int breakpoint_number;

        Breakpoint();
        Breakpoint(uint64_t address, uint64_t origdata, int breakpoint_number, bool is_enabled);
        bool get_state(void) const;
        void disable_breakpoint(void);
        void enable_breakpoint(void);
};

class BreakpointList{

    private:
        std::vector<Breakpoint*> B_List;    // dynamic array of pointers to Breakpoint

    public:
        BreakpointList();
        //~BreakpointList();

        int get_number_of_breakpoints() const;
        void append_element(uint64_t address, uint64_t origdata);
        void remove_element(int breakpoint_number);
        Breakpoint *get_element_by_index(int index);
        Breakpoint *get_element_by_breakpoint_number(int breakpoint_number);
        Breakpoint *get_element_by_address(uint64_t addr);
};
