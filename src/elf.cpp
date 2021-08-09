#include <bits/stdint-uintn.h>
#include <elf.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <unistd.h>

#include "debug.h"
#include "bin.h"
#include "log.h"

#include <sys/mman.h>

#define FAILED 1

inline void Elf::RemoveMap(void){

    if(m_mapping != nullptr){

        if(munmap(m_mapping, m_size) < 0){

            log.PError("Memory unmap failed");
        }
    }
    m_mapping = nullptr;
}

int Elf::OpenElf(const char *filename)
{
    int fd = open(filename, O_RDONLY);
    if(fd < 0){
        log.PError("File open error");
        goto failed;
    }

    struct stat st;
    if(fstat(fd, &st) < 0){
        log.PError("File open error");
        goto file_failed;
    }

    m_size = st.st_size;
    if(LoadFile(fd) < 0)
        goto file_failed;

    close(fd);
    return 0;

file_failed:
    close(fd);
failed:
    return -1;
}

int Elf::LoadFile(int fd)
{
    m_mapping = (uint8_t *)mmap(nullptr, m_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(m_mapping == (uint8_t *) MAP_FAILED){

        log.PError("Memory map failed");
        goto failed;
    }

    m_ehdr = (Elf64_Ehdr *)m_mapping;
    if(m_ehdr->e_ident[0] != 0x7f || m_ehdr->e_ident[1] != 'E' || m_ehdr->e_ident[2] != 'L' || m_ehdr->e_ident[3] != 'F'){

        log.Error("Not an elf binary\n");
        goto not_elf;
    }

    if(m_ehdr->e_type != ET_DYN){

        log.Error("Not a dynamically linked binary\n");
        goto not_elf;
    }

    if(m_ehdr->e_phoff != 0){ m_phdr = (Elf64_Phdr *) &m_mapping[m_ehdr->e_phoff]; }
    if(m_ehdr->e_shoff != 0){ m_shdr = (Elf64_Shdr *) &m_mapping[m_ehdr->e_shoff]; }

    return 0;

not_elf:
    RemoveMap();

failed:
    b_load_failed = true;
    return -FAILED;
}
