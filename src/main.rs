use chrono::{SecondsFormat, Utc};
use civicalert_cloud::{aws, bytes_to_alert_data, fusion::begin_fusion, params};
use log::{LevelFilter, error, info};
use rumqttc::{Event, Packet, QoS};
use std::{
  fs::OpenOptions,
  io::{IsTerminal, Write},
  sync::{
    Arc,
    atomic::{AtomicBool, Ordering},
  },
};

// Initialize logging to either the terminal or a log file depending on the environment
fn initialize_logging() {
  if std::io::stdout().is_terminal() {
    env_logger::Builder::from_env(
      env_logger::Env::default()
        .default_filter_or("info")
        .default_write_style_or("never"),
    )
    .format_timestamp_millis()
    .format_target(false)
    .init();
  } else {
    let target = Box::new(
      OpenOptions::new()
        .append(true)
        .create(true)
        .open(params::PROCESS_LOG_FILE_PATH)
        .expect("Unable to create the process log file"),
    );
    env_logger::Builder::new()
      .format(|buf, record| {
        writeln!(
          buf,
          "[{} {}] {}",
          Utc::now().to_rfc3339_opts(SecondsFormat::Millis, true),
          record.level(),
          record.args()
        )
      })
      .target(env_logger::Target::Pipe(target))
      .filter(None, LevelFilter::Info)
      .write_style(env_logger::WriteStyle::Never)
      .init();
  }
}

#[tokio::main(flavor = "multi_thread")]
async fn main() -> Result<(), String> {
  // Initialize logging
  initialize_logging();
  info!("CivicAlert Cloud Service starting...");

  // Handle graceful shutdown on reception of SIGINT, SIGTERM, or SIGHUP
  let mut mqtt: Option<aws::mqtt::MqttClient> = None;
  let running = Arc::new(AtomicBool::new(true));
  let running_clone = Arc::clone(&running);
  if let Err(e) = ctrlc::set_handler(move || {
    info!("Received process termination signal");
    running_clone.store(false, Ordering::SeqCst);
    if let Some(mqtt_client) = mqtt.as_ref() {
      info!("MQTT: Disconnecting client...");
      match tokio::runtime::Handle::current().block_on(async { mqtt_client.disconnect().await }) {
        Ok(()) => info!("MQTT: Client disconnected successfully"),
        Err(e) => error!("{e}"),
      }
    }
  }) {
    error!("Error setting up process termination handler: {e}");
    return Err(e.to_string());
  }

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
  mqtt = Some(match aws::mqtt::MqttClient::new(&secret_manager, mqtt_settings).await {
    Ok(mqtt) => mqtt,
    Err(e) => {
      error!("{e}");
      return Err(e);
    }
  });
  info!("MQTT client successfully configured");

  // Subscribe to relevant MQTT message topics
  info!("Subscribing to relevant MQTT topics...");
  if let Err(e) = mqtt
    .as_ref()
    .unwrap()
    .subscribe(params::MQTT_ALERTS_TOPIC.as_str(), QoS::AtLeastOnce)
    .await
  {
    error!("{e}");
    return Err(e);
  }
  info!("MQTT topic subscription complete");

  // Listen for incoming MQTT messages until process termination has been requested
  info!("CivicAlert Cloud Service is now running! Send SIGINT or SIGTERM to stop...");
  let mut event_loop = mqtt.as_mut().unwrap().take_event_loop();
  let event_sender = mqtt.as_ref().unwrap().get_sender();
  while running.load(Ordering::SeqCst) {
    match event_loop.poll().await {
      Ok(Event::Incoming(Packet::Publish(message))) => {
        if let Some(alert) = bytes_to_alert_data(&message.payload) {
          info!("MQTT: Received device alert: {alert}");
          let _ = event_sender.send(alert);
          let receiver = mqtt.as_ref().unwrap().get_receiver();
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
  info!("CivicAlert Cloud Service shutting down...");
  Ok(())
}

// TODO: logrotate or use CloudWatch (https://docs.aws.amazon.com/AmazonCloudWatch/latest/monitoring/CloudWatch-Agent-Configuration-File-Details.html)
// TODO: Ensure this is started with a process monitor to automatically restart on failure
// TODO: Move audio evidence collection onto a different EC2 instance (won't need S3 stuff here then)
// TODO: Update device-info table with the latest device information after every successful MQTT reception
