#include <bits/stdint-uintn.h>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <iostream>

#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/personality.h>
#include <sys/user.h>

#include "breakpoint.h"
#include "main.h"
#include "utils.h"
#include "registers.h"
#include "dwarf_information.h"

#define EXIT_STATUS 1
#define STOPPED_STATUS 2
#define SIGNALED_STATUS 3

void kill_process(pid_t pid){

    kill(pid, SIGKILL);
}

int wait_for_process(Debug& debug){

    int wstatus;
    waitpid(debug.get_pid(), &wstatus, 0);

    if(WIFEXITED(wstatus)){

        log.Print("process terminated with %d\n", wstatus);
        debug.set_program_state(false);     // no other function should set program state to false
        return EXIT_STATUS;
    }
    else if(WIFSTOPPED(wstatus)){

        return STOPPED_STATUS;
    }
    else if(WIFSIGNALED(wstatus)){

        log.Print("process recieved a signal %d\n", wstatus);
        return SIGNALED_STATUS;
    }

    return -1;
}

int start_process(Debug& debug){

    if(debug.get_pathname()){    // just an extra checking

        pid_t pid = fork();
        if(pid == -1){

            log.PError("fork failed :");
            return -1;
        }
        else if(pid == 0){

            personality(ADDR_NO_RANDOMIZE);
            if(ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0){

                log.PError("ptrace failed :");
                exit(EXIT_FAILURE);
            }
            char **pathname = debug.get_pathname();
            if(execvp(pathname[0], pathname) == -1){

                log.PError("execvp failed :");
                exit(EXIT_FAILURE);
            }
        }
        else{

            int ret = wait_for_process(debug);
            if(ret == EXIT_STATUS) {

                return 1;
            }
            else if(ret == SIGNALED_STATUS || ret == STOPPED_STATUS){

                debug.set_program_state(true);      // no one should set this to true except start_process an attach_process functions, which are related to start the process
                debug.set_pid(pid);
                return 0;
            }
        }
    }return -1;
}

/* NOTE there is a memory error here when deleting all breakpoints, try valgrind on this */
int remove_all_breakpoints(Debug& debug, BreakpointList& li){

    for (int i = 0; i < li.get_number_of_breakpoints(); i++){

        /* chech errors properly */
        li.remove_element(i + 1);
    }
    return 0;
}

int _disable_breakpoint(BreakpointList& li, int breakpoint_number){

    Breakpoint *b = li.get_element_by_breakpoint_number(breakpoint_number);
    if(b == nullptr){   // if returns null, breakpoint is not found

        log.Print("[X] Breakpoint %d not found\n", breakpoint_number); 
    }

    b->disable_breakpoint();
    log.Print("breakpoint %d disabled\n", breakpoint_number);
    return 0;
}

/* to restore the instruction to its previous state before placing the breakpoint */
int restore_breakpoint(Debug& debug, Breakpoint *b, uint64_t address){

    if(ptrace(PTRACE_POKEDATA, debug.get_pid(), address, b->origdata) < 0){

        log.PError("[X] Ptrace error");
        return -1;
    }

    return 0;
}


/* NOTE there is a error here when placing breakpoints using line numbers */
int place_breakpoint(Debug& debug, BreakpointList& li, uint64_t address){

    uint64_t origdata = ptrace(PTRACE_PEEKDATA, debug.get_pid(), address, nullptr);
    if(origdata < 0){

        log.PError("[X] Ptrace error :");
        return -1;
    }
    li.append_element(address, origdata);

    uint64_t data_w_interrupt = ((origdata & 0xffffffffffffff00) | 0xcc);
    if(ptrace(PTRACE_POKEDATA, debug.get_pid(), address, data_w_interrupt) < 0){

        log.PError("[X] Ptrace error :");
        return -1;
    }

    log.Print("\t[!] Breakpoint placed at address : %lx\n", address);
    return 0;
}

