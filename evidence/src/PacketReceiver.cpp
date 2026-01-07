#include <algorithm>
#include "AwsServices.h"
#include "Common.h"
#include "EvidenceProcessor.h"
#include "PacketReceiver.h"

std::atomic<bool> PacketReceiver::isRunning(false);
std::thread PacketReceiver::receiveThread;
std::mutex PacketReceiver::receptionTimerMutex;
std::unordered_map<uint32_t, std::vector<std::vector<uint8_t>>> PacketReceiver::packetBuffer;
std::unordered_map<uint32_t, std::tuple<std::thread, int32_t, bool, bool>> PacketReceiver::receptionTimers;

void PacketReceiver::listenForPackets(void)
{
  // Start a new thread to listen for incoming packets
  logger.log(Logger::INFO, "Listening for incoming evidence packets...\n");
  isRunning.store(true, std::memory_order_relaxed);
  receiveThread = std::thread(&PacketReceiver::packetReceptionThread);
}

void PacketReceiver::stopListening(void)
{
  // Stop the packet reception thread and disconnect from the MQTT broker
  logger.log(Logger::INFO, "Stopping the evidence listening thread...\n");
  isRunning.store(false, std::memory_order_relaxed);
  AwsServices::mqttDisconnect();
  if (receiveThread.joinable()) receiveThread.join();

  // Wait until all reception timer threads have completed
  bool timersRemaining = true;
  for (int retries = 0; timersRemaining && (retries < 15); retries++)
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
    else
      logger.log(Logger::WARNING, "Received MQTT packet of incorrect size (%zu bytes)\n", receivedMessage.size());
  }
}

void PacketReceiver::receptionTimeoutThread(uint32_t deviceID)
{
  // Loop forever until reception times out or the program is terminated
  bool timedOut = false;
  while (isRunning.load(std::memory_order_relaxed) && !timedOut)
  {
    // Sleep for one second and increment the timer count
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::lock_guard<std::mutex> receptionLock(receptionTimerMutex);
    const uint32_t timerCount = std::get<1>(receptionTimers[deviceID]);
    if ((timerCount >= 0) && (timerCount < CivicAlert::EVIDENCE_PROCESSING_TIMEOUT_SECONDS))
      std::get<1>(receptionTimers[deviceID])++;
    else
      timedOut = true;
  }

  // Force processing of any received packets if timed out without all packets being received
  std::lock_guard<std::mutex> receptionLock(receptionTimerMutex);
  if (timedOut && !std::get<3>(receptionTimers[deviceID]))
  {
    // Fill in missing packets with empty data and process the received packets
    logger.log(Logger::WARNING, "Reception timed out for Device #%lu, processing received packets...\n", deviceID);
    for (auto& packet : packetBuffer[deviceID])
      if (packet.empty())
      {
        packet.emplace_back(CivicAlert::EVIDENCE_OPUS_FRAME_DELIMITER);
        packet.emplace_back(0);
      }
    EvidenceProcessor::processEvidenceData(packetBuffer.extract(deviceID));
  }

  // Clean up the reception timer and packet buffer entries
  receptionTimers.erase(deviceID);
  packetBuffer.erase(deviceID);
}

void PacketReceiver::processPacket(const EvidenceMessage* packet, uint32_t packetLength)
{
  // Extract the message index, final flag, and evidence data from the packet
  const uint8_t messageIndex = packet->messageIdxAndFinal & MQTT_MESSAGE_INDEX_MASK, isFinal = packet->messageIdxAndFinal & MQTT_MESSAGE_FINAL_MASK;
  logger.log(Logger::INFO, "Received evidence packet #%u from Device #%lu\n", messageIndex, packet->deviceID);
  std::vector<uint8_t> evidenceData(packet->data, packet->data + packetLength - offsetof(EvidenceMessage, data));

  // Start or reset a reception timeout timer for the device's evidence queue
  std::lock_guard<std::mutex> receptionLock(receptionTimerMutex);
  if (!receptionTimers.count(packet->deviceID))
  {
    receptionTimers.emplace(packet->deviceID, std::make_tuple(std::thread(&PacketReceiver::receptionTimeoutThread, packet->deviceID), 0, isFinal, false));
    std::get<0>(receptionTimers[packet->deviceID]).detach();
  }
  else
  {
    std::get<1>(receptionTimers[packet->deviceID]) = 0;
    std::get<2>(receptionTimers[packet->deviceID]) = std::get<2>(receptionTimers[packet->deviceID]) || isFinal;
  }

  // Place the packet into its correct slot in the device's evidence queue
  auto& packets = packetBuffer[packet->deviceID];
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
  if (std::get<2>(receptionTimers[packet->deviceID]) && std::none_of(packets.cbegin(), packets.cend(), [](const std::vector<uint8_t>& pkt) { return pkt.empty(); }))
  {
    logger.log(Logger::INFO, "Received all evidence packets from Device #%lu, processing...\n", packet->deviceID);
    EvidenceProcessor::processEvidenceData(packetBuffer.extract(packet->deviceID));
    std::get<3>(receptionTimers[packet->deviceID]) = true;
    std::get<1>(receptionTimers[packet->deviceID]) = -1;
  }
}
