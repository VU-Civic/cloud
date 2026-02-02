#ifndef __POSTGRESQL_HEADER_H__
#define __POSTGRESQL_HEADER_H__

#include <string>
#include <libpq-fe.h>

class PostgreSQL final
{
public:

  // Constructor/destructor
  PostgreSQL(const char* __restrict hostIp, const char* __restrict hostPort, const char* __restrict hostDbName, const char* __restrict username, const char* __restrict password);
  PostgreSQL(const PostgreSQL&) = delete;
  PostgreSQL& operator=(const PostgreSQL&) = delete;
  ~PostgreSQL();

  // Connection management
  bool connect();
  void disconnect();
  bool isConnected() const;

  // Typical database operations
  bool executeQuery(const char* __restrict query);
  bool executeQueryWithResponse(const char* __restrict query, PGresult** __restrict result);

private:

  // Member variables
  const std::string dbIp, dbPort, dbName, dbUser, dbPassword;
  PGconn* connection;
};

#endif  // #ifndef __POSTGRESQL_HEADER_H__
