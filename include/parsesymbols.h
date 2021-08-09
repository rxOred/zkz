#ifdef PARSESYMBOLS_H
#define PARSESYMBOLS_H

#include "elf.h"

class Symbol: public Elf {
    public:
        /*
         * vector of pathnames already searched
         */
        std::vector<char *> P_list;

        /*
         * vector of Syminfo *
         */
        std::vector<Syminfo *> S_list;   /* list of Syminfo */

        ~Symbol();

        int LoadSymbols();
        void RemoveMap(void);
        int ParseDynamic(void);
        void ListSyms(int prange);
}

#endif
