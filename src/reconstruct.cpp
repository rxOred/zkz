#include <sched.h>
#include <elf.h>
#include <sys/ptrace.h>
#include <sys/types.h>

#include "log.h"
#include "elfp.h"

/*
 * reconstruct damaged/stripped dynamically linked binary files
 */

/*
 * first read the text segment and extract it (.plt located in this segment)
 */

/*
 * read and extract data segment (GOT located in this segment)
 */

/*
 * program header table can be used to do this task
 */

/*
 * locate GOT in data segment using dynamic segment->d_tag==DT_PLTGOT
 */
