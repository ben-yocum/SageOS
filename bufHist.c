#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bufHist.h"

#include "os345.h"
#include "sqlite3.h"
#include "logger.h"

sqlite3 *db;

/********************************
* Global Variables
********************************/
long curr = 0;
long max = -1;
char buf[INBUF_SIZE+1];

static int getInBuf(void *NotUsed, int argc, char **argv, char **azColName);
static int maxBID(void *NotUsed, int argc, char **argv, char **azColName);




/********************************
* History External Functions
********************************/
void addBuff(char* str){
    char *zErrMsg = 0;
    int rc;
    char sql[75];

    sprintf(sql, "INSERT INTO commands (text)\
                  VALUES ('%s');", str);

    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    max++;

    return;
}

void prvBuf(char* str){
    curr--;
    if(curr<=0) curr=1;

    char *zErrMsg = 0;
    int rc;
    char sql[75];


    sprintf(sql, "SELECT text FROM commands WHERE id='%ld';", curr);

    rc = sqlite3_exec(db, sql, getInBuf, 0, &zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    strcpy(str, buf);
    return;
}


void nxtBuf(char* str){
    curr++;
    if(curr>max){ //print an empty line
         str[0]=0;
         curr=max+1;
         return;
    }


    char *zErrMsg = 0;
    int rc;
    char sql[75];

    sprintf(sql, "SELECT text FROM commands WHERE id='%ld';", curr);

    rc = sqlite3_exec(db, sql, getInBuf, 0, &zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    strcpy(str, buf);
    return;
}



/********************************
* Utilities
********************************/
void resetBuffHist(){
    curr=max+1;
}

void initDB(){
    logg("Opening database");

    openDB();

    //Initialize pointer to max value
    getMaxBID();
    curr=max+1;

    return;
}

void killDB(){
    logg("Closing database");
    closeDB();
    return;
}

void getMaxBID(){
    char *zErrMsg = 0;
    int rc;
    char* sql;

    sql =  "SELECT MAX(id) AS max_id FROM commands;";

    rc = sqlite3_exec(db, sql, maxBID, 0, &zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    return;
}

void openDB(){
    int rc;

    rc = sqlite3_open("SageOS.db", &db);

    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(0);
    }
    return;
}

void closeDB(){
    sqlite3_close(db);
    return;
}

void clearBuffHist(){
    char *zErrMsg = 0;
    int rc;
    char* sql = "DELETE FROM commands;";

    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }

    max=0;

    return;
}




/********************************
* Callback Functions
********************************/
static int getInBuf(void *NotUsed, int argc, char **argv, char **azColName){
    strcpy(buf, argv[0]);
    return 0;
}

static int maxBID(void *NotUsed, int argc, char **argv, char **azColName){
    max=strtol(argv[0], NULL, 0);
    return 0;
}









