#ifndef __INET_H__
#define __INET_H__

#define INET_ERR 0
#define INET_OK 1

#define INET_NON_BLOCK 1

int inetSetSocketReuseAddr(int sockfd);
int inetCreateUnixServerSocket(char *name, int backlog);
int inetCreateUnixClientSocket(char *name);

int inetConnect(char *addr, int port, int flags);
int inetAcceptBlocking(int sockfd);
int inetAcceptNonBlocking(int sockfd);
int inetCreateServerBlocking(int port, char *bindaddr, int backlog);
int inetCreateServerNonBlocking(int port, char *bindaddr, int backlog);

#endif
