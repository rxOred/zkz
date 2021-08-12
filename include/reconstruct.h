#ifndef RECONSTRUCT_H
#define RECONSTRUCT_H

#include "elfp.h"
#include <cstdint>
#include <sched.h>

#include <sys/types.h>

class Reconstruct : public Elf{
    private:
        pid_t m_pid;

        /* buffer that holds text segment */
        void *m_seg_text;

        /* buffer that hodls data segment */
        void *m_seg_data;


    public:
        Reconstruct(pid_t pid, const char *pathname, uint64_t base_addr)
            : Elf(pathname, base_addr), m_pid(pid), m_seg_data(nullptr)
            , m_seg_text(nullptr)
        {}

        ~Reconstruct();
        int InitReconstruction(void);
        int ReadTextSegment(void);
        int ReadSegment(void *dst, uint32_t p_type, uint32_t p_flags);
        int ReadDataSegment(void);
        void LocateGlobalOffsetTable();
};

#endif /* RECONSTRUCT_H */
