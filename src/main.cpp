#include "main.h"
#include <sys/types.h>

Debug::Debug(){
 
    m_arguments.m_dbg.pathname = nullptr;
    m_arguments.m_dbg.pid = 0;
    m_arguments.b_is_inforeg = false;
    m_arguments.is_systrace = false;
    b_is_running = false;
    b_is_sys_stopped = false;
    b_dwarf_state = true;
    b_sym_state = true;
}

Debug::~Debug(){

    int count = GetCount();
    for(int i = 0; i < count; i++){
 
        free(m_arguments.m_dbg.pathname + i);
    }
}

void Debug::SetDwarf(void){

    b_dwarf_state = false;
}

void Debug::SetSym(void){

    b_sym_state = false;
}

void Debug::SetSystrace(void){

    m_arguments.is_systrace = true;
}

void Debug::SetInforeg(void){

    m_arguments.b_is_inforeg = true;
}

void Debug::SetPid(pid_t pid){

    this->m_arguments.m_dbg.pid = pid;
}

void Debug::SetPathname(char **pathname){

    this->m_arguments.m_dbg.pathname = pathname;
}

void Debug::SetCount(int count){

    this->m_arguments.m_dbg.count = count;
}

void Debug::SetProgramState(bool state){

    b_is_running = state;
}

void Debug::SetSyscallState(bool state){

    b_is_sys_stopped = state;
}

bool Debug::GetDwarfState(void) const {

    return b_dwarf_state;
}

bool Debug::GetSymState(void) const {

    return b_sym_state;
}

bool Debug::GetSystrace(void) const{

    return m_arguments.is_systrace;
}

bool Debug::GetInforeg(void) const{

    return m_arguments.b_is_inforeg;
}

pid_t Debug::GetPid(void) const{

    return m_arguments.m_dbg.pid;
}

char **Debug::GetPathname(void) const{

    return m_arguments.m_dbg.pathname;
}

int Debug::GetCount(void) const{

    return m_arguments.m_dbg.count;
}

bool Debug::GetProgramState(void) const{

    return b_is_running;
}

bool Debug::GetSyscallState(void) const{

    return b_is_sys_stopped;
}
