#ifndef __PACKET_RECEIVER_HEADER_H__
#define __PACKET_RECEIVER_HEADER_H__

#include <thread>
#include <tuple>
#include <unordered_map>
#include "Common.h"

typedef struct
{
  std::thread timerThread;
  int32_t elapsedSeconds;
  bool isFinalPacketReceived;
  bool isProcessingStarted;
} ReceptionTimerInfo;

class PacketReceiver final
{
public:

  // Reception control
  static void listenForPackets(void);
  static void stopListening(void);

private:

  // Thread and processing functions
  static void packetReceptionThread(void);
  static void receptionTimeoutThread(uint64_t deviceClipID, uint64_t deviceID, uint8_t clipID);
  static void processPacket(const EvidenceMessage* __restrict packet, uint32_t packetLength);

  // Private member variables
  static std::atomic_bool isRunning;
  static std::thread receiveThread;
  static std::mutex receptionTimerMutex;
  static std::unordered_map<uint64_t, std::vector<std::vector<uint8_t>>> packetBuffer;
  static std::unordered_map<uint64_t, ReceptionTimerInfo> receptionTimers;
};

#endif  // #ifndef __PACKET_RECEIVER_HEADER_H__
