#include <algorithm>
#include "AwsServices.h"
#include "Common.h"
#include "EvidenceProcessor.h"
#include "PacketReceiver.h"

std::atomic<bool> PacketReceiver::isRunning(false);
std::thread PacketReceiver::receiveThread;
std::mutex PacketReceiver::receptionTimerMutex;
std::unordered_map<uint64_t, std::vector<std::vector<uint8_t>>> PacketReceiver::packetBuffer;
std::unordered_map<uint64_t, ReceptionTimerInfo> PacketReceiver::receptionTimers;

void PacketReceiver::listenForPackets(void)
{
  // Start a new thread to listen for incoming packets
  logger.log(Logger::INFO, "Listening for incoming evidence packets...\n");
  isRunning.store(true, std::memory_order_relaxed);
  receiveThread = std::thread(&PacketReceiver::packetReceptionThread);
}

void PacketReceiver::stopListening(void)
{
  // Only stop if the receiver is currently running
  if (!isRunning.load(std::memory_order_relaxed)) return;

  // Stop the packet reception thread and disconnect from the MQTT broker
  logger.log(Logger::INFO, "Stopping the evidence listening thread...\n");
  isRunning.store(false, std::memory_order_relaxed);
  AwsServices::mqttDisconnect();
  if (receiveThread.joinable()) receiveThread.join();

  // Wait until all reception timer threads have completed
  bool timersRemaining = true;
  for (int retries = 0; timersRemaining && (retries < (CivicAlert::EVIDENCE_PROCESSING_TIMEOUT_SECONDS * 10)); retries++)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::lock_guard<std::mutex> receptionLock(receptionTimerMutex);
    timersRemaining = !receptionTimers.empty();
  }
  logger.log(Logger::INFO, "Evidence listening thread has completed\n");
}

void PacketReceiver::packetReceptionThread(void)
{
  // Loop forever until the program is terminated
  while (isRunning.load(std::memory_order_relaxed))
  {
    // Receive packets from the MQTT broker and validate their size
    const auto receivedMessage = AwsServices::mqttReceive();
    if ((receivedMessage.size() >= offsetof(EvidenceMessage, data)) && (receivedMessage.size() <= sizeof(EvidenceMessage)))
      processPacket(reinterpret_cast<const EvidenceMessage*>(receivedMessage.data()), receivedMessage.size());
    else if (isRunning.load(std::memory_order_relaxed))
      logger.log(Logger::WARNING, "Received MQTT packet of incorrect size (%zu bytes)\n", receivedMessage.size());
  }
}

void PacketReceiver::receptionTimeoutThread(uint64_t deviceClipID, uint64_t deviceID, uint8_t clipID)
{
  // Loop forever until reception times out or the program is terminated
  bool timedOut = false;
  while (isRunning.load(std::memory_order_relaxed) && !timedOut)
  {
    // Sleep for one second and increment the timer count
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::lock_guard<std::mutex> receptionLock(receptionTimerMutex);
    const int32_t timerCount = receptionTimers[deviceClipID].elapsedSeconds;
    if ((timerCount >= 0) && (timerCount < CivicAlert::EVIDENCE_PROCESSING_TIMEOUT_SECONDS))
      receptionTimers[deviceClipID].elapsedSeconds++;
    else
      timedOut = true;
  }

  // Force processing of any received packets if timed out without all packets being received
  std::lock_guard<std::mutex> receptionLock(receptionTimerMutex);
  if (timedOut && !receptionTimers[deviceClipID].isProcessingStarted)
  {
    // Fill in missing packets with empty data and process the received packets
    logger.log(Logger::WARNING, "Reception timed out for Clip #%u from Device #%llu, processing received packets...\n", clipID, deviceID);
    for (auto& packet : packetBuffer[deviceClipID])
      if (packet.empty())
      {
        packet.emplace_back(CivicAlert::EVIDENCE_OPUS_FRAME_DELIMITER);
        packet.emplace_back(0);
      }
    EvidenceProcessor::processEvidenceData(deviceID, clipID, packetBuffer.extract(deviceClipID));
  }

  // Clean up the reception timer and packet buffer entries
  receptionTimers.erase(deviceClipID);
  packetBuffer.erase(deviceClipID);
}

void PacketReceiver::processPacket(const EvidenceMessage* packet, uint32_t packetLength)
{
  // Extract the message index, final flag, and evidence data from the packet
  const uint64_t deviceClipID = *reinterpret_cast<const uint64_t*>(packet->deviceID), deviceID = deviceClipID & 0x00FFFFFFFFFFFFFF;
  const uint8_t messageIndex = packet->messageIdxAndFinal & MQTT_MESSAGE_INDEX_MASK, isFinal = packet->messageIdxAndFinal & MQTT_MESSAGE_FINAL_MASK;
  logger.log(Logger::INFO, "Received evidence packet #%u for Clip #%u from Device #%llu\n", messageIndex, packet->clipID, deviceID);
  std::vector<uint8_t> evidenceData(packet->data, packet->data + packetLength - offsetof(EvidenceMessage, data));

  // Start or reset a reception timeout timer for the device clip's evidence queue
  std::lock_guard<std::mutex> receptionLock(receptionTimerMutex);
  if (!receptionTimers.count(deviceClipID))
  {
    receptionTimers.emplace(deviceClipID, ReceptionTimerInfo{std::thread(&PacketReceiver::receptionTimeoutThread, deviceClipID, deviceID, packet->clipID), 0, isFinal != 0, false});
    receptionTimers[deviceClipID].timerThread.detach();
  }
  else
  {
    receptionTimers[deviceClipID].elapsedSeconds = 0;
    receptionTimers[deviceClipID].isFinalPacketReceived = receptionTimers[deviceClipID].isFinalPacketReceived || isFinal;
  }

  // Place the packet into its correct slot in the device clip's evidence queue
  auto& packets = packetBuffer[deviceClipID];
  if (packets.size() == messageIndex)
    packets.emplace_back(std::move(evidenceData));
  else if (packets.size() < messageIndex)
  {
    packets.resize(messageIndex);
    packets.emplace_back(std::move(evidenceData));
  }
  else
    packets[messageIndex] = std::move(evidenceData);

  // Pass the queue to the evidence processor if the packet series is complete
  if (receptionTimers[deviceClipID].isFinalPacketReceived && std::none_of(packets.cbegin(), packets.cend(), [](const std::vector<uint8_t>& pkt) { return pkt.empty(); }))
  {
    logger.log(Logger::INFO, "Received all evidence packets for Clip #%u from Device #%llu, processing...\n", packet->clipID, deviceID);
    EvidenceProcessor::processEvidenceData(deviceID, packet->clipID, packetBuffer.extract(deviceClipID));
    receptionTimers[deviceClipID].isProcessingStarted = true;
    receptionTimers[deviceClipID].elapsedSeconds = -1;
  }
}
