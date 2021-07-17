#include "debug.h"
#include "log.h"
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

inline void Debug::SetDwarf(void){

    b_dwarf_state = false;
}

inline void Debug::SetSym(void){

    b_sym_state = false;
}

inline void Debug::SetSystrace(void){

    m_arguments.is_systrace = true;
}

inline void Debug::SetInforeg(void){

    m_arguments.b_is_inforeg = true;
}

inline void Debug::SetPid(pid_t pid){

    this->m_arguments.m_dbg.pid = pid;
}

inline void Debug::SetPathname(char **pathname){

    this->m_arguments.m_dbg.pathname = pathname;
}

inline void Debug::SetCount(int count){

    this->m_arguments.m_dbg.count = count;
}

inline void Debug::SetProgramState(bool state){

    b_is_running = state;
}

inline void Debug::SetSyscallState(bool state){

    b_is_sys_stopped = state;
}

inline bool Debug::GetDwarfState(void) const {

    return b_dwarf_state;
}

inline bool Debug::GetSymState(void) const {

    return b_sym_state;
}

inline bool Debug::GetSystrace(void) const{

    return m_arguments.is_systrace;
}

inline bool Debug::GetInforeg(void) const{

    return m_arguments.b_is_inforeg;
}

inline pid_t Debug::GetPid(void) const{

    return m_arguments.m_dbg.pid;
}

inline char **Debug::GetPathname(void) const{

    return m_arguments.m_dbg.pathname;
}

inline int Debug::GetCount(void) const{

    return m_arguments.m_dbg.count;
}

inline bool Debug::GetProgramState(void) const{

    return b_is_running;
}

inline bool Debug::GetSyscallState(void) const{

    return b_is_sys_stopped;
}
