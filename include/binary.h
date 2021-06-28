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
        std::vector<char *> P_list;     /* list of pathnames already searched */
        std::vector<Syminfo *> S_list;   /* list of Syminfo */

        Elf(pid_t pid, const char *pathname){

            m_pid = pid;
            m_size = 0;
            char *m_pathname = strdup(pathname);
            P_list.push_back(m_pathname); 
            m_ehdr = nullptr;
            m_phdr = nullptr;
            m_shdr = nullptr;
            m_mapping = nullptr;
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
        int OpenFile(void);
        int LoadFile(int fd, int size);
        uint64_t GetBaseAddress() const;
        int LoadSymbols(uint64_t base_addr);
        void RemoveMap(void);
        int ParseDynamic(void);
        void ListSyms(int prange);
};
