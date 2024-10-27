// ----------------------------------------
// Description: This module names "Executant", it is used to execute some 
// codes sent to it, in pipe.
//
// Author: madbookhub@github
//
// Created: Aug,18,2024
// 
// Recent: Oct,26,2024
// ----------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../common/common.h"
#include "executant.h"

int Execute(const int Terminal, const int BufSize, char* Buf)
{
int Result;

while (1)
    {
    Result = ERROR_PIPE;

    FILE* Pipe = popen(SCRIPT_RUN, "r");
    if (Pipe == NULL) break;

    int BeRead = 0;
    while (fgets(Buf+BeRead, BufSize-BeRead, Pipe) != NULL)
        {
        BeRead += strlen(Buf + BeRead);
        if (BeRead >= BufSize-1) break;
        }

    int Status = pclose(Pipe);
    if ( Status == -1 || ! WIFEXITED(Status) ) break;

    Result = (WEXITSTATUS(Status) == 0)? 0 : ERROR_CODE; 
    if (Result == 0) write(Terminal, Buf, strlen(Buf));

    break;
    }

return Result;
}
