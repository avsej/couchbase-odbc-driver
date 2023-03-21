#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <string.h>

void getSqlType(SQLLEN id, char * sqlType) {
    char * sqlT;
    switch (id) {
        case 8:
            sqlT = "SQL_DOUBLE";
            break;
        case -5:
            sqlT = "SQL_BIGINT";
            break;
        case 12:
            sqlT = "SQL_VARCHAR";
            break;
    }

    for (int i = 0; i <= strlen(sqlT); i++) {
        sqlType[i] = sqlT[i];
    }
}

main(int argc, char * argv[]) {
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN ret;       /* ODBC API return status */
    SQLSMALLINT columns; /* number of columns in result-set */

    /* Allocate an environment handle */
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    /* We want ODBC 3 support */
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);
    /* Allocate a connection handle */
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

    SQLDriverConnect(dbc, NULL, "DSN=Couchbase DSN (ANSI);", SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    /* Allocate a statement handle */
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    const char * queryTxt
        = "SELECT apv.airport_geo_lat, apv.country, apv.airport_geo_alt FROM `travel-sample`.`inventory`.`airport_view` apv LIMIT 2;";

    SQLExecDirect(stmt, queryTxt, strlen(queryTxt));

    /* How many columns are there */
    SQLNumResultCols(stmt, &columns);
    printf("totalCols: %u\n", columns);

    double doubleColVal;
    char stringColVal[1024];
    long bigIntColVal;


    if (argc == 2) {
        printf("argv[1]: %s\n", argv[1]);
        if (strcmp((const char*) argv[1], (const char *)("bind")) == 0) {
            printf("___BIND___\n");
            for (int i = 1; i <= columns; i++) {
                char nameBuff[1024];
                SQLSMALLINT nameBuffLenUsed;
                SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_NAME, nameBuff, (SQLSMALLINT)1024, &nameBuffLenUsed, NULL);

                SQLLEN columnType = 0;
                SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_CONCISE_TYPE, NULL, 0, NULL, &columnType);
                char sqlType[1024];

                //Binding
                switch (columnType) {
                    case 8: // SQL_DOUBLE => SQL_C_DOUBLE;
                        SQLBindCol(stmt, (SQLUSMALLINT)i, SQL_C_DOUBLE, (SQLPOINTER)(&doubleColVal), (SQLLEN)0, NULL);
                        break;
                    case -5: //SQL_BIGINT => SQL_C_LONG;
                        SQLBindCol(stmt, (SQLUSMALLINT)i, SQL_C_LONG, (SQLPOINTER)(&bigIntColVal), (SQLLEN)0, NULL);
                        break;
                    case 12: //SQL_VARCHAR => SQL_C_CHAR;
                        SQLBindCol(stmt, (SQLUSMALLINT)i, SQL_C_CHAR, (SQLPOINTER)stringColVal, (SQLLEN)1024, NULL);
                        break;
                }
            }

            int row = 1;
            while (SQLFetch(stmt) == SQL_SUCCESS) {
                printf("____ row:%d____\n", row);
                for (int i = 1; i <= columns; i++) {
                    char nameBuff[1024];
                    SQLSMALLINT nameBuffLenUsed;
                    SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_NAME, nameBuff, (SQLSMALLINT)1024, &nameBuffLenUsed, NULL);
                    SQLLEN columnType = 0;
                    SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_CONCISE_TYPE, NULL, 0, NULL, &columnType);

                    switch (columnType) {
                        case 8: // SQL_DOUBLE => SQL_C_DOUBLE;
                            printf("%s: %lf\n", nameBuff, doubleColVal);
                            break;
                        case -5: //SQL_BIGINT => SQL_C_LONG;
                            printf("%s: %ld\n", nameBuff, bigIntColVal);
                            break;
                        case 12: //SQL_VARCHAR => SQL_C_CHAR;
                            printf("%s: %s\n", nameBuff, stringColVal);
                            break;
                    }
                }
                printf("\n\n");
                row++;
            }
        }
    } else {
        printf("___GET_DATA___\n");
        int row = 1;
        while (SQLFetch(stmt) == SQL_SUCCESS) {
            printf("____ row:%d____\n", row);
            for (int i = 1; i <= columns; i++) {
                char nameBuff[1024];
                SQLSMALLINT nameBuffLenUsed;
                SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_NAME, nameBuff, (SQLSMALLINT)1024, &nameBuffLenUsed, NULL);
                SQLLEN columnType = 0;
                SQLColAttribute(stmt, (SQLUSMALLINT)i, SQL_DESC_CONCISE_TYPE, NULL, 0, NULL, &columnType);

                switch (columnType) {
                    case 8: // SQL_DOUBLE => SQL_C_DOUBLE;
                        SQLGetData(stmt, (SQLUSMALLINT)i, SQL_C_DOUBLE, (SQLPOINTER)(&doubleColVal), (SQLLEN)0, NULL);
                        printf("%s: %lf\n", nameBuff, doubleColVal);
                        break;
                    case -5: //SQL_BIGINT => SQL_C_LONG;
                        SQLGetData(stmt, (SQLUSMALLINT)i, SQL_C_LONG, (SQLPOINTER)(&bigIntColVal), (SQLLEN)0, NULL);
                        printf("%s: %ld\n", nameBuff, bigIntColVal);
                        break;
                    case 12: //SQL_VARCHAR => SQL_C_CHAR;
                        SQLGetData(stmt, (SQLUSMALLINT)i, SQL_C_CHAR, (SQLPOINTER)stringColVal, (SQLLEN)1024, NULL);
                        printf("%s: %s\n", nameBuff, stringColVal);
                        break;
                }
            }
            printf("\n\n");
            row++;
        }
    }
}
