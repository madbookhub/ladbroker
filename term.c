// ----------------------------------------
// Description: This module names "Term", it is used to launch a terminal
// to accept any input and pass them to somewhere (to do something) 
//
// Author: madbookhub@github
//
// Created: Aug,18,2024
// 
// Recent: Oct,6,2024
// ----------------------------------------

// #define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 600 // a POSIX flag,help me to work on macOS and Linux

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>

// following work for "fork"
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "common.h"
#include "term.h"

#define BUF_SIZE 200 // user can enter up to this number of characters

#define ERROR_GETWINSIZE 1
#define ERROR_SETWINSIZE 2
#define ERROR_POSIXOPENPT 3
#define ERROR_GRANTPT 4
#define ERROR_NOPTNAME 5
#define ERROR_FORK 10
#define ERROR_OPENSLAVE 11
#define ERROR_REDIRECT 12
#define ERROR_SHELL 13

#define DEF_SHELL "/bin/bash"

void Run(int Master, CALLBACKFUNC Callback)
{
int BeRead, IsTouched=0;
int fd = (Master > STDIN_FILENO) ? Master : STDIN_FILENO;
fd_set fds;
char* Buf = NULL;

if ( (Buf = Alloc(BUF_SIZE)) == NULL ) return;

while (1)
    {
    FD_ZERO(&fds);
    FD_SET(Master, &fds);
    FD_SET(STDIN_FILENO, &fds);

    if (select(fd + 1, &fds, NULL, NULL, NULL) < 0) break;

    if (FD_ISSET(Master, &fds))
        {
        BeRead = read(Master, Buf, BUF_SIZE);
        if (BeRead <= 0) break;

        if (! IsTouched)
            IsTouched=1, write(STDOUT_FILENO, STARTSPEECH, strlen(STARTSPEECH));

        write(STDOUT_FILENO, Buf, BeRead);
        }

    if (FD_ISSET(STDIN_FILENO, &fds))
        {
        BeRead = read(STDIN_FILENO, Buf, BUF_SIZE-1);
        if (BeRead <= 0) break;
        Buf[BeRead] = '\0';

        sscanf(Buf, "%[^\r\n]", Buf);

        // Tell Master to quit if the standard Exit instruction is called!
        if (strcmp(Buf, "exit") == 0) write(Master, Buf, strlen(Buf));
        else Callback(STDOUT_FILENO, Buf);

        // Do not miss one hit of Enter key...this ensures anything sent to
        // Master is completed just as press from keyboard, endding with
        // "Enter" knock.
        write(Master, "\r", 1);
        }

    }

free(Buf);
}

int OpenTerminal(CALLBACKFUNC OnSubmit, void* OnClose)
{
int Master, Slave;
pid_t pid;
struct winsize WinSize;
char* NameofSlave;

// Get the size of the current terminal.
if (ioctl(STDIN_FILENO, TIOCGWINSZ, &WinSize) < 0) return ERROR_GETWINSIZE;

// Open a new pseudo-terminal "master".
Master = posix_openpt(O_RDWR);
if (Master < 0) return ERROR_POSIXOPENPT;

// Grant access to the pseudo-terminal "slave" and unlock it.
if ( (grantpt(Master) < 0) || (unlockpt(Master) < 0) ) return ERROR_GRANTPT;

// Get the name of the slave pseudo-terminal.
NameofSlave = ptsname(Master);
if (NameofSlave == NULL) return ERROR_NOPTNAME;

pid = fork();
if (pid < 0) return ERROR_FORK;

if (pid == 0) // Following for child process...
    {  
    close(Master);
        
    // Open the slave pseudo-terminal.
    Slave = open(NameofSlave, O_RDWR);
    if (Slave < 0) return ERROR_OPENSLAVE;

    // Redirect standard I/O to slave PTY !
    int IsnoRedirection = ( dup2(Slave, STDIN_FILENO) < 0 || \
                    dup2(Slave, STDOUT_FILENO) < 0 || \
                    dup2(Slave, STDERR_FILENO) < 0 ) ;

    close(Slave);

    if (IsnoRedirection) return ERROR_REDIRECT;

    // Create a new session and set the controlling terminal.
    setsid();
    ioctl(STDIN_FILENO, TIOCSCTTY, 0);

    // Set the terminal size.
    if (ioctl(STDIN_FILENO, TIOCSWINSZ, &WinSize) < 0) return ERROR_SETWINSIZE; 
    
    // Open the shell !
    char* Shell = getenv("SHELL") ;
    char *args[] = { Shell?Shell:DEF_SHELL, NULL};
    execvp(args[0], args);
    return ERROR_SHELL;
    }
else // Following for Parent process...
    {  
    Run(Master, OnSubmit);
    close(Master);

    ( (void (*)())OnClose )();

    // Clear screen to avoid confusion caused by residual messages.
    // This is not necessary, but just for elegant and concise.
    printf("\033[H\033[J"); // ANSI escape codes to clear the screen

    kill(pid, SIGTERM), waitpid(pid, NULL, 0);
    }

return 0;
}
