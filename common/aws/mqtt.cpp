#include <aws/crt/mqtt/Mqtt5Packets.h>
#include "mqtt.h"

AwsMQTT::AwsMQTT(const char* __restrict clientId, std::string&& caKey, std::string&& clientCertKey, std::string&& clientKeyKey, const char* __restrict endpointUrl,
                 uint32_t endpointPort, uint32_t keepAliveSeconds)
    : receivedPackets(), mqttClient(nullptr), clientConnected(), clientStopped(), packetReceived(), receiveMutex(), isRunning(false)
{
  // Unescape newline characters in PEM strings
  size_t index = 0;
  while ((index = caKey.find("\\n", index)) != std::string::npos) caKey.replace(index++, 2, "\n");
  index = 0;
  while ((index = clientCertKey.find("\\n", index)) != std::string::npos) clientCertKey.replace(index++, 2, "\n");
  index = 0;
  while ((index = clientKeyKey.find("\\n", index)) != std::string::npos) clientKeyKey.replace(index++, 2, "\n");

  // Generate client builder and set up connection options
  std::shared_ptr<Aws::Iot::Mqtt5ClientBuilder> mqtt5ClientBuilder(Aws::Iot::Mqtt5ClientBuilder::NewMqtt5ClientBuilderWithMtlsFromMemory(
      endpointUrl, Aws::Crt::ByteCursorFromCString(clientCertKey.c_str()), Aws::Crt::ByteCursorFromCString(clientKeyKey.c_str())));
  std::shared_ptr<Aws::Crt::Mqtt5::ConnectPacket> connectOptions = Aws::Crt::MakeShared<Aws::Crt::Mqtt5::ConnectPacket>(Aws::Crt::DefaultAllocatorImplementation());
  connectOptions->WithClientId(clientId);
  connectOptions->WithKeepAliveIntervalSec(keepAliveSeconds);
  mqtt5ClientBuilder->WithPort(endpointPort);
  mqtt5ClientBuilder->WithCertificateAuthority(Aws::Crt::ByteCursorFromCString(caKey.c_str()));
  mqtt5ClientBuilder->WithConnackTimeoutMs(5000);
  mqtt5ClientBuilder->WithAckTimeoutSec(5);
  mqtt5ClientBuilder->WithConnectOptions(connectOptions);

  // Register lifecycle callbacks
  mqtt5ClientBuilder->WithClientAttemptingConnectCallback(std::bind(&AwsMQTT::onAttemptingConnect, this, std::placeholders::_1));
  mqtt5ClientBuilder->WithClientConnectionSuccessCallback(std::bind(&AwsMQTT::onConnectionSuccess, this, std::placeholders::_1));
  mqtt5ClientBuilder->WithClientConnectionFailureCallback(std::bind(&AwsMQTT::onConnectionFailure, this, std::placeholders::_1));
  mqtt5ClientBuilder->WithClientDisconnectionCallback(std::bind(&AwsMQTT::onDisconnection, this, std::placeholders::_1));
  mqtt5ClientBuilder->WithPublishReceivedCallback(std::bind(&AwsMQTT::onPublishReceived, this, std::placeholders::_1));
  mqtt5ClientBuilder->WithClientStoppedCallback(std::bind(&AwsMQTT::onClientStopped, this, std::placeholders::_1));

  // Build the MQTT client
  mqttClient = mqtt5ClientBuilder->Build();
}

AwsMQTT::~AwsMQTT(void)
{
  // Ensure that the MQTT client is stopped
  disconnect();
}

bool AwsMQTT::connect(void)
{
  // Ensure that the MQTT client was properly initialized and not already running
  if (!mqttClient.get())
    return false;
  else if (isRunning)
    return true;

  // Start the MQTT client and wait for the connection result
  clientConnected = {};
  if (!mqttClient->Start())
  {
    logger.log(Logger::ERROR, "AWS MQTT: Failed to start MQTT client\n");
    return false;
  }
  return clientConnected.get_future().get();
}

