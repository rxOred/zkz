#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sched.h>
#include <elf.h>
#include <sys/ptrace.h>
#include <sys/types.h>

#include "elfp.h"
#include "log.h"
#include "reconstruct.h"

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

#ifdef  ELF
    if(m_phdr == nullptr || m_ehdr == nullptr){
        log.Error("Initialize elf header and program header table\n");
        goto failed;
    }

    for(int i = 0; i < m_ehdr->e_phnum; i++){
        if(m_phdr[i].p_type == PT_LOAD && m_phdr[i].p_flags == (PF_R |
                    PF_X)){
            v_addr = m_phdr[i].p_vaddr;
            v_sz = m_phdr[i].p_memsz;
        }
    }

#else
    char pathname[100];
    sprintf(pathname, "/proc/%d/maps", m_pid);
    FILE *fh = fopen(pathname, "r");
    if(fh == nullptr){
        log.PError("error opening proc file");
        goto failed;
    }



#endif
    if(Process::pread(m_pid, m_seg_text, v_addr, v_sz))
failed:
    return -1;
}
/*
 * read and extract data segment (GOT located in this segment)
 */
int Reconstruct::ReadDataSegment(void)
{
    if(ReadSegment(m_seg_data, PT_LOAD, PF_R | PF_W) < 0)
        return -1;
    return 0;
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
