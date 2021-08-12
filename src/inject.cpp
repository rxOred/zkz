#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <cstring>
#include <sys/types.h>
#include <sys/ptrace.h>

#include "log.h"
#include "inject.h"

ShellcodeNode::~ShellcodeNode()
{
    free(m_shellcode);
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
