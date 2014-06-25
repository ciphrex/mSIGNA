///////////////////////////////////////////////////////////////////////////////
//
// Database.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.
//
// Taken from odb examples.
//
// Create concrete database instance based on the DATABASE_* macros.
//

#pragma once

#include <string>
#include <memory>   // std::unique_ptr
#include <cstdlib>  // std::exit
#include <iostream>

#include <odb/database.hxx>

#if defined(DATABASE_MYSQL)
#  include <odb/mysql/database.hxx>
#elif defined(DATABASE_SQLITE)
#  include <odb/connection.hxx>
#  include <odb/transaction.hxx>
#  include <odb/schema-catalog.hxx>
#  include <odb/sqlite/database.hxx>
#elif defined(DATABASE_PGSQL)
#  include <odb/pgsql/database.hxx>
#elif defined(DATABASE_ORACLE)
#  include <odb/oracle/database.hxx>
#elif defined(DATABASE_MSSQL)
#  include <odb/mssql/database.hxx>
#else
#  error unknown database; did you forget to define the DATABASE_* macros?
#endif

namespace CoinDB
{

inline std::unique_ptr<odb::database>
open_database (int& argc, char* argv[], bool create = false)
{
  using namespace std;
  using namespace odb::core;

  if (argc > 1 && argv[1] == string ("--help"))
  {
    cout << "Usage: " << argv[0] << " [options]" << endl
         << "Options:" << endl;

#if defined(DATABASE_MYSQL)
    odb::mysql::database::print_usage (cout);
#elif defined(DATABASE_SQLITE)
    odb::sqlite::database::print_usage (cout);
#elif defined(DATABASE_PGSQL)
    odb::pgsql::database::print_usage (cout);
#elif defined(DATABASE_ORACLE)
    odb::oracle::database::print_usage (cout);
#elif defined(DATABASE_MSSQL)
    odb::mssql::database::print_usage (cout);
#endif

    exit (0);
  }

#if defined(DATABASE_MYSQL)
  unique_ptr<database> db (new odb::mysql::database (argc, argv));
#elif defined(DATABASE_SQLITE)
  unique_ptr<database> db (
    new odb::sqlite::database (
      argc, argv, false, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE));

  // Create the database schema. Due to bugs in SQLite foreign key
  // support for DDL statements, we need to temporarily disable
  // foreign keys.
  //
    if (create) {
        connection_ptr c (db->connection ());

        c->execute ("PRAGMA foreign_keys=OFF");

        transaction t (c->begin ());
        schema_catalog::create_schema (*db);
        t.commit ();

        c->execute ("PRAGMA foreign_keys=ON");
    }
#elif defined(DATABASE_PGSQL)
  unique_ptr<database> db (new odb::pgsql::database (argc, argv));
#elif defined(DATABASE_ORACLE)
  unique_ptr<database> db (new odb::oracle::database (argc, argv));
#elif defined(DATABASE_MSSQL)
  unique_ptr<database> db (new odb::mssql::database (argc, argv));
#endif

  return db;
}

#if defined(DATABASE_MYSQL)
inline std::unique_ptr<odb::database>
openDatabase(const std::string& user, const std::string& passwd, const std::string& dbname)
{
    std::unique_ptr<odb::database> db(new odb::mysql::database(user, passwd, dbname));
    return db;
}
#elif defined(DATABASE_SQLITE)
inline std::unique_ptr<odb::database>
openDatabase(const std::string& filename, bool create = false)
{
    using namespace odb::core;

    int flags = SQLITE_OPEN_READWRITE;
    if (create) flags |= SQLITE_OPEN_CREATE;
    std::unique_ptr<database> db(new odb::sqlite::database(filename, flags, false));

  // Create the database schema. Due to bugs in SQLite foreign key
  // support for DDL statements, we need to temporarily disable
  // foreign keys.
  //
    if (create) {
        connection_ptr c(db->connection ());

        c->execute ("PRAGMA foreign_keys=OFF");

        transaction t (c->begin ());
        schema_catalog::create_schema (*db);
        t.commit ();

        c->execute ("PRAGMA foreign_keys=ON");
    }

    return db;
}
#endif

}
