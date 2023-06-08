#ifndef __DB_CLIENT_H__
#define __DB_CLIENT_H__

#define DB_ERR 0
#define DB_OK 1

typedef struct dbClient {
    void *conn;
} dbClient;

dbClient *dbConnect(char *dbname);
void dbRelease(dbClient *client);

long long dbGetRowCount(dbClient *client, char *stmt);
int dbExec(dbClient *client, char *sql);
void dbForEachRow(dbClient *client, char *stmt, void *p,
        void (*func)(void *, int count, char **data));

#endif