int step_auto(Debug& debug, BreakpointList& li, struct user_regs_struct& regs){

    int ret;
    do{

        if(ptrace(PTRACE_SINGLESTEP, debug.get_pid(), nullptr, nullptr) < 0){

            log.PError("[X] Ptrace error");
            return -1;
        }

        int ret = wait_for_process(debug);
        if(ret == EXIT_STATUS){

            return EXIT_STATUS;
        }
        else if(ret == STOPPED_STATUS || ret == SIGNALED_STATUS){

            log.Print("---> rip: %llx\n", regs.rip);
        }

        if(debug.get_inforeg()){

            info_registers_all(debug, regs);
        }

        if(ptrace(PTRACE_GETREGS, debug.get_pid(), nullptr, &regs) < 0){

            log.PError("[X] Ptrace error");
            return -1;
        }

        Breakpoint *b  = li.get_element_by_address(regs.rip - 1);
        if(b == nullptr){

            continue;
        }
        else{

            if(b->get_state()){


                log.Print("\t[!] Stopped execution at %llx : breakpoint number %d\n", regs.rip -1, b->breakpoint_number);

                restore_breakpoint(debug, b, regs.rip - 1); 
                return 0;
            }
            else{

                restore_breakpoint(debug, b, regs.rip);
                continue;
            }
        }

    } while(ret != EXIT_STATUS);

    return 0;
}

int step_x(Debug& debug, BreakpointList& li, struct user_regs_struct& regs, int number_of_steps){

    for (int i = 0; i < number_of_steps; i++){

        if(ptrace(PTRACE_SINGLESTEP, debug.get_pid(), nullptr, nullptr) < 0){

            log.PError("[X] Ptrace error");
            return -1;
        }

        int ret = wait_for_process(debug);
        if(ret == EXIT_STATUS){

            return EXIT_STATUS;    //handle this, dont accept any commands except run, reset
        }
        else if(ret == STOPPED_STATUS || ret == SIGNALED_STATUS){

            if(debug.get_inforeg()){   // if info reg is set, we print info reg in every damn step

                info_registers_all(debug, regs);
            }

            if(ptrace(PTRACE_GETREGS, debug.get_pid(), nullptr, &regs) < 0){

                log.PError("[X] Ptrace error");
                return -1;
            }

            //we have to check for breakpoints and stuff like that.. not syscalls.
            Breakpoint *b = li.get_element_by_address(regs.rip - 1);
            if(b == nullptr){

                // we are not in a breakpoint and we dont give a shit about syscalls here
                continue;
            }
            else{

                puts("break is set");
                if(b->get_state()){     // return true means breakpoint is enabled

                    log.Print("\n[!] Stopped execution at %llx : breakpoint number %d\n", regs.rip -1, b->breakpoint_number);

                    if(restore_breakpoint(debug, b, regs.rip - 1) < -1) return -1;
                    return 0;
                }
                else{

                    if(restore_breakpoint(debug, b, regs.rip - i) < -1) return -1;
                    continue;
                }
            }
        }
    }

    log.Print("---> rip: %llx\n", regs.rip);
    return 0;
}

int continue_execution(Debug& debug, BreakpointList& li, struct user_regs_struct& regs){

    if(debug.get_syscall_state()){     // if is_sys_stopped == true, we are settting it to false because we continue execution.

        debug.set_syscall_state(false);
    }

    log.Print("\t[!] Continuing execution...\n"); 
    if(debug.get_systrace()){

        if(ptrace(PTRACE_SYSCALL, debug.get_pid(), nullptr, nullptr) < 0){

            log.PError("[X] Ptrace error :");
            return -1;
        }
    }
    else{ 
        if(ptrace(PTRACE_CONT, debug.get_pid(), nullptr, nullptr) < 0){

            log.PError("[X] Ptrace error :");
            return -1;
        }
    }

    int ret = wait_for_process(debug);
    if(ret == EXIT_STATUS) {

        return EXIT_STATUS;
    }
    if(ret == STOPPED_STATUS || ret == SIGNALED_STATUS){    //else, program has stopped

        if(ptrace(PTRACE_GETREGS, debug.get_pid(), nullptr, &regs) < 0){

            log.PError("[X] Ptrace error :");
            return -1;
        }

        Breakpoint *b = li.get_element_by_address(regs.rip - 1);  //if this return null, we are not in a breakpoint, so reason for SIGSTOP/SIGTRAP must be a system call if systrace is enabled
        if(b == nullptr){

            if(debug.get_systrace()){   // if user specified sys trace, then stop and let him execute commands

                log.Print("\t[!] System call intercepted: %llx\n", regs.rax);
                debug.set_syscall_state(true);      // we set system call state to true

                if(debug.get_inforeg()){

                    info_registers_all(debug, regs);
                }
                return 0;
            }
            else {      //if systrace is not enabled, this must be something else, so we just continue

                int ret = continue_execution(debug, li, regs);
                if(ret == EXIT_STATUS) return EXIT_STATUS;    //check if this returns -444, which indates proram has exited
                else if(ret == -1) return -1;
                else return 0;
            }
        }
        else{

            // we have to restore breakpoint instruction before continue
            if(b->get_state()){     // if return true, breakpoint is enabled

                log.Print("\t[!] Stopped execution at %llx : breakpoint number %d\n", regs.rip -1, b->breakpoint_number);

                if(debug.get_inforeg()){

                    info_registers_all(debug, regs);
                }
                if(restore_breakpoint(debug, b, regs.rip - 1) < -1) return -1;
                return 0;
            }
            else{

                if(restore_breakpoint(debug, b, regs.rip - 1) < -1) return -1;
                int ret = continue_execution(debug, li, regs);
                if(ret == EXIT_STATUS) return EXIT_STATUS;
                else if(ret == -1) return -1;
                else return 0;
            }
        }
    }
    return -1;
}

