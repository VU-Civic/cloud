use aws_config::SdkConfig;
use aws_sdk_sns::Client;

pub struct SnsClient {
  client: Client,
}

impl SnsClient {
  #[must_use]
  pub fn new(config: &SdkConfig) -> Self {
    let sns_config = aws_sdk_sns::config::Builder::from(config).build();
    SnsClient {
      client: Client::from_conf(sns_config),
    }
  }

  pub async fn publish_message(&self, topic_arn: &str, message: &str) -> Result<(), String> {
    self
      .client
      .publish()
      .topic_arn(topic_arn)
      .message(message)
      .send()
      .await
      .map_err(|e| format!("AWS SNS: PublishMessage to Topic '{topic_arn}' failed with error: {e}"))?;
    Ok(())
  }
}

#[cfg(test)]
mod tests {
  use crate::{aws, params};

  #[tokio::test(flavor = "multi_thread")]
  async fn test_sns() {
    // Initialize an SnS client
    let sns = aws::sns::SnsClient::new(&params::AWS_SDK_CONFIG);

    // Publish a test message
    let message = "Hello, SNS!";
    let result = sns.publish_message(params::SNS_ALERT_TOPIC_ARN.as_str(), message).await;
    assert!(result.is_ok());
  }
}
