#include <algorithm>
#include "AwsServices.h"
#include "Common.h"
#include "FusionAlgorithm.h"
#include "GeoMath.h"
#include "PacketReceiver.h"
#include "Transcoding.h"

std::mutex PacketReceiver::receptionMutex;
std::atomic_bool PacketReceiver::isRunning(false);
std::condition_variable PacketReceiver::newPacketCondition;
std::condition_variable PacketReceiver::terminationCondition;
std::thread PacketReceiver::receiveThread, PacketReceiver::packetProcessingThread;
std::map<double, std::pair<std::chrono::steady_clock::time_point, std::shared_ptr<GunshotReport>>> PacketReceiver::packetBuffer;
std::unique_ptr<StorageDatabase> PacketReceiver::database(nullptr);
std::string PacketReceiver::deviceInfoTopic, PacketReceiver::alertsTopic;

void PacketReceiver::listenForPackets(const char* mqttDeviceInfoTopic, const char* mqttAlertsTopic)
{
  // Store the MQTT topic names for use in the reception thread
  deviceInfoTopic = mqttDeviceInfoTopic;
  alertsTopic = mqttAlertsTopic;

  // Initialize a connection to the storage database
  logger.log(Logger::INFO, "Initializing connection to the CivicAlert database...\n");
  database = std::make_unique<StorageDatabase>();

  // Start threads to listen for and process incoming device info and alert packets
  logger.log(Logger::INFO, "Listening for incoming device info and alert packets...\n");
  isRunning.store(true, std::memory_order_release);
  packetProcessingThread = std::thread(&PacketReceiver::packetProcessingWorker);
  receiveThread = std::thread(&PacketReceiver::packetReceptionWorker);
}

void PacketReceiver::stopListening(void)
{
  // Only stop if the receiver is currently running
  if (!isRunning.load(std::memory_order_acquire)) return;

  // Stop the packet reception thread and disconnect from the MQTT broker
  logger.log(Logger::INFO, "Stopping the packet listening thread..\n");
  isRunning.store(false, std::memory_order_release);
  AwsServices::mqttDisconnect();
  if (receiveThread.joinable()) receiveThread.join();

  // Wait until the packet processing thread has completed
  terminationCondition.notify_all();
  if (packetProcessingThread.joinable()) packetProcessingThread.join();
  logger.log(Logger::INFO, "Packet listening thread has completed\n");
}

void PacketReceiver::packetReceptionWorker(void)
{
  // Loop forever until the program is terminated
  while (isRunning.load(std::memory_order_acquire))
  {
    // Receive a packet from the MQTT broker and decode its contents
    auto [messageTopic, rawData] = AwsServices::mqttReceive();
    const auto messageData = Transcoding::hexDecode(std::move(rawData));

    // Validate the message size and type, and process accordingly
    if (!messageTopic.compare(deviceInfoTopic) && (messageData.size() == sizeof(DeviceInfoMessage)))
      database->updateDeviceInfo(reinterpret_cast<const DeviceInfoMessage*>(messageData.data()));
    else if (!messageTopic.compare(alertsTopic) && (messageData.size() >= (sizeof(AlertMessage) - sizeof(AlertMessage::events))) &&
             (messageData.size() ==
              (sizeof(AlertMessage) - sizeof(AlertMessage::events) + (sizeof(EventInfo) * reinterpret_cast<const AlertMessage*>(messageData.data())->numEvents))))
    {
      database->updatePartialDeviceInfo(reinterpret_cast<const AlertMessage*>(messageData.data()));
      processAlertPacket(reinterpret_cast<const AlertMessage*>(messageData.data()));
    }
    else if (isRunning.load(std::memory_order_acquire))
      logger.log(Logger::WARNING, "Received MQTT packet on unexpected topic '%s' or of incorrect size (%zu bytes)\n", messageTopic.c_str(), messageData.size());
  }
}

