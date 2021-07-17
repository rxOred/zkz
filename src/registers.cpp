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

static void handler_rax(struct user_regs_struct& regs){


}
static void handler_rcx(struct user_regs_struct& regs){


}
static void handler_rdx(struct user_regs_struct& regs){


}
static void handler_rbx(struct user_regs_struct& regs){


}
static void handler_rsp(struct user_regs_struct& regs){


}
static void handler_rbp(struct user_regs_struct& regs){


}
static void handler_rsi(struct user_regs_struct& regs){


}
static void handler_rdi(struct user_regs_struct& regs){


}
static void handler_r8(struct user_regs_struct& regs){


}
static void handler_r9(struct user_regs_struct& regs){


}
static void handler_r10(struct user_regs_struct& regs){


}
static void handler_r11(struct user_regs_struct& regs){


}
static void handler_r12(struct user_regs_struct& regs){


}
static void handler_r13(struct user_regs_struct& regs){


}
static void handler_r14(struct user_regs_struct& regs){


}
static void handler_r15(struct user_regs_struct& regs){


}

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
        {"rax", handler_rax}, {"rcx", handler_rcx},
        {"rdx", handler_rdx}, {"rbx", handler_rbx},
        {"rsp", handler_rsp}, {"rbp", handler_rbp},
        {"rsi", handler_rsi}, {"rdi", handler_rdi},
        {"r8", handler_r8}, {"r9", handler_r9},
        {"r10", handler_r10}, {"r11", handler_r11},
        {"r12", handler_r12}, {"r13", handler_r13},
        {"r14", handler_r14}, {"r15", handler_r15}
    }; 
    for(int i = 0; sizeof(registers) / sizeof(registers[0]); i++){

        if(!strcmp(registers[i].regname, regname.c_str())){

            registers[i].handler(regs);
        }
    }
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
