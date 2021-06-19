#pragma once

#include <bits/stdint-uintn.h>
#include <iostream>

#define PRINT (1 << 0)
#define ERROR (1 << 1)
#define PERROR (1 << 2)
#define WARN (1 << 3)
#define INFO (1 << 4)
#define DEBUG (1 << 5)
#define PROMPT (1 << 6)

class Log {

    public:
        void SetState(uint8_t state) {
            m_state = state;
        }

        uint8_t CheckState(uint8_t state) { 
            return (m_state & state);
        }

        void Print(const char *msg) {
            if(CheckState(PRINT))
                std::cout << msg << std::endl;
        }

        void Error(const char *error) {
            if(CheckState(ERROR))
                std::cerr << "[ERROR] " << error << std::endl;
        }

        void PError(const char *_perror) {
            if(CheckState(PERROR))
                perror(_perror);
        }

        void Warn(const char *warning) {
            if(CheckState(WARN))
                std::cout << "[WARNING] " << warning << std::endl;
        }

        void Info(const char *info) {
            if(CheckState(INFO))
                std::cout << "[INFO] " << info << std::endl;
        }

        void Debug(const char *debug) {
            if(CheckState(DEBUG))
                std::cout << "[DEBUG]" << debug << std::endl;
        }

        void Prompt(const char *prompt) {
            if(CheckState(PROMPT))
                std::cout << prompt << ">" << std::endl;
        }

    private:
        uint8_t m_state;
};
