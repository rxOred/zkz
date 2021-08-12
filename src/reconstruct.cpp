#include <cstdint>
#include <cstring>
#include <sched.h>
#include <elf.h>
#include <sys/ptrace.h>
#include <sys/types.h>

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
int Reconstruct::ReadSegment(void *dst, uint32_t p_type, 
        uint32_t p_flags)
{
    if(m_phdr == nullptr || m_ehdr == nullptr){
        log.Error("Initialize elf header and program header table\n");
        goto failed;
    }

    for(int i = 0; i < m_ehdr->e_phnum; i++){
        if(m_phdr[i].p_type == p_type && m_phdr[i].p_flags == p_flags){
            dst = calloc(sizeof(uint8_t), m_phdr[i].p_filesz);
            if((uint8_t *)dst == nullptr){
                log.PError("Error allocating memory");
                goto failed;
            }

            if(memcpy((uint8_t *)dst, &m_mapping[m_phdr[i].p_offset], 
                    m_phdr[i].p_filesz) == nullptr){
                log.PError("Error copying data");
                goto mem_failed;
            }
        }
    }

    return 0;

mem_failed:
    free(dst);

failed:
    return -1;
}

int Reconstruct::ReadTextSegment(void)
{
    if(ReadSegment(m_seg_text, PT_LOAD, PF_X | PF_R) < 0)
        return -1;
    return 0;
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
 * program header table can be used to do this task
 */

/*
 * locate GOT in data segment using dynamic segment->d_tag==DT_PLTGOT
 */
void Reconstruct::LocateGlobalOffsetTable()
{

}
