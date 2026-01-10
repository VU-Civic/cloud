#include "Logger.h"
#include "PostgreSQL.h"

extern Logger logger;

PostgreSQL::PostgreSQL(const char* hostIp, const char* hostPort, const char* hostDbName, const char* username, const char* password)
    : dbIp(hostIp), dbPort(hostPort), dbName(hostDbName), dbUser(username), dbPassword(password), connection(nullptr)
{
}

PostgreSQL::~PostgreSQL() { disconnect(); }

bool PostgreSQL::connect()
{
  // Attempt to establish a database connection
  logger.log(Logger::INFO, "Connecting to PostgreSQL database at %s:%s/%s as user %s\n", dbIp.c_str(), dbPort.c_str(), dbName.c_str(), dbUser.c_str());
  connection = PQsetdbLogin(dbIp.c_str(), dbPort.c_str(), nullptr, nullptr, dbName.c_str(), dbUser.c_str(), dbPassword.c_str());
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
    logger.log(Logger::INFO, "Disconnecting from PostgreSQL database at %s:%s/%s\n", dbIp.c_str(), dbPort.c_str(), dbName.c_str());
    PQfinish(connection);
    connection = nullptr;
  }
}

bool PostgreSQL::isConnected() { return connection && (PQstatus(connection) == CONNECTION_OK); }

bool PostgreSQL::executeQuery(const char* __restrict query)
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

bool PostgreSQL::executeQueryWithResponse(const char* __restrict query, PGresult** __restrict result)
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
