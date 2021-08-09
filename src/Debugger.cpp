#include <bits/stdint-uintn.h>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <functional>
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
#include <thread>

#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/personality.h>
#include <sys/user.h>

#include "breakpoint.h"
#include "debug.h"
#include "utils.h"
#include "registers.h"
#include "dwarf_information.h"
#include "bin.h"
#include "Debugger.h"
#include "log.h"

#define EXIT_STATUS 1
#define STOPPED_STATUS 2
#define SIGNALED_STATUS 3

void Main::KillProcess(pid_t pid) const
{
    kill(pid, SIGKILL);
}

int Main::WaitForProcess(void) 
{
    int wstatus = 0;
    waitpid(m_debug.GetPid(), &wstatus, 0);

    if(WIFEXITED(wstatus)){

        log.Info("Process terminated with %d\n", wstatus);
        m_debug.SetProgramState(false);
        return EXIT_STATUS;
    }
    else if(WIFSTOPPED(wstatus)){

        return STOPPED_STATUS;
    }
    else if(WIFSIGNALED(wstatus)){

        log.Info("Process recieved a signal %d\n", wstatus);
        /*
         * replace wstatus with signal
         */
        return SIGNALED_STATUS;
    }

    return -1;
}

int Main::RemoveAllBreakpoints(BreakpointList& li) const
{
    for (int i = 0; i < li.GetNoOfBreakpoints(); i++){

        if(li.RemoveElement(m_debug, i + 1) < 0)
            return -1;
    }
    return 0;
}

int Main::DisableBreakpoint(BreakpointList& li, int b_number) const
{
    Breakpoint *b = li.GetElementByBreakpointNumber(b_number);
    if(b == nullptr){

        log.Error("Breakpoint %d not found\n", b_number); 
    }

    b->DisableBreakpoint();
    log.Print("Breakpoint %d disabled\n", b_number);
    return 0;
}

/* 
 * to restore the instruction to its previous state before 
 * placing the breakpoint 
 */
int Main::RestoreBreakpoint(Breakpoint *b, uint64_t address)
{
    log.Debug("restore address %x\n", address);
    if(ptrace(PTRACE_POKETEXT, m_debug.GetPid(), address, b->m_origdata) < 0){

        /* 
         * do not use gotos to handle errors for any of ptrace fails
         * errors may vary from each
         */
        log.PError("Ptrace error");
        return -1;
    }
    if(ptrace(PTRACE_GETREGS, m_debug.GetPid(), nullptr, &m_regs) < 0){

        log.PError("Ptrace error");
        return -1;
    }
    m_regs.rip = address;
    if(ptrace(PTRACE_SETREGS, m_debug.GetPid(), nullptr, &m_regs) < 0){

        log.PError("Ptrace error");
        return -1;
    }

    if(ptrace(PTRACE_SINGLESTEP, m_debug.GetPid(), nullptr, nullptr) < 0){

        log.PError("Ptrace error");
        return -1;
    }
    log.Debug("does not make it here\n");

    int ret = WaitForProcess();
    if(ret == EXIT_STATUS){

        return EXIT_STATUS;
    }
    else{

        log.Debug("process stopped or signled\n");
    }

    return 0;
}

int Main::PlaceBreakpoint(BreakpointList& li, uint64_t address) const
{
    uint64_t origdata = ptrace(PTRACE_PEEKTEXT, m_debug.GetPid(), address, nullptr);

    if(origdata < 0){

        log.PError("Ptrace error");
        return -1;
    }

    uint64_t data_w_interrupt = ((origdata & 0xffffffffffffff00) | 0xcc);
    if(ptrace(PTRACE_POKETEXT, m_debug.GetPid(), address, data_w_interrupt) < 0){

        log.PError("Ptrace error");
        return -1;
    }
    li.AppendElement(address, origdata);
    log.Print("Breakpoint placed at address : %x\n", address);
    return 0;
}

