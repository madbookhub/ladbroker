/* ----------------------------------------
Author: madbookhub@github

Created: Aug,18,2024
 
Recent: Oct,6,2024
---------------------------------------- */
#include <stdlib.h>
#include "common.h"

char* Alloc(const int Sizetoallocate)
{
return (char*)calloc(Sizetoallocate, sizeof(char));
}

void Freealloc(char* Allocation)
{
if (Allocation != NULL) free(Allocation);
}

void UnEscape(char* Text)
{
char *Src=Text, *Dst=Text;

while (*Src)
	{
	if (*Src == '\\')
		{
		++Src;
		switch (*Src)
			{
			case 'n': *Dst++ = '\n' ; break;
			case 't': *Dst++ = '\t' ; break;
			case '\"': *Dst++ = '\"' ; break;

			case '\\':
				if ( *(Src+1) == 'n' ) Src++;
				else *Dst++ = '\\' ;
				break;

			default:
				*Dst++ = '\\'; // Keep the backslash if not escape sequence
				*Dst++ = *Src++;
				break;
			}  // end Case
		}
	else *Dst++ = *Src;

	Src++;
	}

*Dst = '\0';
}