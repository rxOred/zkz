#include "parsesymbols.h"
#include "log.h"

#define FAILED 1

Symbol::~Symbol()
{
    for(int i = 0; i < S_list.size(); i++){
        free(S_list[i]->m_symbol);
        free(S_list[i]);
    }

    if(!S_list.empty())
        S_list.clear();

    if(P_list.size() > 0){
        for(int i = 0; i < P_list.size(); i++)
            free(P_list[i]);

        if(!P_list.empty())
            P_list.clear();
    }
}

/*
 * this is wrapper around OpenElf, and LoadSymbols
 * corresponding ro InitReconstruction in Reconstruct class
 */
int Symbol::OpenFile(int index){
    log.Debug("%s\n", P_list[index]);
    if(OpenElf((const char *)P_list[index]) < 0){
        goto failed;
    }
    if(LoadSymbols() < 0)
        goto failed;

    return 0;

failed:
    b_load_failed = true;
    return -FAILED;
}

int Symbol::LoadSymbols()
{
    char *str = nullptr;
    Elf64_Sym *sym = nullptr;
    Syminfo *syminfo = nullptr;

    for(int i = 0; i < m_ehdr->e_shnum; i++){
        /*
         * why not SHT_DYNSYM ? because it contains symbols which are to be 
         * relocated and therefore their address is set to 0 
         */
        if(m_shdr[i].sh_type == SHT_SYMTAB){
            sym = (Elf64_Sym *) &m_mapping[m_shdr[i].sh_offset];
            str = (char *) &m_mapping[m_shdr[m_shdr[i].sh_link].sh_offset];

            for (int j = 0; j < m_shdr[i].sh_size / m_shdr[i].sh_entsize; j++){
                if(ELF64_ST_TYPE(sym[j].st_info) == STT_FUNC){
                    syminfo = (Syminfo *) malloc(sizeof(Syminfo));
                    if(syminfo == nullptr){
                        log.PError("Memory allocation failed");
                        goto failed;
                    }

                    if(sym[j].st_value != 0){
                        syminfo->m_symbol = (char *) strdup(&str[sym[j].st_name]);
                        if(!syminfo->m_symbol){
                            log.PError("Memory allocation failed");
                            goto mem_failed;
                        }

                        syminfo->m_address = m_base_addr + sym[j].st_value;
                        S_list.push_back(syminfo);
                    }
                }
            }
        }
    }
    if(ParseDynamic() == -1) {

        goto dyn_failed;
    }
    return 0;

dyn_failed:
    free(syminfo->m_symbol);

mem_failed:
    free(syminfo);

failed:
    RemoveMap();
    b_load_failed = true;
    return -FAILED;
}

inline bool Symbol::SearchForPath(const char *pathname)
{
    for (auto x = P_list.begin(); x != P_list.end(); x++){
        if((*x) == pathname)
            return true;
    }

    return false;
}

int Symbol::ParseDynamic(void)
{
    Elf64_Dyn *dyn = nullptr; 
    Elf64_Sym *dynsym = nullptr;
    char *dynstr = nullptr;

    char *pathname = nullptr;

    char *buf = nullptr;
    char *lib_path = nullptr;
    std::vector<char *> shared_libs;
    bool b_is_runpath = false;

    for (int i = 0; i < m_ehdr->e_shnum; i++){
        if(m_shdr[i].sh_type == SHT_DYNSYM){
            dynsym = (Elf64_Sym *)&m_mapping[m_shdr[i].sh_offset];
            dynstr = (char *)&m_mapping[m_shdr[m_shdr[i].sh_link].sh_offset];
        }
    }

    for (int i = 0; i < m_ehdr->e_shnum; i++){
        if(m_shdr[i].sh_type == SHT_DYNAMIC){
            dyn = (Elf64_Dyn *)&m_mapping[m_shdr[i].sh_offset];

            for(int j = 0; j < m_shdr[i].sh_size / sizeof(Elf64_Dyn); j++){
                if(dyn[j].d_tag == DT_RUNPATH){
                    b_is_runpath = true;
                    lib_path = strdup((char *)&dynstr[dyn[j].d_un.d_val]);
                    if(!lib_path){
                        log.PError("error allocating memory");
                        goto failed;
                    }

                }else if(dyn[j].d_tag == DT_RPATH){
                    if(!b_is_runpath){
                        lib_path = strdup((char *)&dynstr[dyn[j].d_un.d_val]);
                        if(!lib_path){
                            log.PError("error allocating memory");
                            goto failed;
                        }
                    }
                }else if(dyn[j].d_tag == DT_NEEDED){
                    if((&dynstr[dyn[j].d_un.d_val]) != nullptr){
                        buf = (char *)calloc(sizeof(char), strlen(&dynstr[dyn[j]
                                    .d_un.d_val]) + 1);
                        if(!buf){
                            log.PError("Momory allocation error");
                            goto failed;
                        }
                        memcpy(buf, &dynstr[dyn[j].d_un.d_val], strlen(&dynstr[
                                    dyn[j].d_un.d_val]));
                        shared_libs.push_back(buf);
                    }
                }
            }
        }
    }

    for (int i = 0; i < shared_libs.size(); i++){
        if(lib_path){
            pathname = (char *)calloc(sizeof(char), strlen(lib_path) + 
                    strlen(shared_libs[i]));
            if(pathname == nullptr){
                log.PError("Memory allocation error");
                goto mem_failed;
            }

            if(strncpy(pathname, lib_path, strlen(lib_path)) == nullptr){
                log.PError("String operation failed");
                goto str_failed;
            }

            if(strcat(pathname, shared_libs[i]) == nullptr){
                log.PError("String operation failed");
                goto str_failed;
            }

            if(SearchForPath(pathname))
                continue;

            else
                P_list.push_back(pathname);

        }else{
            const char *def_lib = "/lib/";
            pathname = (char *)calloc(sizeof(char), strlen(def_lib) + 
                    strlen(shared_libs[i]) + 3); 
            if(pathname == nullptr){
                log.PError("Memory alloction failed");
                goto mem_failed;
            }

            if(strncpy(pathname, def_lib, strlen(def_lib)) == nullptr){
                log.PError("String operation failed");
                goto str_failed;
            }

            if(strcat(pathname, shared_libs[i]) == nullptr){
                log.PError("String operation failed");
                goto str_failed;
            }

            log.Debug("next search path: %s\tcurrent index :%d\t total number"
                    "of libraries :%d\n", pathname, i, shared_libs.size());

            if(SearchForPath(pathname))
                continue;
            else
                P_list.push_back(pathname);
        }

        RemoveMap();
        OpenFile(P_list.size() - 1);
    }

    if(lib_path != nullptr)
        free(lib_path);

    for(int i = 0; i < shared_libs.size(); i++)
        free(shared_libs[i]);

    shared_libs.clear();

    return 0;

str_failed:
    free(pathname);

mem_failed:
    if(lib_path)
        free(lib_path);
    else if(buf)
        free(buf);

failed:
    RemoveMap();
    b_load_failed = true;
    return -1;
}

void Symbol::ListSyms(int prange)
{
    log.Print("address\t\tsymbol");
    if(prange == -1){
        for(auto x = S_list.begin(); x != S_list.end(); x++)
            log.Print("%x\t\t%s\n", (*x)->m_address, (*x)->m_symbol);
    }

    else{
        for(int i = 0; i < prange; i++)
            log.Print("%x\t\t%s\n", S_list[i]->m_address, S_list[i]->m_symbol);
    }
}