int Main::StepAuto(BreakpointList& li) 
{
    int ret;
    do{

        if(ptrace(PTRACE_SINGLESTEP, m_debug.GetPid(), nullptr, nullptr) < 0){

            log.PError("Ptrace eror");
            return -1;
        }

        int ret = WaitForProcess();
        if(ret == EXIT_STATUS){

            return EXIT_STATUS;
        }
        else if(ret == STOPPED_STATUS || ret == SIGNALED_STATUS){

            log.Print("---> rip: %x\n", m_regs.rip);
        }

        if(m_debug.GetInforeg()){

            InfoRegistersAll(m_debug, m_regs);
        }

        if(ptrace(PTRACE_GETREGS, m_debug.GetPid(), nullptr, &m_regs) < 0){

            log.PError("Ptrace error");
            return -1;
        }

        Breakpoint *b  = li.GetElementByAddress(m_regs.rip - 1);
        if(b == nullptr){

            continue;
        }
        else{

            if(b->GetState()){

                log.Info("Stopped execution at %x : breakpoint number %d\n", 
                        m_regs.rip -1, b->m_breakpoint_number);

                RestoreBreakpoint(b, m_regs.rip - 1);
                return 0;
            }
            else{

                RestoreBreakpoint(b, m_regs.rip - 1);
                continue;
            }
        }

    } while(ret != EXIT_STATUS);

    return 0;
}

int Main::StepX(BreakpointList& li, int number_of_steps) 
{
    for (int i = 0; i < number_of_steps; i++){

        if(ptrace(PTRACE_SINGLESTEP, m_debug.GetPid(), nullptr, nullptr) < 0){

            log.PError("Ptrace error");
            return -1;
        }

        int ret = WaitForProcess();
        if(ret == EXIT_STATUS){

            return EXIT_STATUS;
        }
        else if(ret == STOPPED_STATUS || ret == SIGNALED_STATUS){

            if(m_debug.GetInforeg()){

                InfoRegistersAll(m_debug, m_regs);
            }

            if(ptrace(PTRACE_GETREGS, m_debug.GetPid(), nullptr, &m_regs) < 0){

                log.PError("Ptrace error");
                return -1;
            }

            Breakpoint *b = li.GetElementByAddress(m_regs.rip - 1);
            if(b == nullptr)
                continue;
            else{

                if(b->GetState()){

                    log.Info("Stopped execution at %x : breakpoint number %d\n", m_regs
                            .rip -1, b->m_breakpoint_number);

                    if(RestoreBreakpoint(b, m_regs.rip - 1) < -1)
                        return -1;
                    return 0;
                }
                else{


                    if(RestoreBreakpoint(b, m_regs.rip - i) < -1)
                        return -1;
                    continue;
                }
            }
        }
    }

    log.Print("---> rip: %x\n", m_regs.rip);
    return 0;
}

int Main::ContinueExecution(BreakpointList& li)
{
     /* 
      * if is_sys_stopped == true, we are settting it to false because we continue 
      * execution.
      */
    if(m_debug.GetSyscallState()){

        m_debug.SetSyscallState(false);
    }

    log.Info("Continuing execution...\n"); 
    if(m_debug.GetSystrace()){

        log.Debug("stopping at syscalls\n");
        if(ptrace(PTRACE_SYSCALL, m_debug.GetPid(), nullptr, nullptr) < 0){

            log.PError("Ptrace error");
            return -1;
        }
    }
    else{

        if(ptrace(PTRACE_CONT, m_debug.GetPid(), nullptr, nullptr) < 0){

            log.PError("Ptrace error");
            return -1;
        }
    }

    int ret = WaitForProcess();
    if(ret == EXIT_STATUS) {

        return EXIT_STATUS;
    }
    if(ret == STOPPED_STATUS){

        if(ptrace(PTRACE_GETREGS, m_debug.GetPid(), nullptr, &m_regs) < 0){

            log.PError("Ptrace error");
            return -1;
        }

        log.Debug("Prcoess stopped at address %x\n", m_regs.rip - 1);

        /* 
         * if this return null, we are not in a breakpoint, so reason for SIGSTOP/\
         * SIGTRAP must be a system call if systrace is enabled
         */
        Breakpoint *b = li.GetElementByAddress(m_regs.rip - 1);
        if(b == nullptr){

            if(m_debug.GetSystrace()){
                log.Info("System call intercepted: %x\n", m_regs.rax);
                m_debug.SetSyscallState(true);

                if(m_debug.GetInforeg()){

                    log.Debug("Register information flag is set\n");
                    InfoRegistersAll(m_debug, m_regs);
                }
                return 0;
            }
            else {

                int ret = ContinueExecution(li);
                if(ret == EXIT_STATUS) return EXIT_STATUS;
                else if(ret == -1) return -1;
                else return 0;
            }
        }
        else{

            log.Debug("Breakpoint hit\n");
            /*
             * we have to restore breakpoint instruction before continue
             */
            if(b->GetState()){ 

                log.Info("Stopped execution at %x : breakpoint number %d\n", m_regs.
                        rip -1, b->m_breakpoint_number);

                if(m_debug.GetInforeg()){

                    InfoRegistersAll(m_debug, m_regs);
                }
                log.Debug("Enabled breakpoint\n");
                if(RestoreBreakpoint(b, m_regs.rip - 1) < -1) return -1;
                return 0;
            }
            else{

                /*
                 * if breakpoint is disabled, go restore the instruction and 
                 * continue execution
                 */
                log.Debug("Disabled breakpoint\n");
                if(RestoreBreakpoint(b, m_regs.rip - 1) < -1) return -1;
                int ret = ContinueExecution(li);
                if(ret == EXIT_STATUS) return EXIT_STATUS;
                else if(ret == -1) return -1;
                else return 0;
            }
        }
    }
    return -1;
}

