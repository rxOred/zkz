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
 * first read the text segment and extract it 
 * (.plt located in this segment)
 */

int Reconstruct::ReadTextSegment(void)
{
failed:
    return -1;
}
/*
 * read and extract data segment (GOT located in this segment)
 */
int Reconstruct::ReadDataSegment(void)
{
    Segdata *seg = Process::get_segment_data("rw-", m_pid)
    if(seg == nullptr)
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
