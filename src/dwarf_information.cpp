#include "dwarf_information.h"
#include <bits/stdint-uintn.h>
#include <fcntl.h>
#include <fstream>
#include <ios>
#include <libelfin/dwarf/data.hh>
#include <libelfin/dwarf/dwarf++.hh>
#include <iostream>
#include <exception>
#include <sstream>
#include <string>
#include <cstdio>
#include <stdio.h>

#include "log.h"

Lineinfo::Lineinfo(int line_number, uint64_t address, int unit_number){

    m_line_number = line_number;
    m_address = address;
    m_unit_number = unit_number;
}

DebugLineInfo::~DebugLineInfo(){

    for(auto x = D_lines.begin(); x != D_lines.end(); x++){

        delete((*x));
    }
    D_lines.clear();
}

uint64_t DebugLineInfo::GetAddressByLine(int compilation_unit, int line_number){

    if(compilation_unit > GetNoOfCompilationUnits() || line_number > GetMaxLineNumber(compilation_unit) || line_number < GetMinLineNumber(compilation_unit)){

        log.Error("Unit number is out of range\n");
        return -1;
    }

    for(int i = 0; i < D_lines.size(); i++){

        if(D_lines[i]->m_unit_number  == compilation_unit){

            if(D_lines[i]->m_line_number == line_number)
                return D_lines[i]->m_address;
        }
    }
    return -1;
}

uint64_t DebugLineInfo::GatBaseAddress(pid_t pid){

    const char *filename = (char *)malloc(1024);
    if(!filename){

        log.PError("Memory allocation error");
        return -1;
    }

    memset((char *)filename, 0, 1024);

    if(sprintf((char *)filename, "/proc/%d/maps", pid) < 0){

        log.PError("IO error");
        return -1;
    }

    char *address = (char *)calloc(sizeof(char), 17);
    FILE *fh = fopen(filename, "r");

    free((void *)filename);
    int i = 0;
    for (i = 0; i < 16; i++){

        int c = fgetc(fh);
        if(c == '\0'){
            fclose(fh);
            return -1;
        }
        address[i] = c;
    }
    address[i] = '\0';
    fclose(fh);
    uint64_t addr = 0x0;
    if(address)
        sscanf(address, "%lx", &addr);

    free(address);
    return addr;
}

int DebugLineInfo::GetMaxLineNumber(int compilation_unit){

    if(compilation_unit > GetNoOfCompilationUnits()){

        log.Error("Invalid unit number\n");
        return -1;
    }
    int prev_max = 0;
    for (auto x = D_lines.begin(); x != D_lines.end(); x++){

        if((*x)->m_unit_number == compilation_unit){

            if((*x)->m_line_number > prev_max){

                prev_max = (*x)->m_line_number;
            }
        }
    }
    return prev_max;
}

int DebugLineInfo::GetMinLineNumber(int compilation_unit){

    if(compilation_unit > GetNoOfCompilationUnits()){

        log.Error("Invalid unit number\n");
        return -1;
    }
    int prev_min;
    for (auto x = D_lines.begin(); x != D_lines.end(); x++){

        if((*x)->m_unit_number == compilation_unit){

            if((*x)->m_line_number < prev_min){

                prev_min = (*x)->m_line_number;
            }
        }
    }

    return prev_min;
}

int DebugLineInfo::ListSrcLines(int compilation_unit){

    if(compilation_unit > GetNoOfCompilationUnits()){

        log.Error("Unit number out of range\n");
        return 0;
    }
    for(auto x = D_lines.begin(); x != D_lines.end(); x++){

        if((*x)->m_unit_number == compilation_unit){

            log.Print("%d\t%x\n", (*x)->m_line_number, (*x)->m_address);
        }
    }

    return 0;
}