/* 
 * i just copied this piece of code from one of libelfin examples 
 */
void Main::ParseLines(const dwarf::line_table &lt, int unit_number) const
{
    for (auto &line : lt){

        log.Debug("line number :%d\t address :%x\n", line.line, m_base_addr + 
                line.address);
        m_line_info_ptr->AppendElement(line.line, m_base_addr + line.address, 
                unit_number);
    }
}

void Main::InitDebugLines(void) 
{
    elf::elf elf_f;
    dwarf::dwarf dwarf_f;

    int fd = open(m_debug.GetPathname()[0], O_RDONLY);
    if(fd < 0){

        log.PError("File open error");
        m_debug.SetDwarf();
    }

    elf_f = elf::elf{elf::create_mmap_loader(fd)};
    dwarf_f = dwarf::dwarf(dwarf::elf::create_loader(elf_f));

    int i = 0;

    for(auto cu : dwarf_f.compilation_units()){

        const dwarf::line_table &lt = cu.get_line_table();
        ParseLines(lt, i);
        i++;
    }
}

int Main::MainLoop(void)
{
    log.SetState(PRINT | PROMPT | WARN | INFO | DEBUG | PERROR | ERROR);

    BreakpointList li;
    int default_compilation_unit = 0;

    while (1) {

        std::string command;

        log.Prompt("[zkz] ");
        if(!std::getline(std::cin, command)){

            log.Error("IO error\n");
            return -1;
        }
        if(command.empty()) continue;

        std::stringstream strstrm(command);
        std::vector<std::string> commands{};
        std::string word;

        while(std::getline(strstrm, word, ' ')){

            word.erase(std::remove_if(word.begin(), word.end(), ispunct), word.end());
            commands.push_back(word);
        }

        /* 
         * reset is used to restart a process, while it is still running or exited 
         */
        if(command.compare("run") == 0 || command.compare("r") == 0 || command.compare(
                    "continue") == 0 || command.compare("c") == 0){

            if(!m_debug.GetProgramState()){

                log.Debug("if program is stopped\n");
                if(m_debug.GetPathname() != nullptr){

                    if(StartProcess() < 0) return -1;
                    continue;
                }
            }
            int ret = ContinueExecution(li);
            if(ret == EXIT_STATUS){

                /*
                 * because we cant start a process without knowning its pathname
                 */
                if(m_debug.GetPathname() == nullptr && m_debug.GetPid() != 0){

                    log.Prompt("Press any key to exit zkz");
                    getchar();
                    return EXIT_STATUS;
                }
                else if (m_debug.GetPathname() != nullptr){

                    /* 
                     * if pathname is there, we can continue and let user to enter 
                     * reset command
                     */
                    continue;
                }
            }else if(ret == -1) return -1;
        }

        else if(commands[0].compare("list") == 0){

            if(m_debug.GetProgramState()){

                if(m_debug.GetDwarfState()){

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
                        m_line_info_ptr->ListSrcLines(compilation_unit);
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

                        m_line_info_ptr->ListSrcLines(compilation_unit);
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
        else if(commands[0].compare("listsym") == 0){

            if(m_debug.GetProgramState()){

                if(m_debug.GetSymState()){
                    if(commands.size() <= 1){

                        m_elf_ptr->ListSyms(-1);
                    }
                    else{

                        int prange;
                        if(!commands[1].empty()){

                            try{

                                prange = std::stoi(commands[1], nullptr, 10);
                            }catch(std::invalid_argument& err_1){

                                log.Error("Invalid range %s\n", err_1.what());
                                continue;
                            }catch(std::out_of_range& err_2){

                                log.Error("Invalid range %s\n", err_2.what());
                                continue;
                            }

                            m_elf_ptr->ListSyms(prange);
                        }
                    }
                }
            }
        }
        else if(commands[0].compare("select") == 0){

            if(m_debug.GetProgramState()){

                if(m_debug.GetDwarfState()){
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

                            default_compilation_unit = std::stoi(word, nullptr, 
                                    10);
                        }catch (std::out_of_range const& err_1){

                            log.Error("Invalid range %s\n", err_1.what());
                            continue;
                        }catch (std::invalid_argument& err_2){

                            log.Error("Invalid compilation unit number %d\n", 
                                    err_2.what());
                            continue;
                        }
                    }
                    else{
                        try{

                            default_compilation_unit = std::stoi(commands[1], 
                                    nullptr, 10);
                            if(default_compilation_unit < 0 || default_compilation_unit
                                    > m_line_info_ptr->GetNoOfCompilationUnits()){

                                log.Error("Compilation unit number is not in range\n");
                                continue;
                            }
                            log.Print("Compilation unit : %d selected\n", 
                                    default_compilation_unit);
                        }catch (std::out_of_range const& err_1){

                            log.Error("Invalid range %s\n", err_1.what());
                            continue;
                        }catch (std::invalid_argument& err_2){

                            log.Error("Invalid compilation unit number %s\n",
                                    err_2.what());
                            continue;
                        }
                    }
                }else{

                    log.Print("Debug information not available\n");
                    continue;
                }
            }else{

                log.Print("[!] Process is not running\n");
                continue;
            }
        }
        else if(commands[0].compare("breakl") == 0 || commands[0].compare("bl") == 0){

            if(m_debug.GetProgramState()){

                if(m_debug.GetDwarfState()){

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

                            address = m_line_info_ptr->GetAddressByLine(
                                    default_compilation_unit, 
                                    std::stoi(word, nullptr, 10));
                            if(address < 0){

                                log.Error("Address for corresponding line is not found"
                                        "\n");
                                continue;
                            }
 
                       }catch (std::out_of_range& err_1) {

                            log.Error("Invalid range %s\n", err_1.what());
                            continue;
                        }catch (std::invalid_argument& err_2) {

                            log.Error("Invalid line number %s\n", err_2.what());
                            continue;
                        }

                       if(PlaceBreakpoint(li, address) < 0) continue;
                    }
                    else{

                        for (int i = 1; i < commands.size(); i++){
                            try{

                                address = m_line_info_ptr->GetAddressByLine(
                                        default_compilation_unit, 
                                        std::stoi(commands[i], nullptr, 10));
                            }catch (std::out_of_range& err_1) {

                                log.Error("Invalid range %s\n", err_1.what());
                                continue;
                            }catch (std::invalid_argument& err_2) {

                                log.Error("Invalid line number %s\n", err_2.what());
                                continue;
                            }
                            /* 
                             * if this true, indicates that address is specified line 
                             * does not match to an address
                             */
                            if(address < 0){

                                log.Error("Address for corresponding line is not found \
                                        \n");
                                continue;
                            }
                            if(PlaceBreakpoint(li, address) < 0) continue;
                        }
                    }
                }else {

                    log.Print("Debug information not available\n");
                    continue;
                }
            }
            else{

                log.Print("[!] Process is not running\n");
                continue;
            }
        }

        else if(commands[0].compare("break")  == 0 || commands[0].compare("b") == 0){

            /* 
             * check length of commands vector is less than 2, if yes, user havent 
             * mentioned ann address to place a breakpoint 
             */
            if(m_debug.GetProgramState()){

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

                    /* 
                     * converting user input string address to a uint64_t address 
                     */
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
                    if(PlaceBreakpoint(li, address) < 0) continue;
                }

                /* 
                 * if user has specified more addresses than 1, we are going to place
                 * breakpoints in all of those addresses 
                 */
                else{

                    /*
                     * we are staring from second element [1st index] of the vector
                     * because 0th index is the command 
                     */
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
                        if(PlaceBreakpoint(li, address) < 0) continue;
                    }
                }
            }
            else{

                log.Print("[!] Process is not running\n");
                continue;
            }
        }
        else if(command.compare("info registers") == 0 || command.compare("i r") == 0
                | command.compare("i registers") == 0 || command.compare("info r") == 0)
        {

            if(m_debug.GetProgramState()){
                InfoRegistersAll(m_debug, m_regs);
            }
            else {

                log.Print("[!] Process is not running\n");
                continue;
            }
        }

        else if(command.compare("printb") == 0){

            if(m_debug.GetProgramState()){

                li.ListBreakpoints();
            }else{

                log.Print("Program is not running");
                continue;
            }
        }
        else if(commands[0].compare("delete") == 0 || commands[0].compare("del") == 0 | 
                commands[0].compare("d") == 0){

            if(m_debug.GetProgramState()){
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

                        if(li.RemoveElement(m_debug, std::stoi(word, nullptr, 10)) < 0)
                            return -1;
                    }catch (std::out_of_range& err_1) {

                        log.Error("Invalid range : %s\n", err_1.what());
                        continue;
                    }catch (std::invalid_argument& err_2) {

                        log.Error("Invalid address or line number : %s\n", err_2.what());
                        continue;
                    }

                }
                else{

                    for (int i = 1; i < commands.size(); i++){

                        try {

                            if(li.RemoveElement(m_debug, std::stoi(commands[i], nullptr
                                            , 10)) < 0)
                                return -1;
                        }catch (std::out_of_range& err_1) {

                            log.Error("Invalid range : %s\n", err_1.what());
                            continue;
                        }catch (std::invalid_argument& err_2){

                            log.Error("Invalid address or line number : %s\n", err_2.
                                    what());
                            continue;
                        }
                    }
                }
            }else{

                 log.Print("Program is not running");
                continue;
            }
        }
        else if(commands[0].compare("set") == 0){

            if(m_debug.GetProgramState()){

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
                    if(SetRegister(m_debug, m_regs, regname, value) < -1) return -1;
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
                    if(SetRegister(m_debug, m_regs, commands[1], value) < 0) return -1;
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
                    if(SetRegister(m_debug, m_regs, commands[1], value) < 0) return -1;
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
                if(DisableBreakpoint(li, std::stoi(word, nullptr, 10)) < 0)
                    continue;
            }
            else{

                for (int i = 1; i < commands.size(); i++){

                    try{

                        if(DisableBreakpoint(li, std::stoi(commands[i], nullptr, 10)) 
                                < 0)
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

                    if(InfoRegister(m_debug, m_regs, word) < 0) return -1;
                }
            }
            else{

                if(InfoRegister(m_debug, m_regs, commands[1]) < 0) return -1; 
            }
        }

        else if(commands[0].compare("auto") == 0 || commands[0].compare("a") == 0){

            if(StepAuto(li) < 0)
                return -1;
        }

        else if(commands[0].compare("step") == 0 || commands[0].compare("s") == 0){

            if(commands.size() > 1){

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

                int ret = StepX(li, number_of_steps);
                if(ret == EXIT_STATUS){

                    if(m_debug.GetPathname() == nullptr && m_debug.GetPid() != 0){

                        log.Prompt("[!] Press any key to exit zkz");
                        getchar();
                        return EXIT_STATUS;
                    }

                    else if(m_debug.GetPathname() != nullptr){

                        continue;
                    }
                }
                else if (ret == -1) return -1;
            }
            else{       // else we just step 1

                int ret = StepX(li, 1);
                if(ret == EXIT_STATUS){

                    if(m_debug.GetPathname() == nullptr && m_debug.GetPid() != 0){

                        log.Prompt("[!] Press any key to exit zkz");
                        getchar();
                        return EXIT_STATUS;
                    }

                    else if(m_debug.GetPathname() != nullptr){

                        continue;
                    }
                }
                else if (ret == -1) return -1;
            }
        }

#ifdef UNWIND
        else if(command.compare("unwind") == 0){

        }
#endif
#ifdef INJECT
        else if(command.compare("inject") == 0){

            log.Prompt("shellcode >");
            if(!std::getline(std::cin, word)){

                log.Error("IO error\n");
                return -1;
            }
            if(word.empty()){

                log.Print("invalid code\n");
                continue;
            }

            
        }

#endif
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

                KillProcess(m_debug.GetPid());
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

uint64_t Main::GetBaseAddress(pid_t pid) const
{
    uint64_t base_addr = 0;
    char proc_path[64];
    char addr_buf[16];
    char *p = addr_buf;

    sprintf(proc_path, "/proc/%d/maps", pid);

    FILE *fh = fopen(proc_path, "r");
    if(!fh){

        log.PError("fopen failed");
        goto failed;
    }

    for(int i = 0; i < 16; i++, p++){

        *addr_buf = fgetc(fh);
        if(!std::isalnum(*addr_buf))
            goto char_failed;
    }

    sscanf(addr_buf, "%lx", &base_addr);

char_failed:
    fclose(fh);

failed:
    return base_addr;
}

int Main::StartProcess(void) 
{
    if(m_debug.GetPathname()){

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
            char **pathname = m_debug.GetPathname();
            if(execvp(pathname[0], pathname) == -1){

                log.PError("Execvp failed");
                exit(EXIT_FAILURE);
            }
        }
        else{

            int ret = WaitForProcess();
            if(ret == EXIT_STATUS) {

                return 1;
            }
            else if(ret == SIGNALED_STATUS || ret == STOPPED_STATUS){

                m_debug.SetProgramState(true);
                /* 
                 * no one should set this to true except start_process an attach_
                 * process functions, which are related to start the process
                 */
                m_debug.SetPid(pid);
                return 0;
            }
        }
    }
    return -1;
}

