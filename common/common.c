/* ----------------------------------------
Author: madbookhub@github

Created: Aug,18,2024
 
Recent: Oct,23,2024
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

void SmoothText(char* Src, char* Dst)
{
int IsforCopy = ! (Dst == NULL);
char* Target = Dst;

for (char* p = Src; *p; ++p)
	{
	if (*p == '\n' || *p == '\r') *p=' ' ;// remove CR/LF to make text smoothly
	if (IsforCopy) *Target++ = *p;
	}
}

void UnEscaped(char* Text)
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

char* EnEscaped(const char* Text)
{
char* Buf = NULL;
int TextLength = strlen(Text);

// Count the number of characters to be processed.
int Count = 0;
for (int i=0; i<TextLength; ++i)
	if ( Text[i] == '"' || Text[i] == '\\') ++Count ;

if ( Count == 0 ) return Text; // no need to make converting

// Try to allocate memory to store the text be processed.
Buf = Alloc( TextLength + Count +1 );
if (Buf == NULL) return NULL;

// Convert the text
int p = 0;
for (int i=0; i<TextLength; ++i)
	{
	if (Text[i] == '"' || Text[i] == '\\') Buf[p++]='\\' ;
	Buf[p++] = Text[i];
	}
Buf[p] = '\0';

return Buf;
}