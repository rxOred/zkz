#include <bits/stdint-uintn.h>
#include <stdlib.h>
#include <vector>
#include "breakpoint.h"
#include "main.h"
#include <cstdio>

Breakpoint::Breakpoint(){

    this->m_address = 0;
    this->m_origdata = 0;
    this->m_breakpoint_number = 0;
    this->b_is_enabled = false;
}

Breakpoint::Breakpoint(uint64_t address, uint64_t origdata, int breakpoint_number, bool is_enabled){

    this->m_address = address;
    this->m_origdata = origdata;
    this->m_breakpoint_number = breakpoint_number;
    this->b_is_enabled = is_enabled;
}

bool Breakpoint::GetState(void) const{

    return b_is_enabled;
}

void Breakpoint::DisableBreakpoint(void){

    this->b_is_enabled = false; 
}

void Breakpoint::EnableBreakpoint(void){

    this->b_is_enabled = true;
}

BreakpointList::~BreakpointList(){

    for(auto x = B_List.begin(); x != B_List.end(); x++){

        delete((*x));
    }
    B_List.clear();
}

int BreakpointList::GetNoOfBreakpoints() const{

    return B_List.size();
}

void BreakpointList::AppendElement(uint64_t address, uint64_t origdata){

    Breakpoint *b = new Breakpoint{address, origdata, GetNoOfBreakpoints() + 1, true};
    b->EnableBreakpoint();

    B_List.push_back(b);  
}

void BreakpointList::RemoveElement(int breakpoint_number){

    if(breakpoint_number > GetNoOfBreakpoints()){

        log.Error("Invalid breakpoint number");
        return;
    }
    for (auto x = B_List.begin(); x != B_List.end(); x++){

        if((*x)->m_breakpoint_number == breakpoint_number){

             delete (*x);
             B_List.erase(x);
        }
        else
            return;
    }
    return;
}

Breakpoint *BreakpointList::GetElementByIndex(int index) {

    if(index > GetNoOfBreakpoints()){

        log.PError("Invalid index");
        return nullptr;
    }
    return B_List[index];
}

Breakpoint *BreakpointList::GetElementByBreakpointNumber(int breakpoint_number){

    if(breakpoint_number > GetNoOfBreakpoints()){

        log.Error("Invalid breakpoint number");
        return nullptr;
    }
    for (auto x = B_List.begin(); x != B_List.end(); x++){

        if((*x)->m_breakpoint_number == breakpoint_number){

            return (*x);
        }
    }
    return nullptr;
}

Breakpoint *BreakpointList::GetElementByAddress(uint64_t address){

    for (auto x = B_List.begin(); x != B_List.end(); x++){

        if((*x)->m_address == address){

            return (*x);
        }
    } 
    return nullptr;
}
