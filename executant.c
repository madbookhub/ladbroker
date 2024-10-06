// ----------------------------------------
// Description: This module names "Executant", it is used to execute some 
// codes sent to it, in pipe.
//
// Author: madbookhub@github
//
// Created: Aug,18,2024
// 
// Recent: Oct,6,2024
// ----------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "executant.h"

#define BUF_SIZE 200 // memory block for the output of execution

// Following Error Message ID MUST be greater than zero to avoid chaos.
#define ERROR_NOMEM 1
#define ERROR_NOPIPE 2

int Execute(const int Terminal, char* Content)
{
int Result;
char* Buf = NULL;

while (1)
    {
    // try to make a billboard, a memory block to store the result of execution
    Buf = Alloc(BUF_SIZE);
    if ( (Result = (Buf == NULL)? ERROR_NOMEM : 0) != 0 ) break;

    // Try to execute the codes now, in pipe.

    FILE* Pipe = popen(SCRIPT_RUN, "r");
    if ( (Result = (Pipe == NULL)? ERROR_NOPIPE: 0) != 0 ) break;

    while (fgets(Buf,sizeof(Buf)-1,Pipe)!=NULL) write(Terminal,Buf,strlen(Buf));

    pclose(Pipe);

    Result = 0;
    break;
    }

Freealloc(Buf);

return Result;
}
