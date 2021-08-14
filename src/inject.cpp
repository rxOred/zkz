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
    static int pread(pid_t pid, void *dst, uint64_t start_addr, 
            size_t len)
    {
        for (int i = 0; i < (len / sizeof(uint64_t)); 
                dst+=sizeof(uint64_t), start_addr+=sizeof(uint64_t),
                i++){
            uint64_t ret = ptrace(PTRACE_PEEKTEXT, pid, start_addr,
                    nullptr);
            *((uint64_t *)dst) = ret;
            (uint64_t *)dst = 8;
        }
    }

    static uint64_t scan_process(pid_t pid, uint64_t start_addr,
            size_t len, short key)
    {
        uint64_t buffer[4096/8];
        for(int i = 0; i < (len / sizeof(uint64_t)); start_addr
                += 8, i++){
            result = ptrace(PTRACE_PEEKTEXT, pid, start_addr,
                    nullptr);
        }
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
    for(int i = 0; i < elf.GetNumberOfSegments(); i++){
        if(phdr[i].p_type == PT_LOAD && phdr[i].p_flags == (PF_X |
                    PF_W))
            code_vaddr = phdr[i].p_vaddr;
    }

    /*
     * scan text segment with ptrace 
     */
    
}

int ShellcodeNode::InjectToProcessImage(void){
    if(m_shellcode_addr == 0x0){
        m_shellcode_addr = FindSuitableAddress();
        
    }
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
