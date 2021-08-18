#include <fstream>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ios>
#include <sched.h>
#include <elf.h>
#include <string>
#include <sys/ptrace.h>
#include <sys/types.h>

#include "elfp.h"
#include "log.h"
#include "reconstruct.h"

#define ADDR_SZ     16
#define STR_SZ      64
#define MAP_PATH    "/proc/%d/maps"
#define MEM_PATH    "/proc/%d/mem"

Reconstruct::~Reconstruct()
{
    if(m_seg_text)
        free(m_seg_text);

    if(m_seg_data)
        free(m_seg_data);
}

/* opening elf binary for reconstruction */
int Reconstruct::InitReconstruction(void)
{
    if(OpenElf(m_pathname) < 0){
        log.Error("Elf reconstruction failed");
        return -1;
    }

failed:
    return -1;
}

/*
 * reconstruct damaged/stripped dynamically linked binary files
 */

/*
 * first read the text segment and extract it 
 * (.plt located in this segment)
 */

int Reconstruct::ReadTextSegment(void)
{
    uint64_t v_addr = 0;
    size_t v_sz = 0;

    char proc_path[STR_SZ];
    char addr_buf[ADDR_SZ];
    std::string line;

    sprintf(proc_path, MAP_PATH, m_pid);
    std::ifstream proc(proc_path, std::ios::in | std::ios::binary);
    if(proc.is_open()){
        while(std::getline(proc, line, ' ')){
            char prev_c = 0;
            for(int i = 0; i < line.length(); i++){
                if(line[i] == 'r' && line[i+1] == '-' && line[i+2] ==
                        'x'){
                    for(int j = 0; j < 16; j++){
                        addr_buf[j] = line[j];
                        std::sscanf(addr_buf, "%lx", &v_addr);
                        if(Process::pread(m_pid, m_seg_text, v_addr, 
                                    v_sz) < 0)
                            goto file_failed;
                    }
                }
            }
        }
    } else 
        goto failed;

file_failed:
    proc.close();

failed:
    return -1;
}
/*
 * read and extract data segment (GOT located in this segment)
 */
int Reconstruct::ReadDataSegment(void)
{
    uint64_t v_addr = 0;
    size_t v_sz = 0;

    char proc_path[STR_SZ];
    char addr_buf[ADDR_SZ];
    std::string line;

    sprintf(proc_path, MAP_PATH, m_pid);
    std::ifstream proc(proc_path, std::ios::in | std::ios::binary);
    if(proc.is_open()){
        while(std::getline(proc, line, ' ')){
            char prev_c = 0;
            for(int i = 0; i < line.length(); i++){
                if(line[i] == 'r' && line[i+1] == 'r' && line[i+2] ==
                        '-'){
                    for(int j = 0; j < 16; j++){
                        addr_buf[j] = line[j];
                        std::sscanf(addr_buf, "%lx", &v_addr);
                        if(Process::pread(m_pid, m_seg_data, v_addr, 
                                    v_sz) < 0)
                            goto file_failed;
                    }
                }
            }
        }
    } else 
        goto failed;

file_failed:
    proc.close();

failed:
    return -1;
}

/*
 * locate GOT in data segment using dynamic segment->d_tag==DT_PLTGOT
 */
void Reconstruct::LocateGlobalOffsetTable()
{
    uint64_t got_addr = 0x0;
    for (int i = 0; i < m_ehdr->e_phnum; i++){
        if()
    }
}