void AwsMQTT::disconnect(void)
{
  // Attempt to disconnect from the MQTT broker
  if (isRunning)
  {
    clientStopped = {};
    logger.log(Logger::INFO, "AWS MQTT: Attempting to disconnect from MQTT Broker...\n");
    if (mqttClient->Stop()) clientStopped.get_future().wait();
  }
  packetReceived.notify_one();
}

bool AwsMQTT::subscribe(const char* __restrict topic, uint8_t qos)
{
  // Ensure that there is a valid MQTT client
  if (!mqttClient.get()) return false;

  // Create a subscription object and packet
  Aws::Crt::Mqtt5::Subscription subscription(topic, static_cast<Aws::Crt::Mqtt5::QOS>(qos));
  subscription.WithNoLocal(true);
  auto subPacket = Aws::Crt::MakeShared<Aws::Crt::Mqtt5::SubscribePacket>(Aws::Crt::DefaultAllocatorImplementation());
  subPacket->WithSubscription(std::move(subscription));

  // Attempt to subscribe to the designated topic
  std::promise<bool> subscribeSuccess;
  logger.log(Logger::INFO, "AWS MQTT: Attempting to subscribe to topic '%s'\n", topic);
  if (mqttClient->Subscribe(subPacket,
                            [&subscribeSuccess](int errorCode, std::shared_ptr<Aws::Crt::Mqtt5::SubAckPacket>)
                            {
                              if (errorCode == AWS_ERROR_SUCCESS)
                              {
                                logger.log(Logger::INFO, "AWS MQTT: Successfully subscribed to topic\n");
                                subscribeSuccess.set_value(true);
                              }
                              else
                              {
                                logger.log(Logger::ERROR, "AWS MQTT: Failed to subscribe to topic with error: %s\n", aws_error_debug_str(errorCode));
                                subscribeSuccess.set_value(false);
                              }
                            }))
    return subscribeSuccess.get_future().get();
  return false;
}

void AwsMQTT::unsubscribe(const char* __restrict topic)
{
  // Ensure that there is a valid MQTT client
  if (!mqttClient.get()) return;

  // Create an unsubscribe packet
  auto unsubPacket = Aws::Crt::MakeShared<Aws::Crt::Mqtt5::UnsubscribePacket>(Aws::Crt::DefaultAllocatorImplementation());
  unsubPacket->WithTopicFilter(topic);

  // Attempt to unsubscribe from the designated topic
  std::promise<void> unsubscribeSuccess;
  logger.log(Logger::INFO, "AWS MQTT: Attempting to unsubscribe from topic '%s'\n", topic);
  if (mqttClient->Unsubscribe(unsubPacket,
                              [&unsubscribeSuccess](int errorCode, std::shared_ptr<Aws::Crt::Mqtt5::UnSubAckPacket>)
                              {
                                if (errorCode == AWS_ERROR_SUCCESS)
                                {
                                  logger.log(Logger::INFO, "AWS MQTT: Successfully unsubscribed from topic\n");
                                  unsubscribeSuccess.set_value();
                                }
                                else
                                {
                                  logger.log(Logger::ERROR, "AWS MQTT: Failed to unsubscribe from topic with error: %s\n", aws_error_debug_str(errorCode));
                                  unsubscribeSuccess.set_value();
                                }
                              }))
    unsubscribeSuccess.get_future().wait();
}

