#ifndef TERM_H
#define TERM_H

typedef int (*CALLBACKFUNC)(const int, const char*);

extern int OpenTerminal( CALLBACKFUNC OnSubmit, void* OnClose );

#endif
