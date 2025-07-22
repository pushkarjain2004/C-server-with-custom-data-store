#ifndef CACHE22
#define CACHE22
#define _GNU_SOURCE

#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>

#include<stdbool.h>
#include<assert.h>
#include<errno.h>
#include <stddef.h>
#include<stdarg.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/in.h>


#define HOST   "127.0.0.1"
#define PORT    "12049"

typedef unsigned int int32;
typedef unsigned short int int16;
typedef unsigned char int8;

struct s_client{
    int s;
    
    char ip[16];
    int16 port;

};
typedef struct s_client Client;

typedef int32 (*Callback)(Client *,int8*,int8*);
struct s_cmdhandler{
    int8 *cmd;
    Callback handler;
};

typedef struct s_cmdhandler CmdHandler;

void assert_perror(int system_call_return_value);
void childloop(Client*);
void mainloop(int s);
int initserver(int16);

#endif