bool AwsMQTT::publish(const char* __restrict topic, const uint8_t* __restrict payload, size_t payloadLength, uint8_t qos, bool retain)
{
  // Ensure that there is a valid MQTT client
  if (!mqttClient.get()) return false;

  // Create a publish object and packet
  auto publishPayload = Aws::Crt::ByteCursorFromArray(payload, payloadLength);
  auto publishPacket = Aws::Crt::MakeShared<Aws::Crt::Mqtt5::PublishPacket>(Aws::Crt::DefaultAllocatorImplementation());
  publishPacket->WithTopic(topic);
  publishPacket->WithPayload(publishPayload);
  publishPacket->WithQOS(static_cast<Aws::Crt::Mqtt5::QOS>(qos));
  publishPacket->WithRetain(retain);

  // Attempt to publish the packet
  std::promise<bool> publishSuccess;
  logger.log(Logger::DEBUG, "AWS MQTT: Attempting to publish data of size %zu to topic '%s'\n", payloadLength, topic);
  if (mqttClient->Publish(publishPacket,
                          [&publishSuccess](int, std::shared_ptr<Aws::Crt::Mqtt5::PublishResult> result)
                          {
                            if (result->wasSuccessful())
                            {
                              logger.log(Logger::DEBUG, "AWS MQTT: Successfully published message\n");
                              publishSuccess.set_value(true);
                            }
                            else
                            {
                              logger.log(Logger::ERROR, "AWS MQTT: Failed to publish message with error code: %d\n", result->getErrorCode());
                              publishSuccess.set_value(false);
                            }
                          }))
    return publishSuccess.get_future().get();
  return false;
}

std::pair<std::string, std::vector<uint8_t>> AwsMQTT::receive(void)
{
  // Wait for a published packet to be received or the client disconnected
  std::unique_lock<std::mutex> lock(receiveMutex);
  packetReceived.wait(lock, [this]() { return !isRunning || !receivedPackets.empty(); });
  if (!receivedPackets.empty())
  {
    auto packet = std::move(receivedPackets.front());
    receivedPackets.pop_front();
    return packet;
  }
  else
    return std::make_pair(std::string(), std::vector<uint8_t>());
}

void AwsMQTT::onAttemptingConnect(const Aws::Crt::Mqtt5::OnAttemptingConnectEventData&) { logger.log(Logger::INFO, "AWS MQTT: Attempting to connect to MQTT Broker...\n"); }

void AwsMQTT::onConnectionSuccess(const Aws::Crt::Mqtt5::OnConnectionSuccessEventData&)
{
  logger.log(Logger::INFO, "AWS MQTT: Successfully connected to MQTT Broker!\n");
  clientConnected.set_value(true);
  isRunning = true;
}

void AwsMQTT::onConnectionFailure(const Aws::Crt::Mqtt5::OnConnectionFailureEventData& eventData)
{
  logger.log(Logger::ERROR, "AWS MQTT: Connection to MQTT Broker failed with error: %s\n", aws_error_debug_str(eventData.errorCode));
  clientConnected.set_value(false);
}

void AwsMQTT::onDisconnection(const Aws::Crt::Mqtt5::OnDisconnectionEventData& eventData)
{
  if (eventData.disconnectPacket)
    logger.log(Logger::WARNING, "AWS MQTT: Disconnected from MQTT Broker with reason: %s\n", aws_error_debug_str(eventData.disconnectPacket->getReasonCode()));
  else
    logger.log(Logger::WARNING, "AWS MQTT: Disconnected from MQTT Broker\n");
}

void AwsMQTT::onPublishReceived(const Aws::Crt::Mqtt5::PublishReceivedEventData& eventData)
{
  if (eventData.publishPacket)
  {
    std::lock_guard<std::mutex> lock(receiveMutex);
    std::vector<uint8_t> payload(eventData.publishPacket->getPayload().len);
    memcpy(payload.data(), eventData.publishPacket->getPayload().ptr, eventData.publishPacket->getPayload().len);
    receivedPackets.push_back(std::make_pair(std::string(eventData.publishPacket->getTopic().c_str()), std::move(payload)));
  }
  packetReceived.notify_one();
}

void AwsMQTT::onClientStopped(const Aws::Crt::Mqtt5::OnStoppedEventData&)
{
  logger.log(Logger::INFO, "AWS MQTT: MQTT Client has stopped\n");
  clientStopped.set_value(true);
  isRunning = false;
}
