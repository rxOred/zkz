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