void PacketReceiver::processAlertPacket(const AlertMessage* __restrict packet)
{
  // Extract individual shot alerts from the packet and place them into the packet buffer
  const auto receptionTime = std::chrono::steady_clock::now();
  const auto [ecefX, ecefY, ecefZ] = GeoMath::convertLlhToEcef(packet->sensorLat, packet->sensorLon, packet->sensorHt);
  const auto [qw, qx, qy, qz] = GeoMath::calculateFullQuaternion(packet->sensorQ1, packet->sensorQ2, packet->sensorQ3);
  for (uint8_t i = 0; i < packet->numEvents; ++i)
  {
    // Generate a gunshot report for the individual alert and add it to the database
    logger.log(Logger::INFO, "Received alert packet from Device #%llu @ %.6f: Lat = %.4f, Lon = %.4f, Ht = %.4f, Conf = %.4f\n", packet->deviceID, packet->events[i].timestamp,
               packet->sensorLat, packet->sensorLon, packet->sensorHt, packet->events[i].confidence);
    const auto [azimuth, elevation] = GeoMath::calculateSourceDirection(qw, qx, qy, qz, packet->events[i].angleOfArrival);
    auto report = std::make_shared<GunshotReport>(GunshotReport{.deviceID = packet->deviceID,
                                                                .reportID = 0,
                                                                .timestamp = packet->events[i].timestamp,
                                                                .lat = packet->sensorLat,
                                                                .lon = packet->sensorLon,
                                                                .ht = packet->sensorHt,
                                                                .x = ecefX,
                                                                .y = ecefY,
                                                                .z = ecefZ,
                                                                .sourceAzimuth = azimuth,
                                                                .sourceElevation = elevation,
                                                                .confidence = packet->events[i].confidence,
                                                                .orientationQW = qw,
                                                                .orientationQX = qx,
                                                                .orientationQY = qy,
                                                                .orientationQZ = qz,
                                                                .audioLocatorID = packet->audioLocatorID,
                                                                .usedInFusion = false});
    report->reportID = database->addGunshotReport(report.get());

    // Add the report to the packet buffer for potential fusion with other reports
    std::lock_guard<std::mutex> lock(receptionMutex);
    packetBuffer.emplace(packet->events[i].timestamp, std::make_pair(receptionTime, report));
  }
  newPacketCondition.notify_one();
}

void PacketReceiver::packetProcessingWorker(void)
{
  // Loop forever until the program is terminated
  while (isRunning.load(std::memory_order_acquire))
  {
    // Wait until at least one packet is received to begin processing
    std::unique_lock<std::mutex> lock(receptionMutex);
    newPacketCondition.wait(lock, [] { return !packetBuffer.empty() || !isRunning.load(std::memory_order_acquire); });
    if (!isRunning.load(std::memory_order_acquire)) break;

    // Sleep until time to process the oldest received packet
    auto timeDifference = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - packetBuffer.cbegin()->second.first).count();
    while (timeDifference < CivicAlert::ALERT_PROCESSING_TIMEOUT_MS)
    {
      terminationCondition.wait_for(lock, std::chrono::milliseconds(CivicAlert::ALERT_PROCESSING_TIMEOUT_MS - timeDifference));
      timeDifference = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - packetBuffer.cbegin()->second.first).count();
    }

    // Determine which packets could feasibly relate to the oldest alert
    std::vector<std::shared_ptr<GunshotReport>> messageBundle;
    const auto& validationPacket = packetBuffer.cbegin()->second.second;
    for (const auto& packetDetails : packetBuffer)
    {
      const auto& packet = packetDetails.second.second;
      if (Fusion::validateFeasibility(messageBundle, validationPacket.get(), packet.get())) messageBundle.emplace_back(packet);
    }
    lock.unlock();

    // Check if enough feasible packets exist to perform fusion
    if (messageBundle.size() < CivicAlert::FUSION_MIN_NUM_EVENTS)
    {
      // Skip processing if not enough packets, and mark the validation packet as used to avoid reprocessing
      logger.log(Logger::INFO, "Not enough packets (%zu) to perform fusion; skipping...\n", messageBundle.size());
      if (!messageBundle.empty()) messageBundle.front()->usedInFusion = true;
    }
    else
    {
      // Perform sensor fusion on the feasible message bundle
      logger.log(Logger::INFO, "Beginning sensor fusion using %zu feasible packets\n", messageBundle.size());
      const auto fusionResult = Fusion::performFusion(messageBundle);
      if (!fusionResult.eventTimes.empty())
      {
        // TODO: Something with the result
        logger.log(Logger::INFO, "Sensor fusion was successful!\n");
      }
      else
      {
        // Mark the validation packet as used to avoid reprocessing
        logger.log(Logger::WARNING, "Sensor fusion failed!\n");
        messageBundle.front()->usedInFusion = true;  // TODO: PROBABLY NEED TO MARK ALL AS USED SINCE PROCESSING A LARGE BUNDLE TAKES A LONG TIME AND THERE'S NO REASON A LARGE
                                                     // BUNDLE SHOULD PRODUCE NO FUSION RESULT AT ALL UNLESS THERE ARE LOTS OF ERRORS IN IT
        // TODO: IF A LOT OF UNITS DETECTED A SHOT, SHOULD STILL REPORT THE SHOT EVEN IF FUSION FAILS, MAYBE JUST REPORT THE LOCATION OF THE VALIDATION PACKET AND AVERAGE SOURCE
        // DIRECTION, OR SOMETHING LIKE THAT
      }
    }

    // Clean up all packets that were used in the fusion
    lock.lock();
    for (auto it = packetBuffer.cbegin(); it != packetBuffer.cend();)
      if (it->second.second->usedInFusion)
        it = packetBuffer.erase(it);
      else
        ++it;
  }
}
