use aws_config::SdkConfig;
use aws_sdk_s3::Client;

pub struct S3Client {
  client: Client,
}

impl S3Client {
  pub fn new(config: &SdkConfig) -> Self {
    let s3_config = aws_sdk_s3::config::Builder::from(config).build();
    S3Client {
      client: Client::from_conf(s3_config),
    }
  }

  pub async fn list_buckets(&self) -> Result<Vec<String>, String> {
    let mut buckets = Vec::new();
    let resp = self.client.list_buckets().send().await.map_err(|e| e.to_string())?;
    for bucket in resp.buckets() {
      if let Some(name) = bucket.name() {
        buckets.push(name.to_string());
      }
    }
    Ok(buckets)
  }

  pub async fn create_bucket(&self, bucket_name: &str) -> Result<(), String> {
    self
      .client
      .create_bucket()
      .bucket(bucket_name)
      .send()
      .await
      .map_err(|e| e.to_string())?;
    Ok(())
  }

  pub async fn delete_bucket(&self, bucket_name: &str) -> Result<(), String> {
    self
      .client
      .delete_bucket()
      .bucket(bucket_name)
      .send()
      .await
      .map_err(|e| e.to_string())?;
    Ok(())
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
      .map_err(|e| e.to_string())?;
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
      .map_err(|e| e.to_string())?;
    let body = resp.body.collect().await.map_err(|e| e.to_string())?;
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
      .map_err(|e| e.to_string())?;
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
      .map_err(|e| e.to_string())?;
    for object in resp.contents() {
      if let Some(name) = object.key() {
        objects.push(name.to_string());
      }
    }
    Ok(objects)
  }

  pub async fn upload_file(
    &self,
    bucket_name: &str,
    destination_file_name: &str,
    content_type: &str,
    file_path: &str,
  ) -> Result<(), String> {
    let body = tokio::fs::read(file_path).await.map_err(|e| e.to_string())?;
    self
      .put_object(bucket_name, destination_file_name, content_type, body)
      .await
  }

  pub async fn download_file(
    &self,
    bucket_name: &str,
    file_name: &str,
    destination_file_name: &str,
  ) -> Result<(), String> {
    let body = self.get_object(bucket_name, file_name).await?;
    tokio::fs::write(destination_file_name, body)
      .await
      .map_err(|e| e.to_string())?;
    Ok(())
  }
}

#[cfg(test)]
mod tests {
  use crate::aws::{aws_config, aws_s3};
  use crate::constants;

  #[tokio::test]
  async fn test_s3() {
    let config = aws_config::create().await;
    let s3 = aws_s3::S3Client::new(&config);

    // List buckets
    let buckets = s3.list_buckets().await.unwrap();
    println!("Buckets: {:?}", buckets);

    // Create new object
    s3.put_object(
      constants::AWS_S3_EVIDENCE_BUCKET,
      "test.txt",
      "text/plain",
      b"Hello, S3!".to_vec(),
    )
    .await
    .unwrap();

    // List all objects
    let objects = s3.list_objects(constants::AWS_S3_EVIDENCE_BUCKET).await.unwrap();
    assert!(objects.contains(&"test.txt".to_string()));

    // Download the object
    let content = s3
      .get_object(constants::AWS_S3_EVIDENCE_BUCKET, "test.txt")
      .await
      .unwrap();
    assert_eq!(content, b"Hello, S3!");

    // Delete the object
    s3.delete_object(constants::AWS_S3_EVIDENCE_BUCKET, "test.txt")
      .await
      .unwrap();
  }
}
