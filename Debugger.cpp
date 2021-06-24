#include <bits/stdint-uintn.h>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <libelfin/dwarf/dwarf++.hh>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
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
    waitpid(debug.GetPid(), &wstatus, 0);

    if(WIFEXITED(wstatus)){

        log.Info("Process terminated with %d\n", wstatus);
        debug.SetProgramState(false);     // no other function should set program state to false
        return EXIT_STATUS;
    }
    else if(WIFSTOPPED(wstatus)){

        return STOPPED_STATUS;
    }
    else if(WIFSIGNALED(wstatus)){

        log.Info("Process recieved a signal %d\n", wstatus);
        return SIGNALED_STATUS;
    }

    return -1;
}

int start_process(Debug& debug){

    if(debug.GetPathname()){    // just an extra checking

        pid_t pid = fork();
        if(pid == -1){

            log.PError("Fork failed");
            return -1;
        }
        else if(pid == 0){

            personality(ADDR_NO_RANDOMIZE);
            if(ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0){

                log.PError("Ptrace failed");
                exit(EXIT_FAILURE);
            }
            char **pathname = debug.GetPathname();
            if(execvp(pathname[0], pathname) == -1){

                log.PError("Execvp failed");
                exit(EXIT_FAILURE);
            }
        }
        else{

            int ret = wait_for_process(debug);
            if(ret == EXIT_STATUS) {

                return 1;
            }
            else if(ret == SIGNALED_STATUS || ret == STOPPED_STATUS){

                debug.SetProgramState(true);      // no one should set this to true except start_process an attach_process functions, which are related to start the process
                debug.SetPid(pid);
                return 0;
            }
        }
    }return -1;
}

/* NOTE there is a memory error here when deleting all breakpoints, try valgrind on this */
int remove_all_breakpoints(Debug& debug, BreakpointList& li){

    for (int i = 0; i < li.GetNoOfBreakpoints(); i++){

        /* chech errors properly */
        li.RemoveElement(i + 1);
    }
    return 0;
}

int _disable_breakpoint(BreakpointList& li, int breakpoint_number){

    Breakpoint *b = li.GetElementByBreakpointNumber(breakpoint_number);
    if(b == nullptr){   // if returns null, breakpoint is not found

        log.Error("Breakpoint %d not found\n", breakpoint_number); 
    }

    b->DisableBreakpoint();
    log.Print("Breakpoint %d disabled\n", breakpoint_number);
    return 0;
}

/* to restore the instruction to its previous state before placing the breakpoint */
int restore_breakpoint(Debug& debug, Breakpoint *b, struct user_regs_struct &regs, uint64_t address){


    /* BUG */
    /* Fix : when we hit a breapoint, next step or continue will call this function to remove int cc
     * instruction. but here, we set int cc'ed address to orginal value and all that but we do not
     * execute restored address, so we might need to reverse rip by 1 and execute that instruction and 
     * then continur */

    log.Debug("restore address %x\n", address);
    if(ptrace(PTRACE_POKETEXT, debug.GetPid(), address, b->m_origdata) < 0){

        log.PError("Ptrace error");
        return -1;
    }
    if(ptrace(PTRACE_GETREGS, debug.GetPid(), nullptr, &regs) < 0){

        log.PError("Ptrace error");
        return -1;
    }
    regs.rip = address;
    if(ptrace(PTRACE_SETREGS, debug.GetPid(), nullptr, &regs) < 0){

        log.PError("Ptrace error");
        return -1;
    }

    if(ptrace(PTRACE_SINGLESTEP, debug.GetPid(), nullptr, nullptr) < 0){

        log.PError("Ptrace error");
        return -1;
    }
    log.Debug("does not make it here\n");

    int ret = wait_for_process(debug);
    if(ret == EXIT_STATUS){

        return EXIT_STATUS;
    }
    else{

        log.Debug("process stopped or signled\n");
    }

    /* NOTE do some log.Debug's to make sure we are doing it right */
    return 0;
}


