#ifndef BIN_H
#define BIN_H

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

class Elf{

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

        /*
         * vector of pathnames already searched
         */
        std::vector<char *> P_list;

        /*
         * vector of Syminfo *
         */
        std::vector<Syminfo *> S_list;   /* list of Syminfo */

        Elf(pid_t pid, const char *pathname, uint64_t base_addr)
            : b_load_failed(false), m_size(0), m_base_addr(base_addr), m_ehdr(nullptr), m_phdr(nullptr), m_shdr(nullptr), m_mapping(nullptr){
            P_list.push_back(strdup(pathname));
        }

        ~Elf();

        bool SearchForPath(char *pathname);
        int OpenFile(int index);
        int LoadFile(int fd, int size);
        int LoadSymbols();
        void RemoveMap(void);
        int ParseDynamic(void);
        void ListSyms(int prange);
};

#endif /* BIN_H */
