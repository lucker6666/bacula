/*
   Bacula® - The Network Backup Solution
   Copyright (C) 2009-2010 Free Software Foundation Europe e.V.
   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.
   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.
   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
/*
 * Bacula Catalog Database routines specific to Ingres
 *   These are Ingres specific routines
 *
 *    Stefan Reddig, June 2009
 */
/* The following is necessary so that we do not include
 * the dummy external definition of DB.
 */
#define __SQL_C                       /* indicate that this is sql.c */
#include "bacula.h"
#include "cats.h"
/* # line 43 "myingres.sc" */	
#ifdef HAVE_INGRES
#include <eqpname.h>
#include <eqdefcc.h>
#include <eqsqlca.h>
extern IISQLCA sqlca;   /* SQL Communications Area */
#include <eqsqlda.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "myingres.h"
/*
 * ---Implementations---
 */
int INGcheck()
{
   return (sqlca.sqlcode < 0) ? sqlca.sqlcode : 0;
}
short INGgetCols(B_DB *mdb, const char *query)
{
   bool stmt_free = false;
/* # line 64 "myingres.sc" */	
  
  char *stmt;
/* # line 66 "myingres.sc" */	
  
   short number = 1;
   IISQLDA *sqlda;
   sqlda = (IISQLDA *)malloc(IISQDA_HEAD_SIZE + (number * IISQDA_VAR_SIZE));
   memset(sqlda, 0, (IISQDA_HEAD_SIZE + (number * IISQDA_VAR_SIZE)));
   sqlda->sqln = number;
   /*
    * See if we need to run this through the limit_filter.
    */
   if (strstr(query, "LIMIT") != NULL) {
      stmt = mdb->limit_filter->replace(query);
   } else {
      stmt = bstrdup(query);
      stmt_free = true;
   }
/* # line 86 "myingres.sc" */	/* prepare */
  {
    IIsqInit(&sqlca);
    IIsqPrepare(0,(char *)"s1",(char *)0,0,stmt);
  }
/* # line 87 "myingres.sc" */	/* host code */
   if (INGcheck() < 0) {
      number = -1;
      goto bail_out;
   }
/* # line 92 "myingres.sc" */	/* describe */
  {
    IIsqInit(&sqlca);
    IIsqDescribe(0,(char *)"s1",sqlda,0);
  }
/* # line 93 "myingres.sc" */	/* host code */
   if (INGcheck() < 0) {
      number = -1;
      goto bail_out;
   }
   number = sqlda->sqld;
bail_out:
   if (stmt_free) {
      free(stmt);
   }
   free(sqlda);
   return number;
}
static inline IISQLDA *INGgetDescriptor(B_DB *mdb, short numCols, const char *query)
{
   bool stmt_free = false;
/* # line 111 "myingres.sc" */	
  
  char *stmt;
/* # line 113 "myingres.sc" */	
  
   int i;
   IISQLDA *sqlda;
   sqlda = (IISQLDA *)malloc(IISQDA_HEAD_SIZE + (numCols * IISQDA_VAR_SIZE));
   memset(sqlda, 0, (IISQDA_HEAD_SIZE + (numCols * IISQDA_VAR_SIZE)));
   sqlda->sqln = numCols;
   /*
    * See if we need to run this through the limit_filter.
    */
   if (strstr(query, "LIMIT") != NULL) {
      stmt = mdb->limit_filter->replace(query);
   } else {
      stmt = bstrdup(query);
      stmt_free = true;
   }
/* # line 133 "myingres.sc" */	/* prepare */
  {
    IIsqInit(&sqlca);
    IIsqPrepare(0,(char *)"s2",sqlda,0,stmt);
  }
/* # line 135 "myingres.sc" */	/* host code */
   if (stmt_free) {
      free(stmt);
   }
   for (i = 0; i < sqlda->sqld; ++i) {
      /*
       * Negative type indicates nullable coulumns, so an indicator
       * is allocated, otherwise it's null
       */
      if (sqlda->sqlvar[i].sqltype > 0) {
         sqlda->sqlvar[i].sqlind = NULL;
      } else {
         sqlda->sqlvar[i].sqlind = (short *)malloc(sizeof(short));
      }
      /*
       * Alloc space for variable like indicated in sqllen
       * for date types sqllen is always 0 -> allocate by type
       */
      switch (abs(sqlda->sqlvar[i].sqltype)) {
      case IISQ_TSW_TYPE:
         sqlda->sqlvar[i].sqldata = (char *)malloc(IISQ_TSW_LEN);
         break;
      case IISQ_TSWO_TYPE:
         sqlda->sqlvar[i].sqldata = (char *)malloc(IISQ_TSWO_LEN);
         break;
      case IISQ_TSTMP_TYPE:
         sqlda->sqlvar[i].sqldata = (char *)malloc(IISQ_TSTMP_LEN);
         break;
      default:
         /*
          * plus one to avoid zero mem allocs
          */
         sqlda->sqlvar[i].sqldata = (char *)malloc(sqlda->sqlvar[i].sqllen+1);
         break;
      }
   }
   return sqlda;
}
static void INGfreeDescriptor(IISQLDA *sqlda)
{
   int i;
   if (!sqlda) {
      return;
   }
   for (i = 0; i < sqlda->sqld; ++i) {
      if (sqlda->sqlvar[i].sqldata) {
         free(sqlda->sqlvar[i].sqldata);
      }
      if (sqlda->sqlvar[i].sqlind) {
         free(sqlda->sqlvar[i].sqlind);
      }
   }
   free(sqlda);
   sqlda = NULL;
}
static inline int INGgetTypeSize(IISQLVAR *ingvar)
{
   int inglength = 0;
   /*
    * TODO: add date types (at least TSTMP,TSW TSWO)
    */
   switch (ingvar->sqltype) {
   case IISQ_DTE_TYPE:
      inglength = 25;
      break;
   case IISQ_MNY_TYPE:
      inglength = 8;
      break;
   default:
      inglength = ingvar->sqllen;
      break;
   }
   return inglength;
}
static inline INGresult *INGgetINGresult(IISQLDA *sqlda)
{
   int i;
   INGresult *result = NULL;
   if (!sqlda) {
      return NULL;
   }
   result = (INGresult *)malloc(sizeof(INGresult));
   memset(result, 0, sizeof(INGresult));
   result->sqlda = sqlda;
   result->num_fields = sqlda->sqld;
   result->num_rows = 0;
   result->first_row = NULL;
   result->status = ING_EMPTY_RESULT;
   result->act_row = NULL;
   memset(result->numrowstring, 0, sizeof(result->numrowstring));
   if (result->num_fields) {
      result->fields = (INGRES_FIELD *)malloc(sizeof(INGRES_FIELD) * result->num_fields);
      memset(result->fields, 0, sizeof(INGRES_FIELD) * result->num_fields);
      for (i = 0; i < result->num_fields; ++i) {
         memset(result->fields[i].name, 0, 34);
         bstrncpy(result->fields[i].name, sqlda->sqlvar[i].sqlname.sqlnamec, sqlda->sqlvar[i].sqlname.sqlnamel);
         result->fields[i].max_length = INGgetTypeSize(&sqlda->sqlvar[i]);
         result->fields[i].type = abs(sqlda->sqlvar[i].sqltype);
         result->fields[i].flags = (sqlda->sqlvar[i].sqltype < 0) ? 1 : 0;
      }
   }
   return result;
}
static inline void INGfreeRowSpace(ING_ROW *row, IISQLDA *sqlda)
{
   int i;
   if (row == NULL || sqlda == NULL) {
      return;
   }
   for (i = 0; i < sqlda->sqld; ++i) {
      if (row->sqlvar[i].sqldata) {
         free(row->sqlvar[i].sqldata);
      }
      if (row->sqlvar[i].sqlind) {
         free(row->sqlvar[i].sqlind);
      }
   }
   free(row->sqlvar);
   free(row);
}
static void INGfreeINGresult(INGresult *ing_res)
{
   int rows;
   ING_ROW *rowtemp;
   if (!ing_res) {
      return;
   }
   /*
    * Free all rows and fields, then res, not descriptor!
    *
    * Use of rows is a nasty workaround til I find the reason,
    * why aggregates like max() don't work
    */
   rows = ing_res->num_rows;
   ing_res->act_row = ing_res->first_row;
   while (ing_res->act_row != NULL && rows > 0) {
      rowtemp = ing_res->act_row->next;
      INGfreeRowSpace(ing_res->act_row, ing_res->sqlda);
      ing_res->act_row = rowtemp;
      --rows;
   }
   if (ing_res->fields) {
      free(ing_res->fields);
   }
   free(ing_res);
}
static inline ING_ROW *INGgetRowSpace(INGresult *ing_res)
{
   int i;
   unsigned short len; /* used for VARCHAR type length */
   IISQLDA *sqlda = ing_res->sqlda;
   ING_ROW *row = NULL;
   IISQLVAR *vars = NULL;
   row = (ING_ROW *)malloc(sizeof(ING_ROW));
   memset(row, 0, sizeof(ING_ROW));
   vars = (IISQLVAR *)malloc(sizeof(IISQLVAR) * sqlda->sqld);
   memset(vars, 0, sizeof(IISQLVAR) * sqlda->sqld);
   row->sqlvar = vars;
   row->next = NULL;
   for (i = 0; i < sqlda->sqld; ++i) {
      /*
       * Make strings out of the data, then the space and assign 
       * (why string? at least it seems that way, looking into the sources)
       */
      vars[i].sqlind = (short *)malloc(sizeof(short));
      if (sqlda->sqlvar[i].sqlind) {
         memcpy(vars[i].sqlind,sqlda->sqlvar[i].sqlind,sizeof(short));
      } else {
         *vars[i].sqlind = NULL;
      }
      /*
       * if sqlind pointer exists AND points to -1 -> column is 'null'
       */
      if ( *vars[i].sqlind && (*vars[i].sqlind == -1)) {
         vars[i].sqldata = NULL;
      } else {
         switch (ing_res->fields[i].type) {
         case IISQ_VCH_TYPE:
            len = ((ING_VARCHAR *)sqlda->sqlvar[i].sqldata)->len;
            vars[i].sqldata = (char *)malloc(len+1);
            memcpy(vars[i].sqldata,sqlda->sqlvar[i].sqldata+2,len);
            vars[i].sqldata[len] = '\0';
            break;
         case IISQ_CHA_TYPE:
            vars[i].sqldata = (char *)malloc(ing_res->fields[i].max_length+1);
            memcpy(vars[i].sqldata,sqlda->sqlvar[i].sqldata,sqlda->sqlvar[i].sqllen);
            vars[i].sqldata[ing_res->fields[i].max_length] = '\0';
            break;
         case IISQ_INT_TYPE:
            vars[i].sqldata = (char *)malloc(20);
            memset(vars[i].sqldata, 0, 20);
            switch (sqlda->sqlvar[i].sqllen) {
            case 2:
               bsnprintf(vars[i].sqldata, 20, "%d",*(short*)sqlda->sqlvar[i].sqldata);
               break;
            case 4:
               bsnprintf(vars[i].sqldata, 20, "%ld",*(int*)sqlda->sqlvar[i].sqldata);
               break;
            case 8:
               bsnprintf(vars[i].sqldata, 20, "%lld",*(long*)sqlda->sqlvar[i].sqldata);
               break;
            }
            break;
         case IISQ_TSTMP_TYPE:
            vars[i].sqldata = (char *)malloc(IISQ_TSTMP_LEN+1);
            vars[i].sqldata[IISQ_TSTMP_LEN] = '\0';
            break;
         case IISQ_TSWO_TYPE:
            vars[i].sqldata = (char *)malloc(IISQ_TSWO_LEN+1);
            vars[i].sqldata[IISQ_TSWO_LEN] = '\0';
            break;
         case IISQ_TSW_TYPE:
            vars[i].sqldata = (char *)malloc(IISQ_TSW_LEN+1);
            vars[i].sqldata[IISQ_TSW_LEN] = '\0';
            break;
         }
      }
   }
   return row;
}
static inline int INGfetchAll(const char *query, INGresult *ing_res)
{
   int linecount = 0;
   ING_ROW *row;
   IISQLDA *desc;
   int check = -1;
   desc = ing_res->sqlda;
/* # line 392 "myingres.sc" */	/* host code */
   if ((check = INGcheck()) < 0) {
      return check;
   }
/* # line 396 "myingres.sc" */	/* open */
  {
    IIsqInit(&sqlca);
    IIcsOpen((char *)"c2",4150,23546);
    IIwritio(0,(short *)0,1,32,0,(char *)"s2");
    IIcsQuery((char *)"c2",4150,23546);
  }
/* # line 397 "myingres.sc" */	/* host code */
   if ((check = INGcheck()) < 0) {
      return check;
   }
   /* for (linecount = 0; sqlca.sqlcode == 0; ++linecount) */
   do {
/* # line 403 "myingres.sc" */	/* fetch */
  {
    IIsqInit(&sqlca);
    if (IIcsRetScroll((char *)"c2",4150,23546,-1,-1) != 0) {
      IIcsDaGet(0,desc);
      IIcsERetrieve();
    } /* IIcsRetrieve */
  }
/* # line 405 "myingres.sc" */	/* host code */
      if ( (sqlca.sqlcode == 0) || (sqlca.sqlcode == -40202) ) {
         row = INGgetRowSpace(ing_res); /* alloc space for fetched row */
         /*
          * Initialize list when encountered first time
          */
         if (ing_res->first_row == 0) {
            ing_res->first_row = row; /* head of the list */
            ing_res->first_row->next = NULL;
            ing_res->act_row = ing_res->first_row;
         }      
         ing_res->act_row->next = row; /* append row to old act_row */
         ing_res->act_row = row; /* set row as act_row */
         row->row_number = linecount;
         ++linecount;
      }
   } while ( (sqlca.sqlcode == 0) || (sqlca.sqlcode == -40202) );
/* # line 423 "myingres.sc" */	/* close */
  {
    IIsqInit(&sqlca);
    IIcsClose((char *)"c2",4150,23546);
  }
/* # line 425 "myingres.sc" */	/* host code */
   ing_res->status = ING_COMMAND_OK;
   ing_res->num_rows = linecount;
   return linecount;
}
static inline ING_STATUS INGresultStatus(INGresult *res)
{
   if (res == NULL) {
      return ING_NO_RESULT;
   } else {
      return res->status;
   }
}
static void INGrowSeek(INGresult *res, int row_number)
{
   ING_ROW *trow = NULL;
   if (res->act_row->row_number == row_number) {
      return;
   }
   /*
    * TODO: real error handling
    */
   if (row_number<0 || row_number>res->num_rows) {
      return;
   }
   for (trow = res->first_row; trow->row_number != row_number; trow = trow->next) ;
   res->act_row = trow;
   /*
    * Note - can be null - if row_number not found, right?
    */
}
char *INGgetvalue(INGresult *res, int row_number, int column_number)
{
   if (row_number != res->act_row->row_number) {
      INGrowSeek(res, row_number);
   }
   return res->act_row->sqlvar[column_number].sqldata;
}
bool INGgetisnull(INGresult *res, int row_number, int column_number)
{
   if (row_number != res->act_row->row_number) {
      INGrowSeek(res, row_number);
   }
   return (*res->act_row->sqlvar[column_number].sqlind == -1) ? true : false;
}
int INGntuples(const INGresult *res)
{
   return res->num_rows;
}
int INGnfields(const INGresult *res)
{
   return res->num_fields;
}
char *INGfname(const INGresult *res, int column_number)
{
   if ((column_number > res->num_fields) || (column_number < 0)) {
      return NULL;
   } else {
      return res->fields[column_number].name;
   }
}
short INGftype(const INGresult *res, int column_number)
{
   return res->fields[column_number].type;
}
int INGexec(B_DB *mdb, INGconn *conn, const char *query)
{
   bool stmt_free = false;
   int check;
/* # line 507 "myingres.sc" */	
  
  int rowcount;
  char *stmt;
/* # line 510 "myingres.sc" */	
  
   /*
    * See if we need to run this through the limit_filter.
    */
   if (strstr(query, "LIMIT") != NULL) {
      stmt = mdb->limit_filter->replace(query);
   } else {
      stmt = bstrdup(query);
      stmt_free = true;
   }
   rowcount = -1;
/* # line 523 "myingres.sc" */	/* execute */
  {
    IIsqInit(&sqlca);
    IIsqExImmed(stmt);
    IIsyncup((char *)0,0);
  }
/* # line 525 "myingres.sc" */	/* host code */
   if (stmt_free) {
      free(stmt);
   }
   if ((check = INGcheck()) < 0) {
      return check;
   }
/* # line 533 "myingres.sc" */	/* inquire_ingres */
  {
    IILQisInqSqlio((short *)0,1,30,sizeof(rowcount),&rowcount,8);
  }
/* # line 534 "myingres.sc" */	/* host code */
   if ((check = INGcheck()) < 0) {
      return check;
   }
   return rowcount;
}
INGresult *INGquery(B_DB *mdb, INGconn *conn, const char *query)
{
   /*
    * TODO: error handling
    */
   IISQLDA *desc = NULL;
   INGresult *res = NULL;
   int rows = -1;
   int cols = INGgetCols(mdb, query);
   desc = INGgetDescriptor(mdb, cols, query);
   if (!desc) {
      return NULL;
   }

   res = INGgetINGresult(desc);
   if (!res) {
      return NULL;
   }

   rows = INGfetchAll(query, res);

   if (rows < 0) {
     INGfreeDescriptor(desc);
     INGfreeINGresult(res);
     return NULL;
   }

   return res;
}
void INGclear(INGresult *res)
{
   if (res == NULL) {
      return;
   }
   INGfreeDescriptor(res->sqlda);
   INGfreeINGresult(res);
}
INGconn *INGconnectDB(char *dbname, char *user, char *passwd)
{
   INGconn *dbconn;
   if (dbname == NULL || strlen(dbname) == 0) {
      return NULL;
   }
   dbconn = (INGconn *)malloc(sizeof(INGconn));
   memset(dbconn, 0, sizeof(INGconn));
/* # line 590 "myingres.sc" */	
  
  char ingdbname[24];
  char ingdbuser[32];
  char ingdbpasw[32];
  char conn_name[32];
  int sess_id;
/* # line 596 "myingres.sc" */	
  
   bstrncpy(ingdbname, dbname, sizeof(ingdbname));
   if (user != NULL) {
      bstrncpy(ingdbuser, user, sizeof(ingdbuser));
      if (passwd != NULL) {
         bstrncpy(ingdbpasw, passwd, sizeof(ingdbpasw));
      } else {
         memset(ingdbpasw, 0, sizeof(ingdbpasw));
      }
/* # line 607 "myingres.sc" */	/* connect */
  {
    IIsqInit(&sqlca);
    IIsqUser(ingdbuser);
    IIsqConnect(0,ingdbname,(char *)"-dbms_password",ingdbpasw,(char *)0, 
    (char *)0, (char *)0, (char *)0, (char *)0, (char *)0, (char *)0, 
    (char *)0, (char *)0, (char *)0, (char *)0);
  }
/* # line 611 "myingres.sc" */	/* host code */
   } else {
/* # line 612 "myingres.sc" */	/* connect */
  {
    IIsqInit(&sqlca);
    IIsqConnect(0,ingdbname,(char *)0, (char *)0, (char *)0, (char *)0, 
    (char *)0, (char *)0, (char *)0, (char *)0, (char *)0, (char *)0, 
    (char *)0, (char *)0, (char *)0);
  }
/* # line 613 "myingres.sc" */	/* host code */
   }   
/* # line 615 "myingres.sc" */	/* inquire_sql */
  {
    IILQisInqSqlio((short *)0,1,32,31,conn_name,13);
  }
/* # line 616 "myingres.sc" */	/* inquire_sql */
  {
    IILQisInqSqlio((short *)0,1,30,sizeof(sess_id),&sess_id,11);
  }
/* # line 618 "myingres.sc" */	/* host code */
   bstrncpy(dbconn->dbname, ingdbname, sizeof(dbconn->dbname));
   bstrncpy(dbconn->user, ingdbuser, sizeof(dbconn->user));
   bstrncpy(dbconn->password, ingdbpasw, sizeof(dbconn->password));
   bstrncpy(dbconn->connection_name, conn_name, sizeof(dbconn->connection_name));
   dbconn->session_id = sess_id;
   dbconn->msg = (char*)malloc(257);
   memset(dbconn->msg, 0, 257);
   return dbconn;
}
void INGdisconnectDB(INGconn *dbconn)
{
   /*
    * TODO: check for any real use of dbconn: maybe whenn multithreaded?
    */
/* # line 634 "myingres.sc" */	/* disconnect */
  {
    IIsqInit(&sqlca);
    IIsqDisconnect();
  }
/* # line 635 "myingres.sc" */	/* host code */
   if (dbconn != NULL) {
      free(dbconn->msg);
      free(dbconn);
   }
}
char *INGerrorMessage(const INGconn *conn)
{
/* # line 643 "myingres.sc" */	
  
  char errbuf[256];
/* # line 645 "myingres.sc" */	
  
/* # line 647 "myingres.sc" */	/* inquire_ingres */
  {
    IILQisInqSqlio((short *)0,1,32,255,errbuf,63);
  }
/* # line 648 "myingres.sc" */	/* host code */
   memcpy(conn->msg, &errbuf, 256);
   return conn->msg;
}
char *INGcmdTuples(INGresult *res)
{
   return res->numrowstring;
}
/* TODO?
int INGputCopyEnd(INGconn *conn, const char *errormsg);
int INGputCopyData(INGconn *conn, const char *buffer, int nbytes);
*/
/* # line 662 "myingres.sc" */	
#endif
