#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define SOCKET int

typedef struct sockaddr SOCKADDR;

#define closesocket(s) close(s)

typedef unsigned DWORD;

#define _snprintf snprintf
