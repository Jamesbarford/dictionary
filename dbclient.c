#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>

#include "dbclient.h"

static int
noop(void *d, int argc, char **argv, char **colname)
{
    (void)d;
    (void)argc;
    (void)argv;
    (void)colname;
    return 0;
}

void
dbRelease(dbClient *client)
{
    sqlite3 *db = client->conn;
    sqlite3_close(db);
    free(client);
}

void
dbForEachRow(dbClient *client, char *stmt, void *p,
        void (*func)(void *, int count, char **data))
{
    sqlite3 *db = client->conn;
    int columncount, i;
    sqlite3_stmt *res;
    char **tuple;

    tuple = 0;
    if (sqlite3_prepare_v2(db, stmt, -1, &res, 0) != SQLITE_OK)
        goto cleanup;

    if (sqlite3_step(res) != SQLITE_ROW)
        goto cleanup;

    if ((columncount = sqlite3_column_count(res)) == 0)
        goto cleanup;

    if ((tuple = malloc(sizeof(char *) * columncount)) == NULL)
        goto cleanup;

    // first row
    for (i = 0; i < columncount; ++i)
        tuple[i] = (char *)sqlite3_column_text(res, i);
    func(p, columncount, tuple);

    while (sqlite3_step(res) == SQLITE_ROW) {
        for (i = 0; i < columncount; ++i)
            tuple[i] = (char *)sqlite3_column_text(res, i);
        func(p, columncount, tuple);
    }

cleanup:
    if (res)
        sqlite3_finalize(res);
    if (tuple)
        free(tuple);
}

/* The stmt has to follow SELECT COUNT .... otherwise this will fail */
long long
dbGetRowCount(dbClient *client, char *stmt)
{
    sqlite3 *db = client->conn;
    char query[2000];
    unsigned const char *val;
    long long rowcount;
    int len, rc, step;
    sqlite3_stmt *res;

    rowcount = 0;
    len = snprintf(query, 2000, "%s", stmt);
    query[len] = '\0';
    rc = sqlite3_prepare_v2(db, query, -1, &res, 0);

    if (rc != SQLITE_OK)
        return rowcount;

    if ((step = sqlite3_step(res)) == SQLITE_ROW) {
        val = sqlite3_column_text(res, 0);
        rowcount = atoll((char *)val);
    }

    sqlite3_finalize(res);
    return rowcount;
}

int
dbExec(dbClient *client, char *sql)
{
    sqlite3 *db = client->conn;
    int rc;

    if ((rc = sqlite3_exec(db, sql, noop, 0, NULL)) != SQLITE_OK)
        return DB_ERR;

    return DB_OK;
}

dbClient *
dbConnect(char *dbname)
{
    sqlite3 *db;
    dbClient *c;

    if ((c = malloc(sizeof(dbClient))) == NULL)
        return NULL;

    if (sqlite3_open(dbname, &db) != SQLITE_OK) {
        free(c);
        return NULL;
    }

    c->conn = db;
    return c;
}
