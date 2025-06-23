mod parser;

use civicalert_cloud_common::{EvidenceClip, aws, bytes_to_evidence_clip_data, params};
use rumqttc::{Event, Packet, QoS};
use std::{
  io::IsTerminal,
  sync::{
    Arc,
    atomic::{AtomicBool, Ordering},
  },
};
use tokio::sync::Mutex;
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
  info!("CivicAlert Evidence Parser starting...");

  // Initialize AWS clients and configurations
  info!("Configuring AWS clients...");
  let secret_manager = aws::secrets::SecretManagerClient::new(&params::AWS_SDK_CONFIG);
  let s3 = Arc::new(Mutex::new(aws::s3::S3Client::new(&params::AWS_SDK_CONFIG)));
  let db = Arc::new(Mutex::new(aws::db::DynamoDbClient::new(&params::AWS_SDK_CONFIG)));
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
    match aws::mqtt::MqttClient::<EvidenceClip>::new(&secret_manager, mqtt_settings).await {
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
  let running_clone = running.clone();
  let mqtt_clone = mqtt.clone();
  std::mem::drop(tokio::task::spawn_local(async move {
    let _ = tokio::signal::ctrl_c().await;
    info!("CivicAlert Evidence Parser shutting down...");
    running_clone.store(false, Ordering::SeqCst);
    let _ = mqtt_clone.lock().await.disconnect().await;
  }));

  // Subscribe to relevant MQTT message topics
  info!("Subscribing to relevant MQTT topics...");
  if let Err(e) = mqtt
    .lock()
    .await
    .subscribe(params::MQTT_AUDIO_EVIDENCE_TOPIC.as_str(), QoS::AtLeastOnce)
    .await
  {
    error!("{e}");
    return Err(e);
  }
  info!("MQTT topic subscription complete");

  // Initiate a task to send incoming evidence clips to the correct device parser
  let parser_sender = parser::create_clip_parser_task(s3, db);

  // Listen for incoming MQTT messages until process termination has been requested
  info!("CivicAlert Evidence Parser is now running! Send SIGINT to stop...");
  let mut event_loop = mqtt.lock().await.take_event_loop();
  while running.load(Ordering::SeqCst) {
    match event_loop.poll().await {
      Ok(Event::Incoming(Packet::Publish(message))) => {
        info!("MQTT: Received evidence clip segment from: {}", message.pkid); // TODO: Fix this to use the correct field (client ID)
        if let Some(clip) = bytes_to_evidence_clip_data(&message.payload) {
          let _ = parser_sender.send(clip).await;
        } else {
          error!("MQTT: Invalid evidence clip bytes");
        }
      }
      Err(error) => {
        error!("MQTT: Client error: {error}");
      }
      _ => {}
    }
  }

  // Cloud service has been gracefully shut down
  Ok(())
}

// TODO: Use CloudWatch (https://docs.aws.amazon.com/AmazonCloudWatch/latest/monitoring/CloudWatch-Agent-Configuration-File-Details.html)
// TODO: Ensure this is started with a process monitor to automatically restart on failure
