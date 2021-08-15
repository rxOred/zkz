#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <elf.h>
#include <sys/types.h>
#include <sys/ptrace.h>

#include "log.h"
#include "elfp.h"
#include "inject.h"

namespace Process {
    /*
     * read len size chunk of memory from the process backing pid
     */
    static int pread(pid_t pid, void *dst, uint64_t start_addr, 
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

    static int pwrite();

    static uint64_t find_free_space(pid_t pid, uint64_t start_addr,
            size_t len, size_t shellcode_sz, short key)
    {
        uint8_t *buffer = (uint8_t*) calloc(len, sizeof(uint8_t));
        if(buffer == nullptr){
            log.PError("Memory allocation failed");
            return 1;
        }

        if(pread(pid, buffer, start_addr, len) < 0)
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
}

ShellcodeNode::~ShellcodeNode()
{
    free(m_shellcode);
}

/*
 * read a uint64_t with ptrace, check if it is 0x0
 * if yes, continue until, len / uint64_t
 * else break out of the loop
 */


uint64_t ShellcodeNode::FindSuitableAddress(Elf &elf, pid_t pid) 
    const
{
    Elf64_Phdr *phdr = elf.GetProgramHeaderTable();
    off_t code_vaddr = 0x0;
    size_t code_sz = 0;
    for(int i = 0; i < elf.GetNumberOfSegments(); i++){
        if(phdr[i].p_type == PT_LOAD && phdr[i].p_flags == (PF_X |
                    PF_W)){
            code_vaddr = phdr[i].p_vaddr;
            code_sz = phdr[i].p_memsz;
            /*
             * scan text segment with ptrace 
             */
            uint64_t address = Process::find_free_space(pid,
                    code_vaddr, code_sz, m_shellcode_len, 0);
            if(address == 1)
                goto failed;

            return address;
        }
    }


seg_failed:
    log.Error("could not locate code segment");

failed:
    return 1;
}

int ShellcodeNode::InjectToProcessImage(void){
    if(m_shellcode_addr == 0x0){
        m_shellcode_addr = FindSuitableAddress();
        if(m_shellcode_addr == 1)
            goto failed;
    }

    
failed:
    return -1;
}

ShellcodeList::~ShellcodeList()
{
    m_head = nullptr;
    m_nodes = 0;
}

int ShellcodeList::AppendNode(void *shellcode, uint64_t shellcode_addr, 
        size_t shellcode_len)
{
    void *_shellcode = calloc(shellcode_len, sizeof(uint8_t));
    if(_shellcode == nullptr)
        return -1;

    memcpy(_shellcode, shellcode, shellcode_len);

    ShellcodeNode *node = new ShellcodeNode(_shellcode, shellcode_addr,
            shellcode_len);

    if(m_head == nullptr)
        m_head = node;
    else {
        node->m_next = m_head;
        m_head = node;
    }

    if(m_tail == nullptr)
        m_tail = node;

    return 0;
}

ShellcodeNode *ShellcodeList::GetNodeByAddress(uint64_t shellcode_addr) const
{
    ShellcodeNode *cursor = m_head;
    while(cursor != nullptr){
        if(cursor->GetShellcodeAddr() == shellcode_addr){
            log.Print("Shellcode %d length found at address %x",
                    cursor->GetShellcodeLen(), cursor->GetShellcodeAddr());
            return cursor;
        }
        cursor = cursor->m_next;
    }
    log.Print("Could not find a shellcode at address %x", shellcode_addr);
    return nullptr;
}

int ShellcodeList::RemoveNode(uint64_t shellcode_addr)
{
    ShellcodeNode *cursor = m_head;
    ShellcodeNode *prev = nullptr;
    while(cursor != nullptr){
        if(cursor->GetShellcodeAddr() == shellcode_addr){
            log.Print("Shellcode with %d length found at address %x",
                    cursor->GetShellcodeLen(), cursor->GetShellcodeAddr());
            //remove shellcode from the process
            delete(cursor);
            if(prev != nullptr){
                prev->m_next = cursor->m_next;
            }
            return 0;
        }
        prev = cursor;
        cursor = cursor->m_next;
    }

    log.Print("Could not find a shellcode at address %x", shellcode_addr);
    return -1;
}
