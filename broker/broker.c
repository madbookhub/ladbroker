// ----------------------------------------
// Description: This is module names "Proxy" which is used to transfer any 
// requests from user to LLM server and send the feedback to user later.
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
#include <regex.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "../common/common.h"
#include "broker.h"

#define POSTSIZE 4000 // memory block for the HTTP POST data sent to LLM

#define DEF_SERVERADDR "127.0.0.1"
#define DEF_SERVERPORT "8080"
#define DEF_SERVICETIMEOUT 100

#define SERVER_PATH  "/v1/chat/completions"
#define SERVICE_HEADER "Content-Type: application/json;charset=utf-8\r\nConnection: close"

#define PROMPT_FIX "prompt/prompt_fix.txt"
#define PROMPT_GENERATION "prompt/prompt_generation.txt"

#define WRAP_HEAD "do{"
#define WRAP_TAIL "};exit $@?1:0;"

#define PATTERN_CODE "```(perl)?([^`]+)```.+(```.+```)*"
#define PATTERN_WRAP "\"(.+[.,:?![:space:]])*[.,:?![:space:]]*(do\\{.+\\}[[:space:]]*;exit[[:space:]]+\\$@\\?[[:space:]]*1[[:space:]]*:[[:space:]]*0[[:space:]]*;).*\""
// refer to WRAP_HEAD and WRAP_TAIL

// Following Error Message ID MUST be greater than zero to avoid chaos.
#define ERROR_GETADDRINFO 1
#define ERROR_CONNECTION 2
#define ERROR_SUBMIT 3
#define ERROR_RESPONSE 4
#define ERROR_NOCODES 5
#define ERROR_NOSAVE 6
#define ERROR_PROMPT 10

#define CODES_ORIGINAL ERROR_NOCODES+100
#define CODES_WRAPPED  ERROR_NOCODES+200

char *PromptofGeneration=NULL, *PromptofFix=NULL, *PromptText=NULL;
char *ServerAddr=NULL, *ServerPort=NULL;
int ServiceTimeout = -1;

int LoadPrompt(const int IsforFix)
{
PromptText = IsforFix ? PromptofFix : PromptofGeneration ;

while (PromptText == NULL)
    {
    FILE* File = fopen( IsforFix? PROMPT_FIX: PROMPT_GENERATION, "r" );
    if (File == NULL) break;
     
    fseek(File, 0, SEEK_END);
    int FileSize = ftell(File);
    fseek(File, 0, SEEK_SET);

    if ( (PromptText = Alloc(FileSize+1)) != NULL )
        fread(PromptText, sizeof(char), FileSize, File);

    fclose(File);

    if (IsforFix) PromptofFix = PromptText;
    else PromptofGeneration = PromptText;

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

BeRead = getaddrinfo(ServerAddr, ServerPort, &Hints, &ServerInfo);
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
    Timeout.tv_sec = ServiceTimeout;
    Timeout.tv_usec = 0;
    setsockopt(*Socket,SOL_SOCKET,SO_RCVTIMEO,(char*)&Timeout,sizeof Timeout);

    break;
    }

freeaddrinfo(ServerInfo);

return (p == NULL)? ERROR_CONNECTION: 0;
}

