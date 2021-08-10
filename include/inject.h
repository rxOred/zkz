#ifndef INJECT_H
#define INJECT_H

#include <cstddef>
#include <cstdint>
#include <sys/types.h>

class ShellcodeNode {
    private:
        void *m_shellcode;
        uint64_t m_shellcode_addr;
        size_t m_shellcode_len;
        ShellcodeNode *m_next;

    public:
        ShellcodeNode(void *shellcode, uint64_t shellcode_addr, size_t 
                shellcode_len)
            : m_shellcode(shellcode), m_shellcode_addr(shellcode_addr), 
            m_shellcode_len(shellcode_len)
        {}

        ~ShellcodeNode();
        int InjectToProcessImage();
};

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
        void AppendShellcode(void *shellcode, uint64_t shellcode_addr, size_t
                shellcode_len);
        void RemoveShellcode(uint64_t *shellcode_addr, size_t shellcode_len);
};

#endif /* inject.h */
