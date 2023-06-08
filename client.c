#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "inet.h"
#include "panic.h"

#define MAX_MSG     1024
#define PORT        5050
#define SERVER_NAME "dictionary_daemon"

static char *progname;

static void
clientUsage(void)
{
    panic("Usage: %s <string>\n"
          "Print dictionary definition of a word\n",
            progname);
}

static int
clientFindDefinition(char *word)
{
    int sockfd;
    char buf[BUFSIZ], msg[BUFSIZ];
    int len, rbytes;

    len = strlen(word);
    len = snprintf(msg, MAX_MSG, "%s:%d", word, len);
    msg[len] = '\0';

    if ((sockfd = inetConnect(NULL, PORT, 0)) == INET_ERR)
        panic("Failed to create unix socket %s\n", strerror(errno));

    if (write(sockfd, msg, len) <= 0) {
        close(sockfd);
        panic("Failed to write to server %s\n", strerror(errno));
    }

    if ((rbytes = read(sockfd, buf, BUFSIZ)) < 0) {
        warning("CLIENT ERROR: Failed to read reply %s\n", strerror(errno));
    }
    buf[rbytes] = '\0';
    printf("%s\n", buf);

    return 1;
}

int
main(int argc, char **argv)
{
    int retval;
    progname = argv[0];

    if (argc != 2)
        clientUsage();

    retval = clientFindDefinition(argv[1]);

    return retval == 1 ? 0 : 1;
}
