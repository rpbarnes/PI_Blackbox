#include "execution_engine.h"

/* This is responsible for executing commands as the come from the server or from the recipe parser.
*/
result execute_command(char* command, size_t command_length){
    result res = Success;

    char cat[50] = "hello world";

    char *first = strtok(cat, " ");

    //printf( " %s\n", first ); 

    return res;
}