int SendRequest(const int BufSize, const char* Content, int* Socket, char* Buf)
{
char* PostText = Alloc(POSTSIZE);
if ( PostText == NULL ) return ERROR_SUBMIT;

// prepare the POST data
snprintf(PostText, \
        POSTSIZE, \
        "{\"model\":\"gpt-3.5-turbo\", \"temperature\":0.2, \"repeat_last_n\":256, \"repeat_penalty\":1.18, \"penalize_nl\":false, \"top_k\":40, \"top_p\":0.95, \"min_p\":0.05, \"tfs_z\":1, \"typical_p\":1, \"presence_penalty\":0, \"frequency_penalty\":0, \"mirostat\":0, \"mirostat_tau\":5, \"mirostat_eta\":0.1, \"grammar\":\"\", \"n_probs\":0, \"min_keep\":0, \"messages\":[{\"role\":\"system\", \"content\":\"%s\"},{\"role\":\"user\", \"content\":\"%s\"}]}", \
        PromptText, Content \
        ); // change them with your requirement

// prepare the complete HTTP request
snprintf(Buf, \
        BufSize, \
        "POST %s HTTP/1.1\r\nHost:%s:%s\r\nContent-Length:%zu\r\n%s\r\n\r\n%s",\
        SERVER_PATH, ServerAddr, ServerPort, strlen(PostText),SERVICE_HEADER,\
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

int MatchCodes(const char* Pattern, char* Response)
{
int Result = -1;
regex_t Regx;
regmatch_t Matches[3];

if ( regcomp(&Regx, Pattern, REG_EXTENDED) != 0 ) return Result;

if ( regexec(&Regx, Response, 3, Matches, 0) == 0 )
    {
    int LengthofCopy = Matches[2].rm_eo - Matches[2].rm_so;

    strncpy(Response, Response+Matches[2].rm_so, LengthofCopy);
    Response[LengthofCopy] = '\0';

    Result = 0;
    }

regfree(&Regx);

return Result;
}

int GetCodes(char* Response)
{
int Result = ERROR_NOCODES;

if (MatchCodes(PATTERN_WRAP, Response) == 0) Result = CODES_WRAPPED;

if ( Result == ERROR_NOCODES )
    if (MatchCodes(PATTERN_CODE, Response) == 0) Result = CODES_ORIGINAL ;

if ( Result != ERROR_NOCODES ) UnEscaped(Response);

return Result;
}

int SaveCodes(const int IsforWrap, char* BlockofCodes)
{
FILE* File = fopen(SCRIPT_NAME, "w");
if (File == NULL) return ERROR_NOSAVE;

if (IsforWrap) fprintf(File, WRAP_HEAD);

SmoothText(BlockofCodes, NULL);
fprintf(File, "%s", BlockofCodes);

if (IsforWrap) fprintf(File, WRAP_TAIL);

fclose(File);

return 0;
}

int Ask(const int IsforFix, const int BufSize, const char* Content, char* Buf)
{
int Socket=-1, Result, BeRead, TotalBytes;

while (1)
    {
    // load prompt (text) which is the core component of LLM service
    if ( (Result = LoadPrompt(IsforFix)) != 0 ) break;

    // Connect
    if ( (Result = Connect( &Socket )) != 0 ) break;

    // Send request.
    if ((Result = SendRequest(BufSize, Content, &Socket, Buf)) != 0) break;

    // Receive response.
    TotalBytes = BeRead = 0;
    while ( TotalBytes < BufSize )
        {
        BeRead = recv(Socket, Buf+TotalBytes, BufSize-TotalBytes, 0) ;
        if (BeRead <= 0) break;

        TotalBytes += BeRead;
        if (TotalBytes >= BufSize) TotalBytes=BufSize-1; // do not overflow
        }
    Buf[TotalBytes] = '\0';

    if ( (Result = IsResponseValid(Buf)) != 0 ) break;
    if ( (Result = GetCodes(Buf)) == ERROR_NOCODES ) break;
    if ( (Result = SaveCodes(Result==CODES_ORIGINAL, Buf)) != 0 ) break;

    Result = 0;
    break;
    }

close(Socket);
return Result;
}

void NomoreAsk(){ Freealloc(PromptofGeneration), Freealloc(PromptofFix); }

void InitProxy(const int argc, char* argv[])
{
int Option;

opterr = 0;

while ( (Option = getopt(argc, argv, "h:p:t:")) != -1 )
    {
    switch (Option)
        {
        case 'h': ServerAddr = optarg; break;
        case 'p': ServerPort = optarg; break;
        case 't': ServiceTimeout = atoi(optarg); break;
        default : break;
        }
    }

if (ServerAddr == NULL) ServerAddr = DEF_SERVERADDR ;
if (ServerPort == NULL) ServerPort = DEF_SERVERPORT ;
if (ServiceTimeout < 0) ServiceTimeout = DEF_SERVICETIMEOUT ;
}
