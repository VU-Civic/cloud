#include "CivicAlertDB.h"

#include "Common.h"

CivicAlertDB::CivicAlertDB(const char* host_ip, const char* host_port, const char* host_db_name, const char* username, const char* password)
    : db(host_ip, host_port, host_db_name, username, password)
{
}
CivicAlertDB::~CivicAlertDB() {}

bool CivicAlertDB::connect() { return db.connect(); }
void CivicAlertDB::disconnect() { db.disconnect(); }

bool CivicAlertDB::storeEvidenceClipLocation(const char* event_id, const char* clip_url)
{
  // Prepare and execute the SQL query
  static char query[256] = {0};
  snprintf(query, sizeof(query), "UPDATE %s SET %s = %s WHERE %s = %s;", CivicAlert::EVIDENCE_DATABASE_NAME, CivicAlert::EVIDENCE_DATABASE_CLIP_URL_KEY, clip_url,
           CivicAlert::EVIDENCE_DATABASE_EVENT_ID_KEY, event_id);
  return db.executeQuery(query);
}