int Main::AttachProcess(void) 
{
    if(m_debug.GetPid() != 0){
        if(ptrace(PTRACE_ATTACH, m_debug.GetPid(), nullptr, nullptr) < 0){

            log.PError("Ptrace error");
            return -1;
        }

        int ret = WaitForProcess();
        if(ret ==  EXIT_STATUS) return 1;
        else if(ret == STOPPED_STATUS || ret == SIGNALED_STATUS)
            return 0;
    }
    return -1;
}

void Main::ParseArguments(int argc, char *argv[])
{
    char **pathname = nullptr;
    for(int i = 1; i < argc; i++){

        if (strcmp(argv[i], "-f") == 0){

            int j, count = 0;
            i++;
            for(j = i; j < argc; j++){

                if(argv[j][0] == '-') break;

                pathname = (char **)realloc(pathname, sizeof(char **) * (count + 2));
                if(!pathname){

                    log.PError("Memory allocation error");
                    goto failed;
                }
 
                pathname[count] = argv[j];
                count++;
            }

            pathname[count] = nullptr;
            m_debug.SetPathname(pathname);
            m_debug.SetCount(count);
            i = j - 1;
        }
        else if(strcmp(argv[i], "-p") == 0){

            i++;
            if(!argv[i] || i == argc) {

                log.Error("Expected a process id\n");
                /* 
                 * user have to specify a process id 
                 */
                goto failed;
            }
            else m_debug.SetPid(atoi(argv[i]));
        }
        else if(strcmp(argv[i], "-s") == 0){

            i++;
            if(i == argc || !argv[i]){

                goto failed;
            }
            if(atoi(argv[i]) > 0){

                m_debug.SetSystrace();
            }
        }
        else if(strcmp(argv[i], "-i") == 0){

            i++;
            if(i == argc || !argv[i]) {

                goto failed;
            }
            if(atoi(argv[i]) > 0){

                m_debug.SetInforeg();
            }
        }
        else{

            goto failed;
        }
    }

failed:
    if(pathname)
        free(pathname);

    PrintUsage();
    exit(EXIT_FAILURE);
}

