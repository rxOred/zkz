#ifndef REGISTERS_H
#define REGISTERS_H

#include <sys/user.h>
#include "debug.h"
#include <bits/stdint-uintn.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <map>

int InfoRegistersAll(Debug& debug, struct user_regs_struct& regs);
int InfoRegister(Debug& debug, struct user_regs_struct& regs, std::string reg);
int SetRegister(Debug& debug, struct user_regs_struct& regs, std::string reg, uint64_t value);

#endif /* REGISTERS_H */
