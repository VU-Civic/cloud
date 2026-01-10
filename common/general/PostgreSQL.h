#ifndef __POSTGRESQL_HEADER_H__
#define __POSTGRESQL_HEADER_H__

#include <string>
#include <libpq-fe.h>

class PostgreSQL final
{
public:

  // Constructor/destructor
  PostgreSQL(const char* hostIp, const char* hostPort, const char* hostDbName, const char* username, const char* password);
  ~PostgreSQL();

  // Connection management
  bool connect();
  void disconnect();
  bool isConnected();

  // Typical database operations
  bool executeQuery(const char* __restrict query);
  bool executeQueryWithResponse(const char* __restrict query, PGresult** __restrict result);

private:

  // Member variables
  std::string dbIp, dbPort, dbName, dbUser, dbPassword;
  PGconn* connection;
};

#endif  // #ifndef __POSTGRESQL_HEADER_H__
