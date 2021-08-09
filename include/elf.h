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
    private:

        /* 
         * size of mapped buffer and mapped buffer 
         */
        int m_size;

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

        bool b_load_failed;

        Elf(pid_t pid, const char *pathname, uint64_t base_addr)
            : b_load_failed(false), m_size(0), m_base_addr(base_addr), m_ehdr(nullptr), 
            m_phdr(nullptr), m_shdr(nullptr), m_mapping(nullptr)
        {}

        ~Elf();

        bool SearchForPath(char *pathname);
        int OpenElf(const char *filename);
        int LoadFile(int fd);
};

#endif /* ELF_H */
