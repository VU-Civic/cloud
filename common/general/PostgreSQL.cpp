#include "Logger.h"
#include "PostgreSQL.h"

extern Logger logger;

PostgreSQL::PostgreSQL(const char* host_ip, const char* host_port, const char* host_db_name, const char* username, const char* password)
    : db_ip(host_ip), db_port(host_port), db_name(host_db_name), db_user(username), db_password(password), connection(nullptr)
{
}

PostgreSQL::~PostgreSQL() { disconnect(); }

bool PostgreSQL::connect()
{
  // Attempt to establish a database connection
  logger.log(Logger::INFO, "Connecting to PostgreSQL database at %s:%s/%s as user %s\n", db_ip, db_port, db_name, db_user);
  connection = PQsetdbLogin(db_ip, db_port, nullptr, nullptr, db_name, db_user, db_password);
  if (PQstatus(connection) != CONNECTION_OK)
  {
    logger.log(Logger::ERROR, "PostgreSQL connection failed: %s\n", PQerrorMessage(connection));
    PQfinish(connection);
    connection = nullptr;
    return false;
  }

  // Set the always-secure search path to protect against malicious users
  PQclear(PQexec(connection, "SELECT pg_catalog.set_config('search_path', '', false)"));
  logger.log(Logger::INFO, "PostgreSQL connection established successfully\n");
  return true;
}

void PostgreSQL::disconnect()
{
  // Disconnect if a connection exists
  if (connection)
  {
    logger.log(Logger::INFO, "Disconnecting from PostgreSQL database at %s:%s/%s\n", db_ip, db_port, db_name);
    PQfinish(connection);
    connection = nullptr;
  }
}

bool PostgreSQL::isConnected() { return connection && (PQstatus(connection) == CONNECTION_OK); }

bool PostgreSQL::executeQuery(const char* query)
{
  // Ensure there is an active connection
  if (!connection)
  {
    logger.log(Logger::ERROR, "Cannot execute query: No active PostgreSQL connection\n");
    return false;
  }

  // Execute the provided SQL query
  PGresult* result = PQexec(connection, query);
  if (PQresultStatus(result) != PGRES_COMMAND_OK)
  {
    logger.log(Logger::ERROR, "Query execution failed: %s\n", PQerrorMessage(connection));
    PQclear(result);
    return false;
  }

  // Clean up and return success
  PQclear(result);
  return true;
}

bool PostgreSQL::executeQueryWithResponse(const char* query, PGresult** result)
{
  // Ensure there is an active connection
  if (!connection)
  {
    logger.log(Logger::ERROR, "Cannot execute query: No active PostgreSQL connection\n");
    return false;
  }

  // Execute the provided SQL query
  *result = PQexec(connection, query);
  if (PQresultStatus(*result) != PGRES_TUPLES_OK)
  {
    logger.log(Logger::ERROR, "Query execution failed: %s\n", PQerrorMessage(connection));
    PQclear(*result);
    *result = nullptr;
    return false;
  }
  return true;
}
