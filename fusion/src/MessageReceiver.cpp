#include <algorithm>
#include "AwsServices.h"
#include "Common.h"
#include "FusionAlgorithm.h"
#include "GeoMath.h"
#include "MessageReceiver.h"

std::mutex MessageReceiver::bufferMutex;
std::atomic_bool MessageReceiver::isRunning(false);
std::atomic_uint32_t MessageReceiver::numProcessingThreads(0);
std::condition_variable MessageReceiver::terminationCondition;
std::unordered_map<uint64_t, std::deque<ReportPacket>> MessageReceiver::processingBuffer;
std::unique_ptr<StorageDatabase> MessageReceiver::database(nullptr);
std::string MessageReceiver::deviceInfoTopic, MessageReceiver::alertsTopic;
std::thread MessageReceiver::receiveThread;

void MessageReceiver::listenForMessages(const char* mqttDeviceInfoTopic, const char* mqttAlertsTopic)
{
  // Store the MQTT topic names for use in the reception thread
  alertsTopic = mqttAlertsTopic;
  deviceInfoTopic = mqttDeviceInfoTopic;

  // Initialize a connection to the storage database
  logger.log(Logger::INFO, "Initializing connection to the CivicAlert database...\n");
  database = std::make_unique<StorageDatabase>();

  // Start thread to listen for incoming device info and alert messages
  logger.log(Logger::INFO, "Listening for incoming device info and alert messages...\n");
  isRunning.store(true, std::memory_order_release);
  receiveThread = std::thread(&MessageReceiver::messageReceptionWorker);
}