/* NOTE there is a error here when placing breakpoints using line numbers */
int place_breakpoint(Debug& debug, BreakpointList& li, uint64_t address){

    uint64_t origdata = ptrace(PTRACE_PEEKTEXT, debug.GetPid(), address, nullptr);

    if(origdata < 0){

        log.PError("Ptrace error");
        return -1;
    }
    li.AppendElement(address, origdata);

    uint64_t data_w_interrupt = ((origdata & 0xffffffffffffff00) | 0xcc);
    if(ptrace(PTRACE_POKETEXT, debug.GetPid(), address, data_w_interrupt) < 0){

        log.PError("Ptrace error");
        return -1;
    }

    log.Print("Breakpoint placed at address : %x\n", address);
    return 0;
}

int step_auto(Debug& debug, BreakpointList& li, struct user_regs_struct& regs){

    int ret;
    do{

        if(ptrace(PTRACE_SINGLESTEP, debug.GetPid(), nullptr, nullptr) < 0){

            log.PError("Ptrace error");
            return -1;
        }

        int ret = wait_for_process(debug);
        if(ret == EXIT_STATUS){

            return EXIT_STATUS;
        }
        else if(ret == STOPPED_STATUS || ret == SIGNALED_STATUS){

            log.Print("---> rip: %x\n", regs.rip);
        }

        if(debug.GetInforeg()){

            InfoRegistersAll(debug, regs);
        }

        if(ptrace(PTRACE_GETREGS, debug.GetPid(), nullptr, &regs) < 0){

            log.PError("Ptrace error");
            return -1;
        }

        Breakpoint *b  = li.GetElementByAddress(regs.rip - 1);
        if(b == nullptr){

            continue;
        }
        else{

            if(b->GetState()){


                log.Info("Stopped execution at %x : breakpoint number %d\n", regs.rip -1, b->m_breakpoint_number);

                restore_breakpoint(debug, b, regs, regs.rip - 1);
                return 0;
            }
            else{

                restore_breakpoint(debug, b, regs, regs.rip - 1);
                continue;
            }
        }

    } while(ret != EXIT_STATUS);

    return 0;
}

int step_x(Debug& debug, BreakpointList& li, struct user_regs_struct& regs, int number_of_steps){

    for (int i = 0; i < number_of_steps; i++){

        if(ptrace(PTRACE_SINGLESTEP, debug.GetPid(), nullptr, nullptr) < 0){

            log.PError("Ptrace error");
            return -1;
        }

        int ret = wait_for_process(debug);
        if(ret == EXIT_STATUS){

            return EXIT_STATUS;    //handle this, dont accept any commands except run, reset
        }
        else if(ret == STOPPED_STATUS || ret == SIGNALED_STATUS){

            if(debug.GetInforeg()){   // if info reg is set, we print info reg in every damn step

                InfoRegistersAll(debug, regs);
            }

            if(ptrace(PTRACE_GETREGS, debug.GetPid(), nullptr, &regs) < 0){

                log.PError("Ptrace error");
                return -1;
            }

            //we have to check for breakpoints and stuff like that.. not syscalls.
            Breakpoint *b = li.GetElementByAddress(regs.rip - 1);
            if(b == nullptr){

                // we are not in a breakpoint and we dont give a shit about syscalls here
                continue;
            }
            else{

                if(b->GetState()){     // return true means breakpoint is enabled

                    log.Info("Stopped execution at %x : breakpoint number %d\n", regs.rip -1, b->m_breakpoint_number);

                    if(restore_breakpoint(debug, b, regs, regs.rip - 1) < -1) return -1;
                    return 0;
                }
                else{


                    if(restore_breakpoint(debug, b, regs, regs.rip - i) < -1) return -1;
                    continue;
                }
            }
        }
    }

    log.Print("---> rip: %x\n", regs.rip);
    return 0;
}

