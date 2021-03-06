/***************************************************************************
                          qtdatabase.cpp  -  description
                             -------------------
    begin                : Fri May 28 2010
    http://pvbrowser.org
 ***************************************************************************/

#include "qtdatabase.h"
qtDatabase::qtDatabase()
{
  db = NULL;
  result = new QSqlQuery();
  error  = new QSqlError();
}

qtDatabase::~qtDatabase()
{
  if(db != NULL)
  {
    close();
  }
  delete result;
  delete error;
}

int qtDatabase::open(const char *dbtype, const char *hostname, const char *dbname, const char *user, const char *pass)
{
  if(db != NULL) return -1;
  db = new QSqlDatabase;

  *db = QSqlDatabase::addDatabase(dbtype);
  db->setHostName(hostname);
  db->setDatabaseName(dbname);
  db->setUserName(user);
  db->setPassword(pass);
//  db->setPort(3306);
  db->setConnectOptions("MYSQL_OPT_RECONNECT=TRUE");
  if(db->open())
  {
    return 0;
  }
  else
  {
    delete db;
    db = NULL;
    return -1;
  }
}

int qtDatabase::close()
{
  if(db == NULL) 
  {
    return -1;
  }  
  db->close();
  delete db;
  db = NULL;
  return 0;
}

int qtDatabase::query(PARAM *p, const char *sqlcommand)
{
  if(db == NULL) return -1;
  QString qsqlcommand = QString::fromUtf8(sqlcommand);
  *result = db->exec(qsqlcommand);
  *error = db->lastError();
  if(error->isValid())
  {
    QString e = error->databaseText();
    printf("qtDatabase::query ERROR: %s\n", (const char *) e.toUtf8());
    pvStatusMessage(p,255,0,0,"ERROR: qtDatabase::query(%s) %s", sqlcommand, (const char *) e.toUtf8());
    return -1;
  }
  return 0;
}

int qtDatabase::populateTable(PARAM *p, int id)
{
  int x,y,xmax,ymax;
  
  if(db == NULL)
  {
    pvStatusMessage(p,255,0,0,"ERROR: qtDatabase::populateTable() db==NULL");
    return -1;
  }  

  // set table dimension
  xmax = result->record().count();
  //
  // Using SQLITE a user from our forum found an issue
  // getting ymax.
  // With SQLITE numRowsAffected() does not return the correct value.
  // Other database systems do.
  //
  if(db->driverName() == "QSQLITE")
  {
    result->last();
    ymax = result->at()+1;
    result->first();
    //printf("SQLITE ymax = %d \n",ymax);
  }
  else
  {
    ymax = result->numRowsAffected();
    //printf("no SQLITE, ymax = %d \n",ymax);
  }
  pvSetNumRows(p,id,ymax);
  pvSetNumCols(p,id,xmax);

  // populate table
  QSqlRecord record = result->record();
  if(record.isEmpty())
  {
    pvStatusMessage(p,255,0,0,"ERROR: qtDatabase::populateTable() record is empty");
    return -1;
  }

  for(x=0; x<xmax; x++)
  { // write name of fields
    pvSetTableText(p, id, x, -1, (const char *) record.fieldName(x).toUtf8());
  }
  result->next();
  for(y=0; y<ymax; y++)
  { // write fields
    QSqlRecord record = result->record();
    for(x=0; x<xmax; x++)
    {
      QSqlField f = record.field(x);
      if(f.isValid())
      {
        QVariant v = f.value();
        pvSetTableText(p, id, x, y, (const char *) v.toString().toUtf8());
      }
      else
      {
        pvSetTableText(p, id, x, y, "ERROR:");
      }
    }
    result->next();
  }

  return 0;
}

const char *qtDatabase::recordFieldValue(PARAM *p, int x)
{
  QSqlRecord record = result->record();
  if(record.isEmpty())
  {
    pvStatusMessage(p,255,0,0,"ERROR: qtDatabase::recordFieldValue(%d) record is empty", x);
    return "ERROR:";
  }
  QSqlField f = record.field(x);
  if(f.isValid())
  {
    QVariant v = f.value();
    return v.toString().toUtf8();
  }
  else
  {
    pvStatusMessage(p,255,0,0,"ERROR: qtDatabase::recordFieldValue(%d) field is invalid", x);
    return "ERROR:";
  }
}

int qtDatabase::nextRecord()
{
  result->next();
  QSqlRecord record = result->record();
  if(record.isEmpty()) return -1;
  return 0;
}
