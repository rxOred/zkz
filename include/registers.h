#pragma once

#include <sys/user.h>
#include "main.h"
#include <bits/stdint-uintn.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <map>

int info_registers_all(Debug& debug, struct user_regs_struct& regs);
int info_register(Debug& debug, struct user_regs_struct& regs, std::string reg);
int set_register(Debug& debug, struct user_regs_struct& regs, std::string reg, uint64_t value);
