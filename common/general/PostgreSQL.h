#ifndef __POSTGRESQL_HEADER_H__
#define __POSTGRESQL_HEADER_H__

#include <libpq-fe.h>

class PostgreSQL final
{
public:

  // Constructor/destructor
  PostgreSQL(const char* host_ip, const char* host_port, const char* host_db_name, const char* username, const char* password);
  ~PostgreSQL();

  // Connection management
  bool connect();
  void disconnect();
  bool isConnected();

  // Typical database operations
  bool executeQuery(const char* query);
  bool executeQueryWithResponse(const char* query, PGresult** result);

private:

  // Member variables
  const char *db_ip, *db_port, *db_name, *db_user, *db_password;
  PGconn* connection;
};

#endif  // #ifndef __POSTGRESQL_HEADER_H__
