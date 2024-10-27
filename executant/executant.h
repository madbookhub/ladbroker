#ifndef EXECUTANT_H
#define EXECUTANT_H

// Following Error Message ID MUST be greater than zero to avoid chaos.
static const int ERROR_PIPE	= 100;
static const int ERROR_CODE	= 101;

extern int Execute(const int Terminal, const int BufSize, char* Buf);

#endif