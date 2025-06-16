use aws_config::SdkConfig;
use aws_sdk_dynamodb::{Client, types::AttributeValue};
use std::collections::HashMap;

pub struct DynamoDbClient {
  client: Client,
}

impl DynamoDbClient {
  pub fn new(config: &SdkConfig) -> Self {
    let dynamodb_config = aws_sdk_dynamodb::config::Builder::from(config).build();
    DynamoDbClient {
      client: Client::from_conf(dynamodb_config),
    }
  }

  pub async fn list_tables(&self) -> Result<Vec<String>, String> {
    let mut tables = Vec::new();
    let resp = self.client.list_tables().send().await.map_err(|e| e.to_string())?;
    for table_name in resp.table_names() {
      tables.push(table_name.to_string());
    }
    Ok(tables)
  }

  pub async fn create_table(
    &self,
    table_name: &str,
    key_schema: aws_sdk_dynamodb::types::KeySchemaElement,
    attribute_definitions: aws_sdk_dynamodb::types::AttributeDefinition,
  ) -> Result<(), String> {
    self
      .client
      .create_table()
      .table_name(table_name)
      .key_schema(key_schema)
      .attribute_definitions(attribute_definitions)
      .send()
      .await
      .map_err(|e| e.to_string())?;
    Ok(())
  }

  pub async fn delete_table(&self, table_name: &str) -> Result<(), String> {
    self
      .client
      .delete_table()
      .table_name(table_name)
      .send()
      .await
      .map_err(|e| e.to_string())?;
    Ok(())
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
  ) -> Result<Vec<HashMap<String, AttributeValue>>, String> {
    let resp = self
      .client
      .query()
      .table_name(table_name)
      .key_condition_expression(key_condition_expression)
      .set_expression_attribute_values(Some(expression_attribute_values))
      .send()
      .await
      .map_err(|e| e.to_string())?;
    Ok(resp.items.unwrap_or_default())
  }
}

#[cfg(test)]
mod tests {
  use crate::aws::{aws_config, aws_db};
  use crate::constants;
  use std::collections::HashMap;

  #[tokio::test]
  async fn test_dynamo_db() {
    let config = aws_config::create().await;
    let db = aws_db::DynamoDbClient::new(&config);

    // List all tables
    let tables = db.list_tables().await.unwrap();
    println!("Tables: {:?}", tables);

    // Create a test table
    let table_name = "TestTable";
    let key_schema = aws_sdk_dynamodb::types::KeySchemaElement::builder()
      .attribute_name("id")
      .key_type(aws_sdk_dynamodb::types::KeyType::Hash)
      .build()
      .unwrap();
    let attribute_definitions = aws_sdk_dynamodb::types::AttributeDefinition::builder()
      .attribute_name("id")
      .attribute_type(aws_sdk_dynamodb::types::ScalarAttributeType::S)
      .build()
      .unwrap();
    db.create_table(table_name, key_schema, attribute_definitions)
      .await
      .unwrap();

    // Put an item in two tables
    let mut item = HashMap::new();
    item.insert(
      "id".to_string(),
      aws_sdk_dynamodb::types::AttributeValue::S("1".to_string()),
    );
    item.insert(
      "name".to_string(),
      aws_sdk_dynamodb::types::AttributeValue::S("Test Item".to_string()),
    );
    db.put_item(table_name, item.clone()).await.unwrap();
    db.put_item(constants::AWS_DYNAMODB_ERRORS_TABLE, item).await.unwrap();

    // Get the item from both tables
    let mut key = HashMap::new();
    key.insert(
      "id".to_string(),
      aws_sdk_dynamodb::types::AttributeValue::S("1".to_string()),
    );
    let retrieved_item1 = db.get_item(table_name, key.clone()).await.unwrap();
    let retrieved_item2 = db
      .get_item(constants::AWS_DYNAMODB_ERRORS_TABLE, key.clone())
      .await
      .unwrap();
    assert_eq!(retrieved_item1.get("name").unwrap().as_s().unwrap(), "Test Item");
    assert_eq!(retrieved_item2.get("name").unwrap().as_s().unwrap(), "Test Item");
    println!("Retrieved Item 1: {:?}", retrieved_item1);
    println!("Retrieved Item 2: {:?}", retrieved_item2);

    // Query items from the test table
    let key_condition_expression = "id = :id".to_string();
    let mut expression_attribute_values = HashMap::new();
    expression_attribute_values.insert(
      ":id".to_string(),
      aws_sdk_dynamodb::types::AttributeValue::S("1".to_string()),
    );
    let queried_items = db
      .query_items(
        table_name,
        key_condition_expression.clone(),
        expression_attribute_values.clone(),
      )
      .await
      .unwrap();
    assert_eq!(queried_items.len(), 1);
    assert_eq!(queried_items[0].get("name").unwrap().as_s().unwrap(), "Test Item");
    println!("Queried Items: {:?}", queried_items);

    // Query items from the errors table
    let queried_errors = db
      .query_items(
        constants::AWS_DYNAMODB_ERRORS_TABLE,
        key_condition_expression,
        expression_attribute_values,
      )
      .await
      .unwrap();
    assert_eq!(queried_errors.len(), 1);
    assert_eq!(queried_errors[0].get("name").unwrap().as_s().unwrap(), "Test Item");
    println!("Queried Errors: {:?}", queried_errors);

    // Delete the item from both tables
    db.delete_item(table_name, key.clone()).await.unwrap();
    db.delete_item(constants::AWS_DYNAMODB_ERRORS_TABLE, key).await.unwrap();

    // Deleting the temporary table
    db.delete_table(table_name).await.unwrap();
  }
}
