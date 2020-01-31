#ifndef EXECUTION_ENGINE_H
#define EXECUTION_ENGINE_H

#include <stddef.h>

typedef enum {Success, CommandNotFound, FailedExecution} result;

result execute_command(char* command, size_t command_length);

#endif
