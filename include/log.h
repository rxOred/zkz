#ifndef LOG_H
#define LOG_H

#include <bits/stdint-uintn.h>
#include <cstdio>
#include <iostream>
#include <cstdarg>
#include <cstring>

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

        Log(){

            m_state = PRINT | ERROR | PERROR;
        }
        Log(uint8_t state){

            m_state = state;
        }

        void Print(const char *msg, ...) {
            if(CheckState(PRINT)){

                va_list arg;
                va_start(arg, msg);

                for(int i = 0; i < strlen(msg); i++){

                    if(msg[i] == '%'){
                        i++;
                        switch(msg[i]){

                            case 'd':
                                {
                                    int d = va_arg(arg, int);
                                    char number[10];
                                    sprintf(number, "%d", d);
                                    printf("%s", number);
                                    break;
                                }
                            case 's':
                                {
                                    char *s = va_arg(arg, char *);
                                    for(int j = 0; j < strlen(s); j++){

                                        putchar(s[j]);
                                    }
                                    break;
                                }

                            case 'x':
                                {
                                    uint64_t p = va_arg(arg, unsigned long long);
                                    char hex_str[16];
                                    sprintf(hex_str, "%lX", p);
                                    putchar('0');
                                    putchar('x');
                                    for(int j = 0; j < strlen(hex_str); j++){

                                        putchar(hex_str[j]);
                                    }
                                }
                        }
                    }else{

                        putchar(msg[i]);
                    }
                }
            }
        }

        void Error(const char *error, ...) {
            if(CheckState(ERROR)){

                va_list arg;
                va_start(arg, error);

                printf("[ERROR]");
                for(int i = 0; i < strlen(error); i++){

                    if(error[i] == '%'){
                        i++;
                        switch(error[i]){

                            case 'd':
                                {
                                    int d = va_arg(arg, int);
                                    char number[10];
                                    sprintf(number, "%d", d);
                                    printf("%s", number);
                                    break;
                                }
                            case 's':
                                {
                                    char *s = va_arg(arg, char *);
                                    for(int j = 0; j < strlen(s); j++){

                                        putchar(s[j]);
                                    }
                                    break;
                                }

                            case 'x':
                                {
                                    uint64_t p = va_arg(arg, unsigned long long);
                                    char hex_str[16];
                                    sprintf(hex_str, "%lX", p);
                                    putchar('0');
                                    putchar('x');
                                    for(int j = 0; j < strlen(hex_str); j++){

                                        putchar(hex_str[j]);
                                    }
                                }
                        }
                    }else{

                        putchar(error[i]);
                    }
                }
            }
        }

        void PError( const char *_perror) {
            if(CheckState(PERROR)){

                char string[strlen(_perror) + 8];
                sprintf(string, "%s %s", "[ERROR]", _perror);
                perror(string);
            }
        }

        void Warn(const char *warning, ...) {
            if(CheckState(WARN)){

                va_list arg;
                va_start(arg, warning);

                printf("[WARNING] ");
                for(int i = 0; i < strlen(warning); i++){

                    if(warning[i] == '%'){
                        i++;
                        switch(warning[i]){

                            case 'd':
                                {
                                    int d = va_arg(arg, int);
                                    char number[10];
                                    sprintf(number, "%d", d);
                                    printf("%s", number);
                                    break;
                                }
                            case 's':
                                {
                                    char *s = va_arg(arg, char *);
                                    for(int j = 0; j < strlen(s); j++){

                                        putchar(s[j]);
                                    }
                                    break;
                                }

                            case 'x':
                                {
                                    uint64_t p = va_arg(arg, unsigned long long);
                                    char hex_str[16];
                                    sprintf(hex_str, "%lX", p);
                                    putchar('0');
                                    putchar('x');
                                    for(int j = 0; j < strlen(hex_str); j++){

                                        putchar(hex_str[j]);
                                    }
                                }
                        }
                    }else{

                        putchar(warning[i]);
                    }
                }

            }
        }

        void Info(const char *info, ...) {
            if(CheckState(INFO)){

                va_list arg;
                va_start(arg, info);

                printf("[INFO] ");
                for(int i = 0; i < strlen(info); i++){

                    if(info[i] == '%'){
                        i++;
                        switch(info[i]){

                            case 'd':
                                {
                                    int d = va_arg(arg, int);
                                    char number[10];
                                    sprintf(number, "%d", d);
                                    printf("%s", number);
                                    break;
                                }
                            case 's':
                                {
                                    char *s = va_arg(arg, char *);
                                    for(int j = 0; j < strlen(s); j++){

                                        putchar(s[j]);
                                    }
                                    break;
                                }

                            case 'x':
                                {
                                    uint64_t p = va_arg(arg, unsigned long long);
                                    char hex_str[16];
                                    sprintf(hex_str, "%lX", p);
                                    putchar('0');
                                    putchar('x');
                                    for(int j = 0; j < strlen(hex_str); j++){

                                        putchar(hex_str[j]);
                                    }
                                }
                        }
                    }else{

                        putchar(info[i]);
                    }
                }
            }
        }

        void Debug(const char *debug, ...) {
            if(CheckState(DEBUG)){

                va_list arg;
                va_start(arg, debug);

                printf("[DEBUG] ");
                for(int i = 0; i < strlen(debug); i++){

                    if(debug[i] == '%'){
                        i++;
                        switch(debug[i]){

                            case 'd':
                                {
                                    int d = va_arg(arg, int);
                                    char number[10];
                                    sprintf(number, "%d", d);
                                    printf("%s", number);
                                    break;
                                }
                            case 's':
                                {
                                    char *s = va_arg(arg, char *);
                                    for(int j = 0; j < strlen(s); j++){

                                        putchar(s[j]);
                                    }
                                    break;
                                }

                            case 'x':
                                {
                                    uint64_t p = va_arg(arg, unsigned long long);
                                    char hex_str[16];
                                    sprintf(hex_str, "%lX", p);
                                    putchar('0');
                                    putchar('x');
                                    for(int j = 0; j < strlen(hex_str); j++){

                                        putchar(hex_str[j]);
                                    }
                                }
                        }
                    }else{

                        putchar(debug[i]);
                    }
                }
            }
        }

        void Prompt(const char *prompt, ...) {
            if(CheckState(PROMPT)){
                printf("%s> ", prompt);
            }
        }

    private:
        uint8_t m_state;
};

static Log log(DEBUG | PRINT | PROMPT | ERROR | INFO | PERROR);

#endif /* log.h */
