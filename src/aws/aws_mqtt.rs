use rumqttc::{AsyncClient, MqttOptions, QoS, TlsConfiguration, Transport};
use rumqttc::{Event, EventLoop, NetworkOptions, Packet};
use std::time::Duration;
use tokio::sync::broadcast::{self, Receiver, Sender};

pub struct MqttSettings {
  client_id: String,
  ca_path: String,
  client_cert_path: String,
  client_key_path: String,
  aws_iot_endpoint: String,
  port: u16,
  clean_session: bool,
  keep_alive_seconds: u64,
}

impl MqttSettings {
  pub fn new(
    client_id: &str,
    ca_path: &str,
    client_cert_path: &str,
    client_key_path: &str,
    aws_iot_endpoint: &str,
    port: u16,
    clean_session: bool,
    keep_alive_seconds: u64,
  ) -> MqttSettings {
    MqttSettings {
      client_id: String::from(client_id),
      ca_path: String::from(ca_path),
      client_cert_path: String::from(client_cert_path),
      client_key_path: String::from(client_key_path),
      aws_iot_endpoint: String::from(aws_iot_endpoint),
      port,
      clean_session,
      keep_alive_seconds,
    }
  }
}

pub struct MqttClient {
  client: AsyncClient,
  event_sender: Sender<Packet>,
  listen_thread: tokio::task::JoinHandle<()>,
}

impl Drop for MqttClient {
  fn drop(&mut self) {
    let _ = self.client.disconnect();
    let _ = self.listen_thread.abort();
  }
}

impl MqttClient {
  pub async fn new(settings: MqttSettings) -> Result<MqttClient, String> {
    // Read the AWS certificates and credentials and create the necessary transport mechanism
    let ca = tokio::fs::read(&settings.ca_path)
      .await
      .map_err(|e| format!("Failed to read CA file: {}", e))?;
    let client_cert = tokio::fs::read(&settings.client_cert_path)
      .await
      .map_err(|e| format!("Failed to read client certificate file: {}", e))?;
    let client_key = tokio::fs::read(&settings.client_key_path)
      .await
      .map_err(|e| format!("Failed to read client key file: {}", e))?;
    let transport = Transport::Tls(TlsConfiguration::Simple {
      ca,
      alpn: None,
      client_auth: Some((client_cert, client_key)),
    });

    // Create the MQTT options with the provided settings
    let mut mqtt_options = MqttOptions::new(settings.client_id, settings.aws_iot_endpoint, settings.port);
    mqtt_options.set_transport(transport);
    mqtt_options.set_keep_alive(Duration::from_secs(settings.keep_alive_seconds));
    mqtt_options.set_clean_session(settings.clean_session);

    // Create a new MQTT client and event loop with the specified options
    let (client, mut event_loop) = AsyncClient::new(mqtt_options, 10000);
    let mut network_options = NetworkOptions::new();
    network_options.set_connection_timeout(settings.keep_alive_seconds);
    event_loop.set_network_options(network_options);

    // Create a new async task to listen for incoming MQTT messages
    let (event_sender, _) = broadcast::channel(10000);
    let event_sender_clone = event_sender.clone();
    let listen_thread = tokio::spawn(async move {
      MqttClient::mqtt_listener(event_loop, event_sender_clone).await.unwrap();
    });
    Ok(MqttClient {
      client,
      event_sender,
      listen_thread,
    })
  }

  pub async fn subscribe<S: Into<String>>(&self, topic: S, qos: QoS) -> Result<(), String> {
    self
      .client
      .subscribe(topic, qos)
      .await
      .map_err(|e| format!("Failed to subscribe: {:?}", e))
  }

  pub async fn get_receiver(&self) -> Receiver<Packet> {
    self.event_sender.subscribe()
  }

  async fn mqtt_listener(mut event_loop: EventLoop, event_sender: Sender<Packet>) -> Result<(), i32> {
    loop {
      match event_loop.poll().await {
        Ok(Event::Incoming(event)) => {
          let _ = event_sender.send(event);
        }
        Err(error) => {
          println!("AWS MQTT client error: {:?}", error);
        }
        _ => {}
      }
    }
  }
}
