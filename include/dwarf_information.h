#pragma once

#include "main.h"
#include <bits/stdint-uintn.h>
#include <libelfin/dwarf/dwarf++.hh>
#include <map>
#include <sys/types.h>
#include <vector>

/* NOTE each of these represent a compilation_unit */
typedef struct {

    dwarf::compilation_unit *cu;
    int unit_number;
} Cuinfo;

/* NOTE each of these represent a line and its corresponding address, compilation_unit */
class Lineinfo{

    public:
        int line_number;
        uint64_t address;
        Cuinfo cuinfo;

        Lineinfo(int line_number, uint64_t address, int unit_number, dwarf::compilation_unit *cu);
};

/* NOTE if init_debug_lines fails, do not end the process */
/* NOTE if init_debug_lines fails, user should not be able to use select && other related commands */
class DebugLineInfo{

    private:
        std::vector<Lineinfo*> D_lines;

    public:
        int init_debug_lines(Debug& debug);
        int parse_lines(Debug& debug);
        void append_element(int line_number, uint64_t address, int unit_number, dwarf::compilation_unit *cu);
        uint64_t get_address_by_index(int index);
        uint64_t get_address_by_line(int compilation_unit, int line_number);
        uint64_t get_base_addr(pid_t pid);
        int get_number_of_compilation_units(void);
        int get_max_line_number(int compilation_unit);
        int list_src_lines(int compilation_unit);
};
