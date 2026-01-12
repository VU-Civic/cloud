#include <algorithm>
#include "AwsServices.h"
#include "Common.h"
#include "FusionAlgorithm.h"
#include "PacketReceiver.h"

std::atomic<bool> PacketReceiver::isRunning(false);
std::atomic_flag PacketReceiver::isProcessing(false);
std::thread PacketReceiver::receiveThread;
std::thread PacketReceiver::packetProcessingThread;
std::mutex PacketReceiver::receptionMutex;
std::map<double, std::pair<time_t, std::shared_ptr<AlertMessage>>> PacketReceiver::packetBuffer;

void PacketReceiver::listenForPackets(void)
{
  // Start a new thread to listen for incoming packets
  logger.log(Logger::INFO, "Listening for incoming alert packets...\n");
  isRunning.store(true, std::memory_order_relaxed);
  receiveThread = std::thread(&PacketReceiver::packetReceptionWorker);
}

void PacketReceiver::stopListening(void)
{
  // Only stop if the receiver is currently running
  if (!isRunning.load(std::memory_order_relaxed)) return;

  // Stop the packet reception thread and disconnect from the MQTT broker
  logger.log(Logger::INFO, "Stopping the alert listening thread...\n");
  isRunning.store(false, std::memory_order_relaxed);
  AwsServices::mqttDisconnect();
  if (receiveThread.joinable()) receiveThread.join();

  // Wait until the packet processing thread has completed
  if (packetProcessingThread.joinable()) packetProcessingThread.join();
  logger.log(Logger::INFO, "Alert listening thread has completed\n");
}

void PacketReceiver::packetReceptionWorker(void)
{
  // Loop forever until the program is terminated
  while (isRunning.load(std::memory_order_relaxed))
  {
    // Receive packets from the MQTT broker and validate their size
    const auto receivedMessage = AwsServices::mqttReceive();
    if (receivedMessage.size() == sizeof(AlertMessage))
      processPacket(reinterpret_cast<const AlertMessage*>(receivedMessage.data()));
    else if (isRunning.load(std::memory_order_relaxed))
      logger.log(Logger::WARNING, "Received MQTT packet of incorrect size (%zu bytes)\n", receivedMessage.size());
  }
}

void PacketReceiver::processPacket(const AlertMessage* packet)
{
  logger.log(Logger::INFO, "Received alert packet from Device #%lu @ %.6f: Lat = %.4f, Lon = %.4f, Ht = %.4f, Conf = %.4f\n", packet->deviceID, packet->timestamp, packet->lat,
             packet->lon, packet->height, packet->confidence);
  const auto receptionTime = time(nullptr);
  auto receptionTimeAndPacket = std::make_pair(receptionTime, std::make_shared<AlertMessage>(*packet));
  std::lock_guard<std::mutex> lock(receptionMutex);
  packetBuffer.emplace(packet->timestamp, std::move(receptionTimeAndPacket));
  if (!isProcessing.test_and_set(std::memory_order_acquire)) packetProcessingThread = std::thread(&PacketReceiver::packetProcessingWorker, receptionTime);
}

void PacketReceiver::packetProcessingWorker(time_t receptionTime)
{
  // Continue processing while unused data remains and the program is still running
  bool moreToProcess = true;
  while (moreToProcess && isRunning.load(std::memory_order_relaxed))
  {
    // Sleep until time to process the oldest received packet
    const time_t timeDifference = time(nullptr) - receptionTime;
    if (timeDifference < CivicAlert::ALERT_PROCESSING_TIMEOUT_SECONDS)
      std::this_thread::sleep_for(std::chrono::seconds(CivicAlert::ALERT_PROCESSING_TIMEOUT_SECONDS - timeDifference));

    // Determine which packets could feasibly relate to the oldest alert
    std::map<uint32_t, std::shared_ptr<AlertMessage>> messageBundle;
    {
      std::lock_guard<std::mutex> lock(receptionMutex);
      const auto& validationPacket = packetBuffer.cbegin()->second.second;
      for (const auto& packetDetails : packetBuffer)
      {
        const auto& packet = packetDetails.second.second;
        if (Fusion::validateFeasibility(messageBundle, validationPacket.get(), packet.get())) messageBundle.emplace(packet->deviceID, packet);
      }
    }

    // Check if enough feasible packets exist to perform fusion
    logger.log(Logger::INFO, "Beginning sensor fusion using %zu feasible packets\n", messageBundle.size());
    if (messageBundle.size() < CivicAlert::FUSION_MIN_NUM_EVENTS)
    {
      // Skip processing if not enough packets
      logger.log(Logger::INFO, "Not enough packets (%zu) to perform fusion; skipping...\n", messageBundle.size());
      for (auto& message : messageBundle) message.second->usedInFusion = true;
    }
    else
    {
      // Perform sensor fusion on the feasible message bundle
      const auto fusionResult = Fusion::performFusion(messageBundle);
      if (fusionResult.fusionSuccessful)
      {
        // TODO: Something with the result
        logger.log(Logger::INFO, "Sensor fusion was successful!\n");
      }
      else
      {
        // Mark the validation packet as used to avoid reprocessing
        logger.log(Logger::WARNING, "Sensor fusion failed!\n");
        messageBundle.cbegin()->second->usedInFusion = true;
      }
    }

    // Clean up all packets that were used in the fusion
    std::lock_guard<std::mutex> lock(receptionMutex);
    for (auto it = packetBuffer.cbegin(); it != packetBuffer.cend();)
      if (it->second.second->usedInFusion)
        it = packetBuffer.erase(it);
      else
        ++it;

    // Update the reception time of the oldest packet if one exists
    moreToProcess = !packetBuffer.empty();
    if (moreToProcess)
      receptionTime = packetBuffer.cbegin()->second.first;
    else
      isProcessing.clear(std::memory_order_release);
  }
}
