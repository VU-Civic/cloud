use civicalert_cloud::{aws, constants};
use rumqttc::QoS;

#[tokio::main(flavor = "multi_thread")]
async fn main() -> Result<(), String> {
  // Initialize AWS clients and configurations
  let aws_config = aws::config::create().await?;
  let secret_manager = aws::secrets::SecretManagerClient::new(&aws_config);
  let s3 = aws::s3::S3Client::new(&aws_config);
  let db = aws::db::DynamoDbClient::new(&aws_config);

  // Initialize MQTT client settings and create the client
  let mqtt_settings = aws::mqtt::MqttSettings::new(
    constants::MQTT_CLIENT_ID,
    constants::MQTT_CREDENTIALS_KEY.lock().unwrap().as_str(),
    constants::MQTT_CA_KEY.lock().unwrap().as_str(),
    constants::MQTT_CLIENT_CERT_KEY.lock().unwrap().as_str(),
    constants::MQTT_CLIENT_KEY_KEY.lock().unwrap().as_str(),
    constants::MQTT_ENDPOINT.lock().unwrap().as_str(),
    u16::from_str_radix(constants::MQTT_PORT.lock().unwrap().as_str(), 10).unwrap_or(8883),
    constants::MQTT_CLEAN_SESSION,
    constants::MQTT_KEEP_ALIVE,
  );
  let mqtt = aws::mqtt::MqttClient::new(&secret_manager, mqtt_settings).await?;

  // Subscribe to relevant MQTT message topics
  mqtt
    .subscribe(
      constants::MQTT_DEVICE_INFO_TOPIC.lock().unwrap().as_str(),
      QoS::AtLeastOnce,
    )
    .await?;
  mqtt
    .subscribe(constants::MQTT_ALERTS_TOPIC.lock().unwrap().as_str(), QoS::AtLeastOnce)
    .await?;
  mqtt
    .subscribe(
      constants::MQTT_AUDIO_EVIDENCE_TOPIC.lock().unwrap().as_str(),
      QoS::AtMostOnce,
    )
    .await?; // TODO: Move audio evidence collection onto a different EC2 instance

  // JUST AN EXAMPLE OF WHAT WE'LL BE DOING HERE (each time a message comes in, we start a new thread with the event and also send the event to all existing threads)
  /*{
    let mut receiver = mqtt.get_receiver();
    tokio::spawn(async move {
      loop {
        if let Ok(Packet::Publish(packet)) = receiver.recv().await {
          println!("Received message {:?} on topic: {}", packet.payload, packet.topic)
        }
      }
    });
  }*/

  // TODO: Need to have some sort of watchdog write to DynamoDB every 4 minutes. Set AWS alert to trigger if no writes in past 5 minutes.

  // Gracefully shutdown the MQTT client before exiting
  mqtt.disconnect().await?;
  Ok(())
}
