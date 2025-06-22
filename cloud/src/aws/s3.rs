use aws_config::SdkConfig;
use aws_sdk_s3::Client;

pub struct S3Client {
  client: Client,
}

impl S3Client {
  #[must_use]
  pub fn new(config: &SdkConfig) -> Self {
    let s3_config = aws_sdk_s3::config::Builder::from(config).build();
    S3Client {
      client: Client::from_conf(s3_config),
    }
  }

  pub async fn put_object(
    &self,
    bucket_name: &str,
    file_name: &str,
    content_type: &str,
    body: Vec<u8>,
  ) -> Result<(), String> {
    self
      .client
      .put_object()
      .bucket(bucket_name)
      .content_type(content_type)
      .key(file_name)
      .body(body.into())
      .send()
      .await
      .map_err(|e| {
        format!("AWS S3: PutObject failed on Bucket '{bucket_name}' for file '{file_name}' with error: {e}")
      })?;
    Ok(())
  }

  pub async fn get_object(&self, bucket_name: &str, file_name: &str) -> Result<Vec<u8>, String> {
    let resp = self
      .client
      .get_object()
      .bucket(bucket_name)
      .key(file_name)
      .send()
      .await
      .map_err(|e| {
        format!("AWS S3: GetObject failed on Bucket '{bucket_name}' for file '{file_name}' with error: {e}")
      })?;
    let body = resp
      .body
      .collect()
      .await
      .map_err(|e| format!("AWS S3: GetObject byte retrieval failed with error: {e}"))?;
    Ok(body.into_bytes().to_vec())
  }

  pub async fn delete_object(&self, bucket_name: &str, file_name: &str) -> Result<(), String> {
    self
      .client
      .delete_object()
      .bucket(bucket_name)
      .key(file_name)
      .send()
      .await
      .map_err(|e| {
        format!("AWS S3: DeleteObject failed on Bucket '{bucket_name}' for file '{file_name}' with error: {e}")
      })?;
    Ok(())
  }

  pub async fn list_objects(&self, bucket_name: &str) -> Result<Vec<String>, String> {
    let mut objects = Vec::new();
    let resp = self
      .client
      .list_objects_v2()
      .bucket(bucket_name)
      .send()
      .await
      .map_err(|e| format!("AWS S3: ListObjects failed on Bucket '{bucket_name}' with error: {e}"))?;
    for object in resp.contents() {
      if let Some(name) = object.key() {
        objects.push(name.to_string());
      }
    }
    Ok(objects)
  }
}

#[cfg(test)]
mod tests {
  use crate::{aws, params};

  #[tokio::test(flavor = "multi_thread")]
  async fn test_s3() {
    // Initialize an S3 client
    let s3 = aws::s3::S3Client::new(&params::AWS_SDK_CONFIG);

    // Create new object
    s3.put_object(
      params::S3_EVIDENCE_BUCKET.as_str(),
      "test.txt",
      "text/plain",
      b"Hello, S3!".to_vec(),
    )
    .await
    .unwrap();

    // List all objects
    let objects = s3.list_objects(params::S3_EVIDENCE_BUCKET.as_str()).await.unwrap();
    assert!(objects.contains(&"test.txt".to_string()));

    // Download the object
    let content = s3
      .get_object(params::S3_EVIDENCE_BUCKET.as_str(), "test.txt")
      .await
      .unwrap();
    assert_eq!(content, b"Hello, S3!");

    // Delete the object
    s3.delete_object(params::S3_EVIDENCE_BUCKET.as_str(), "test.txt")
      .await
      .unwrap();
  }
}
