#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

typedef int SOCKET;
typedef int UDPSOCKET;
typedef uint32_t IPADDRESS; // On linux use uint32_t in network byte order (per getpeername)
typedef uint16_t IPPORT; // on linux use network byte order

#define NULLSOCKET 0

inline void closesocket(SOCKET s) {
    close(s);
}

#define getRandom() rand()

inline void socketpeeraddr(SOCKET s, IPADDRESS *addr, IPPORT *port) {

    sockaddr_in r;
    socklen_t len = sizeof(r);
    if(getpeername(s,(struct sockaddr*)&r,&len) < 0) {
        printf("getpeername failed\n");
        *addr = 0;
        *port = 0;
    }
    else {
        //htons

        *port  = r.sin_port;
        *addr = r.sin_addr.s_addr;
    }
}

inline void udpsocketclose(UDPSOCKET s) {
    close(s);
}

inline UDPSOCKET udpsocketcreate(unsigned short portNum)
{
    sockaddr_in addr;

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    int s     = socket(AF_INET, SOCK_DGRAM, 0);
    addr.sin_port = htons(portNum);
    if (bind(s,(sockaddr*)&addr,sizeof(addr)) != 0) {
        printf("Error, can't bind\n");
        close(s);
        s = 0;
    }

    return s;
}

// TCP sending
inline ssize_t socketsend(SOCKET sockfd, const void *buf, size_t len)
{
    return send(sockfd, buf, len, 0);
}

inline ssize_t udpsocketsend(UDPSOCKET sockfd, const void *buf, size_t len,
                             IPADDRESS destaddr, IPPORT destport)
{
    sockaddr_in addr;

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = destaddr;
    addr.sin_port = destport;

    return sendto(sockfd, buf, len, 0, (sockaddr *) &addr, sizeof(addr));
}
