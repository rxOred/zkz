#include "utils.h"
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include "main.h"

void print_help(void){

    //print information
}

void print_usage(void){

    //free memory and exit
    printf("something\n");
}

void parse_arguments(Debug& debug, int argc, char *argv[]){

    for(int i = 1; i < argc; i++){

        if (strcmp(argv[i], "-f") == 0){

            int j, count = 0;
            char **pathname = nullptr;
            i++;
            for(j = i; j < argc; j++){

                if(argv[j][0] == '-') break;

                pathname = (char **)realloc(pathname, sizeof(char **) * (count + 2));
                if(!pathname){

                    perror("[X] Memory allocation error :: ");
                    exit(EXIT_FAILURE);
                }
 
                pathname[count] = argv[j];
                count++;
            }

            pathname[count] = nullptr;
            debug.set_pathname(pathname);
            i = j - 1;      // j - 1 because we loop to the next flag too
        }
        else if(strcmp(argv[i], "-p") == 0){

            i++;
            if(!argv[i] || i == argc) {

                fprintf(stderr, "[X] Expected a process id\n");
                print_usage();     /* user have to specify a process id */
                exit(EXIT_FAILURE);
            }
            else debug.set_pid(atoi(argv[i]));
        }
        else if(strcmp(argv[i], "-s") == 0){

            i++;
            if(i == argc || !argv[i]){

                print_usage();
                exit(EXIT_FAILURE);
            }
            if(atoi(argv[i]) > 0){

                debug.set_systrace();
            }
        }
        else if(strcmp(argv[i], "-i") == 0){

            i++;
            if(i == argc || !argv[i]) {

                print_usage();
                exit(EXIT_FAILURE);
            }
            if(atoi(argv[i]) > 0){

                debug.set_inforeg();
            }
        }
        else{

            print_usage();
            exit(EXIT_FAILURE);
        }
    }
}