void MessageReceiver::stopListening(void)
{
  // Only stop if the receiver is currently running
  if (!isRunning.load(std::memory_order_acquire)) return;

  // Disconnect from the MQTT broker
  isRunning.store(false, std::memory_order_release);
  AwsServices::mqttDisconnect();

  // Stop the reception thread and wait for it to complete
  logger.log(Logger::INFO, "Stopping the message listening thread..\n");
  if (receiveThread.joinable()) receiveThread.join();
  logger.log(Logger::INFO, "Message listening thread has completed\n");

  // Terminate all message processing threads
  logger.log(Logger::INFO, "Stopping all data processing threads...\n");
  terminationCondition.notify_all();
  while (numProcessingThreads.load(std::memory_order_acquire) > 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
  logger.log(Logger::INFO, "Data processing threads have completed\n");
  database.reset();
}

void MessageReceiver::messageReceptionWorker(void)
{
  // Loop forever until the program is terminated
  while (isRunning.load(std::memory_order_acquire))
  {
    // Receive a message from the MQTT broker and validate its size and type
    const auto [messageTopic, messageData] = AwsServices::mqttReceive();
    if (!messageTopic.compare(deviceInfoTopic) && (messageData.size() == sizeof(DeviceInfoMessage)))
      database->updateDeviceInfo(reinterpret_cast<const DeviceInfoMessage*>(messageData.data()));
    else if (!messageTopic.compare(alertsTopic) && (messageData.size() >= (sizeof(AlertMessage) - sizeof(AlertMessage::events))) &&
             (messageData.size() ==
              (sizeof(AlertMessage) - sizeof(AlertMessage::events) + (sizeof(EventInfo) * reinterpret_cast<const AlertMessage*>(messageData.data())->numEvents))))
    {
      database->updatePartialDeviceInfo(reinterpret_cast<const AlertMessage*>(messageData.data()));
      processAlertMessage(reinterpret_cast<const AlertMessage*>(messageData.data()));
    }
    else if (isRunning.load(std::memory_order_acquire))
      logger.log(Logger::WARNING, "Received MQTT message on unexpected topic '%s' or of incorrect size (%zu bytes)\n", messageTopic.c_str(), messageData.size());
  }
}

void MessageReceiver::processAlertMessage(const AlertMessage* __restrict message)
{
  // Extract individual shot alerts from the message and place them into the processing buffer
  const auto receptionTime = std::chrono::steady_clock::now();
  const auto [ecefX, ecefY, ecefZ] = GeoMath::convertLlhToEcef(message->sensorLat, message->sensorLon, message->sensorHt);
  const auto [qw, qx, qy, qz] = GeoMath::calculateFullQuaternion(message->sensorQ1, message->sensorQ2, message->sensorQ3);
  for (uint8_t i = 0; i < message->numEvents; ++i)
  {
    // Generate a gunshot report for the individual alert and add it to the database
    logger.log(Logger::INFO, "Received alert message from Device #%llu @ %.6f: Lat = %.4f, Lon = %.4f, Ht = %.4f, Conf = %.4f\n", message->deviceID, message->events[i].timestamp,
               message->sensorLat, message->sensorLon, message->sensorHt, message->events[i].confidence);
    const auto [azimuth, elevation] =
        GeoMath::calculateSourceDirection(qw, qx, qy, qz, message->events[i].angleOfArrival[0], message->events[i].angleOfArrival[1], message->events[i].angleOfArrival[2]);
    const auto report = std::make_shared<GunshotReport>(receptionTime, message->deviceID, message->events[i].timestamp, message->sensorLat, message->sensorLon, message->sensorHt,
                                                        ecefX, ecefY, ecefZ, azimuth, elevation, message->events[i].confidence, qw, qx, qy, qz, message->audioLocatorID);
    report->reportID = database->addGunshotReport(report.get());
    addReportToProcessingBuffer(report);
  }
}

void MessageReceiver::addReportToProcessingBuffer(const std::shared_ptr<GunshotReport>& report)
{
  // Processing buffer management variables for per-thread packet processing
  auto threadPrimaryReportID = 0ull;
  auto existingValidationReport = false;

  // Determine if this report could feasibly relate to any existing message packets
  std::lock_guard<std::mutex> lock(bufferMutex);
  for (auto& [primaryReportID, reportPackets] : processingBuffer)
    for (auto& reportPacket : reportPackets)
      if (Fusion::validateFeasibility(reportPacket.validationReport.get(), report.get()))
      {
        threadPrimaryReportID = primaryReportID;
        existingValidationReport |= (reportPacket.validationReport->deviceID == report->deviceID);
        reportPacket.reports.push_back(report);
      }

  // Check if the reporting device has no validation report in any existing message packets
  if (!existingValidationReport)
  {
    // If this report will already be investigated by an existing thread, create a top-level report packet in that thread's queue
    if (threadPrimaryReportID > 0)
      processingBuffer.at(threadPrimaryReportID).emplace_back(ReportPacket{.validationReport = report, .reports = {report}});
    else
    {
      // Create a new processing thread for a completely unrelated report message
      processingBuffer.emplace(report->reportID, std::deque<ReportPacket>{ReportPacket{.validationReport = report, .reports = {report}}});
      std::thread(&MessageReceiver::packetProcessingWorker, report->reportID).detach();
      numProcessingThreads.fetch_add(1, std::memory_order_acq_rel);
    }
  }
}

void MessageReceiver::packetProcessingWorker(uint64_t reportID)
{
  // Lock the processing buffer for this thread and log the start of processing for this report ID
  logger.log(Logger::INFO, "Started processing thread for report ID #%llu\n", reportID);
  std::unique_lock<std::mutex> lock(bufferMutex);
  auto* processingQueue = &processingBuffer.at(reportID);
  std::vector<std::shared_ptr<GunshotReport>> unusedReports;
  std::shared_ptr<GunshotReport> newValidationReport;

  // Loop until the processing queue for this thread is empty
  while (isRunning.load(std::memory_order_acquire) && !processingQueue->empty())
  {
    // Sleep until time to process the current message packet
    {
      const auto& messageBundle = processingQueue->front().reports;
      const auto receptionTime = messageBundle.front()->receptionTime;
      auto packetAgeMS = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - receptionTime).count();
      while (isRunning.load(std::memory_order_acquire) && (packetAgeMS < CivicAlert::ALERT_MIN_PROCESSING_TIMEOUT_MS))
      {
        terminationCondition.wait_for(lock, std::chrono::milliseconds(CivicAlert::ALERT_MIN_PROCESSING_TIMEOUT_MS - packetAgeMS));
        packetAgeMS = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - receptionTime).count();
      }

      // Wait until no new packets have been added to this report for a short period of time
      packetAgeMS = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - messageBundle.back()->receptionTime).count();
      while (isRunning.load(std::memory_order_acquire) && (packetAgeMS < CivicAlert::ALERT_NON_REPORT_PROCESSING_TIMEOUT_MS))
      {
        terminationCondition.wait_for(lock, std::chrono::milliseconds(CivicAlert::ALERT_NON_REPORT_PROCESSING_TIMEOUT_MS - packetAgeMS));
        packetAgeMS = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - messageBundle.back()->receptionTime).count();
      }
    }

    // Take the first message bundle, remove it from the queue, and unlock the buffer mutex while processing
    auto messageBundle = std::move(processingQueue->front());
    processingQueue->pop_front();
    lock.unlock();

    // Lock all report "in-use" flags to prevent concurrent use and remove any reports that have already been used
    newValidationReport.reset();
    auto validationReportUsed = false;
    for (auto it = messageBundle.reports.cbegin(); isRunning.load(std::memory_order_acquire) && (it != messageBundle.reports.cend());)
    {
      while (isRunning.load(std::memory_order_acquire) && (*it)->reportInUse.test_and_set(std::memory_order_acquire)) std::this_thread::sleep_for(std::chrono::milliseconds(1));
      if ((*it)->usedInFusion)
      {
        if ((*it).get() == messageBundle.validationReport.get()) validationReportUsed = true;
        (*it)->reportInUse.clear(std::memory_order_release);
        it = messageBundle.reports.erase(it);
      }
      else
      {
        if (!newValidationReport && ((*it)->deviceID == messageBundle.validationReport->deviceID)) newValidationReport = *it;
        ++it;
      }
    }

    // If the validation report has already been used, check if there is another report from the same device that can be used as the new validation report
    if (validationReportUsed)
    {
      if (newValidationReport)
        messageBundle.validationReport = newValidationReport;
      else
        messageBundle.reports.clear();
    }
    newValidationReport.reset();

    // Ignore if the packet does not contain enough reports to perform the fusion
    if (messageBundle.reports.size() < CivicAlert::FUSION_MIN_NUM_EVENTS)
      logger.log(Logger::INFO, "Not enough packets (%zu) to perform fusion; skipping...\n", messageBundle.reports.size());
    else if (isRunning.load(std::memory_order_acquire))
    {
      // Perform sensor fusion on the message bundle
      const auto fusionResult = Fusion::performFusion(messageBundle.reports);

      // Check if the sensor fusion was successful
      if (fusionResult.eventTimes.empty())
      {
        logger.log(Logger::WARNING, "Sensor fusion failed!\n");
        // TODO: IF LOTS OF UNITS DETECTED A SHOT, SHOULD STILL REPORT EVEN IF FUSION FAILS, MAYBE JUST REPORT THE LOCATION OF THE VALIDATION PACKET AND AVERAGE SOURCE DIRECTION
        // TODO: DETECT FIREWORK BASED ON SOURCE DIRECTION, HOW MANY UNITS HEARD SAME SHOT (REQUIRES CHANGE TO FEASIBILITY CHECK?), ONSET MAGNITUDE, ETC.
      }
      else
      {
        // Log the results of the fusion and the contributing reports
        logger.log(Logger::INFO, "Incident location: Lat = %.4f, Lon = %.4f, Ht = %.4f, Conf = %.4f; Contributing Reports = %zu; Times: [ ", fusionResult.lat, fusionResult.lon,
                   fusionResult.ht, fusionResult.confidence, fusionResult.contributingReports.size());
        for (const auto eventTime : fusionResult.eventTimes) logger.log(Logger::INFO, "%.6f ", eventTime);
        logger.log(Logger::INFO, "]\n");

        // TODO: Store the fusion result in the database and issue an alert notification through the API

        // Keep track of any unused reports and determine if a new message bundle should be created
        unusedReports.clear();
        for (const auto& report : messageBundle.reports)
          if (!report->usedInFusion)
          {
            if (!newValidationReport && (report->deviceID == messageBundle.validationReport->deviceID)) newValidationReport = report;
            unusedReports.push_back(report);
          }
      }
    }

    // Unlock all report "in-use" flags for the next processing iteration and re-lock the buffer mutex
    for (const auto& report : messageBundle.reports) report->reportInUse.clear(std::memory_order_release);
    lock.lock();
    processingQueue = &processingBuffer.at(reportID);

    // Create a new message bundle for any unused reports that relate to the validation report and add it to the front of the processing queue
    if (newValidationReport) processingQueue->emplace_front(ReportPacket{.validationReport = newValidationReport, .reports = std::move(unusedReports)});
  }

  // Erase the processing queue for this thread and log completion
  processingBuffer.erase(reportID);
  numProcessingThreads.fetch_sub(1, std::memory_order_acq_rel);
  logger.log(Logger::INFO, "Processing thread for report ID #%llu has completed\n", reportID);
}
