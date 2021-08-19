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

ShellcodeNode::~ShellcodeNode()
{
    free(m_shellcode);
}

/*
 * read a uint64_t with ptrace, check if it is 0x0
 * if yes, continue until, len / uint64_t
 * else break out of the loop
 */

uint64_t ShellcodeNode::FindSuitableAddress(Elf &elf, pid_t pid) const
{
    uint64_t address = 1;
    Segdata *seg = Process::get_segment_data("r-x", pid);
    if(seg == nullptr) goto ret;

    address = Process::find_free_space(pid, seg->m_addr, seg->m_size, 
            m_shellcode_len, 0);
    if(address == 1) goto failed;

failed:
    free(seg);

ret:
    return address;
}

/*
 * NOTE probably inject code into anywhere exec in memory by saving original 
 * content in a buffer. then set rip.. may be this can be another feature
 */
int ShellcodeNode::InjectToProcessImage(Elf &elf, pid_t pid){
    if(m_shellcode_addr == 0x0){
        m_shellcode_addr = FindSuitableAddress(elf, pid);
        if(m_shellcode_addr == 1)
            goto failed;
    }

    Process::pwrite(pid, m_shellcode, m_shellcode_addr, m_shellcode_len);
failed:
    return -1;
}

ShellcodeList::~ShellcodeList()
{
    m_head = nullptr;
    m_nodes = 0;
}

int ShellcodeList::AppendNode(void *shellcode, uint64_t shellcode_addr, size_t
        shellcode_len)
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
            log.Print("Shellcode %d length found at address %x",cursor->
                    GetShellcodeLen(), cursor->GetShellcodeAddr());
            return cursor;
        }
        cursor = cursor->m_next;
    }
    log.Print("Could not find a shellcode at address %x", 
            shellcode_addr);

    return nullptr;
}

int ShellcodeList::RemoveNode(uint64_t shellcode_addr)
{
    ShellcodeNode *cursor = m_head;
    ShellcodeNode *prev = nullptr;
    while(cursor != nullptr){
        if(cursor->GetShellcodeAddr() == shellcode_addr){
            log.Print("Shellcode with %d length found at address %x",cursor->
                    GetShellcodeLen(), cursor->GetShellcodeAddr());
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
