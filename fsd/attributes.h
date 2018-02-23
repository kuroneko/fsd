/*
 *  MDBMS source code
 *
 *  (c) 1997 HINT InterNetworking Technologies
 *
 * This is beta software. This version of mdbms can be used without
 * charge. Redistribution in source and binary forms, without any
 * modification, is permitted.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED 'AS IS'. NO RESPONSIBILITY FOR
 * ANY DAMAGE CAUSED BY THE USE OF THIS SOFTWARE, CAN BE ACCEPTED BY
 * THE AUTHOR AND/OR PUBLISHER.
 *
 * See the README.source file for information.
 *
 *  Please report bugs/fixes to marty@hinttech.com
 */

#ifndef attributesh
#define attributesh
extern char *attnames[];
#define ATT_INT         1
#define ATT_FLOAT       2
#define ATT_MONEY       3
#define ATT_CHAR        4
#define ATT_VARCHAR     5
#define ATT_DATE        6
#define ATT_BLOB        7
#define ATT_NULL        10

#define TBL_NORMAL 1
#define TBL_VIEW 2
#define TBL_SYSTEM 3

#define DB_SYSTEM 1
#define DB_NORMAL 2
#endif
