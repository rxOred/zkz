#include "registers.h"
#include "log.h"
#include <bits/stdint-uintn.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <vector>
#include <cstdio>
#include <stdlib.h>

int InfoRegistersAll(Debug& debug, struct user_regs_struct& regs){

   if(ptrace(PTRACE_GETREGS, debug.GetPid(), nullptr, &regs) < 0){

       perror("[X] Ptrace error");
       return -1;
    }
    log.Print("Info registers :\n\tRAX: %llx\n\tRCX: %llx\n\tRDX: %llx\n\tRBX: %llx\n\tRSP: "
          "%x\n\tRBP: %x\n\tRSI: %x\n\tRDI: %x\n\tR8: %x\n\tR9: %x\n\tR10: "
          "%x\n\tR11: %x\n\tR12: %x\n\tR13: %x\n\tR14: %x\nR15: %x\n", 
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

int InfoRegister(Debug& debug, struct user_regs_struct& regs, std::string reg){

    static std::map<unsigned long long int, std::string> registers = {
        {RAX, "rax"}, {RCX, "rcx"}, {RDX,"rdx"}, 
        {RBX, "rbx"}, {RSP, "rsp"}, {RBP,"rbp"},
        {RSI, "rsi"}, {RDI, "rdi"}, {R8, "r8"}, 
        {R9, "r9"}, {R10,"r10"}, {R11, "r11"}, 
        {R12, "r12"}, {R13, "r13"}, {R14, "r14"}, 
        {R15, "r15"}
    };

    std::map<unsigned long long int, std::string>::iterator cursor;

    if(ptrace(PTRACE_GETREGS, debug.GetPid(), nullptr, regs) < 0){

        log.PError("Ptrace failed");
        return -1;
    }
    for (cursor = registers.begin(); cursor != registers.end(); cursor++){

        if(!cursor->second.compare(reg)){

            log.Print("info %s :%x\n", cursor->second.c_str(), cursor->first);
        }
    }

    //registers.clear();
    return 0;
}

int SetRegister(Debug& debug, struct user_regs_struct& regs, std::string reg, uint64_t value){

    static std::vector<std::string> registers = {"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"};

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

            if(ptrace(PTRACE_SETREGS, debug.GetPid(), nullptr, &regs) < 0){

                log.PError("Ptrace error");
                return -1;
            }

            return 0;
        }
    }
    //registers.clear();
    log.Error("Invalid register name\n");
    return 0;
}
