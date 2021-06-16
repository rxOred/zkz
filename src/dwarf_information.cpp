#include "dwarf_information.h"
#include <bits/stdint-uintn.h>
#include <fcntl.h>
#include <fstream>
#include <ios>
#include <libelfin/dwarf/dwarf++.hh>
#include <iostream>
#include <exception>
#include <sstream>
#include <string>
#include <cstdio>
#include <stdio.h>

Lineinfo::Lineinfo(int line_number, uint64_t address, int unit_number, dwarf::compilation_unit *cu){

    this->line_number = line_number;
    this->address = address;
    this->cuinfo.unit_number = unit_number;
    this->cuinfo.cu = cu;
}

int DebugLineInfo::get_number_of_compilation_units(void){

    int max = 0;
    for (auto x = D_lines.begin(); x != D_lines.end(); x++){

        if((*x)->cuinfo.unit_number > max){

            max = (*x)->cuinfo.unit_number;
        }
    }
    return max;
}

void DebugLineInfo::append_element(int line_number, uint64_t address, int unit_number, dwarf::compilation_unit *cu){

    Lineinfo *l = new Lineinfo(line_number, address, unit_number, cu);

    D_lines.push_back(l);
}

uint64_t DebugLineInfo::get_address_by_index(int index){

    return D_lines[index]->address;
}

uint64_t DebugLineInfo::get_address_by_line(int compilation_unit, int line_number){

    if(compilation_unit > get_number_of_compilation_units() || line_number > get_max_line_number(compilation_unit)){

        printf("[X] unit number is out of range\n");
        return -1;
    }

    for(int i = 0; i < D_lines.size(); i++){

        if(D_lines[i]->cuinfo.unit_number  == compilation_unit){

            if(D_lines[i]->line_number == line_number)
                return D_lines[i]->address;
        }
    }
    return -1;
}

int DebugLineInfo::init_debug_lines(Debug& debug){

    char **pathname = debug.get_pathname();
    int fd = open(pathname[0], O_RDONLY);
    if(fd < 0){

        perror("[X] File open error");
        return -1;
    }

    try{

        debug.sourceifo.elf = elf::elf{elf::create_mmap_loader(fd)};
        debug.sourceifo.dwarf = dwarf::dwarf{dwarf::elf::create_loader(debug.sourceifo.elf)};
    }catch(dwarf::format_error& err_1){

        std::cerr << err_1.what() << std::endl;
        return -1;
    }
    return 0;
}

uint64_t DebugLineInfo::get_base_addr(pid_t pid){

    const char *filename = (char *)malloc(1024);
    if(!filename){

        perror("[X] Memory allocation error");
        return -1;
    }

    memset((char *)filename, 0, 1024);

    if(sprintf((char *)filename, "/proc/%d/maps", pid) < 0){

        perror("[X] IO error :");
        return -1;
    }

    std::ifstream file;
    file.open(filename, std::ios::in);
    if(!file){

        printf("[X] Failed to open proc file\n");
        return -1;
    }

    free((void *)filename);

    std::string line;
    std::string address_str;

    while(std::getline(file, line)){

        std::istringstream str(line);
        std::getline(str, line, 'p');

        if(line[line.length() - 1] == 'x'){

            std::istringstream str_1(line);
            std::getline(str_1, address_str, '-');

            uint64_t base_addr = std::stoll(address_str, nullptr, 16);
            return base_addr;
        }
    }

    return -1;
}

int DebugLineInfo::parse_lines(Debug& debug){

    int i = 0;
    uint64_t base_addr = get_base_addr(debug.get_pid());      // read the base address of text segment using /proc/pid/maps
    try{
        for(auto cu : debug.sourceifo.dwarf.compilation_units()){

            const dwarf::line_table &lt = cu.get_line_table();
            for (auto line : lt){

                append_element(line.line, base_addr + line.address, i, &cu);
            }
            i++;
        }
    }catch(std::exception& e){

        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    return 0;
}

int DebugLineInfo::get_max_line_number(int compilation_unit){

    if(compilation_unit > get_number_of_compilation_units()){

        printf("[X] Invalid unit number\n");
        return -1;
    }
    int prev_max = 0;
    for (auto x = D_lines.begin(); x != D_lines.end(); x++){

        if((*x)->cuinfo.unit_number == compilation_unit){

            if((*x)->line_number > prev_max){

                prev_max = (*x)->line_number;
            }
        }
    }
    return prev_max;
}

int DebugLineInfo::list_src_lines(int compilation_unit){

    if(compilation_unit > get_number_of_compilation_units()){

        printf("[X] Unit number out of range\n");
        return 0;
    }
    for(auto x = D_lines.begin(); x != D_lines.end(); x++){

        if((*x)->cuinfo.unit_number == compilation_unit){

            printf("%d\t%lx\n", (*x)->line_number, (*x)->address);
        }
    }

    return 0;
}
