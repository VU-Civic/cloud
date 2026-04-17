#include <curl/curl.h>
#include "AwsServices.h"
#include "GeoMath.h"
#include "StorageDatabase.h"

StorageDatabase::StorageDatabase(void) : curl(nullptr), apiEndpointUrl(), apiEndpointToken(), civicAlertDatabase(nullptr)
{
  // Initialize the cURL library and set default options for all future cURL requests
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "CivicAlertFusion/1.0");
  curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "deflate");

  // Establish a connection to the CivicAlert database
  connectToDatabase();
}

StorageDatabase::~StorageDatabase(void)
{
  // Clean up the database connection
  if (civicAlertDatabase)
  {
    civicAlertDatabase->disconnect();
    civicAlertDatabase.reset();
  }

  // Clean up the cURL library
  if (curl)
  {
    curl_easy_cleanup(curl);
    curl = nullptr;
  }
}

size_t StorageDatabase::writeCurlDataString(void* downloadedData, size_t wordSize, size_t numWords, void* outputStringPtr)
{
  const auto outputSize = wordSize * numWords;
  static_cast<std::string*>(outputStringPtr)->append(static_cast<char*>(downloadedData), outputSize);
  return outputSize;
}

void StorageDatabase::connectToDatabase(void)
{
  // Retrieve the database endpoint, username, and password from AWS secrets
  apiEndpointUrl = AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_API_ENDPOINT_URL);
  const auto apiEndpointTokenKey = AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_API_ENDPOINT_TOKEN_KEY);
  const auto apiEndpointTokenSecret = AwsServices::getSecret(apiEndpointTokenKey.c_str());
  apiEndpointToken = AwsServices::extractSecretValue(apiEndpointTokenSecret, "token");
  const auto dbEndpoint = AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_DB_ENDPOINT);
  const auto dbSecretsKey = AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_DB_SECRETS_KEY);
  const auto dbPort = AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_DB_PORT);
  const auto dbSecrets = AwsServices::getSecret(dbSecretsKey.c_str());
  const auto dbUsername = AwsServices::extractSecretValue(dbSecrets, "username");
  const auto dbPassword = AwsServices::extractSecretValue(dbSecrets, "password");
  if (dbEndpoint.empty() || dbPort.empty() || dbSecretsKey.empty() || dbUsername.empty() || dbPassword.empty() || apiEndpointUrl.empty() || apiEndpointToken.empty())
  {
    logger.log(Logger::ERROR, "Database/API endpoint, port, or secrets parameter is empty...restarting process!\n");
    exit(EXIT_FAILURE);
  }

  // Create the database client and attempt to connect
  civicAlertDatabase = std::make_unique<PostgreSQL>(dbEndpoint.c_str(), dbPort.c_str(), CivicAlert::DATABASE_NAME, dbUsername.c_str(), dbPassword.c_str());
  if (!civicAlertDatabase->connect())
  {
    logger.log(Logger::ERROR, "Failed to connect to the CivicAlert database...restarting process!\n");
    exit(EXIT_FAILURE);
  }
  civicAlertDatabase->executeQuery("SET search_path TO public;");
}