int mainloop(Debug& debug){

    struct user_regs_struct regs;   //for dumping registers
    BreakpointList list;

    DebugLineInfo line_info;
    bool dwarf_state = true;        // to check if debug information is available to us.
 
    if(line_info.init_debug_lines(debug) < 0){

        log.Error("[X] Error reading dwarf information\n");
        dwarf_state = false;
    }
    if(line_info.parse_lines(debug) < 0){

        log.Error("[X] Error reading dwarf information\n");
        dwarf_state  = false;
    }

    int default_compilation_unit = 0;

    while (1) {

        /*  tokenizing commands */
        std::string command;

        log.Print("[zkz] > ");     //prompt
        if(!std::getline(std::cin, command)){   // if returns nothing, IO errorr

            log.Error("[X] IO error\n");
            return -1;
        }
        if(command.empty()) continue;   // if user didnt input anything, just continue

        std::stringstream strstrm(command);
        std::vector<std::string> commands{};
        std::string word;   // use this when getting additional user input

        while(std::getline(strstrm, word, ' ')){

            word.erase(std::remove_if(word.begin(), word.end(), ispunct), word.end());
            commands.push_back(word);
        }

        /* reset is used to restart a process, while it is still running or exited */
        if(command.compare("reset") == 0){

/* TODO there's a bug with reset */
            if(list.get_number_of_breakpoints() >= 1){

                log.Print("[!] Do you want to keep breakpoints? [yes/no] ");
                if(!std::getline(std::cin, word)){

                    log.Error("[X] IO error\n");
                    return -1;
                }

                if(word.empty()){

                    log.Print("Assumed [yes]...\nCleaning all breakpoints...\n");
                    remove_all_breakpoints(debug, list);
                }
                if(word.compare("no") == 0 || word.compare("n") == 0){

                    log.Print("Saving breakpoints...\n");
                }
                else if(word.compare("yes") == 0 || word.compare("y") == 0){

                    log.Print("Cleaning all breakpoints...\n");
                    remove_all_breakpoints(debug, list);
                }
            }
            if(debug.get_pathname() != nullptr) {

                start_process(debug);
                continue;
            }
        }
        if(command.compare("run") == 0 || command.compare("r") == 0 || command.compare("continue") == 0 || command.compare("c") == 0){

            if(!debug.get_program_state()){

                if(debug.get_pathname() != nullptr){

                    if(start_process(debug) < 0) return -1;
                    continue;
                }
            }
            int ret = continue_execution(debug, list, regs);
            if(ret == EXIT_STATUS){     // EXIT_STATUS defines exit of the debugee, but we dont have to care about setting program state because, wait_for_process is taking care of it

                if(debug.get_pathname() == nullptr && debug.get_pid() != 0){    // because we cant start a process without knowning its pathname

                    log.Print("Press any key to exit zkz\n");
                    getchar();
                    return EXIT_STATUS;
                }
                else if (debug.get_pathname() != nullptr){

                    continue;       // if pathname is there, we can continue and let user to enter reset command
                }
            }else if(ret == -1) return -1;
        }

        else if(commands[0].compare("list") == 0){

            if(debug.get_program_state()){

                if(dwarf_state){

                    if(commands.size() < 2){

                        log.Print("Please select a compilation unit > ");
                        if(!std::getline(std::cin, word)){

                            log.Error("[X] IO error");
                            return -1;
                        }

                        if(word.empty()){

                            log.Print("Invalid unit number.. default unit selected");
                            // dump list
                            continue;
                        }

                        int compilation_unit;
                        try{

                            compilation_unit = std::stoi(word, nullptr, 10);
                        }catch (std::out_of_range& err_1){

                            log.Error("[X] Invalid range :: %s\n", err_1.what());
                            continue;
                        }catch(std::invalid_argument& err_2){

                            log.Error("[X] Invalid unit number :: %s\n", err_2.what());
                            continue;
                        }

                        puts("1");
                        line_info.list_src_lines(compilation_unit);
                    }
                    else{

                        int compilation_unit;
                        try{

                            compilation_unit = std::stoi(commands[1],nullptr, 10);
                        }catch(std::out_of_range& err_1){

                            log.Error("[X] Invalid range :: %s\n", err_1.what());
                            continue;
                        }catch(std::invalid_argument& err_2){

                            log.Error("[X] Invalid unit number :: %s\n", err_2.what());
                            continue;
                        }

                        line_info.list_src_lines(compilation_unit);
                    }
                }
                else{

                    log.Print("[X] debug information not available\n");
                    continue;
                }
            }else{

                log.Print("[X] Program is not running\n");
                continue;
            }
        }
        else if(commands[0].compare("select") == 0){

            if(debug.get_program_state()){

                if(dwarf_state){
                    if(commands.size() <= 1){

                        log.Print("\t[!] Please select a comilation unit\n");
                        if(!std::getline(std::cin, word)){

                            log.Error("[X] IO error\n");
                            return -1;
                        }

                        if(word.empty()){

                            log.Print("[X] Invalid comilation unit number\n");
                            continue;
                        }

                        try{

                            default_compilation_unit = std::stoi(word, nullptr, 10);
                        }catch (std::out_of_range const& err_1){

                            log.Error("[X] Invalid range\n");
                            continue;
                        }catch (std::invalid_argument& err_2){

                            log.Error("[X] Invalid compilation unit number\n");
                            continue;
                        }
                    }
                    else{
                        try{

                            default_compilation_unit = std::stoi(commands[1], nullptr, 10);
                            if(default_compilation_unit < 0 || default_compilation_unit > line_info.get_number_of_compilation_units()){

                                log.Print("Compilation unit number is not in range\n");
                                continue;
                            }
                            log.Print("Compilation unit : %d selected\n", default_compilation_unit);
                        }catch (std::out_of_range const& err_1){

                            log.Error("[X] Invalid range\n");
                            continue;
                        }catch (std::invalid_argument& err_2){

                            log.Error("[X] Invalid compilation unit number\n");
                            continue;
                        }
                    }
                }else{

                    log.Print("[X] Debug information not available\n");
                    continue;
                }
            }else{

                log.Print("[!] Process is not running, try restarting the process by 'reset'\n");
                continue;
            }
        }
        else if(commands[0].compare("breakl") == 0 || commands[0].compare("bl") == 0){

            if(debug.get_program_state()){

                if(dwarf_state){

                    uint64_t address;
                    if(commands.size() < 2){

                        log.Print("\t[!] Enter line to place a breakpoint >");
                        if(!std::getline(std::cin, word)){

                            log.Error("[X] IO error\n");
                            return -1;
                        }

                        if(word.empty()){

                            log.Print("[X] Invalid line number\n");
                            continue;
                        }

                        try{

                            address = line_info.get_address_by_line(default_compilation_unit, std::stoi(word, nullptr, 10));
                            if(address < 0){

                                log.Print("[X] Address for corresponding line is not found\n");
                                continue;
                            }
 
                       }catch (std::out_of_range& err_1) {

                            log.Error("[X] Invalid range\n");
                            continue;
                        }catch (std::invalid_argument& err_2) {

                            log.Error("[X] Invalid line number\n");
                            continue;
                        }

                       if(place_breakpoint(debug, list, address) < 0) continue;
                    }
                    else{

                        for (int i = 1; i < commands.size(); i++){
                            try{

                                address = line_info.get_address_by_line(default_compilation_unit, std::stoi(commands[i], nullptr, 10));
                            }catch (std::out_of_range& err_1) {

                                log.Error("[X] Invalid range\n");
                                continue;
                            }catch (std::invalid_argument& err_2) {

                                log.Error("[X] Invalid line number\n");
                                continue;
                            }

                            if(address < 0){        // if this true, indicates that address is specified line does not match to an address;

                                log.Print("[X] Address for corresponding line is not found\n");
                                continue;
                            }
                            if(place_breakpoint(debug, list, address) < 0) continue;
                        }
                    }
                }else {

                    log.Print("[X] Debug information not available\n");
                    continue;
                }
            }
            else{

                log.Print("[!] Process is not running, try restarting the process by 'reset'\n");
                continue;
            }
        }

        else if(commands[0].compare("break")  == 0 || commands[0].compare("b") == 0){

            /* check length of commands vector is less than 2, if yes, user havent mentioned ann address to place a breakpoint */
            if(debug.get_program_state()){

                if(commands.size() <= 1){

                    log.Print("\t[!] Enter memory address to place a breakpoint >");
                    if(!std::getline(std::cin, word)){

                        log.Error("[X] IO error\n");
                        return -1;
                    }

                    if(word.empty()){

                        log.Print("[X] Invalid memory address\n");
                        continue;
                    }

                    /* converting user input string address to a uint64_t address */
                    uint64_t address;

                    try{

                        address = std::stoll(word, nullptr, 16);
                    }catch(std::invalid_argument& err_1){

                        log.Error("[X] Invalid memory address :: %s\n", err_1.what());
                        continue;
                    }catch(std::out_of_range& err_2){

                        log.Error("[X] Invalid range :: %s\n", err_2.what());
                        continue;
                    }
                    if(place_breakpoint(debug, list, address) < 0) continue;
                }

                /* if user has specified more addresses than 1, we are going to place breakpoints in all of those addresses */
                else{

                    /* we are staring from second element [1st index] of the vector because 0th index is the command */
                    for (int i = 1; i < static_cast<int>(commands.size()); i++){

                        uint64_t address;
                        try{

                            address = std::stoll(commands[i], nullptr, 16);
                        }catch(std::out_of_range& err_1){

                            log.Error("[X] Invalid range :: %s\n", err_1.what());
                            continue;
                        }catch(std::invalid_argument& err_2){

                            log.Error("[X] Invalid memory address :: %s\n", err_2.what());
                            continue;
                        }
                        if(place_breakpoint(debug, list, address) < 0) continue;
                    }
                }
            }
            else{

                log.Print("[!] Process is not running, try restarting the process by 'reset'\n");
                continue;
            }
        }
        else if(command.compare("info registers") == 0 || command.compare("i r") == 0 || command.compare("i registers") == 0 || command.compare("info r") == 0){

            if(debug.get_program_state()){
                info_registers_all(debug, regs);
            }
            else {

                log.Print("[!] Process is not running, try restarting the process by 'reset'\n");
                continue; 
            }
        }

        else if(commands[0].compare("delete") == 0 || commands[0].compare("del") == 0 || commands[0].compare("d") == 0){

            if(commands[1].empty()){

                log.Error("\t[!] Enter breakpoint number to remove");
                if(!std::getline(std::cin, word)){

                    log.Error("[X] IO error\n");
                    return -1;
                }

                if(word.empty()){

                    log.Error("[X] Invalid breakpoint number\n");
                    return -1;
                }

                try{

                    list.remove_element(std::stoi(word, nullptr, 10));
                }catch (std::out_of_range& err_1) {

                    log.Error("[X] Invalid range :: %s\n", err_1.what());
                    continue;
                }catch (std::invalid_argument& err_2) {

                    log.Error("[X] Invalid address or line number :: %s\n", err_2.what());
                    continue;
                }

            }
            else{

                for (int i = 1; i < commands.size(); i++){

                    try {

                        list.remove_element(std::stoi(commands[i], nullptr, 10));
                    }catch (std::out_of_range& err_1) {

                        log.Error("[X] Invalid range\n");
                        continue;
                    }catch (std::invalid_argument& err_2){

                        log.Error("[X] Invali address or line number\n");
                        continue;
                    }
                }
            }
        }

        else if(commands[0].compare("set") == 0){

            if(debug.get_program_state()){

                if(commands.size() == 1){

                    log.Print("[!] Enter register name >");
                    if(!std::getline(std::cin, word)){

                        log.Error("[X] IO error ::\n");
                        return -1;
                    }

                    if(word.empty()){

                        log.Print("Invalid register name\n");
                        continue;
                    }

                    std::string regname = word;

                    log.Print("[!] Enter value >");
                    if(!std::getline(std::cin, word)){

                        log.Error("[X] IO error\n");
                        return -1;
                    }

                    if(word.empty()){

                        log.Print("Invalid value\n");
                        continue;
                    }

                    uint64_t value;
                    try{

                        value = std::stoll(word, nullptr, 16);
                    }catch (std::out_of_range& err_1){

                        log.Print("Invalid range :: %s\n", err_1.what());
                        continue;
                    }catch (std::invalid_argument& err_2){

                        log.Print("Invalid value ::%s\n", err_2.what());
                        continue;
                    }
                    if(set_register(debug, regs, regname, value) < -1) return -1;
                }
                else if(commands.size() == 2){

                    log.Print("Enter value >");
                    if(!std::getline(std::cin, word)){

                        log.Error("[X] IO error");
                        return -1;
                    }

                    if(word.empty()){

                        log.Print("[X] Invalid value");
                        continue;
                    }

                    uint64_t value;
                    try{

                        value = std::stoll(word, nullptr, 16);
                    }catch (std::out_of_range& err_1){

                        log.Print("Invalid range :: %s\n", err_1.what());
                        continue;
                    }catch (std::invalid_argument& err_2){

                        log.Print("Invalid value ::%s\n", err_2.what());
                        continue;
                    }
                    if(set_register(debug, regs, commands[1], value) < 0) return -1;
                }
                else if(command.size() == 3){

                    uint64_t value;

                    try{

                        value = std::stoll(commands[2], nullptr, 16);
                    }catch(std::out_of_range& err_1){

                        log.Print("Invalid range :: %s\n", err_1.what());
                        continue;
                    }catch(std::invalid_argument& err_2){
                        log.Print("[X] Invalid argument :: %s\n", err_2.what());
                        continue;
                    }
                    if(set_register(debug, regs, commands[1], value) < 0) return -1;
                }
            }else{

                log.Print("[X] Program is not running");
                continue;
            }
        }
        else if(commands[0].compare("disable") == 0 || commands[0].compare("dis") == 0){

            if(commands.size() < 2){

                log.Error("\t[!] Enter breakpoint number to disable");
                if(!std::getline(std::cin, word)){

                    log.Error("[X] IO error\n");
                    return -1;
                }

                if(word.empty()){

                    log.Error("[X] Invalid breakpoint number\n");
                    return -1;
                }
                //check errors
                if(_disable_breakpoint(list, std::stoi(word, nullptr, 10)) < 0)
                    continue;
            }
            else{

                for (int i = 1; i < commands.size(); i++){

                    try{

                        if(_disable_breakpoint(list, std::stoi(commands[i], nullptr, 10)) < 0)
                            continue;
                    }catch (std::out_of_range& err_1) {

                        log.Error("[X] Invalid range\n");
                        continue;
                    }catch (std::invalid_argument& err_2) {

                        log.Error("[X] Invalid address or line number\n");
                        continue;
                    }
                }
            }
        }

        else if(commands[0].compare("info") == 0){
 
            if(commands[1].empty()){

                log.Print("\t[!] Enter a register > ");
                std::getline(std::cin, word);
                if(word.empty()){

                    log.Error("[X] Invalid register\n");
                    continue;
                }
                else{

                    if(info_register(debug, regs, word) < 0) return -1;
                }
            }
            else{

                if(info_register(debug, regs, commands[1]) < 0) return -1; 
            }
        }

        else if(commands[0].compare("auto") == 0 || commands[0].compare("a") == 0){

            if(step_auto(debug, list, regs) < 0)
                return -1;
        }

        else if(commands[0].compare("step") == 0){

            if(commands.size() > 1){       // if commands vector's length is greater than 1, it means that user provided a number of steps

                int number_of_steps;
                try{

                    number_of_steps = std::stoi(commands[1], nullptr, 10);
                    log.Print("%d\n", number_of_steps);
                }catch (std::out_of_range& err_1){

                    log.Error("[X] Invalid range\n");
                    continue;
                }catch (std::invalid_argument& err_2){

                    log.Error("[X] Invalid address or line number\n");
                    continue;
                }

                int ret = step_x(debug, list, regs, number_of_steps);
                if(ret == EXIT_STATUS){

                    if(debug.get_pathname() == nullptr && debug.get_pid() != 0){

                        log.Print("[!] Press any key to exit zkz > ");
                        getchar();
                        return EXIT_STATUS;
                    }

                    else if(debug.get_pathname() != nullptr){

                        continue;
                    }
                }
                else if (ret == -1) return -1;
            }
            else{       // else we just step 1

                int ret = step_x(debug, list, regs, 1);
                if(ret == EXIT_STATUS){

                    if(debug.get_pathname() == nullptr && debug.get_pid() != 0){

                        log.Print("[!] Press any key to exit zkz > ");
                        getchar();
                        return EXIT_STATUS;
                    }

                    else if(debug.get_pathname() != nullptr){

                        continue;
                    }
                }
                else if (ret == -1) return -1;
            }
        }

        /* else if(command.compare("unwind") == 0){

            // this is some try out thing

            struct user_regs_struct regs;
            ptrace(PTRACE_GETREGS, debug.get_pid(), nullptr, &regs);

            uint64_t rbp = regs.rbp;    // we get the base of current stack frame, base 
            uint64_t prev_base;
            ptrace(PTRACE_PEEKDATA, debug.get_pid(), rbp, &prev_base);    // then we use it 
        } */

        else if(command.compare("exit") == 0){

            log.Print("[!] Do you want to kill the process? [yes/no] ");
            if(!std::getline(std::cin, word)){

                log.Error("IO error\n");
                return -1;
            }
            if(word.empty()){

                log.Print("Assumed [no]\n");
                continue;
            }
            if(word.compare("yes") == 0 || word.compare("y") == 0){

                kill_process(debug.get_pid());
                return 0;
            }
            else continue;
        }

        else if(command.compare("help") == 0){

            print_help();
        }
    }
    return 0;
}

