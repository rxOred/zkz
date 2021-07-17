#ifndef DWARF_INFORMATION_H
#define DWARF_INFORMATION_H

#include <bits/stdint-uintn.h>
#include <libelfin/dwarf/dwarf++.hh>
#include <map>
#include <sys/types.h>
#include <vector>

/* 
 * NOTE each of these represent a line and its corresponding address, compilation_unit 
 */
class Lineinfo{

    public:
        int m_line_number;
        uint64_t m_address;
        int m_unit_number;

        Lineinfo(int line_number, uint64_t address, int unit_number);
};

/*
 * NOTE if init_debug_lines fails, do not end the process 
 */

/* 
 * NOTE if init_debug_lines fails, user should not be able to use select && other related commands 
 */

class DebugLineInfo{

    private:
        std::vector<Lineinfo*> D_lines;

    public:
        ~DebugLineInfo();

        inline void AppendElement(int line_number, uint64_t address, 
                int unit_number){

            Lineinfo *l = new Lineinfo(line_number, address, unit_number);
            D_lines.push_back(l);
        }

        /*
         * this method is kinda useless rn
         */
        inline uint64_t GetElementByIndex(int index){

            return D_lines[index]->m_address;
        }

        inline int GetNoOfCompilationUnits(void){

            int max = 0;
            for (auto x = D_lines.begin(); x != D_lines.end(); x++){

                if((*x)->m_unit_number > max){

                    max = (*x)->m_unit_number;
                }
            }
            return max;
        }

        uint64_t GetAddressByLine(int compilation_unit, int line_number);
        uint64_t GatBaseAddress(pid_t pid);
        int GetMaxLineNumber(int compilation_unit);
        int GetMinLineNumber(int compilation_unit);
        int ListSrcLines(int compilation_unit);
};

#endif /* DWARF_INFORMATION_H */
