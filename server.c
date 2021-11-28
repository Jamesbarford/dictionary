#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/resource.h>

#include "cstr.h"
#include "dbclient.h"
#include "eloop.h"
#include "hmap.h"
#include "htmlgrep.h"
#include "http.h"
#include "inet.h"
#include "panic.h"

#define SERVER_NAME  "dictionary_daemon"
#define SERVER_ERR 0
#define SERVER_OK  1
#define DB_NAME "dict.db"
#define DB_TABLE "dict"
#define MAX_MSG 1024
#define BACKLOG 500
#define PORT 5000
#define MERRIAM_WEBSTER "https://www.merriam-webster.com/dictionary"

typedef struct dictionaryServer {
    int sfd;
    int maxclients;
    int clientcount;
    pid_t pid;
    hmap *cache;
    dbClient *db;
    eloop *evtloop;
} dictionaryServer;

dictionaryServer server;

int serverDaemonise(char *dir) {
    pid_t proccessId;

    if ((proccessId = fork()) < 0) {
        warning("Error Daemon: Failed to fork() 1 %s\n", strerror(errno));
        return -1;
    }

    if (proccessId > 0)
        exit(EXIT_SUCCESS);

    if (setsid() < 0)
        exit(EXIT_FAILURE);

    if ((proccessId = fork()) < 0) {
        warning("Error Daemon: Failed to fork() 2 %s\n", strerror(errno));
        return -1;
    }

    if (proccessId > 0)
        exit(EXIT_SUCCESS);

    (void)umask(0);
    chdir(dir);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    return 0;
}

cstr *sanitizeText(char *buf) {
    char *ptr, *outptr, tmp[65535];
    int linecount, collect, len;

    collect = 0;
    linecount = 0;
    outptr = tmp;
    ptr = buf;

    while (*ptr != '\0') {
        if (collect == 1) *outptr++ = *ptr;
        if (*ptr == '\n' && collect == 1) {
            linecount++;
            collect = 0;
        }
        if (*ptr == ':' && collect == 0) {
            collect = 1;
            ptr++;
            len = snprintf(outptr, 4, "%d) ", linecount + 1);
            outptr += len;
        }
        ptr++;
    }
    *outptr = '\0';
    len = strlen(tmp);
    return cstrCreate(tmp, len);
}

int serverPesistToDb(char *word, char *definition) {
    char sqlstmt[65535];
    int len;

    len = snprintf(sqlstmt,
            65535,
            "INSERT INTO %s (word, definitions) VALUES ('%s', '%s');",
            DB_TABLE, word, definition);
    sqlstmt[len] = '\0';

    return dbExec(server.db, sqlstmt);
}

httpResponse *serverConsultMerriam(char *word) {
    char url[500] = {'\0'};
    int len;

    len = snprintf(url, 500, "%s/%s", MERRIAM_WEBSTER, word);
    url[len] = '\0';

    return curlHttpGet(url);
}

int serverReadClientMessage(char *msg, char *word, int *len) {
    char *ptr, strnum[200] = {'\0'}, tmpword[200] = {'\0'};
    ptr = msg;

    for (unsigned long i = 0;
            i < sizeof(tmpword) && *ptr != ':' && *ptr != '\0'; ++i) {
        tmpword[i] = *ptr++;
    }

    if (*ptr == '\0') return SERVER_ERR;

    ptr++;

    for (unsigned long i = 0; i < sizeof(strnum) && *ptr != '\0'; ++i) {
        if (!isdigit(*ptr)) return SERVER_ERR;
        strnum[i] = *ptr++;
    }

    *len = atoi(strnum);
    tmpword[*len] = '\0';
    memcpy(word, tmpword, *len);
    word[*len] = '\0';

    return SERVER_OK;
}

cstr *serverLookupClientRequest(char *reqword, int reqwordlen) {
    char *definition;
    httpResponse *resp;

    if ((definition = hmapGet(server.cache, reqword)) != NULL) {
        return definition;
    } else {
        cstr *sanitised;
        sanitised = NULL;
        // go to the internet and find a definition
        if ((resp = serverConsultMerriam(reqword)) == NULL)
            return SERVER_ERR;
       
        if (resp->status_code == 200) {
            char outbuf[65355] = {'\0'};
            htmlGrepFromUntil(resp->body, "<span class=\"dtText\">", 21,
                    "</span>", 7, outbuf);

            sanitised = sanitizeText(outbuf);
            hmapAdd(server.cache, cstrCreate(reqword, reqwordlen), sanitised);
            serverPesistToDb(reqword, sanitised);
        }
        httpResponseRelease(resp);
        return sanitised;
    }

    return NULL;
}

