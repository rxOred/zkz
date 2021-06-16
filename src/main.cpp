#include "main.h"
#include <sys/types.h>

Debug::Debug(){
 
    this->arguments.u_dbg.pathname = nullptr;
    this->arguments.u_dbg.pid = 0;
    this->arguments.is_inforeg = false;
    this->arguments.is_systrace = false;
    this->is_running = false;
    this->is_sys_stopped = false;
}

void Debug::set_systrace(void){

    this->arguments.is_systrace = true;
}

void Debug::set_inforeg(void){

    this->arguments.is_inforeg = true;
}

void Debug::set_pid(pid_t pid){

    this->arguments.u_dbg.pid = pid;
}

void Debug::set_pathname(char **pathname){

    this->arguments.u_dbg.pathname = pathname;
}

void Debug::set_program_state(bool state){

    this->is_running = state;
}

void Debug::set_syscall_state(bool state){

    this->is_sys_stopped = state;
}

bool Debug::get_systrace(void) const{

    return this->arguments.is_systrace;
}

bool Debug::get_inforeg(void) const{

    return this->arguments.is_inforeg;
}

pid_t Debug::get_pid(void) const{

    return this->arguments.u_dbg.pid;
}

char **Debug::get_pathname(void) const{

    return this->arguments.u_dbg.pathname;
}

bool Debug::get_program_state(void) const{

    return this->is_running;
}

bool Debug::get_syscall_state(void) const{

    return this->is_sys_stopped;
}
