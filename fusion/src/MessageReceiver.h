#ifndef __MESSAGE_RECEIVER_HEADER_H__
#define __MESSAGE_RECEIVER_HEADER_H__

#include <condition_variable>
#include <map>
#include <memory>
#include <thread>
#include <tuple>
#include <unordered_map>
#include "Common.h"
#include "StorageDatabase.h"

class MessageReceiver final
{
public:

  // Reception control
  static void listenForMessages(const char* mqttDeviceInfoTopic, const char* mqttAlertsTopic);
  static void stopListening(void);

private:

  // Thread and processing functions
  static void messageReceptionWorker(void);
  static void processAlertMessage(const AlertMessage* __restrict message);
  static void addReportToProcessingBuffer(const std::shared_ptr<GunshotReport>& report);
  static void packetProcessingWorker(uint64_t reportID);

  // Private member variables
  static std::mutex bufferMutex;
  static std::atomic_bool isRunning;
  static std::atomic_uint32_t numProcessingThreads;
  static std::condition_variable terminationCondition;
  static std::unordered_map<uint64_t, std::deque<ReportPacket>> processingBuffer;
  static std::unique_ptr<StorageDatabase> database;
  static std::string deviceInfoTopic, alertsTopic;
  static std::thread receiveThread;
};

#endif  // #ifndef __MESSAGE_RECEIVER_HEADER_H__
