#include <vector>
#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <elf.h>
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

        /* elf section header information */
        Elf64_Ehdr *m_ehdr;
        Elf64_Phdr *m_phdr;
        Elf64_Shdr *m_shdr;

    public:

        pid_t m_pid;
        char *m_pathname;       /* pathname of binary */
        std::vector<Syminfo *> S_list;  /* NOTE free */

        Elf(pid_t pid, const char *pathname){

            m_pid = pid;
            m_size = 0;
            m_pathname = strdup((char *)pathname);
            m_ehdr = nullptr;
            m_phdr = nullptr;
            m_shdr = nullptr;
            m_mapping = nullptr;
        }

        ~Elf(){

            for(int i = 0; i < S_list.size(); i++){

                free(S_list[i]->m_symbol);       // freeing strdup'ed memory
                free(S_list[i]);         // freeing  what ever the fuck this is
            }

            if(!S_list.empty())
                S_list.clear();
        }

        int OpenFile(void);
        int LoadFile(int fd, int size);
        uint64_t GetBaseAddress();
        int LoadSymbols(std::vector<Syminfo *> &S_list, uint64_t base_addr);
        void Free(void);
        void RemoveMap(void);
        int ParseDynamic(void);
};
