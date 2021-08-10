#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <sys/types.h>
#include <sys/ptrace.h>

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

int AppendShellcode(void *shellcode, uint64_t shellcode_addr, size_t
        shellcode_len)
{
    void *_shellcode = calloc(shellcode_len, sizeof(uint8_t));
    if(shellcode == nullptr){
        goto failed;
    }

    ShellcodeNode *node = new ShellcodeNode(shellcode, shellcode_addr,
            shellcode_len);

    if(m_head == nullptr)
        m_head = node;

    else {
        
    }
    if(m_tail == nullptr)
        m_tail = node;


failed:
    return -1;
}
