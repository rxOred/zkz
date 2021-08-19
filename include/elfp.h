#ifndef ELF_H
#define ELF_H

#include <cstddef>
#include <elf.h>
#include <bits/stdint-uintn.h>
#include <fcntl.h>
#include <vector>
#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

typedef struct _segdata Segdata;

struct _segdata{
    uint64_t m_addr;
    size_t m_size;

    _segdata(void){
        m_addr = 0;
        m_size = 0;
    }
};

namespace Process {
    /*
     * read len size chunk of memory from the process backing pid
     */
    int pread(pid_t pid, void *dst, uint64_t start_addr, size_t
            len);
    /* 
     * write len size chunk of memory to process backing pid
     */
    int pwrite(pid_t pid, void *src, uint64_t start_addr, size_t
            len);
    Segdata *get_segment_data(const char *permission_str, pid_t
            pid);
    uint64_t find_free_space(pid_t pid, uint64_t start_addr, 
            size_t len, size_t shellcode_sz, short key);
}

typedef struct {
    uint64_t m_address;
    char *m_symbol;
} Syminfo;

class Elf {
    protected:
        /* 
         * size of mapped buffer and mapped buffer 
         */
        int m_size;

        /*
         * pathname of the binary
         */
        const char *m_pathname;

        /*
         * pointer to mapped region
         */
        uint8_t *m_mapping;

        /* 
         * base load address of the child
         */
        uint64_t m_base_addr;

        /* 
         * elf header
         */
        Elf64_Ehdr *m_ehdr;

        /*
         * program header table
         */
        Elf64_Phdr *m_phdr;

        /*
         * section header table
         */
        Elf64_Shdr *m_shdr;

    public:
        Elf(const char *pathname, uint64_t base_addr)
            : m_size(0), m_pathname(pathname), m_base_addr(base_addr)
              , m_ehdr(nullptr), m_phdr(nullptr), m_shdr(nullptr),
              m_mapping(nullptr)
        {}

        ~Elf();

        inline int GetNumberOfSections(void) const
        {
            return m_ehdr->e_shnum;
        }

        inline int GetNumberOfSegments(void) const
        {
            return m_ehdr->e_phnum;
        }

        inline Elf64_Shdr *GetSectionHeaderTable(void) const
        {
            return m_shdr;
        }

        inline Elf64_Phdr *GetProgramHeaderTable(void) const
        {
            return m_phdr;
        }

        inline void *GetMemoryMap(void) const
        {
            return m_mapping;
        }

        inline Elf64_Ehdr *GetElfHeader(void) const
        {
            return m_ehdr;
        }

        void RemoveMap(void);
        int OpenElf(const char *pathname);
        int LoadFile(int fd);
};

#endif /* ELF_H */
