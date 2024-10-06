/* ----------------------------------------
Name: Ladbroker

Description: This is a LLM application, which passes any user's requirements 
to LLM to translate them into program codes, and then executes them.
For example: "What time is it now?" --> LLM -->
--> "use DateTime;my $time = DateTime->now();print "$time\n";"
--> execute above codes locally and output the result

Author: madbookhub@github

Created: Aug,18,2024
 
Recent: Oct,5,2024
---------------------------------------- */
#include <stdlib.h>
#include "common.h"
#include "term.h"
#include "proxy.h"
#include "executant.h"

int OnSubmitfromTerminal(const int Terminal, const char* Submission)
{
int Result;
char* Buffer = NULL;

Result = Ask(Submission, &Buffer); // talk to LLM to get some codes
if (! Result) Execute(Terminal, Buffer); // execute codes responded by LLM

Freealloc(Buffer);
return Result;
}

void OnTerminalCloses()
{
NomoreAsk();
}

int main()
{
return OpenTerminal( &OnSubmitfromTerminal, &OnTerminalCloses );
}
