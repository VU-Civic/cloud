#ifndef __CIVICALERT_DB_HEADER_H__
#define __CIVICALERT_DB_HEADER_H__

#include "PostgreSQL.h"

class CivicAlertDB final
{
public:

  // Constructor/destructor
  CivicAlertDB(const char* host_ip, const char* host_port, const char* host_db_name, const char* username, const char* password);
  ~CivicAlertDB();

  // Connection management
  bool connect();
  void disconnect();

  // CivicAlert-specific database operations
  bool storeEvidenceClipLocation(const char* event_id, const char* clip_url);

private:

  // Member variables
  PostgreSQL db;
};

#endif  // #ifndef __CIVICALERT_DB_HEADER_H__
