#include "registers.h"
#include "log.h"
#include <bits/stdint-uintn.h>
#include <string>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <vector>
#include <cstdio>
#include <stdlib.h>
#include <string.h>

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
/* 
 * TODO rip, eflags and other registers
 */

#define DEFINE_HANDLER(__reg) static void reg_handler_##__reg(struct user_regs_struct& regs){ \
        log.Print("%s : %lx\n", #__reg, regs.__reg);            \
        return;                                                 \
    }

DEFINE_HANDLER(rax)
DEFINE_HANDLER(rcx)
DEFINE_HANDLER(rdx)
DEFINE_HANDLER(rbx)
DEFINE_HANDLER(rsp)
DEFINE_HANDLER(rbp)
DEFINE_HANDLER(rsi)
DEFINE_HANDLER(rdi)
DEFINE_HANDLER(r8)
DEFINE_HANDLER(r9)
DEFINE_HANDLER(r10)
DEFINE_HANDLER(r11)
DEFINE_HANDLER(r12)
DEFINE_HANDLER(r13)
DEFINE_HANDLER(r14)
DEFINE_HANDLER(r15)

struct reg{
    char regname[6];
    void (*handler)(struct user_regs_struct&);
};

int InfoRegister(Debug& debug, struct user_regs_struct& regs, 
        std::string regname){

    if(ptrace(PTRACE_GETREGS, debug.GetPid(), nullptr, regs) < 0) {

        log.PError("Ptrace failed");
        return -1;
    }

    struct reg registers[] = {
        {"rax", reg_handler_rax}, {"rcx", reg_handler_rcx},
        {"rdx", reg_handler_rdx}, {"rbx", reg_handler_rbx},
        {"rsp", reg_handler_rsp}, {"rbp", reg_handler_rbp},
        {"rsi", reg_handler_rsi}, {"rdi", reg_handler_rdi},
        {"r8", reg_handler_r8}, {"r9", reg_handler_r9},
        {"r10", reg_handler_r10}, {"r11", reg_handler_r11},
        {"r12", reg_handler_r12}, {"r13", reg_handler_r13},
        {"r14", reg_handler_r14}, {"r15", reg_handler_r15}
    }; 
    for(int i = 0; sizeof(registers) / sizeof(registers[0]); i++){

        if(!strcmp(registers[i].regname, regname.c_str())){

            registers[i].handler(regs);
        }
    }

    return 0;
}


int SetRegister(Debug& debug, struct user_regs_struct& regs, 
        std::string reg, uint64_t value){

    static std::vector<std::string> registers = {"rax", "rcx", "rdx", 
        "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", 
        "r12", "r13", "r14", "r15"};

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
