#ifndef PROXY_H
#define PROXY_H

extern void InitProxy(const int argc, char* argv[]);
extern int Ask(const int IsforFix, const int BufSize, const char* Content, char* Buf);
extern void NomoreAsk();

#endif
