#pragma once

#include "main.h"
#include <bits/stdint-uintn.h>
#include <libelfin/dwarf/dwarf++.hh>
#include <map>
#include <sys/types.h>
#include <vector>

/* NOTE each of these represent a line and its corresponding address, compilation_unit */
class Lineinfo{

    public:
        int m_line_number;
        uint64_t m_address;
        int m_unit_number;

        Lineinfo(int line_number, uint64_t address, int unit_number);
};

/* NOTE if init_debug_lines fails, do not end the process */
/* NOTE if init_debug_lines fails, user should not be able to use select && other related commands */
class DebugLineInfo{

    private:
        std::vector<Lineinfo*> D_lines;

    public:
        bool b_dwarf_state;

        DebugLineInfo()
            :b_dwarf_state(true) {}

        ~DebugLineInfo(){

            for(auto x = D_lines.begin(); x != D_lines.end(); x++){

                delete((*x));
            }
            D_lines.clear();
        }

        //int InitDebugLines(Debug& debug);
        //void ParseLines(Debug& debug, const::dwarf::line_table &lt, uint64_t base_addr, int unit_number);
        void AppendElement(int line_number, uint64_t address, int unit_number);
        uint64_t GetElementByIndex(int index);
        uint64_t GetAddressByLine(int compilation_unit, int line_number);
        uint64_t GatBaseAddress(pid_t pid);
        int GetNoOfCompilationUnits(void);
        int GetMaxLineNumber(int compilation_unit);
        int GetMinLineNumber(int compilation_unit);
        int ListSrcLines(int compilation_unit);
};

