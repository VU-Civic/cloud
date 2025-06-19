use aws_config::SdkConfig;
use aws_sdk_dynamodb::{Client, types::AttributeValue};
use std::collections::HashMap;

pub struct DynamoDbClient {
  client: Client,
}

impl DynamoDbClient {
  #[must_use]
  pub fn new(config: &SdkConfig) -> Self {
    let dynamodb_config = aws_sdk_dynamodb::config::Builder::from(config).build();
    DynamoDbClient {
      client: Client::from_conf(dynamodb_config),
    }
  }

  pub async fn put_item(&self, table_name: &str, item: HashMap<String, AttributeValue>) -> Result<(), String> {
    self
      .client
      .put_item()
      .table_name(table_name)
      .set_item(Some(item))
      .send()
      .await
      .map_err(|e| e.to_string())?;
    Ok(())
  }

  pub async fn get_item(
    &self,
    table_name: &str,
    key: HashMap<String, AttributeValue>,
  ) -> Result<HashMap<String, AttributeValue>, String> {
    let resp = self
      .client
      .get_item()
      .table_name(table_name)
      .set_key(Some(key))
      .send()
      .await
      .map_err(|e| e.to_string())?;
    Ok(resp.item.unwrap_or_default())
  }

  pub async fn delete_item(&self, table_name: &str, key: HashMap<String, AttributeValue>) -> Result<(), String> {
    self
      .client
      .delete_item()
      .table_name(table_name)
      .set_key(Some(key))
      .send()
      .await
      .map_err(|e| e.to_string())?;
    Ok(())
  }

  pub async fn query_items(
    &self,
    table_name: &str,
    key_condition_expression: String,
    expression_attribute_values: HashMap<String, AttributeValue>,
    expression_attribute_names: Option<HashMap<String, String>>,
  ) -> Result<Vec<HashMap<String, AttributeValue>>, String> {
    let resp = self
      .client
      .query()
      .table_name(table_name)
      .key_condition_expression(key_condition_expression)
      .set_expression_attribute_values(Some(expression_attribute_values))
      .set_expression_attribute_names(expression_attribute_names)
      .send()
      .await
      .map_err(|e| e.to_string())?;
    Ok(resp.items.unwrap_or_default())
  }
}

#[cfg(test)]
mod tests {
  use crate::{aws, constants};
  use std::collections::HashMap;

  #[tokio::test]
  async fn test_db() {
    let config = aws::config::create().await.expect("Failed to create AWS config");
    let db = aws::db::DynamoDbClient::new(&config);

    // Put an item into a table
    let mut item = HashMap::new();
    item.insert(
      "timestamp".to_string(),
      aws_sdk_dynamodb::types::AttributeValue::N("1234567".to_string()),
    );
    item.insert(
      "name".to_string(),
      aws_sdk_dynamodb::types::AttributeValue::S("Test Item".to_string()),
    );
    db.put_item(constants::DYNAMODB_ERRORS_TABLE.lock().unwrap().as_str(), item)
      .await
      .unwrap();

    // Get the item from the table
    let mut key = HashMap::new();
    key.insert(
      "timestamp".to_string(),
      aws_sdk_dynamodb::types::AttributeValue::N("1234567".to_string()),
    );
    let retrieved_item = db
      .get_item(constants::DYNAMODB_ERRORS_TABLE.lock().unwrap().as_str(), key.clone())
      .await
      .unwrap();
    assert_eq!(retrieved_item.get("name").unwrap().as_s().unwrap(), "Test Item");

    // Query items from the table
    let key_condition_expression = "#ts = :ts".to_string();
    let mut expression_attribute_values = HashMap::new();
    expression_attribute_values.insert(
      ":ts".to_string(),
      aws_sdk_dynamodb::types::AttributeValue::N("1234567".to_string()),
    );
    let queried_items = db
      .query_items(
        constants::DYNAMODB_ERRORS_TABLE.lock().unwrap().as_str(),
        key_condition_expression,
        expression_attribute_values,
        Some(HashMap::from([("#ts".to_string(), "timestamp".to_string())])),
      )
      .await
      .unwrap();
    assert_eq!(queried_items.len(), 1);
    assert_eq!(queried_items[0].get("name").unwrap().as_s().unwrap(), "Test Item");

    // Delete the item from the table
    db.delete_item(constants::DYNAMODB_ERRORS_TABLE.lock().unwrap().as_str(), key)
      .await
      .unwrap();
  }
}
