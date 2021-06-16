#include <bits/stdint-uintn.h>
#include <stdlib.h>
#include <vector>
#include "breakpoint.h"
#include <cstdio>

Breakpoint::Breakpoint(){

    this->address = 0;
    this->origdata = 0;
    this->breakpoint_number = 0;
    this->is_enabled = false;
}

Breakpoint::Breakpoint(uint64_t address, uint64_t origdata, int breakpoint_number, bool is_enabled){

    this->address = address;
    this->origdata = origdata;
    this->breakpoint_number = breakpoint_number;
    this->is_enabled = is_enabled;
}

bool Breakpoint::get_state(void) const{

    return is_enabled;
}

void Breakpoint::disable_breakpoint(void){

    this->is_enabled = false; 
}

void Breakpoint::enable_breakpoint(void){

    this->is_enabled = true;
}

BreakpointList::BreakpointList(){

    std::vector<Breakpoint*> B_List;
}

int BreakpointList::get_number_of_breakpoints() const{

    return B_List.size();
}

void BreakpointList::append_element(uint64_t address, uint64_t origdata){

    Breakpoint *b = new Breakpoint{address, origdata, get_number_of_breakpoints() + 1, true};
    b->enable_breakpoint();

    B_List.push_back(b);  
}

void BreakpointList::remove_element(int breakpoint_number){

    if(breakpoint_number > get_number_of_breakpoints()){

        printf("[X] Invalid breakpoint number");
        return;
    }
    for (auto x = B_List.begin(); x != B_List.end(); x++){

        if((*x)->breakpoint_number == breakpoint_number){

             delete (*x);
             B_List.erase(x);
        }
        else
            return;
    }
    return;
}

Breakpoint *BreakpointList::get_element_by_index(int index) {

    if(index > get_number_of_breakpoints()){

        printf("[X] Invalid index");
        return nullptr;
    }
    return B_List[index];
}

Breakpoint *BreakpointList::get_element_by_breakpoint_number(int breakpoint_number){

    if(breakpoint_number > get_number_of_breakpoints()){

        printf("[X] Invalid breakpoint number");
        return nullptr;
    }
    for (auto x = B_List.begin(); x != B_List.end(); x++){

        if((*x)->breakpoint_number == breakpoint_number){

            return (*x);
        }
    }
    return nullptr;
}

Breakpoint *BreakpointList::get_element_by_address(uint64_t address){

    for (auto x = B_List.begin(); x != B_List.end(); x++){

        if((*x)->address == address){

            return (*x);
        }
    } 
    return nullptr;
}
