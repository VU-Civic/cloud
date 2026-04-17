#ifndef __PACKET_RECEIVER_HEADER_H__
#define __PACKET_RECEIVER_HEADER_H__

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <thread>
#include <tuple>
#include "Common.h"
#include "StorageDatabase.h"

class PacketReceiver final
{
public:

  // Reception control
  static void listenForPackets(const char* mqttDeviceInfoTopic, const char* mqttAlertsTopic);
  static void stopListening(void);

private:

  // Thread and processing functions
  static void packetReceptionWorker(void);
  static void processAlertPacket(const AlertMessage* __restrict packet);
  static void packetProcessingWorker(void);

  // Private member variables
  static std::mutex receptionMutex;
  static std::atomic_bool isRunning;
  static std::condition_variable newPacketCondition;
  static std::condition_variable terminationCondition;
  static std::thread receiveThread, packetProcessingThread;
  static std::map<double, std::pair<std::chrono::steady_clock::time_point, std::shared_ptr<GunshotReport>>> packetBuffer;
  static std::unique_ptr<StorageDatabase> database;
  static std::string deviceInfoTopic, alertsTopic;
};

#endif  // #ifndef __PACKET_RECEIVER_HEADER_H__
