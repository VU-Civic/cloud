#ifndef __PACKET_RECEIVER_HEADER_H__
#define __PACKET_RECEIVER_HEADER_H__

#include <atomic>
#include <thread>
#include <tuple>
#include <map>

class PacketReceiver final
{
public:

  // Reception control
  static void listenForPackets(void);
  static void stopListening(void);

private:

  // Thread and processing functions
  static void packetReceptionWorker(void);
  static void processPacket(const AlertMessage* packet);
  static void packetProcessingWorker(time_t receptionTime);

  // Private member variables
  static std::atomic<bool> isRunning;
  static std::atomic_flag isProcessing;
  static std::thread receiveThread, packetProcessingThread;
  static std::mutex receptionMutex;
  static std::map<double, std::pair<time_t, std::shared_ptr<AlertMessage>>> packetBuffer;
};

#endif  // #ifndef __PACKET_RECEIVER_HEADER_H__
