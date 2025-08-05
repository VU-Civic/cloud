use super::secrets::SecretManagerClient;
use rumqttc::{AsyncClient, EventLoop, MqttOptions, NetworkOptions, QoS, TlsConfiguration, Transport};
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
  #[allow(clippy::too_many_arguments)]
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

pub struct MqttClient<T> {
  client: AsyncClient,
  event_sender: Sender<T>,
  event_loop: Option<EventLoop>,
}

impl<T: Clone> MqttClient<T> {
  pub async fn new(secret_manager: &SecretManagerClient, settings: MqttSettings) -> Result<MqttClient<T>, String> {
    // Fetch the AWS credentials necessary to create the MQTT transport mechanism
    let secret = secret_manager.get_secret(&settings.credentials_key).await?;
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

    // Generate a set of MQTT options with the provided settings
    let mut mqtt_options = MqttOptions::new(settings.client_id, settings.aws_iot_endpoint, settings.port);
    mqtt_options.set_transport(transport);
    mqtt_options.set_keep_alive(Duration::from_secs(settings.keep_alive_seconds));
    mqtt_options.set_clean_session(settings.clean_session);

    // Create a new MQTT client and event loop with the specified options
    let (event_sender, _) = broadcast::channel::<T>(10000);
    let (client, mut event_loop) = AsyncClient::new(mqtt_options, 10000);
    let mut network_options = NetworkOptions::new();
    network_options.set_connection_timeout(settings.keep_alive_seconds);
    event_loop.set_network_options(network_options);

    // Return the MQTT client and corresponding event structures
    Ok(MqttClient {
      client,
      event_sender,
      event_loop: Some(event_loop),
    })
  }

  pub async fn disconnect(&self) -> Result<(), String> {
    self
      .client
      .disconnect()
      .await
      .map_err(|e| format!("MQTT: Failed to disconnect with error: {e}"))
  }

  #[must_use]
  pub fn get_sender(&self) -> Sender<T> {
    self.event_sender.clone()
  }

  #[must_use]
  pub fn get_receiver(&self) -> Receiver<T> {
    self.event_sender.subscribe()
  }

  #[must_use]
  pub fn take_event_loop(&mut self) -> EventLoop {
    self.event_loop.take().expect("MQTT: Event loop not initialized")
  }

  pub async fn subscribe(&self, topic: &str, qos: QoS) -> Result<(), String> {
    self.client.subscribe(topic, qos).await.map_err(|e| {
      format!(
        "MQTT: Failed to subscribe to topic '{topic}' at QoS '{}' with error: {e}",
        qos as i8
      )
    })
  }
}

#[cfg(test)]
mod tests {
  use crate::{EventInfo, aws, params};

  #[tokio::test(flavor = "multi_thread")]
  async fn test_mqtt() {
    // Create an MQTT client
    let secret_manager = aws::secrets::SecretManagerClient::new(&params::AWS_SDK_CONFIG);
    let settings = aws::mqtt::MqttSettings::new(
      params::MQTT_CLOUD_CLIENT_ID,
      params::MQTT_CREDENTIALS_KEY.as_str(),
      params::MQTT_CA_KEY.as_str(),
      params::MQTT_CLIENT_CERT_KEY.as_str(),
      params::MQTT_CLIENT_KEY_KEY.as_str(),
      params::MQTT_ENDPOINT.as_str(),
      params::MQTT_PORT.as_str().parse().unwrap_or(8883),
      params::MQTT_CLEAN_SESSION,
      params::MQTT_KEEP_ALIVE,
    );
    let mqtt_client = aws::mqtt::MqttClient::<EventInfo>::new(&secret_manager, settings)
      .await
      .expect("Failed to create MQTT client");
    mqtt_client
      .disconnect()
      .await
      .expect("Failed to disconnect MQTT client");
  }
}
