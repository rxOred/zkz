#ifndef PARSESYMBOLS_H
#define PARSESYMBOLS_H

#include <cstdint>
#include "elfp.h"
#include <vector>

class Symbol : public Elf {
    private:
        /*
         * vector of pathnames already searched
         */
        std::vector<char *> P_list;

        /*
         * vector of Syminfo *
         */
        std::vector<Syminfo *> S_list;

    public:
        bool b_load_failed;
        Symbol(const char *pathname, uint64_t base_addr):Elf(pathname, base_addr)
        {
            P_list.push_back((char *)pathname);
        }

        ~Symbol();

        int OpenFile(int index);
        bool SearchForPath(const char *pathname);
        int LoadSymbols(void);
        int ParseDynamic(void);
        void ListSyms(int prange);
};

#endif /* PARSESYMBOLS_H */
