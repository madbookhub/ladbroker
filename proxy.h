#ifndef PROXY_H
#define PROXY_H

// These entities work for LLM service endpoint.
static  const char* SERVER_PORT = "8080";
static  const char* SERVER_ADDR = "192.168.64.1"; // change it on your fact
static	const char*	SERVER_PATH = "/v1/chat/completions";

static const unsigned int SERVICE_TIMEOUT = 120; // second
static 	const char* SERVICE_HEADER = "Content-Type: application/json;charset=utf-8\r\nConnection: close";

// These are components for LLM service.
static const char* PROMPTFILE = "prompt.txt";
static const char* PATTERN = "```(perl)?([^`]+)```.+(```.+```)*" ;

extern int Ask(const char* Content, char** Buf);
extern void NomoreAsk();

#endif
