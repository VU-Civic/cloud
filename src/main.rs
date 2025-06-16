use civicalert_fusion::aws::{aws_config, aws_db, aws_mqtt, aws_s3};
use civicalert_fusion::constants;
use rumqttc::QoS;

#[tokio::main(flavor = "multi_thread")]
async fn main() -> Result<(), String> {
  // Initialize all AWS clients and configurations
  let aws_config = aws_config::create().await;
  let s3 = aws_s3::S3Client::new(&aws_config);
  let db = aws_db::DynamoDbClient::new(&aws_config);

  // Initialize the MQTT client with the AWS IoT settings
  let mqtt_settings = aws_mqtt::MqttSettings::new(
    constants::AWS_MQTT_CLIENT_ID,
    constants::AWS_MQTT_CA_PATH,
    constants::AWS_MQTT_CLIENT_CERT_PATH,
    constants::AWS_MQTT_CLIENT_KEY_PATH,
    constants::AWS_MQTT_ENDPOINT,
    constants::AWS_MQTT_PORT,
    constants::AWS_MQTT_CLEAN_SESSION,
    constants::AWS_MQTT_KEEP_ALIVE,
  );
  let mqtt = aws_mqtt::MqttClient::new(mqtt_settings).await?;

  // Subscribe to relevant MQTT device alert topics
  mqtt
    .subscribe(constants::MQTT_DEVICE_INFO_TOPIC, QoS::AtLeastOnce)
    .await?;
  mqtt.subscribe(constants::MQTT_ALERTS_TOPIC, QoS::AtLeastOnce).await?;
  mqtt
    .subscribe(constants::MQTT_AUDIO_EVIDENCE_TOPIC, QoS::AtMostOnce)
    .await?;

  // JUST AN EXAMPLE OF WHAT WE'LL BE DOING HERE (each time a message comes in, we start a new thread with the event and also send the event to all existing threads)
  /*{
    let mut receiver = mqtt.get_receiver().await;
    tokio::spawn(async move {
      loop {
        if let Ok(event) = receiver.recv().await {
          match event {
            Packet::Publish(p) => {
              println!("Received message {:?} on topic: {}", p.payload, p.topic)
            }
            _ => println!("Got event on receiver1: {:?}", event),
          }
        }
      }
    });
  }*/

  Ok(())
}
