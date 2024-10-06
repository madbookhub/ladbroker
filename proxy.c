// ----------------------------------------
// Description: This is module names "Proxy" which is used to transfer any 
// requests from user to LLM server and send the feedback to user later.
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
#include <regex.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "common.h"
#include "proxy.h"

#define BUF_SIZE 8192
#define POST_SIZE 1000 // memory block for the HTTP POST data sent to LLM

// Following Error Message ID MUST be greater than zero to avoid chaos.
#define ERROR_GETADDRINFO 1
#define ERROR_CONNECTION 2
#define ERROR_ALLOC 3
#define ERROR_SUBMIT 4
#define ERROR_RESPONSE 5
#define ERROR_NOCODES 6
#define ERROR_NOSAVE 7
#define ERROR_PROMPT 10

char* PromptText = NULL;

int LoadPrompt()
{
while (PromptText == NULL)
    {
    FILE* File = fopen(PROMPTFILE, "r");
    if (File == NULL) break;
     
    fseek(File, 0, SEEK_END);
    int FileSize = ftell(File);
    fseek(File, 0, SEEK_SET);

    if ( (PromptText = Alloc(FileSize+1)) != NULL )
        fread(PromptText, sizeof(char), FileSize, File);

    fclose(File);

    break;
    }

return (PromptText == NULL)? ERROR_PROMPT : 0;
}

int Connect(int* Socket)
{
int BeRead;
struct addrinfo Hints, *ServerInfo, *p;

// initialize
memset(&Hints, 0, sizeof Hints);
Hints.ai_family = AF_UNSPEC;
Hints.ai_socktype = SOCK_STREAM;

// trt to make connection to remote server

BeRead = getaddrinfo(SERVER_ADDR, SERVER_PORT, &Hints, &ServerInfo);
if (BeRead != 0) return ERROR_GETADDRINFO;

for (p = ServerInfo; p != NULL; p = p->ai_next)
    {
    *Socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (*Socket == -1) continue;

    if (connect(*Socket, p->ai_addr, p->ai_addrlen) == -1)
        { close(*Socket); continue; }

    // set Timeout for this socket,it is useful to get rid of the "freeze"
    // problem (no response) of LLM server. It is recommended that setting this
    // value as required.
    struct timeval Timeout;
    Timeout.tv_sec = SERVICE_TIMEOUT;
    Timeout.tv_usec = 0;
    setsockopt(*Socket,SOL_SOCKET,SO_RCVTIMEO,(char*)&Timeout,sizeof Timeout);

    break;
    }

freeaddrinfo(ServerInfo);

return (p == NULL)? ERROR_CONNECTION: 0;
}

int SendRequest(const int BufSize, const char* Content, int* Socket, char* Buf)
{
char* PostText = Alloc(POST_SIZE);
if ( PostText == NULL ) return ERROR_SUBMIT;

// prepare the POST data
snprintf(PostText, \
        POST_SIZE, \
        "{\"model\":\"gpt-3.5-turbo\", \"temperature\":0.2, \"repeat_last_n\":256, \"repeat_penalty\":1.18, \"penalize_nl\":false, \"top_k\":40, \"top_p\":0.95, \"min_p\":0.05, \"tfs_z\":1, \"typical_p\":1, \"presence_penalty\":0, \"frequency_penalty\":0, \"mirostat\":0, \"mirostat_tau\":5, \"mirostat_eta\":0.1, \"grammar\":\"\", \"n_probs\":0, \"min_keep\":0, \"messages\":[{\"role\":\"system\", \"content\":\"%s\"},{\"role\":\"user\", \"content\":\"%s\"}]}", \
        PromptText, Content \
        ); // change them with your requirement

// prepare the complete HTTP request
snprintf(Buf, \
        BufSize, \
        "POST %s HTTP/1.1\r\nHost:%s:%s\r\nContent-Length:%zu\r\n%s\r\n\r\n%s",\
        SERVER_PATH, SERVER_ADDR, SERVER_PORT, strlen(PostText),SERVICE_HEADER,\
        PostText \
        );

free(PostText);

// send request
return (send(*Socket, Buf, strlen(Buf), 0) == -1)? ERROR_SUBMIT : 0 ;
}

int IsResponseValid(const char* Response)
{
char* Match = strstr(Response, "HTTP/1.1 200");
return ( (Match != NULL) && (Match - Response == 0) )? 0 : ERROR_RESPONSE;
}

int GetCodes(char* Response)
{
int Result = ERROR_NOCODES;
regex_t Regx;
regmatch_t Matches[3];

while (1)
    {
    if ( regcomp(&Regx, PATTERN, REG_EXTENDED) ) break;

    if ( regexec(&Regx, Response, 3, Matches, 0) == 0 )
        {
        int NumberofChartoCopy = Matches[2].rm_eo - Matches[2].rm_so;

        strncpy(Response, Response+Matches[2].rm_so, NumberofChartoCopy);
        Response[NumberofChartoCopy] = '\0';

        UnEscape(Response);

        Result = 0;
        }

    regfree(&Regx);
    break;
    }

return Result;
}

int SaveCodes(char* BlockofCodes)
{
int Result = ERROR_NOSAVE;
FILE* File = fopen(SCRIPT_NAME, "w");

if (File != NULL) fprintf(File, "%s", BlockofCodes), fclose(File), Result=0;

return Result;
}

int Ask(const char* Content, char** Buf)
{
int Socket=-1, Result, SizeofBuf, BeRead, TotalBytes;

while (1)
    {
    // load prompt (text) which is the core component of LLM service
    if ( (Result = LoadPrompt()) != 0 ) break;

    // Connect
    if ( (Result = Connect( &Socket )) != 0 ) break;

    // Send request.
    *Buf = Alloc(BUF_SIZE);
    if ( (Result = (*Buf == NULL)? ERROR_ALLOC: 0) != 0 ) break;

    SizeofBuf = BUF_SIZE * sizeof(char);
    if ((Result = SendRequest(SizeofBuf, Content, &Socket, *Buf)) != 0) break;

    // Receive response.
    TotalBytes = BeRead = 0;
    while ( TotalBytes < SizeofBuf )
        {
        BeRead = recv(Socket, *Buf+TotalBytes, SizeofBuf-TotalBytes, 0) ;
        if (BeRead <= 0) break;

        TotalBytes += BeRead;
        if (TotalBytes >= SizeofBuf) TotalBytes=SizeofBuf-1; // do not overflow
        }
    (*Buf)[TotalBytes] = '\0';

    if ( (Result = IsResponseValid(*Buf)) != 0 ) break;
    if ( (Result = GetCodes(*Buf)) != 0 ) break;
    if ( (Result = SaveCodes(*Buf)) != 0 ) break;

    Result = 0;
    break;
    }

close(Socket);

return Result; // the memory block ("Buf") is not concerned here
}

void NomoreAsk(){ Freealloc(PromptText); }
