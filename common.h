#ifndef COMMON_H
#define COMMON_H

// These are declaration of external executable components.
static const char* SCRIPT_NAME = "_.pl";
static const char* SCRIPT_RUN = "perl _.pl"; // refer to SCRIPT_NAME !

extern char* Alloc(const int Sizetoallocate);
extern void Freealloc(char* Allocation);
extern void UnEscape(char* Text);

#endif
