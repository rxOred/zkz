#ifndef ELF_H
#define ELF_H

#include <elf.h>
#include <bits/stdint-uintn.h>
#include <vector>
#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

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
            : m_size(0), m_pathname(pathname), m_base_addr(base_addr), m_ehdr(nullptr)
            , m_phdr(nullptr), m_shdr(nullptr), m_mapping(nullptr)
        {}

        ~Elf();

        void RemoveMap(void);
        int OpenElf(const char *pathname);
        int LoadFile(int fd);
};

#endif /* ELF_H */
