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
        void ReadTextSegment(void *src, void *dst, int len);
        void ReadSegment(void *src, void *dst, int len);
        void ReadDataSegment(void *src, void *dst, int len);
        void LocateGlobalOffsetTable();
};

#endif /* RECONSTRUCT_H */