void StorageDatabase::updateDeviceInfo(const DeviceInfoMessage* __restrict packet)
{
  // Construct and execute the SQL stored procedure to update the device info record for the specified device
  char fwVersion[FIRMWARE_VERSION_LENGTH + 1] = {0}, aiFwVersion[FIRMWARE_VERSION_LENGTH + 1] = {0};
  memcpy(fwVersion, packet->firmwareVersion, FIRMWARE_VERSION_LENGTH);
  memcpy(aiFwVersion, packet->aiFirmwareVersion, FIRMWARE_VERSION_LENGTH);
  const auto [qw, qx, qy, qz] = GeoMath::calculateFullQuaternion(packet->q1, packet->q2, packet->q3);
  const auto updateQuery =
      std::string("CALL update_device_info(") + std::to_string(packet->deviceID) + "::int8, '" + std::string(packet->imsi) + "'::char(15), '" + std::string(fwVersion) +
      "'::char(8), '" + std::string(aiFwVersion) + "'::char(8), " + std::to_string(packet->lat) + "::float4, " + std::to_string(packet->lon) + "::float4, " +
      std::to_string(packet->ht) + "::float4, " + std::to_string(qx) + "::float4, " + std::to_string(qy) + "::float4, " + std::to_string(qz) + "::float4, " + std::to_string(qw) +
      "::float4, " + std::to_string(packet->signalPower) + "::int2, " + std::to_string(packet->signalQuality) + "::int2, " + std::to_string(packet->channelAlarms.alarm.ch1 != 0) +
      "::bool, " + std::to_string(packet->channelAlarms.alarm.ch2 != 0) + "::bool, " + std::to_string(packet->channelAlarms.alarm.ch3 != 0) + "::bool, " +
      std::to_string(packet->channelAlarms.alarm.ch4 != 0) + "::bool, " + std::to_string(packet->deviceConfig.mqttDeviceInfoQos) + "::int2, " +
      std::to_string(packet->deviceConfig.mqttAlertQos) + "::int2, " + std::to_string(packet->deviceConfig.mqttAudioQos) + "::int2, " +
      std::to_string(packet->deviceConfig.shotDetectionMinThreshold) + "::int2, " + std::to_string(packet->deviceConfig.shotDetectionGoodThreshold) + "::int2, " +
      std::to_string(packet->deviceConfig.storageClassificationThreshold) + "::int2, " + std::to_string(packet->deviceConfig.audioClipLengthSeconds) + "::int2, " +
      std::to_string(packet->deviceConfig.deviceStatusTransmissionIntervalMinutes) + "::int2, " + std::to_string(packet->deviceConfig.badAudioRestartAttempted != 0) + "::bool, " +
      std::to_string(packet->deviceConfig.badAiRestartAttempted != 0) + "::bool);";
  while (!civicAlertDatabase->executeQuery(updateQuery.c_str()) && !civicAlertDatabase->isConnected())
  {
    // Attempt to reestablish a lost database connection
    logger.log(Logger::ERROR, "Failed to update device info database record for Device #%llu\n", packet->deviceID);
    connectToDatabase();
  }
}

void StorageDatabase::updatePartialDeviceInfo(const AlertMessage* __restrict packet)
{
  // Construct and execute the SQL stored procedure to partially update the device info record for the specified device
  const auto [qw, qx, qy, qz] = GeoMath::calculateFullQuaternion(packet->sensorQ1, packet->sensorQ2, packet->sensorQ3);
  const auto updateQuery = std::string("CALL update_partial_device_info(") + std::to_string(packet->deviceID) + "::int8, " + std::to_string(packet->sensorLat) + "::float4, " +
                           std::to_string(packet->sensorLon) + "::float4, " + std::to_string(packet->sensorHt) + "::float4, " + std::to_string(qx) + "::float4, " +
                           std::to_string(qy) + "::float4, " + std::to_string(qz) + "::float4, " + std::to_string(qw) + "::float4, " + std::to_string(packet->cellSignalPower) +
                           "::int2, " + std::to_string(packet->cellSignalQuality) + "::int2);";
  while (!civicAlertDatabase->executeQuery(updateQuery.c_str()) && !civicAlertDatabase->isConnected())
  {
    // Attempt to reestablish a lost database connection
    logger.log(Logger::ERROR, "Failed to update device info database record for Device #%llu\n", packet->deviceID);
    connectToDatabase();
  }
}