int attach_process(Debug& debug){

    if(debug.get_pid() != 0){
        if(ptrace(PTRACE_ATTACH, debug.get_pid(), nullptr, nullptr) < 0){

            log.PError("ptrace error");
            return -1;
        }

        int ret = wait_for_process(debug);
        if(ret ==  EXIT_STATUS) return 1;
        else if(ret == STOPPED_STATUS || ret == SIGNALED_STATUS)
            return 0;
    }
    return -1;
}

int main(int argc, char *argv[]){

    Debug debug;

    parse_arguments(debug, argc, argv);

    if((debug.get_pathname()[0] != nullptr) && (debug.get_pid() != 0)){    //you cant use both 
        print_usage();
    }
    else if(debug.get_pid() != 0){

        int ret = attach_process(debug);
        if(ret == 1) return 0;   // if we are attaching with a pid, we need a way to get elf binary's pathname
        else if(ret == -1) return -1;
    }
    else if(debug.get_pathname() != nullptr){

        int ret = start_process(debug);
        if(ret == 1) return 0;      // return 1 means debugee is exited.
        else if(ret == -1) return -1;
    }
    /* priting a where we are currently at in child process's code */
    struct user_regs_struct regs;
    if(ptrace(PTRACE_GETREGS, debug.get_pid(), nullptr, &regs) < 0){

        log.PError("[X] Ptrace error :");
        return -1;
    }
    log.Print("[zkz] Started debugging\trip : %llx\n", regs.rip);
    if(mainloop(debug) < 0) return -1;
    return 0;
}
