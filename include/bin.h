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

        /* size of mapped buffer and mapped buffer */
        int m_size;
        uint8_t *m_mapping;
        uint64_t m_base_addr;

        /* elf section header information */
        Elf64_Ehdr *m_ehdr;
        Elf64_Phdr *m_phdr;
        Elf64_Shdr *m_shdr;

    public:

        bool b_load_failed;
        std::vector<char *> P_list;     /* list of pathnames already searched */
        std::vector<Syminfo *> S_list;   /* list of Syminfo */

        Elf(pid_t pid, const char *pathname, uint64_t base_addr)
            : b_load_failed(false), m_size(0), m_base_addr(base_addr), m_ehdr(nullptr), m_phdr(nullptr), m_shdr(nullptr), m_mapping(nullptr){
            P_list.push_back(strdup(pathname));
        }

        ~Elf(){

            for(int i = 0; i < S_list.size(); i++){

                free(S_list[i]->m_symbol);
                free(S_list[i]);
            }

            if(!S_list.empty())
                S_list.clear();

            if(P_list.size() > 0){

                for(int i = 0; i < P_list.size(); i++){

                    free(P_list[i]);
                }

                if(!P_list.empty()){

                    P_list.clear();
                }
            }
        }

        bool SearchForPath(char *pathname);
        int OpenFile(int index);
        int LoadFile(int fd, int size);
        int LoadSymbols();
        void RemoveMap(void);
        int ParseDynamic(void);
        void ListSyms(int prange);
};

#endif /* bin.h */
