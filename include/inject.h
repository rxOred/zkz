#ifndef INJECT_H
#define INJECT_H

#include <cstddef>
#include <cstdint>
#include <sys/types.h>

#include "elfp.h"

/*
 * NOTE zkz just injects whatever user send in to debugee's memory space.
 * it does not perform run-time reloactions. hell it does not even set rip
 * so you have to do that ~\()/~. but there will be commands to that.
 */

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
        ShellcodeNode(void *shellcode, uint64_t shellcode_addr, size_t shellcode_len)
            : m_shellcode(shellcode), m_shellcode_addr(shellcode_addr), 
            m_shellcode_len(shellcode_len), m_next(nullptr)
        {}

        ~ShellcodeNode();
        inline uint64_t GetShellcodeAddr(void) const
        {
            return m_shellcode_addr;
        }

        inline size_t GetShellcodeLen(void) const
        {
            return m_shellcode_len;
        }

        int InjectToProcessImage(Elf &elf, pid_t pid);
        uint64_t FindSuitableAddress(Elf &elf, pid_t pid) const;
};

/*
 * singly linked list of shellcodes injected to the process
 */
class ShellcodeList{
    private:
        ShellcodeNode *m_head;
        ShellcodeNode *m_tail;
        size_t m_nodes;

    public:
        ShellcodeList()
            :m_head(nullptr), m_nodes(0)
        {}

        ~ShellcodeList();
        int AppendNode(void *shellcode, uint64_t shellcode_addr, size_t 
                shellcode_len);
        ShellcodeNode *GetNodeByAddress(uint64_t shellcode_addr) const;
        int RemoveNode(uint64_t shellcode_addr);
};

#endif /* INJECT_H */
