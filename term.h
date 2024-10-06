#ifndef TERM_H
#define TERM_H

static const char* STARTSPEECH = "It is activated.Type \"exit\" to quit.\n";

typedef int (*CALLBACKFUNC)(const int, const char*);

extern int OpenTerminal( CALLBACKFUNC OnSubmit, void* OnClose );

#endif
