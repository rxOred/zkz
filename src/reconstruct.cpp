#include <sched.h>
#include <elf.h>
#include <sys/ptrace.h>
#include <sys/types.h>

#include "log.h"
#include "bin.h"

int Elf::ReadTextSegment(pid_t pid, )
