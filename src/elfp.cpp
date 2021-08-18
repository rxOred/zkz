#include <bits/stdint-uintn.h>
#include <cstddef>
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
#include "elfp.h"
#include "log.h"

#include <sys/ptrace.h>
#include <sys/mman.h>

#define FAILED      1
#define STR_SZ      64
#define ADDR_SZ     16
#define MAP_PATH    "/proc/%d/maps"

Segdata *Process::get_segment_data(char *permission_str, pid_t 
        pid)
{
    Segdata *seg = new Segdata;
    std::string line;
    /* dont wanna waste stack lol.. just kidding */
    char *proc_path = (char *)calloc(sizeof(char), STR_SZ);
    if(proc_path == nullptr){
        log.PError("Memory allocation error");
        goto failed;
    }

    char *addr_buf = (char *)calloc(sizeof(char), ADDR_SZ);
    if(addr_buf == nullptr){
        log.PError("Memory allocation error");
        goto m_failed;
    }

    sprintf(proc_path, MAP_PATH, pid);
m_failed:
    free(proc_path);

failed:
    return seg;
}

int Process::pread(pid_t pid, void *dst, uint64_t start_addr, 
        size_t len)
{
    uint64_t *_dst = (uint64_t *)dst;
    for(int i = 0; i < (len / sizeof(uint64_t)); i++, _dst +=
            sizeof(uint64_t), start_addr += sizeof(uint64_t)){
        uint64_t ret = ptrace(PTRACE_PEEKTEXT, pid, start_addr,
                nullptr);
        if(ret < 0) {
            log.PError("Ptrace failed");
            return -1;
        }
        *_dst =  ret;
    }

    return 0;
}

int Process::pwrite(pid_t pid, void *src, uint64_t start_addr,
        size_t len)
{
    uint64_t *_src = (uint64_t *)src;
    uint64_t _addr = start_addr;
    for (int i = 0; i < (len / sizeof(uint64_t)); i++, start_addr
            +=sizeof(uint64_t), _src+=sizeof(uint64_t)){
        if(ptrace(PTRACE_POKETEXT, pid, _addr, _src) < 0){
            log.PError("Ptrace error");
            goto failed;
        }
    }
    return 0;

failed:
    return -1;
}

uint64_t Process::find_free_space(pid_t pid, uint64_t start_addr, size_t
        len, size_t shellcode_sz, short key)
{
    uint8_t *buffer = (uint8_t*) calloc(len, sizeof(uint8_t));
    if(buffer == nullptr){
        log.PError("Memory allocation failed");
        return 1;
    }

    if(Process::pread(pid, buffer, start_addr, len) < 0)
        return 1;

    uint64_t p_addr = 0x0, c_addr = 0x0;
    size_t p_count = 0, c_count = 0;

    for (size_t i = 0; i < len; i++){
        if(buffer[i] == 0){
            c_count++;
            if(c_count == 0) {
                c_addr = start_addr + i;
            }
            if(c_count == shellcode_sz)
                return c_addr;
        } else {
            if(c_count > p_count){
                p_count = c_count;
                p_addr = c_addr;
            }
            c_count = 0;
            c_addr = 0;
        }
    }

    log.Print("Free space of size %d not found in the process",
            shellcode_sz);
    return 1;
}


inline void Elf::RemoveMap(void)
{
    if(m_mapping != nullptr){
        if(munmap(m_mapping, m_size) < 0){

            log.PError("Memory unmap failed");
        }
    }
    m_mapping = nullptr;
}

Elf::~Elf()
{
    RemoveMap();
    m_ehdr = nullptr;
    m_phdr = nullptr;
    m_shdr = nullptr;
}

int Elf::OpenElf(const char *pathname)
{
    int fd = open(pathname, O_RDONLY);
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
    return -FAILED;
}

int Elf::LoadFile(int fd)
{
    m_mapping = (uint8_t *)mmap(nullptr, m_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if(m_mapping == (uint8_t *) MAP_FAILED){
        log.PError("Memory map failed");
        goto failed;
    }

    m_ehdr = (Elf64_Ehdr *)m_mapping;
    if(m_ehdr->e_ident[0] != 0x7f || m_ehdr->e_ident[1] != 'E' || m_ehdr->e_ident
            [2] != 'L' || m_ehdr->e_ident[3] != 'F'){
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
    return -FAILED;
}
