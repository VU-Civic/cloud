use civicalert_cloud::{aws, bytes_to_alert_data, fusion::begin_fusion, params};
use rumqttc::{Event, Packet, QoS};
use std::{
  io::IsTerminal,
  sync::{
    Arc, Mutex,
    atomic::{AtomicBool, Ordering},
  },
};
use tracing::{error, info};

#[tokio::main(flavor = "multi_thread")]
async fn main() -> Result<(), String> {
  // Initialize logging to stdout if a terminal is available, otherwise log to a file
  let (target, _guard) = if std::io::stdout().is_terminal() {
    tracing_appender::non_blocking(std::io::stdout())
  } else {
    let file_appender = tracing_appender::rolling::daily(params::PROCESS_LOG_FILE_DIR, params::PROCESS_LOG_FILE_NAME);
    tracing_appender::non_blocking(file_appender)
  };
  tracing_subscriber::fmt()
    .with_writer(target)
    .with_ansi(false)
    .with_target(false)
    .with_max_level(tracing_subscriber::filter::LevelFilter::INFO)
    .compact()
    .init();
  info!("CivicAlert Cloud Service starting...");

  // Initialize AWS clients and configurations
  info!("Configuring AWS clients...");
  let secret_manager = aws::secrets::SecretManagerClient::new(&params::AWS_SDK_CONFIG);
  let s3 = aws::s3::S3Client::new(&params::AWS_SDK_CONFIG);
  let db = aws::db::DynamoDbClient::new(&params::AWS_SDK_CONFIG);
  info!("AWS clients successfully configured");

  // Initialize MQTT client settings and create the client
  info!("Configuring MQTT client...");
  let mqtt_settings = aws::mqtt::MqttSettings::new(
    params::MQTT_CLIENT_ID,
    params::MQTT_CREDENTIALS_KEY.as_str(),
    params::MQTT_CA_KEY.as_str(),
    params::MQTT_CLIENT_CERT_KEY.as_str(),
    params::MQTT_CLIENT_KEY_KEY.as_str(),
    params::MQTT_ENDPOINT.as_str(),
    params::MQTT_PORT.as_str().parse().unwrap_or(8883),
    params::MQTT_CLEAN_SESSION,
    params::MQTT_KEEP_ALIVE,
  );
  let mqtt = Arc::new(Mutex::new(
    match aws::mqtt::MqttClient::new(&secret_manager, mqtt_settings).await {
      Ok(mqtt) => mqtt,
      Err(e) => {
        error!("{e}");
        return Err(e);
      }
    },
  ));
  info!("MQTT client successfully configured");

  // Handle graceful shutdown upon reception of a SIGINT
  let running = Arc::new(AtomicBool::new(true));
  let running_clone = Arc::clone(&running);
  let mqtt_clone = Arc::clone(&mqtt);
  std::mem::drop(tokio::task::spawn_local(async move {
    let _ = tokio::signal::ctrl_c().await;
    info!("CivicAlert Cloud Service shutting down...");
    running_clone.store(false, Ordering::SeqCst);
    let _ = mqtt_clone.lock().unwrap().disconnect().await;
  }));

  // Subscribe to relevant MQTT message topics
  info!("Subscribing to relevant MQTT topics...");
  if let Err(e) = mqtt
    .lock()
    .unwrap()
    .subscribe(params::MQTT_ALERTS_TOPIC.as_str(), QoS::AtLeastOnce)
    .await
  {
    error!("{e}");
    return Err(e);
  }
  info!("MQTT topic subscription complete");

  // Listen for incoming MQTT messages until process termination has been requested
  info!("CivicAlert Cloud Service is now running! Send SIGINT to stop...");
  let mut event_loop = mqtt.lock().unwrap().take_event_loop();
  let event_sender = mqtt.lock().unwrap().get_sender();
  while running.load(Ordering::SeqCst) {
    match event_loop.poll().await {
      Ok(Event::Incoming(Packet::Publish(message))) => {
        if let Some(alert) = bytes_to_alert_data(message.payload.as_ref()) {
          info!("MQTT: Received device alert: {alert}");
          let _ = event_sender.send(alert);
          let receiver = mqtt.lock().unwrap().get_receiver();
          std::mem::drop(tokio::spawn(async move { begin_fusion(receiver, alert).await }));
        } else {
          error!("MQTT: Invalid device alert bytes: {:?}", message.payload);
        }
      }
      Err(error) => {
        error!("MQTT: Client error: {error}");
      }
      _ => {}
    }
  }

  // Gracefully shut down the cloud service
  Ok(())
}

// TODO: logrotate or use CloudWatch (https://docs.aws.amazon.com/AmazonCloudWatch/latest/monitoring/CloudWatch-Agent-Configuration-File-Details.html)
// TODO: Ensure this is started with a process monitor to automatically restart on failure
// TODO: Move audio evidence collection onto a different EC2 instance (won't need S3 stuff here then)
// TODO: Update device-info table with the latest device information after every successful MQTT reception
