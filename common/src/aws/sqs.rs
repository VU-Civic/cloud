use aws_config::SdkConfig;
use aws_sdk_sqs::Client;

pub struct SqsClient {
  client: Client,
}

impl SqsClient {
  #[must_use]
  pub fn new(config: &SdkConfig) -> Self {
    let sqs_config = aws_sdk_sqs::config::Builder::from(config).build();
    SqsClient {
      client: Client::from_conf(sqs_config),
    }
  }

  pub async fn send_message(&self, queue_url: &str, message_body: &str) -> Result<(), String> {
    self
      .client
      .send_message()
      .queue_url(queue_url)
      .message_body(message_body)
      .send()
      .await
      .map_err(|e| format!("AWS SQS: SendMessage failed on Queue '{queue_url}' with error: {e}"))?;
    Ok(())
  }

  pub async fn receive_message(&self, queue_url: &str) -> Result<Option<(String, String)>, String> {
    let resp = self
      .client
      .receive_message()
      .queue_url(queue_url)
      .send()
      .await
      .map_err(|e| format!("AWS SQS: ReceiveMessage failed on Queue '{queue_url}' with error: {e}"))?;
    if let Some(message) = resp.messages.unwrap_or_default().into_iter().next() {
      let receipt_handle = message.receipt_handle.unwrap_or_default();
      let body = message.body.unwrap_or_default();
      Ok(Some((receipt_handle, body)))
    } else {
      Ok(None)
    }
  }

  pub async fn delete_message(&self, queue_url: &str, receipt_handle: &str) -> Result<(), String> {
    self
      .client
      .delete_message()
      .queue_url(queue_url)
      .receipt_handle(receipt_handle)
      .send()
      .await
      .map_err(|e| format!("AWS SQS: DeleteMessage failed on Queue '{queue_url}' with error: {e}"))?;
    Ok(())
  }

  pub async fn purge_queue(&self, queue_url: &str) -> Result<(), String> {
    self
      .client
      .purge_queue()
      .queue_url(queue_url)
      .send()
      .await
      .map_err(|e| format!("AWS SQS: PurgeQueue failed on Queue '{queue_url}' with error: {e}"))?;
    Ok(())
  }
}

#[cfg(test)]
mod tests {
  use crate::{aws, params};

  #[tokio::test(flavor = "multi_thread")]
  async fn test_sqs() {
    // Initialize an SQS client
    let sqs = aws::sqs::SqsClient::new(&params::AWS_SDK_CONFIG);
    let queue_url = "https://sqs.us-east-1.amazonaws.com/123456789012/my-queue"; // TODO: Put actual queue URL here

    // Test sending a message
    let send_result = sqs.send_message(queue_url, "Test message").await;
    assert!(send_result.is_ok());

    // Test receiving and deleting a message
    let receive_result = sqs.receive_message(queue_url).await;
    assert!(receive_result.is_ok());
    if let Some((receipt_handle, message)) = receive_result.unwrap() {
      println!("Received message: {}", message); // TODO: Compare to "Test message"
      let delete_result = sqs.delete_message(queue_url, &receipt_handle).await;
      assert!(delete_result.is_ok());
    }

    // Test purging the queue
    let purge_result = sqs.purge_queue(queue_url).await;
    assert!(purge_result.is_ok());
  }
}
