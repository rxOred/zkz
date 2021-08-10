#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <cstring>
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

int ShellcodeList::AppendShellcode(void *shellcode, uint64_t shellcode_addr, size_t
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
        m_head->m_next = node;
        m_head = node;
    }

    if(m_tail == nullptr)
        m_tail = node;

    return 0;
}