int continue_execution(Debug& debug, BreakpointList& li, struct user_regs_struct& regs){

    if(debug.GetSyscallState()){     // if is_sys_stopped == true, we are settting it to false because we continue execution.

        debug.SetSyscallState(false);
    }

    log.Info("Continuing execution...\n"); 
    if(debug.GetSystrace()){

        log.Debug("stopping at syscalls\n");
        if(ptrace(PTRACE_SYSCALL, debug.GetPid(), nullptr, nullptr) < 0){

            log.PError("Ptrace error");
            return -1;
        }
    }
    else{

        if(ptrace(PTRACE_CONT, debug.GetPid(), nullptr, nullptr) < 0){

            log.PError("Ptrace error");
            return -1;
        }
    }

    int ret = wait_for_process(debug);
    if(ret == EXIT_STATUS) {

        return EXIT_STATUS;
    }
    if(ret == STOPPED_STATUS){    //else, program has stopped

        if(ptrace(PTRACE_GETREGS, debug.GetPid(), nullptr, &regs) < 0){

            log.PError("Ptrace error");
            return -1;
        }

        log.Debug("Prcoess stopped at address %x\n", regs.rip - 1);

        Breakpoint *b = li.GetElementByAddress(regs.rip - 1);  //if this return null, we are not in a breakpoint, so reason for SIGSTOP/SIGTRAP must be a system call if systrace is enabled
        if(b == nullptr){

            if(debug.GetSystrace()){   // if user specified sys trace, then stop and let him execute commands
                log.Info("System call intercepted: %x\n", regs.rax);
                debug.SetSyscallState(true);      // we set system call state to true

                if(debug.GetInforeg()){

                    log.Debug("Register information flag is set\n");
                    InfoRegistersAll(debug, regs);
                }
                return 0;
            }
            else {      //if systrace is not enabled, this must be something else, so we just continue

                log.Debug("this is weird\n");

                int ret = continue_execution(debug, li, regs);
                if(ret == EXIT_STATUS) return EXIT_STATUS;
                else if(ret == -1) return -1;
                else return 0;
            }
        }
        else{

            log.Debug("Breakpoint hit\n");
            // we have to restore breakpoint instruction before continue
            if(b->GetState()){     // if return true, breakpoint is enabled

                log.Info("Stopped execution at %x : breakpoint number %d\n", regs.rip -1, b->m_breakpoint_number);

                if(debug.GetInforeg()){

                    InfoRegistersAll(debug, regs);
                }
                log.Debug("Enabled breakpoint\n");
                if(restore_breakpoint(debug, b, regs, regs.rip - 1) < -1) return -1;
                return 0;
            }
            else{

                // if breakpoint is disabled, go restore the instruction and continue execution
                log.Debug("Disabled breakpoint\n");
                if(restore_breakpoint(debug, b, regs, regs.rip - 1) < -1) return -1;
                int ret = continue_execution(debug, li, regs);
                if(ret == EXIT_STATUS) return EXIT_STATUS;
                else if(ret == -1) return -1;
                else return 0;
            }
        }
    }
    return -1;
}

/* i just copied this piece of code from one of libelfin examples */
void parse_lines(Debug &debug, DebugLineInfo &li, const dwarf::line_table &lt, uint64_t base_addr, int unit_number){

    for (auto &line : lt){

        log.Debug("line number :%d\t address :%x\n", line.line, base_addr + line.address);
        li.AppendElement(line.line, base_addr + line.address, unit_number);
    }
}

int init_debug_lines(Debug& debug, DebugLineInfo &li){

    elf::elf elf_f;
    dwarf::dwarf dwarf_f;

    char **pathname = debug.GetPathname();
    int fd = open(pathname[0], O_RDONLY);
    if(fd < 0){

        log.PError("File open error");
        return -1;
    }

    elf_f = elf::elf{elf::create_mmap_loader(fd)};
    dwarf_f = dwarf::dwarf(dwarf::elf::create_loader(elf_f));

    int i = 0;
    uint64_t base_addr = 0x0;
    base_addr = li.GatBaseAddress(debug.GetPid());      // read the base address of text segment using /proc/pid/maps

    for(auto cu : dwarf_f.compilation_units()){

        const dwarf::line_table &lt = cu.get_line_table();
        parse_lines(debug, li, lt, base_addr, i);
        i++;
    }

    log.Debug("Object should not be accessible\n");

    return 0;
}

