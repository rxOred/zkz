#ifndef INJECT_H
#define INJECT_H

#include <cstddef>
#include <cstdint>
#include <sys/types.h>

#include "elfp.h"

/* 
 * node that holds shellcode, length and injected address
 */
class ShellcodeNode {
    private:
        void *m_shellcode;
        uint64_t m_shellcode_addr;
        size_t m_shellcode_len;

    public:
        ShellcodeNode *m_next;
        ShellcodeNode(void *shellcode, uint64_t shellcode_addr, 
                size_t shellcode_len)
            : m_shellcode(shellcode), m_shellcode_addr(shellcode_addr), 
            m_shellcode_len(shellcode_len), m_next(nullptr)
        {}

        ~ShellcodeNode();
        int InjectToProcessImage();
};

/*
 * double linked list of shellcodes
 */
class ShellcodeList {
    private:
        ShellcodeNode *m_head;
        ShellcodeNode *m_tail;
        size_t m_nodes;

    public:
        ShellcodeList()
            :m_head(nullptr), m_nodes(0)
        {}

        ~ShellcodeList();
        int AppendShellcode(void *shellcode, uint64_t shellcode_addr, size_t
                shellcode_len);
        void RemoveShellcode(uint64_t *shellcode_addr, size_t shellcode_len);
};

#endif /* inject.h */
