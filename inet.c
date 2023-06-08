#include <sys/socket.h>
#include <sys/un.h>

#include <netinet/in.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "inet.h"
#include "panic.h"

int
inetSetSocketReuseAddr(int sockfd)
{
    int yes = 1;

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        warning("INET ERROR: Failed to set REUSEADDR: %s\n", strerror(errno));
        return INET_ERR;
    }

    return INET_OK;
}

int
inetSetSocketNonBlocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK)) {
        warning("INET ERROR: Failed to set NONBLOCK: %s\n", strerror(errno));
        return INET_ERR;
    }

    return INET_OK;
}

int
inetCreateUnixServerSocket(char *name, int backlog)
{
    int sockfd;
    struct sockaddr_un srv;

    srv.sun_family = AF_UNIX;
    strcpy(srv.sun_path, name);
    sockfd = -1;

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        goto error;

    if (unlink(srv.sun_path) == -1)
        goto error;

    if ((bind(sockfd, (struct sockaddr *)&srv, sizeof(struct sockaddr_un))) < 0)
        goto error;

    if (listen(sockfd, backlog) == -1)
        goto error;

    if (inetSetSocketReuseAddr(sockfd) == INET_ERR)
        goto error;

    return sockfd;

error:
    if (sockfd != -1)
        close(sockfd);
    return INET_ERR;
}

int
inetCreateUnixClientSocket(char *name)
{
    int sockfd;
    struct sockaddr_un srv;

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        return INET_ERR;

    srv.sun_family = AF_UNIX;
    strcpy(srv.sun_path, name);

    if (connect(sockfd, (struct sockaddr *)&srv, sizeof(struct sockaddr_un)) ==
            -1) {
        close(sockfd);
        return INET_ERR;
    }

    return sockfd;
}

void *
getInAddress(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in *)sa)->sin_addr);
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

static int
inetCleanupAfterFailure(int sockfd, struct addrinfo *servinfo)
{
    if (sockfd != -1)
        (void)close(sockfd);
    warning("INET ERR: %s\n", strerror(errno));
    freeaddrinfo(servinfo);
    return INET_ERR;
}

static inline int
getaddrinfoError(int err)
{
    warning("INET ERROR: getaddrinfo: %s\n", gai_strerror(err));
    return INET_ERR;
}

int
inetConnect(char *addr, int port, int flags)
{
    char strport[6];
    int sockfd; // connect on sockfd
    struct addrinfo hints, *servinfo, *ptr;
    int rv;

    snprintf(strport, sizeof(strport), "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    sockfd = -1;

    if ((rv = getaddrinfo(addr, strport, &hints, &servinfo)) != 0)
        return getaddrinfoError(rv);

    // connect on the first one we can
    for (ptr = servinfo; ptr != NULL; ptr = ptr->ai_next) {
        if ((sockfd = socket(ptr->ai_family, ptr->ai_socktype,
                     ptr->ai_protocol)) == -1)
            continue;

        if (connect(sockfd, ptr->ai_addr, ptr->ai_addrlen) == -1)
            return inetCleanupAfterFailure(sockfd, servinfo);

        if (inetSetSocketReuseAddr(sockfd) == INET_ERR)
            return inetCleanupAfterFailure(sockfd, servinfo);

        if (flags & INET_NON_BLOCK)
            if (inetSetSocketNonBlocking(sockfd) == INET_ERR)
                return INET_ERR;

        break;
    }

    if (ptr == NULL) {
        return inetCleanupAfterFailure(sockfd, servinfo);
    }

#ifdef DEBUG
    char ipstr[INET6_ADDRSTRLEN];
    if (inetGetIpv4Address(ptr->ai_addr, ptr->ai_family, ipstr,
                sizeof(ipstr)) == INET_OK) {
        printf("connected to %s (%s) port %s\n", addr, ipstr, strport);
    }
#endif

    freeaddrinfo(servinfo);

    return sockfd;
}

static int
_inetCreateServer(int port, char *bindaddr, int addrfam, int backlog, int flags)
{
    int sockfd = -1, rv;
    char strport[6];
    struct addrinfo hints, *servinfo, *ptr;

    snprintf(strport, sizeof(strport), "%d", port);
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = addrfam;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (bindaddr != NULL && !strcmp("*", bindaddr))
        bindaddr = NULL;
    if (addrfam == AF_INET6 && bindaddr && !strcmp("::*", bindaddr))
        bindaddr = NULL;

    if ((rv = getaddrinfo(bindaddr, strport, &hints, &servinfo)) != 0)
        return getaddrinfoError(rv);

    for (ptr = servinfo; ptr != NULL; ptr = ptr->ai_next) {
        if ((sockfd = socket(ptr->ai_family, ptr->ai_socktype,
                     ptr->ai_protocol)) == -1)
            continue;

        if (inetSetSocketReuseAddr(sockfd) == INET_ERR)
            return inetCleanupAfterFailure(sockfd, servinfo);

        if (bind(sockfd, ptr->ai_addr, ptr->ai_addrlen) == -1)
            return inetCleanupAfterFailure(sockfd, servinfo);

        if (listen(sockfd, backlog) == -1)
            return inetCleanupAfterFailure(sockfd, servinfo);

        if (flags & INET_NON_BLOCK)
            if (inetSetSocketNonBlocking(sockfd) == INET_ERR)
                return inetCleanupAfterFailure(sockfd, servinfo);

        break;
    }

    if (ptr == NULL) {
        return inetCleanupAfterFailure(-1, servinfo);
    }

    freeaddrinfo(servinfo);

    return sockfd;
}

int
inetCreateServerBlocking(int port, char *bindaddr, int backlog)
{
    return _inetCreateServer(port, bindaddr, AF_UNSPEC, backlog, 0);
}

int
inetCreateServerNonBlocking(int port, char *bindaddr, int backlog)
{
    return _inetCreateServer(port, bindaddr, AF_UNSPEC, backlog,
            INET_NON_BLOCK);
}

int
_inetAccept(int sockfd, int nonBlocking)
{
    int acceptedfd;
    struct sockaddr_storage in_addr;
    socklen_t socklen;

    if ((acceptedfd = accept(sockfd, (struct sockaddr *)&in_addr, &socklen)) ==
            -1)
        return INET_ERR;

#ifdef DEBUG
    char buf[BUFSIZ];
    inet_ntop(in_addr.ss_family, getInAddress((struct sockaddr *)&in_addr), buf,
            sizeof(buf));
    printf("Connection from: %s\n", buf);
#endif

    if (nonBlocking == 1) {
        if (inetSetSocketNonBlocking(acceptedfd) == INET_ERR) {
            close(acceptedfd);
            return INET_ERR;
        }
    }

    return acceptedfd;
}

/**
 * Either returns the returned accepted file descriptor or indication
 * of an error.
 */
int
inetAcceptBlocking(int sockfd)
{
    return _inetAccept(sockfd, 0);
}

int
inetAcceptNonBlocking(int sockfd)
{
    return _inetAccept(sockfd, 1);
}