void serverSendClientReply(eloop *el, int fd, void *data, int mask) {
    (void)el;
    (void)data;
    cstr *response;
    char msg[MAX_MSG] = {'\0'}, word[MAX_MSG - 100] = {'\0'};
    int rbytes, wordlen, definitionlen;

    response = NULL;
    definitionlen = 0;

    if ((rbytes = read(fd, msg, MAX_MSG)) < 0)
        goto error;

    if (!serverReadClientMessage(msg, word, &wordlen))
        goto error;

    if ((response = serverLookupClientRequest(word, wordlen)) == NULL) {
        write(fd, "Failed to find word", 19);
        goto error;
    }

    definitionlen = cstrlen(response);

    if (definitionlen != 0)
        write(fd, response, definitionlen);
    
    // We're done with this
error:
    server.clientcount--;
    eloopDeleteEvent(el, fd, mask);
    close(fd);
}

void serverAccept(eloop *el, int fd, void *data, int mask) {
    (void)el; (void)fd; (void)data;
    int sockfd;
    
    if ((sockfd = inetAcceptNonBlocking(fd)) == INET_ERR)
        return;

    if (eloopAddEvent(el, sockfd, mask,
                serverSendClientReply, NULL) == EVT_ERR)
        return;

    server.clientcount++;
}

void serverTransferToCache(void *_cache, int columncount, char **row) {
    hmap *cache = _cache;
    cstr *key, *value;
    if (columncount != 2)
        panic("SERVER ERROR: expected 2 columns got %d\n", columncount);

    key = cstrCreate(row[0], strlen(row[0]));
    value = cstrCreate(row[1], strlen(row[1]));
    hmapAdd(cache, key, value);
}

void serverInitDictionary() {
    char sqltablestmt[2000], sqlcountstmt[200], sqlselectstmt[200];
    int len;
    long long rowcount;

    len = snprintf(sqltablestmt, 2000,
            "CREATE TABLE IF NOT EXISTS %s ( "
            " word TEXT NOT NULL,"
            " definitions TEXT"
            ");",
            DB_TABLE);
    sqltablestmt[len] = '\0';

    if (!dbExec(server.db, sqltablestmt))
        panic("SERVER ERROR: Failed to create table\n");

    len = snprintf(sqlcountstmt, 200, "SELECT COUNT(*) FROM %s;", DB_TABLE);
    sqlcountstmt[len] = '\0';

    rowcount = dbGetRowCount(server.db, sqlcountstmt);

    if (rowcount != 0) {
        len = snprintf(sqlselectstmt, 200, "SELECT * FROM %s ;", DB_TABLE);
        sqlselectstmt[len] = '\0';
        dbForEachRow(server.db, sqlselectstmt, server.cache,
                serverTransferToCache);
    }
}

void serverSetFileDescriptorLimit() {
    struct rlimit fdlim, newfdlim;
    rlim_t currentlimit, targetlimit, decrement;

    targetlimit = 10024;
    decrement = 12;

    if (getrlimit(RLIMIT_NOFILE, &fdlim) == -1)
        panic("SERVER ERROR: Failed to get file descriptor limit %s\n",
                strerror(errno));
    currentlimit = fdlim.rlim_cur;

    while (targetlimit > currentlimit) {
        newfdlim.rlim_cur = targetlimit;
        newfdlim.rlim_max = targetlimit;

        if (setrlimit(RLIMIT_NOFILE, &newfdlim) != -1) break;
        targetlimit -= decrement;
    }

    server.maxclients = targetlimit;
    printf("[%d]: server file descriptors increased from %lu to %lu\n",
            server.pid, fdlim.rlim_cur, targetlimit);
}

void serverInit() {
    server.pid = getpid();

    serverSetFileDescriptorLimit();

    if ((server.cache = hmapCreate()) == NULL)
        panic("SERVER ERROR: Failed to create cache\n");

    if ((server.db = dbConnect(DB_NAME)) == NULL)
        panic("SERVER ERROR: Failed to init database\n");

    if ((server.sfd = inetCreateServerNonBlocking(PORT, NULL, BACKLOG)) <= 0)
        panic("SERVER ERROR: Failed to create socket %s\n", strerror(errno));
    
    if ((server.evtloop = eloopCreate(server.maxclients)) == NULL)
        panic("SERVER ERROR: Failed to create eventloop\n",
                strerror(errno));

    eloopAddEvent(server.evtloop, server.sfd, EVT_READ, serverAccept, NULL);

    serverInitDictionary();
    printf("[%d]: server cache initalized\n", server.pid);
}

int main(void) {
    serverInit();

    printf("[%d]: server started on port :: %d\n", server.pid, PORT);

#ifdef DAEMON
    serverDaemonise("");
#endif
    eloopMain(server.evtloop);
}
