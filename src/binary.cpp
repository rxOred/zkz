#include <bits/stdint-uintn.h>
#include <elf.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "log.h"
#include "main.h"
#include "binary.h"

#include <sys/mman.h>

void Elf::RemoveMap(void){

    if(m_mapping != nullptr){

        if(munmap(m_mapping, m_size) < 0){

            log.PError("Memory unmap failed");
        }
    }
    m_mapping = nullptr;
}

int Elf::OpenFile(void){

    int fd = open((char *)S_list[S_list.size() - 1], O_RDONLY);
    if(fd < 0){

        log.PError("File open error");
        return -1;
    }

    struct stat st;
    if(fstat(fd, &st) < 0){

        log.PError("File open error");
        return -1;
    }

    if(LoadFile(fd, st.st_size) < 0) return -1;     // assigns main header tables


/* NOTE we might want to change what base address we want to get, in case of ld.so */
    uint64_t base_addr = GetBaseAddress();       // retrieve base address so it can be added to address in binary 
    if(base_addr == 0) return -1;
    if(LoadSymbols(base_addr) < 0) return -1;

    return 0;
}

int Elf::LoadFile(int fd, int size){

    m_size = size;

    m_mapping = (uint8_t *)mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(m_mapping == (uint8_t *) MAP_FAILED){

        log.PError("Memory map failed");
        return -1;
    }

    m_ehdr = (Elf64_Ehdr *)m_mapping;
    if(m_ehdr->e_ident[0] != 0x7f || m_ehdr->e_ident[1] != 'E' || m_ehdr->e_ident[2] != 'L' || m_ehdr->e_ident[3] != 'F'){

        log.Error("Not an elf binary\n");
        return -1;
    }

    if(m_ehdr->e_type != ET_DYN){

        log.Error("Not a dynamically linked binary\n");
        return -1;
    }

    if(m_ehdr->e_phoff != 0){ m_phdr = (Elf64_Phdr *) &m_mapping[m_ehdr->e_phoff]; }
    if(m_ehdr->e_shoff != 0){ m_shdr = (Elf64_Shdr *) &m_mapping[m_ehdr->e_shoff]; }

    return 0;
}

uint64_t Elf::GetBaseAddress() const{

    char pathname[1024];
    if(sprintf(pathname, "/proc/%d/maps", m_pid) < 0) {

        log.PError("String operation failed");
        return -1;
    }
    FILE *fh = fopen(pathname, "r");
    if(!fh){

        log.PError("File open error");
        return 0;
    }

    char *buffer = (char *)calloc(16, sizeof(char) * 16);
    if(!buffer){

        log.PError("Memory allocation error");
        return 0;
    }

    for(int i = 0; i < 16; i++){

        char c = fgetc(fh);
        if(c == EOF || c == '\0'){

            log.Print("errror reading proc file\n");
            return 0;
        }

        else if(c == '-') break;

        buffer[i] = c;
    }

    fclose(fh);

    uint64_t addr;
    if(buffer){

        if(sscanf((const char *) buffer, "%lx", &addr) < 0) return -1;
        free(buffer);
    }

    return addr;
}

int Elf::LoadSymbols(uint64_t base_addr){

    char *str = nullptr;
    Elf64_Sym *sym = nullptr;

    for(int i = 0; i < m_ehdr->e_shnum; i++){

        if(m_shdr[i].sh_type == SHT_SYMTAB){    /* why not SHT_DYNSYM ? because it contains symbols which are to be relocated and therefore their address is set to 0 */

            sym = (Elf64_Sym *) &m_mapping[m_shdr[i].sh_offset];
            str = (char *) &m_mapping[m_shdr[m_shdr[i].sh_link].sh_offset];
            for (int j = 0; j < m_shdr[i].sh_size / m_shdr[i].sh_entsize; j++){

                if(ELF64_ST_TYPE(sym[j].st_info) == STT_FUNC){

                    Syminfo *syminfo = (Syminfo *) malloc(sizeof(Syminfo));
                    if(syminfo == nullptr){

                        log.PError("Memory allocation failed");
                        return -1;
                    }

                    if(sym[j].st_value != 0){

                        syminfo->m_symbol = (char *) strdup(&str[sym[j].st_name]);
                        syminfo->m_address = base_addr + sym[j].st_value;
                        S_list.push_back(syminfo);
                    }
                }
            }
        }
    }
    if(ParseDynamic() == -1) return -1;
    return 0;
}

int Elf::ParseDynamic(void){

    Elf64_Dyn *dyn = nullptr; 
    Elf64_Sym *dynsym = nullptr;
    char *dynstr = nullptr;

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

                if(dyn[j].d_tag == DT_RUNPATH){     // NOTE first look for DT_RUNPATH

                    b_is_runpath = true;
                    lib_path = strdup((char *)&dynstr[dyn[j].d_un.d_val]);
                }else if(dyn[j].d_tag == DT_RPATH){

                    if(!b_is_runpath){

                        lib_path = strdup((char *)&dynstr[dyn[j].d_un.d_val]);
                    }
                }else if(dyn[j].d_tag == DT_NEEDED){

                    if((&dynstr[dyn[j].d_un.d_val]) != nullptr){
                        char *buf = (char *)calloc(sizeof(char), strlen(&dynstr[dyn[j].d_un.d_val]) + 1);
                        if(!buf){

                            log.PError("Momory allocation error");
                            return -1;
                        }
                        memcpy(buf, &dynstr[dyn[j].d_un.d_val], strlen(&dynstr[dyn[j].d_un.d_val]));
                        shared_libs.push_back(buf);
                    }
                }
            }
        }
    }

    for (int i = 0; i < shared_libs.size(); i++){

        if(lib_path){

            char *pathname = (char *)calloc(sizeof(char), strlen(lib_path) + strlen(shared_libs[i]));
            if(pathname == nullptr){

                log.PError("Memory allocation error");
                return -1;
            }
            if(strncpy(pathname, lib_path, strlen(lib_path)) == nullptr){

                log.PError("String operation failed");
                return -1;
            }
            if(strcat(pathname, shared_libs[i]) == nullptr){

                log.PError("String operation failed");
                return -1;
            }

            P_list.push_back(pathname);

        }else{

            const char *def_lib = "/lib/";

            char *pathname = (char *)calloc(sizeof(char), strlen(def_lib) + strlen(shared_libs[i]) + 3); 
            if(pathname == nullptr){

                log.PError("Memory alloction failed");
                return -1;
            }

            if(strncpy(pathname, def_lib, strlen(def_lib)) == nullptr){

                log.PError("String operation failed");
                return -1;
            }
            if(strcat(pathname, shared_libs[i]) == nullptr){

                log.PError("String operation failed");
                return -1;
            }

            log.Debug("next search path: %s\tcurrent index :%d\t total number of libraries :%d\n", pathname, i, shared_libs.size());

            P_list.push_back(pathname);
        }

        RemoveMap();
        OpenFile();
    }

    if(lib_path != nullptr)
        free(lib_path);

    for(int i = 0; i < shared_libs.size(); i++){

        free(shared_libs[i]);
    }
    shared_libs.clear();

    return 0;       // NOTE modify this so this function will recurse
}
