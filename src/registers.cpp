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

#define DEFINE_INFO_HANDLER(__reg) static void reg_info_handler_##__reg(struct user_regs_struct& regs){ \
        log.Print("%s : %lx\n", #__reg, regs.__reg);            \
        return;                                                 \
    }

#define DEFINE_SET_HANDLER(__reg) static void reg_set_handler_##__reg(struct user_regs_struct& regs, uint64_t value){     \
    regs.__reg = value;                                     \
    return;                                                 \
}

DEFINE_SET_HANDLER(rax)
DEFINE_SET_HANDLER(rcx)
DEFINE_SET_HANDLER(rdx)
DEFINE_SET_HANDLER(rbx)
DEFINE_SET_HANDLER(rsp)
DEFINE_SET_HANDLER(rbp)
DEFINE_SET_HANDLER(rsi)
DEFINE_SET_HANDLER(rdi)
DEFINE_SET_HANDLER(r8)
DEFINE_SET_HANDLER(r9)
DEFINE_SET_HANDLER(r10)
DEFINE_SET_HANDLER(r11)
DEFINE_SET_HANDLER(r12)
DEFINE_SET_HANDLER(r13)
DEFINE_SET_HANDLER(r14)
DEFINE_SET_HANDLER(r15)


DEFINE_INFO_HANDLER(rax)
DEFINE_INFO_HANDLER(rcx)
DEFINE_INFO_HANDLER(rdx)
DEFINE_INFO_HANDLER(rbx)
DEFINE_INFO_HANDLER(rsp)
DEFINE_INFO_HANDLER(rbp)
DEFINE_INFO_HANDLER(rsi)
DEFINE_INFO_HANDLER(rdi)
DEFINE_INFO_HANDLER(r8)
DEFINE_INFO_HANDLER(r9)
DEFINE_INFO_HANDLER(r10)
DEFINE_INFO_HANDLER(r11)
DEFINE_INFO_HANDLER(r12)
DEFINE_INFO_HANDLER(r13)
DEFINE_INFO_HANDLER(r14)
DEFINE_INFO_HANDLER(r15)

struct reg{
    char regname[6];
    void (*info_handler)(struct user_regs_struct&);
    void (*set_handler)(struct user_regs_struct&, uint64_t);
};

int InfoRegister(Debug& debug, struct user_regs_struct& regs, 
        std::string reg){

    if(ptrace(PTRACE_GETREGS, debug.GetPid(), nullptr, &regs) < 0) {

        log.PError("Ptrace failed");
        return -1;
    }

    struct reg registers[] = {
        {"rax", reg_info_handler_rax, nullptr}, {"rcx", reg_info_handler_rcx, nullptr},
        {"rdx", reg_info_handler_rdx, nullptr}, {"rbx", reg_info_handler_rbx, nullptr},
        {"rsp", reg_info_handler_rsp, nullptr}, {"rbp", reg_info_handler_rbp, nullptr},
        {"rsi", reg_info_handler_rsi, nullptr}, {"rdi", reg_info_handler_rdi, nullptr},
        {"r8", reg_info_handler_r8, nullptr}, {"r9", reg_info_handler_r9, nullptr},
        {"r10", reg_info_handler_r10, nullptr}, {"r11", reg_info_handler_r11, nullptr},
        {"r12", reg_info_handler_r12, nullptr}, {"r13", reg_info_handler_r13, nullptr},
        {"r14", reg_info_handler_r14, nullptr}, {"r15", reg_info_handler_r15, nullptr}
    }; 
    for(int i = 0; sizeof(registers) / sizeof(struct reg); i++)
        if(!strcmp(registers[i].regname, reg.c_str()))
            registers[i].info_handler(regs);

    return 0;
}

int SetRegister(Debug &debug, struct user_regs_struct &regs, 
        std::string reg, uint64_t value){

    struct reg registers[] = {
        {"rax", nullptr, reg_set_handler_rax}, {"rcx", nullptr, reg_set_handler_rcx},
        {"rdx", nullptr, reg_set_handler_rdx}, {"rbx", nullptr, reg_set_handler_rbx},
        {"rsp", nullptr, reg_set_handler_rsp}, {"rbp", nullptr, reg_set_handler_rbp},
        {"rsi", nullptr, reg_set_handler_rsi}, {"rdi", nullptr, reg_set_handler_rdi},
        {"r8", nullptr, reg_set_handler_r8}, {"r9", nullptr, reg_set_handler_r9},
        {"r10", nullptr, reg_set_handler_r10}, {"r11", nullptr, reg_set_handler_r11},
        {"r12", nullptr, reg_set_handler_r12}, {"r13", nullptr, reg_set_handler_r13},
        {"r14", nullptr, reg_set_handler_r14}, {"r15", nullptr, reg_set_handler_r15}
    };

    for(int i = 0; i < sizeof(registers) /  sizeof(struct reg); i++)
        if(!strcmp(registers[i].regname, reg.c_str()))
            registers[i].set_handler(regs, value);

    if(ptrace(PTRACE_SETREGS, debug.GetPid(), nullptr, regs) < 0){

        log.PError("Ptrace failed");
        return -1;
    }

    return 0;
}
