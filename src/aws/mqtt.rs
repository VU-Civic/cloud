use super::secrets::SecretManagerClient;
use rumqttc::{AsyncClient, MqttOptions, QoS, TlsConfiguration, Transport};
use rumqttc::{Event, EventLoop, NetworkOptions, Packet};
use std::time::Duration;
use tokio::sync::broadcast::{self, Receiver, Sender};

pub struct MqttSettings {
  client_id: String,
  credentials_key: String,
  ca_key: String,
  client_cert_key: String,
  client_key_key: String,
  aws_iot_endpoint: String,
  port: u16,
  clean_session: bool,
  keep_alive_seconds: u64,
}

impl MqttSettings {
  #[must_use]
  pub fn new(
    client_id: &str,
    credentials_key: &str,
    ca_key: &str,
    client_cert_key: &str,
    client_key_key: &str,
    aws_iot_endpoint: &str,
    port: u16,
    clean_session: bool,
    keep_alive_seconds: u64,
  ) -> MqttSettings {
    MqttSettings {
      client_id: String::from(client_id),
      credentials_key: String::from(credentials_key),
      ca_key: String::from(ca_key),
      client_cert_key: String::from(client_cert_key),
      client_key_key: String::from(client_key_key),
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
    self.listen_thread.abort();
  }
}

impl MqttClient {
  pub async fn new(secret_manager: &SecretManagerClient, settings: MqttSettings) -> Result<MqttClient, String> {
    // Fetch the AWS credentials necessary to create the transport mechanism
    let secret = secret_manager
      .get_secret(&settings.credentials_key)
      .await
      .map_err(|e| format!("Failed to get secret: {e}"))?;
    let ca = secret_manager
      .extract_secret_value(&secret, &settings.ca_key)?
      .replace("\\n", "\n")
      .as_bytes()
      .to_owned();
    let client_cert = secret_manager
      .extract_secret_value(&secret, &settings.client_cert_key)?
      .replace("\\n", "\n")
      .as_bytes()
      .to_owned();
    let client_key = secret_manager
      .extract_secret_value(&secret, &settings.client_key_key)?
      .replace("\\n", "\n")
      .as_bytes()
      .to_owned();
    let transport = Transport::Tls(TlsConfiguration::Simple {
      ca,
      alpn: Some(vec!["mqtt".into()]),
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

  pub async fn disconnect(&self) -> Result<(), String> {
    self
      .client
      .disconnect()
      .await
      .map_err(|e| format!("Failed to disconnect MQTT client: {e}"))
  }

  #[must_use]
  pub fn get_receiver(&self) -> Receiver<Packet> {
    self.event_sender.subscribe()
  }

  pub async fn subscribe<S: Into<String>>(&self, topic: S, qos: QoS) -> Result<(), String> {
    self
      .client
      .subscribe(topic, qos)
      .await
      .map_err(|e| format!("Failed to subscribe: {e:?}"))
  }

  async fn mqtt_listener(mut event_loop: EventLoop, event_sender: Sender<Packet>) -> Result<(), i32> {
    loop {
      match event_loop.poll().await {
        Ok(Event::Incoming(event)) => {
          let _ = event_sender.send(event);
        }
        Err(error) => {
          println!("AWS MQTT client error: {error:?}");
        }
        _ => {}
      }
    }
  }
}

#[cfg(test)]
mod tests {
  use crate::{aws, constants};

  #[tokio::test]
  async fn test_mqtt() {
    // Create an MQTT client
    let config = aws::config::create().await.expect("Failed to create AWS config");
    let secret_manager = aws::secrets::SecretManagerClient::new(&config);
    let settings = aws::mqtt::MqttSettings::new(
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
    let mqtt_client = aws::mqtt::MqttClient::new(&secret_manager, settings)
      .await
      .expect("Failed to create MQTT client");
    mqtt_client
      .disconnect()
      .await
      .expect("Failed to disconnect MQTT client");
  }
}