int mainloop(Debug& debug, DebugLineInfo& line_info){

    log.SetState(PRINT | PROMPT | WARN | INFO | DEBUG | PERROR | ERROR);

    struct user_regs_struct regs;   //for dumping registers
    BreakpointList break_list;


    int default_compilation_unit = 0;

    while (1) {

        /*  tokenizing commands */
        std::string command;

        log.Prompt("[zkz] ");     //prompt
        if(!std::getline(std::cin, command)){   // if returns nothing, IO errorr

            log.Error("IO error\n");
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

/* BUG there's a bug with reset */
            if(break_list.GetNoOfBreakpoints() >= 1){

                log.Prompt("[!] Do you want to keep breakpoints? [yes/no] ");
                if(!std::getline(std::cin, word)){

                    log.Error("IO error\n");
                    return -1;
                }

                if(word.empty()){

                    log.Print("Assumed [yes]...\nCleaning all breakpoints...\n");
                    remove_all_breakpoints(debug, break_list);
                }
                if(word.compare("no") == 0 || word.compare("n") == 0){

                    log.Print("Saving breakpoints...\n");
                }
                else if(word.compare("yes") == 0 || word.compare("y") == 0){

                    log.Print("Cleaning all breakpoints...\n");
                    remove_all_breakpoints(debug, break_list);
                }
            }
            if(debug.GetPathname() != nullptr) {

                start_process(debug);
                continue;
            }
        }
        if(command.compare("run") == 0 || command.compare("r") == 0 || command.compare("continue") == 0 || command.compare("c") == 0){

            if(!debug.GetProgramState()){

                log.Debug("if program is stopped\n");
                if(debug.GetPathname() != nullptr){

                    if(start_process(debug) < 0) return -1;
                    continue;
                }
            }
            int ret = continue_execution(debug, break_list, regs);
            if(ret == EXIT_STATUS){     // E  IT_STATUS defines exit of the debugee, but we dont have to care about setting program state because, wait_for_process is taking care of it

                if(debug.GetPathname() == nullptr && debug.GetPid() != 0){    // because we cant start a process without knowning its pathname

                    log.Prompt("Press any key to exit zkz");
                    getchar();
                    return EXIT_STATUS;
                }
                else if (debug.GetPathname() != nullptr){

                    continue;       // if pathname is there, we can continue and let user to enter reset command
                }
            }else if(ret == -1) return -1;
        }

        else if(commands[0].compare("list") == 0){

            if(debug.GetProgramState()){

                if(line_info.b_dwarf_state){

                    if(commands.size() < 2){

                        log.Prompt("Please select a compilation unit");
                        if(!std::getline(std::cin, word)){

                            log.Error("IO error");
                            return -1;
                        }

                        if(word.empty()){

                            log.Error("Invalid unit number.. default unit selected");
                            // dump list
                            continue;
                        }

                        int compilation_unit;
                        try{

                            compilation_unit = std::stoi(word, nullptr, 10);
                        }catch (std::out_of_range& err_1){

                            log.Error("Invalid range :%s\n", err_1.what());
                            continue;
                        }catch(std::invalid_argument& err_2){

                            log.Error("Invalid unit number :%s\n", err_2.what());
                            continue;
                        }

                        puts("1");
                        line_info.ListSrcLines(compilation_unit);
                    }
                    else{

                        int compilation_unit;
                        try{

                            compilation_unit = std::stoi(commands[1],nullptr, 10);
                        }catch(std::out_of_range& err_1){

                            log.Error("Invalid range :%s\n", err_1.what());
                            continue;
                        }catch(std::invalid_argument& err_2){

                            log.Error("Invalid unit number :%s\n", err_2.what());
                            continue;
                        }

                        line_info.ListSrcLines(compilation_unit);
                    }
                }
                else{

                    log.Print("Debug information not available\n");
                    continue;
                }
            }else{

                log.Print("Program is not running\n");
                continue;
            }
        }
        else if(commands[0].compare("select") == 0){

            if(debug.GetProgramState()){

                if(line_info.b_dwarf_state){
                    if(commands.size() <= 1){

                        log.Prompt("Please select a comilation unit\n");
                        if(!std::getline(std::cin, word)){

                            log.Error("IO error\n");
                            return -1;
                        }

                        if(word.empty()){

                            log.Error("Invalid comilation unit number\n");
                            continue;
                        }

                        try{

                            default_compilation_unit = std::stoi(word, nullptr, 10);
                        }catch (std::out_of_range const& err_1){

                            log.Error("Invalid range\n");
                            continue;
                        }catch (std::invalid_argument& err_2){

                            log.Error("Invalid compilation unit number\n");
                            continue;
                        }
                    }
                    else{
                        try{

                            default_compilation_unit = std::stoi(commands[1], nullptr, 10);
                            if(default_compilation_unit < 0 || default_compilation_unit > line_info.GetNoOfCompilationUnits()){

                                log.Error("Compilation unit number is not in range\n");
                                continue;
                            }
                            log.Print("Compilation unit : %d selected\n", default_compilation_unit);
                        }catch (std::out_of_range const& err_1){

                            log.Error("Invalid range\n");
                            continue;
                        }catch (std::invalid_argument& err_2){

                            log.Error("Invalid compilation unit number\n");
                            continue;
                        }
                    }
                }else{

                    log.Print("Debug information not available\n");
                    continue;
                }
            }else{

                log.Print("[!] Process is not running, try restarting the process by 'reset'\n");
                continue;
            }
        }
        else if(commands[0].compare("breakl") == 0 || commands[0].compare("bl") == 0){

            if(debug.GetProgramState()){

                if(line_info.b_dwarf_state){

                    log.Debug("Breakpoint at line number\n");
                    uint64_t address;
                    if(commands.size() < 2){

                        log.Prompt("Enter line to place a breakpoint >");
                        if(!std::getline(std::cin, word)){

                            log.Error("IO error\n");
                            return -1;
                        }

                        if(word.empty()){

                            log.Error("Invalid line number\n");
                            continue;
                        }

                        try{

                            address = line_info.GetAddressByLine(default_compilation_unit, std::stoi(word, nullptr, 10));
                            if(address < 0){

                                log.Error("Address for corresponding line is not found\n");
                                continue;
                            }
 
                       }catch (std::out_of_range& err_1) {

                            log.Error("Invalid range\n");
                            continue;
                        }catch (std::invalid_argument& err_2) {

                            log.Error("Invalid line number\n");
                            continue;
                        }

                       if(place_breakpoint(debug, break_list, address) < 0) continue;
                    }
                    else{

                        for (int i = 1; i < commands.size(); i++){
                            try{

                                address = line_info.GetAddressByLine(default_compilation_unit, std::stoi(commands[i], nullptr, 10));
                            }catch (std::out_of_range& err_1) {

                                log.Error("Invalid range\n");
                                continue;
                            }catch (std::invalid_argument& err_2) {

                                log.Error("Invalid line number\n");
                                continue;
                            }

                            if(address < 0){        // if this true, indicates that address is specified line does not match to an address;

                                log.Error("Address for corresponding line is not found\n");
                                continue;
                            }
                            if(place_breakpoint(debug, break_list, address) < 0) continue;
                        }
                    }
                }else {

                    log.Print("Debug information not available\n");
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
            if(debug.GetProgramState()){

                if(commands.size() <= 1){

                    log.Prompt("Enter memory address to place a breakpoint");
                    if(!std::getline(std::cin, word)){

                        log.Error("IO error\n");
                        return -1;
                    }

                    if(word.empty()){

                        log.Error("Invalid memory address\n");
                        continue;
                    }

                    /* converting user input string address to a uint64_t address */
                    uint64_t address;

                    try{

                        address = std::stoll(word, nullptr, 16);
                    }catch(std::invalid_argument& err_1){

                        log.Error("Invalid memory address :%s\n", err_1.what());
                        continue;
                    }catch(std::out_of_range& err_2){

                        log.Error("Invalid range :%s\n", err_2.what());
                        continue;
                    }
                    if(place_breakpoint(debug, break_list, address) < 0) continue;
                }

                /* if user has specified more addresses than 1, we are going to place breakpoints in all of those addresses */
                else{

                    /* we are staring from second element [1st index] of the vector because 0th index is the command */
                    for (int i = 1; i < static_cast<int>(commands.size()); i++){

                        uint64_t address;
                        try{

                            address = std::stoll(commands[i], nullptr, 16);
                        }catch(std::out_of_range& err_1){

                            log.Error("Invalid range :%s\n", err_1.what());
                            continue;
                        }catch(std::invalid_argument& err_2){

                            log.Error("Invalid memory address :%s\n", err_2.what());
                            continue;
                        }
                        if(place_breakpoint(debug, break_list, address) < 0) continue;
                    }
                }
            }
            else{

                log.Print("[!] Process is not running, try restarting the process by 'reset'\n");
                continue;
            }
        }
        else if(command.compare("info registers") == 0 || command.compare("i r") == 0 || command.compare("i registers") == 0 || command.compare("info r") == 0){

            if(debug.GetProgramState()){
                InfoRegistersAll(debug, regs);
            }
            else {

                log.Print("[!] Process is not running, try restarting the process by 'reset'\n");
                continue;
            }
        }

        else if(commands[0].compare("delete") == 0 || commands[0].compare("del") == 0 || commands[0].compare("d") == 0){

            if(commands[1].empty()){

                log.Prompt("Enter breakpoint number to remove");
                if(!std::getline(std::cin, word)){

                    log.Error("IO error\n");
                    return -1;
                }

                if(word.empty()){

                    log.Error("Invalid breakpoint number\n");
                    return -1;
                }

                try{

                    break_list.RemoveElement(std::stoi(word, nullptr, 10));
                }catch (std::out_of_range& err_1) {

                    log.Error("Invalid range :%s\n", err_1.what());
                    continue;
                }catch (std::invalid_argument& err_2) {

                    log.Error("Invalid address or line number :: %s\n", err_2.what());
                    continue;
                }

            }
            else{

                for (int i = 1; i < commands.size(); i++){

                    try {

                        break_list.RemoveElement(std::stoi(commands[i], nullptr, 10));
                    }catch (std::out_of_range& err_1) {

                        log.Error("Invalid range\n");
                        continue;
                    }catch (std::invalid_argument& err_2){

                        log.Error("Invalid address or line number\n");
                        continue;
                    }
                }
            }
        }

        else if(commands[0].compare("set") == 0){

            if(debug.GetProgramState()){

                if(commands.size() == 1){

                    log.Prompt("Enter register name");
                    if(!std::getline(std::cin, word)){

                        log.Error("IO error\n");
                        return -1;
                    }

                    if(word.empty()){

                        log.Error("Invalid register name\n");
                        continue;
                    }

                    std::string regname = word;

                    log.Prompt("Enter value");
                    if(!std::getline(std::cin, word)){

                        log.Error("IO error\n");
                        return -1;
                    }

                    if(word.empty()){

                        log.Error("Invalid value\n");
                        continue;
                    }

                    uint64_t value;
                    try{

                        value = std::stoll(word, nullptr, 16);
                    }catch (std::out_of_range& err_1){

                        log.Error("Invalid range :%s\n", err_1.what());
                        continue;
                    }catch (std::invalid_argument& err_2){

                        log.Error("Invalid value :%s\n", err_2.what());
                        continue;
                    }
                    if(SetRegister(debug, regs, regname, value) < -1) return -1;
                }
                else if(commands.size() == 2){

                    log.Prompt("Enter value");
                    if(!std::getline(std::cin, word)){

                        log.Error("IO error");
                        return -1;
                    }

                    if(word.empty()){

                        log.Error("Invalid value");
                        continue;
                    }

                    uint64_t value;
                    try{

                        value = std::stoll(word, nullptr, 16);
                    }catch (std::out_of_range& err_1){

                        log.Error("Invalid range :%s\n", err_1.what());
                        continue;
                    }catch (std::invalid_argument& err_2){

                        log.Error("Invalid value :%s\n", err_2.what());
                        continue;
                    }
                    if(SetRegister(debug, regs, commands[1], value) < 0) return -1;
                }
                else if(command.size() == 3){

                    uint64_t value;

                    try{

                        value = std::stoll(commands[2], nullptr, 16);
                    }catch(std::out_of_range& err_1){

                        log.Error("Invalid range :%s\n", err_1.what());
                        continue;
                    }catch(std::invalid_argument& err_2){

                        log.Error("Invalid argument :%s\n", err_2.what());
                        continue;
                    }
                    if(SetRegister(debug, regs, commands[1], value) < 0) return -1;
                }
            }else{

                log.Print("Program is not running");
                continue;
            }
        }
        else if(commands[0].compare("disable") == 0 || commands[0].compare("dis") == 0){

            if(commands.size() < 2){

                log.Prompt("Enter breakpoint number to disable");
                if(!std::getline(std::cin, word)){

                    log.Error("IO error\n");
                    return -1;
                }

                if(word.empty()){

                    log.Error("Invalid breakpoint number\n");
                    return -1;
                }
                //check errors
                if(_disable_breakpoint(break_list, std::stoi(word, nullptr, 10)) < 0)
                    continue;
            }
            else{

                for (int i = 1; i < commands.size(); i++){

                    try{

                        if(_disable_breakpoint(break_list, std::stoi(commands[i], nullptr, 10)) < 0)
                            continue;
                    }catch (std::out_of_range& err_1) {

                        log.Error("Invalid range\n");
                        continue;
                    }catch (std::invalid_argument& err_2) {

                        log.Error("Invalid address or line number\n");
                        continue;
                    }
                }
            }
        }

        else if(commands[0].compare("info") == 0){
 
            if(commands[1].empty()){

                log.Prompt("Enter a register");
                std::getline(std::cin, word);
                if(word.empty()){

                    log.Error("Invalid register\n");
                    continue;
                }
                else{

                    if(InfoRegister(debug, regs, word) < 0) return -1;
                }
            }
            else{

                if(InfoRegister(debug, regs, commands[1]) < 0) return -1; 
            }
        }

        else if(commands[0].compare("auto") == 0 || commands[0].compare("a") == 0){

            if(step_auto(debug, break_list, regs) < 0)
                return -1;
        }

        else if(commands[0].compare("step") == 0 || commands[0].compare("s") == 0){

            if(commands.size() > 1){       // if commands vector's length is greater than 1, it means that user provided a number of steps

                int number_of_steps;
                try{

                    number_of_steps = std::stoi(commands[1], nullptr, 10);
                    log.Debug("%d\n", number_of_steps);
                }catch (std::out_of_range& err_1){

                    log.Error("Invalid range\n");
                    continue;
                }catch (std::invalid_argument& err_2){

                    log.Error("Invalid address or line number\n");
                    continue;
                }

                int ret = step_x(debug, break_list, regs, number_of_steps);
                if(ret == EXIT_STATUS){

                    if(debug.GetPathname() == nullptr && debug.GetPid() != 0){

                        log.Prompt("[!] Press any key to exit zkz");
                        getchar();
                        return EXIT_STATUS;
                    }

                    else if(debug.GetPathname() != nullptr){

                        continue;
                    }
                }
                else if (ret == -1) return -1;
            }
            else{       // else we just step 1

                int ret = step_x(debug, break_list, regs, 1);
                if(ret == EXIT_STATUS){

                    if(debug.GetPathname() == nullptr && debug.GetPid() != 0){

                        log.Prompt("[!] Press any key to exit zkz");
                        getchar();
                        return EXIT_STATUS;
                    }

                    else if(debug.GetPathname() != nullptr){

                        continue;
                    }
                }
                else if (ret == -1) return -1;
            }
        }

        /* else if(command.compare("unwind") == 0){

        } */

        else if(command.compare("exit") == 0){

            log.Prompt("Do you want to kill the process? [yes/no] ");
            if(!std::getline(std::cin, word)){

                log.Error("IO error\n");
                return -1;
            }
            if(word.empty()){

                log.Print("Assumed [no]\n");
                continue;
            }
            if(word.compare("yes") == 0 || word.compare("y") == 0){

                kill_process(debug.GetPid());
                return 0;
            }
            else continue;
        }

        else if(command.compare("help") == 0){

            PrintHelp();
        }
    }
    return 0;
}

int attach_process(Debug& debug){

    if(debug.GetPid() != 0){
        if(ptrace(PTRACE_ATTACH, debug.GetPid(), nullptr, nullptr) < 0){

            log.PError("Ptrace error");
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

    {
        Debug debug;

        ParseArguments(debug, argc, argv);

        if((debug.GetPathname()[0] != nullptr) && (debug.GetPid() != 0)){    //you cant use both 
            PrintUsage();
        }
        else if(debug.GetPid() != 0){

            int ret = attach_process(debug);
            if(ret == 1) return 0;   // if we are attaching with a pid, we need a way to get elf binary's pathname
            else if(ret == -1) return -1;
        }
        else if(debug.GetPathname() != nullptr){

            int ret = start_process(debug);
            if(ret == 1) return 0;      // return 1 means debugee is exited.
            else if(ret == -1) return -1;
        }

        //------ 
        DebugLineInfo line_info;

        if(init_debug_lines(debug, line_info) < 0){

            log.Error("Error reading dwarf information\n");
            line_info.b_dwarf_state = false;
        }

        //------

        /* printing where rip is at */
        struct user_regs_struct regs;
        if(ptrace(PTRACE_GETREGS, debug.GetPid(), nullptr, &regs) < 0){

            log.PError("Ptrace error");
            return -1;
        }
        log.Print("[zkz] Started debugging\trip : %x\n", regs.rip);
        if(mainloop(debug, line_info) < 0) return -1;
    }
    exit(0);
}
