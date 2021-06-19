#include "registers.h"
#include <bits/stdint-uintn.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <vector>
#include <cstdio>
#include <stdlib.h>

int info_registers_all(Debug& debug, struct user_regs_struct& regs){

   if(ptrace(PTRACE_GETREGS, debug.get_pid(), nullptr, &regs) < 0){

       perror("[X] Ptrace error");
       return -1;
    }
    printf("Info registers :\n\tRAX: 0x%llx\n\tRCX: 0x%llx\n\tRDX: 0x%llx\n\tRBX: 0x%llx\n\tRSP: "
          "0x%llx\n\tRBP: 0x%llx\n\tRSI: 0x%llx\n\tRDI: 0x%llx\n\tR8: 0x%llx\n\tR9: 0x%llx\n\tR10: "
          "0x%llx\n\tR11: 0x%llx\n\tR12: 0x%llx\n\tR13: 0x%llx\n\tR14: 0x%llx\nR15: 0x%llx\n", 
          regs.rax, regs.rcx, regs.rdx, regs.rbx, regs.rsp, regs.rbp, regs.rsi, regs.rdi, 
          regs.r8, regs.r9, regs.r10, regs.r11, regs.r12, regs.r13, regs.r14, regs.r15);

    return 0;
}

#define RAX     regs.rax
#define RCX     regs.rcx
#define RDX     regs.rdx
#define RBX     regs.rbx
#define RSP     regs.rsp
#define RBP     regs.rbp
#define RSI     regs.rsi
#define RDI     regs.rdi
#define R8      regs.r8
#define R9      regs.r9
#define R10     regs.r10
#define R11     regs.r11
#define R12     regs.r12
#define R13     regs.r13 
#define R14     regs.r14
#define R15     regs.r15

int info_register(Debug& debug, struct user_regs_struct& regs, std::string reg){

    std::map<unsigned long long int, std::string> registers = {
        {RAX, "rax"}, {RCX, "rcx"}, {RDX,"rdx"}, 
        {RBX, "rbx"}, {RSP, "rsp"}, {RBP,"rbp"},
        {RSI, "rsi"}, {RDI, "rdi"}, {R8, "r8"}, 
        {R9, "r9"}, {R10,"r10"}, {R11, "r11"}, 
        {R12, "r12"}, {R13, "r13"}, {R14, "r14"}, 
        {R15, "r15"}
    };

    std::map<unsigned long long int, std::string>::iterator cursor;

    if(ptrace(PTRACE_GETREGS, debug.get_pid(), nullptr, regs) < 0){

        perror("[X] Ptrace failed");
        return -1;
    }
    for (cursor = registers.begin(); cursor != registers.end(); cursor++){

        if(!cursor->second.compare(reg)){

            fprintf(stdout, "info %s :%llx\n", cursor->second.c_str(), cursor->first);
        }
    }
    return 0;
}

int set_register(Debug& debug, struct user_regs_struct& regs, std::string reg, uint64_t value){

    std::vector<std::string> registers = {"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"};

    for(int i = 0; i < registers.size(); i++){

        if(registers[i].compare(reg)){

            switch(i){

            case 0:
                RAX = value;
                break;

            case 1:
                RCX = value;
                break;

            case 2:
                RDX = value;
                break;

            case 3:
                RBX = value;
                break;

            case 4:
                RSP = value;
                break;

            case 5:
                RBP = value;
                break;

            case 6:
                RSI = value;
                break;

            case 7:
                RDI = value;
                break;

            case 8:
                R8 = value;
                break;

            case 9:
                R9 = value;
                break;

            case 10:
                R10 = value;
                break;

            case 11:
                R11 = value;
                break;

            case 12:
                R12 = value;
                break;

            case 13:
                R13 = value;
                break;

            case 14:
                R14 = value;
                break;

            case 15:
                R15 = value;
                break;
            }

            if(ptrace(PTRACE_SETREGS, debug.get_pid(), nullptr, &regs) < 0){

                perror("[X] Ptrace error");
                return -1;
            }

            return 0;
        }
    }
    printf("Invalid register name\n");
    return 0;
}
