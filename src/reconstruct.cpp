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

/*
 * reconstruct damaged/stripped dynamically linked binary files
 */

/*
 * first read the text segment and extract it (.plt located in this segment)
 */
void Reconstruct::ReadSegment(void *src, void *dst, int len)
{
    
}

void Reconstruct::ReadTextSegment(void *src, void *dst, int len)
{

}
/*
 * read and extract data segment (GOT located in this segment)
 */
void Reconstruct::ReadDataSegment(void *src, void *dst, int len)
{

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