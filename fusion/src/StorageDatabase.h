#ifndef __STORAGE_DATABASE_HEADER_H__
#define __STORAGE_DATABASE_HEADER_H__

#include <memory>
#include "Common.h"
#include "PostgreSQL.h"

class StorageDatabase final
{
public:

  // Constructor/destructor
  StorageDatabase(void);
  ~StorageDatabase(void);

  // Database operations
  void updateDeviceInfo(const DeviceInfoMessage* __restrict packet);
  void updatePartialDeviceInfo(const AlertMessage* __restrict packet);
  uint64_t addGunshotReport(const GunshotReport* __restrict report);
  void addIncidentReport(const IncidentMessage* __restrict message);

private:

  // Database connection management
  void connectToDatabase(void);

  // Helper function for storing cURL response data in a string
  static size_t writeCurlDataString(void* downloadedData, size_t wordSize, size_t numWords, void* outputStringPtr);

  // Private member variables
  void* curl;
  std::string apiEndpointUrl, apiEndpointToken;
  std::unique_ptr<PostgreSQL> civicAlertDatabase;
};

#endif  // #ifndef __STORAGE_DATABASE_HEADER_H__
