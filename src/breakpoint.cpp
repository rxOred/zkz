#include <bits/stdint-uintn.h>
#include <stdlib.h>
#include <vector>
#include "breakpoint.h"
#include "debug.h"
#include <cstdio>
#include <sys/ptrace.h>
#include "log.h"

Breakpoint::Breakpoint()
{
    m_address = 0;
    m_origdata = 0;
    m_breakpoint_number = 0;
    b_is_enabled = false;
}

Breakpoint::Breakpoint(uint64_t address, uint64_t origdata, int 
        breakpoint_number, bool is_enabled)
{
    m_address = address;
    m_origdata = origdata;
    m_breakpoint_number = breakpoint_number;
    b_is_enabled = is_enabled;
}

bool Breakpoint::GetState(void) const
{
    return b_is_enabled;
}

void Breakpoint::DisableBreakpoint(void)
{
    b_is_enabled = false; 
}

void Breakpoint::EnableBreakpoint(void)
{
    b_is_enabled = true;
}

BreakpointList::~BreakpointList()
{
    for(auto x = B_List.begin(); x != B_List.end(); x++){
        log.Debug("cleaning B_List\n");
        delete((*x));
    }

    B_List.clear();
}

int BreakpointList::GetNoOfBreakpoints() const
{
    return B_List.size();
}

/* add breakpoint to B_List */
void BreakpointList::AppendElement(uint64_t address, uint64_t origdata)
{
    Breakpoint *b = new Breakpoint{address, origdata, 
        GetNoOfBreakpoints() + 1, true};
    b->EnableBreakpoint();

    B_List.push_back(b);  
}

/* BUG Remove breakpoint from its number */
int BreakpointList::RemoveElement(const Debug& debug, int breakpoint_number)
{
    if(breakpoint_number > GetNoOfBreakpoints() || breakpoint_number <= 0){
        log.Error("Invalid breakpoint number\n");
        return 0;
    }

    std::vector<Breakpoint*>::iterator it = B_List.begin();
    int i = 0;
    for(i = 0; i < B_List.size(); i++){
        if(B_List[i]->m_breakpoint_number == breakpoint_number){

            /*
             * Restoring instruction at the address of the breakpoint 
             * we are removing 
             */
            uint64_t data = ptrace(PTRACE_PEEKTEXT, debug.GetPid(),B_List[i]
                    ->m_address, nullptr);
            if((data & 0xcc) == 0xcc){
                if(ptrace(PTRACE_POKETEXT, debug.GetPid(), B_List[i]->m_address
                            , B_List[i]->m_origdata) < 0){
                    log.PError("Ptrace error");
                    return -1;
                }
            }
            delete(B_List[i]);
            B_List.erase(it + i);
            log.Print("Removed breakpoint %d\n", breakpoint_number);
            return 0;
        }
    }
    log.Error("Invalid breakpoint number\n");
    return 0;
}

/* Get element by B_List index */
Breakpoint *BreakpointList::GetElementByIndex(int index) const
{
    if(index > GetNoOfBreakpoints()){
        log.PError("Invalid index");
        return nullptr;
    }

    return B_List[index];
}

/* Get breakpoint using its number */
Breakpoint *BreakpointList::GetElementByBreakpointNumber(int breakpoint_number)
    const
{
    if(breakpoint_number > GetNoOfBreakpoints()){
        log.Error("Invalid breakpoint number\n");
        goto failed;
    }

    for (auto x = B_List.begin(); x != B_List.end(); x++){
        if((*x)->m_breakpoint_number == breakpoint_number){
            return (*x);
        }
    }

failed:
    return nullptr;
}

/* get breakpoint by its address */
Breakpoint *BreakpointList::GetElementByAddress(uint64_t address) const
{
    for (auto x = B_List.begin(); x != B_List.end(); x++){
        if((*x)->m_address == address){
            return (*x);
        }
    }

    return nullptr;
}

void BreakpointList::ListBreakpoints(void) const
{
    log.Print("\taddress\t\tnumber\t\tstate\n");
    for(auto x = B_List.begin(); x !=  B_List.end(); x++){
        char *state = (char *)"enabled";
        if((*x)->GetState() == false){
            state = (char *)"disabled";
        }

        log.Print("\t%x\t%d\t\t%s\n", (*x)->m_address, (*x)->
                m_breakpoint_number, state);
    }
}