uint64_t StorageDatabase::addGunshotReport(const GunshotReport* __restrict report)
{
  // Construct and execute the SQL stored procedure to add a new gunshot report record for the specified report and return the generated report ID
  uint64_t reportID = 0;
  PGresult* queryResult = nullptr;
  const auto updateQuery = std::string("CALL insert_raw_event(") + std::to_string(report->deviceID) + std::string("::int8, ") + std::to_string(report->audioLocatorID) +
                           std::string("::int2, ") + std::to_string(report->lat) + std::string("::float4, ") + std::to_string(report->lon) + std::string("::float4, ") +
                           std::to_string(report->sourceAzimuth) + std::string("::float4, ") + std::to_string(report->sourceElevation) + std::string("::float4, ") +
                           std::to_string(report->timestamp) + std::string("::float8, ") + std::to_string(report->confidence) + std::string("::float4, ") +
                           std::to_string(report->orientationQX) + std::string("::float4, ") + std::to_string(report->orientationQY) + std::string("::float4, ") +
                           std::to_string(report->orientationQZ) + std::string("::float4, ") + std::to_string(report->orientationQW) + std::string("::float4, 0);");
  while (!civicAlertDatabase->executeQueryWithResponse(updateQuery.c_str(), &queryResult) && !civicAlertDatabase->isConnected())
  {
    // Attempt to reestablish a lost database connection
    logger.log(Logger::ERROR, "Failed to add gunshot report database record for Device #%llu\n", report->deviceID);
    connectToDatabase();
  }
  if (PQntuples(queryResult) > 0) reportID = strtoull(PQgetvalue(queryResult, 0, 0), nullptr, 10);
  PQclear(queryResult);
  return reportID;
}

void StorageDatabase::addIncidentReport(const IncidentMessage* __restrict message)
{
  // Generate the JSON body for the API request based on the incident message data
  std::string rawEventIds;
  for (const auto& report : message->contributingReports) rawEventIds += std::to_string(report->reportID) + ",";
  if (!rawEventIds.empty()) rawEventIds.pop_back();  // Remove trailing comma
  const auto json_body = std::string("{\"latitude\": ") + std::to_string(message->lat) + std::string(", \"longitude\": ") + std::to_string(message->lon) +
                         std::string(", \"elevation\": ") + std::to_string(message->ht) + std::string(", \"confidence\": ") + std::to_string(message->confidence) +
                         std::string(", \"raw_events\": [") + rawEventIds + std::string("], \"notify\": false}");

  // Call the API endpoint to store an incident report and trigger associated workflows
  std::string output;
  auto headers = curl_slist_append(nullptr, "Content-Type: application/json");
  headers = curl_slist_append(headers, (std::string("Authorization: Token ") + apiEndpointToken).c_str());
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &StorageDatabase::writeCurlDataString);
  curl_easy_setopt(curl, CURLOPT_URL, (apiEndpointUrl + CivicAlert::API_FUSED_EVENT_ENDPOINT).c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_body.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
  if (curl_easy_perform(curl) != CURLE_OK)
  {
    // Update the API endpoint URL and bearer token in case they changed and retry
    curl_slist_free_all(headers);
    apiEndpointUrl = AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_API_ENDPOINT_URL);
    const auto apiEndpointTokenKey = AwsServices::getSecretParameter(CivicAlert::AWS_PARAMETER_KEY_API_ENDPOINT_TOKEN_KEY);
    const auto apiEndpointTokenSecret = AwsServices::getSecret(apiEndpointTokenKey.c_str());
    apiEndpointToken = AwsServices::extractSecretValue(apiEndpointTokenSecret, "token");
    headers = curl_slist_append(nullptr, "Content-Type: application/json");
    headers = curl_slist_append(headers, (std::string("Authorization: Token ") + apiEndpointToken).c_str());
    curl_easy_setopt(curl, CURLOPT_URL, (apiEndpointUrl + CivicAlert::API_FUSED_EVENT_ENDPOINT).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    if (curl_easy_perform(curl) != CURLE_OK)
      logger.log(Logger::ERROR, "Failed to send fused event data to API endpoint for incident at (%.6f, %.6f)\n", message->lat, message->lon);
  }
  curl_slist_free_all(headers);
}
