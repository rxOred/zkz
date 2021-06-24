#include "utils.h"
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include "main.h"

void PrintHelp(void){

    //print information
}

void PrintUsage(void){

    //free memory and exit
    log.Print("something\n");
}

void ParseArguments(Debug& debug, int argc, char *argv[]){

    for(int i = 1; i < argc; i++){

        if (strcmp(argv[i], "-f") == 0){

            int j, count = 0;
            char **pathname = nullptr;
            i++;
            for(j = i; j < argc; j++){

                if(argv[j][0] == '-') break;

                pathname = (char **)realloc(pathname, sizeof(char **) * (count + 2));
                if(!pathname){

                    log.PError("Memory allocation error");
                    exit(EXIT_FAILURE);
                }
 
                pathname[count] = argv[j];
                count++;
            }

            pathname[count] = nullptr;
            debug.SetPathname(pathname);
            debug.SetCount(count);
            i = j - 1;      // j - 1 because we loop to the next flag too
        }
        else if(strcmp(argv[i], "-p") == 0){

            i++;
            if(!argv[i] || i == argc) {

                log.Error("Expected a process id\n");
                PrintUsage();     /* user have to specify a process id */
                exit(EXIT_FAILURE);
            }
            else debug.SetPid(atoi(argv[i]));
        }
        else if(strcmp(argv[i], "-s") == 0){

            i++;
            if(i == argc || !argv[i]){

                PrintUsage();
                exit(EXIT_FAILURE);
            }
            if(atoi(argv[i]) > 0){

                debug.SetSystrace();
            }
        }
        else if(strcmp(argv[i], "-i") == 0){

            i++;
            if(i == argc || !argv[i]) {

                PrintUsage();
                exit(EXIT_FAILURE);
            }
            if(atoi(argv[i]) > 0){

                debug.SetInforeg();
            }
        }
        else{

            PrintUsage();
            exit(EXIT_FAILURE);
        }
    }
}
