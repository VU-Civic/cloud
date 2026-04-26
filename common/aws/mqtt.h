#ifndef __AWS_MQTT_HEADER_H__
#define __AWS_MQTT_HEADER_H__

#include <atomic>
#include <deque>
#include <future>
#include <vector>
#include <aws/iot/Mqtt5Client.h>
#include "aws.h"

class AwsMQTT final
{
public:

  // Constructor/destructor
  AwsMQTT(const char* __restrict clientId, std::string&& caKey, std::string&& clientCertKey, std::string&& clientKeyKey, const char* __restrict endpointUrl, uint32_t endpointPort,
          uint32_t keepAliveSeconds);
  ~AwsMQTT(void);

  // AWS MQTT5 management functionality
  bool connect(void);
  void disconnect(void);
  bool subscribe(const char* __restrict topic, uint8_t qos);
  void unsubscribe(const char* __restrict topic);
  bool publish(const char* __restrict topic, const uint8_t* __restrict payload, size_t payloadLength, uint8_t qos, bool retain);
  std::pair<std::string, std::vector<uint8_t>> receive(void);

private:

  // Lifecycle callback functions
  void onAttemptingConnect(const Aws::Crt::Mqtt5::OnAttemptingConnectEventData&);
  void onConnectionSuccess(const Aws::Crt::Mqtt5::OnConnectionSuccessEventData&);
  void onConnectionFailure(const Aws::Crt::Mqtt5::OnConnectionFailureEventData& eventData);
  void onDisconnection(const Aws::Crt::Mqtt5::OnDisconnectionEventData& eventData);
  void onPublishReceived(const Aws::Crt::Mqtt5::PublishReceivedEventData& eventData);
  void onClientStopped(const Aws::Crt::Mqtt5::OnStoppedEventData&);

  // Private member variables
  std::deque<std::pair<std::string, std::vector<uint8_t>>> receivedPackets;
  std::shared_ptr<Aws::Crt::Mqtt5::Mqtt5Client> mqttClient;
  std::promise<bool> clientConnected, clientStopped;
  std::condition_variable packetReceived;
  std::atomic<bool> isRunning;
  std::mutex receiveMutex;
};
#endif  // #ifndef __AWS_MQTT_HEADER_H__