int Main::Debugger(void)
{
    if((m_debug.GetPathname()[0] != nullptr) && (m_debug.GetPid() != 0)){
        PrintUsage();
    }
    else if(m_debug.GetPid() != 0){

        int ret = AttachProcess();
        if(ret == 1) return 0;
        else if(ret == -1) return -1;
    }
    else if(m_debug.GetPathname()){

        int ret = StartProcess();
        if(ret == 1) return 0;
        else if(ret == -1) return -1;
    }

    m_base_addr = GetBaseAddress(m_debug.GetPid());
    m_elf_ptr = new Elf(m_debug.GetPid(), m_debug.GetPathname()[0], m_base_addr);
    m_line_info_ptr = new DebugLineInfo(m_base_addr);

    std::thread worker_debuglines(&Main::InitDebugLines, this);
    std::thread worker_elfsyms(&Elf::OpenFile, m_elf_ptr, 0);

    /*
     * printing where rip is at 
     */
    if(ptrace(PTRACE_GETREGS, m_debug.GetPid(), nullptr, &m_regs) < 0){

        log.PError("Ptrace error");
        return -1;
    }
    log.Print("[zkz] Started debugging\trip : %x\n", m_regs.rip);

    worker_elfsyms.join();
    worker_debuglines.join();

    if(m_elf_ptr->b_load_failed){

        log.Error("Error parsing symbols");
        m_debug.SetSym();
    }

    if(MainLoop() < 0) return -1;

    return 0;
}

Main::~Main(void)
{
    delete m_elf_ptr;
    delete m_line_info_ptr;
}

int main(int argc, char *argv[])
{
    Main m;

    m.ParseArguments(argc, argv);
    if(m.Debugger() < 0)
        return -1;

    return 0;
}
