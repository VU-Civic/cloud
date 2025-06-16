use crate::constants;
use aws_config::meta::region::RegionProviderChain;
use aws_config::{BehaviorVersion, SdkConfig};
use aws_credential_types::provider::ProvideCredentials;

pub async fn create() -> SdkConfig {
  let config = aws_config::defaults(BehaviorVersion::latest())
    .region(RegionProviderChain::default_provider().or_else(constants::AWS_REGION))
    .load()
    .await;
  let provider = aws_config::sts::AssumeRoleProvider::builder(constants::AWS_IAM_ROLE)
    .configure(&config)
    .build()
    .await;
  let credentials = provider.provide_credentials().await.unwrap();
  aws_config::defaults(BehaviorVersion::latest())
    .region(RegionProviderChain::default_provider().or_else(constants::AWS_REGION))
    .credentials_provider(credentials)
    .load()
    .await
}
