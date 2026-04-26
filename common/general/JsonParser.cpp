#include <cstdio>
#include <cstring>
#include "JsonParser.h"

// Tokenizer JSON characters
static const std::string validJsonValueChars(".-+eE0123456789auflnrstAUFLNRST");
static const std::string invalidJsonValueStartingChars(" \t\f\r\n");

// One-line stringifier template
template <typename S, typename T, class = typename std::enable_if<std::is_base_of<std::basic_ostream<typename S::char_type, typename S::traits_type>, S>::value>::type>
S&& operator<<(S&& out, const T& t)
{
  static_cast<std::basic_ostream<typename S::char_type, typename S::traits_type>&>(out) << t;
  return std::move(out);
}

JsonValue& JsonObject::operator[](const char* key)
{
  auto [it, inserted] = keyValuePairs.try_emplace(key, std::unique_ptr<JsonValue>(new JsonValue()));
  if (inserted) insertionOrder.emplace_back(key);
  return *it->second;
}

JsonValue& JsonObject::operator[](const JsonString& key)
{
  auto [it, inserted] = keyValuePairs.try_emplace(key, std::unique_ptr<JsonValue>(new JsonValue()));
  if (inserted) insertionOrder.emplace_back(key);
  return *it->second;
}

JsonValue& JsonObject::operator[](JsonString&& key)
{
  auto [it, inserted] = keyValuePairs.try_emplace(std::move(key), std::unique_ptr<JsonValue>(new JsonValue()));
  if (inserted) insertionOrder.emplace_back(it->first);
  return *it->second;
}

JsonValue& JsonObject::emplace(const char* key, JsonValue&& value)
{
  auto [it, inserted] = keyValuePairs.try_emplace(key, std::unique_ptr<JsonValue>(new JsonValue(std::move(value))));
  if (inserted) insertionOrder.emplace_back(key);
  return *it->second;
}

JsonValue& JsonObject::emplace(const JsonString& key, JsonValue&& value)
{
  auto [it, inserted] = keyValuePairs.try_emplace(key, std::unique_ptr<JsonValue>(new JsonValue(std::move(value))));
  if (inserted) insertionOrder.emplace_back(key);
  return *it->second;
}

JsonValue& JsonObject::emplace(JsonString&& key, JsonValue&& value)
{
  auto [it, inserted] = keyValuePairs.try_emplace(std::move(key), std::unique_ptr<JsonValue>(new JsonValue(std::move(value))));
  if (inserted) insertionOrder.emplace_back(it->first);
  return *it->second;
}

void JsonObject::remove(const char* key)
{
  if (keyValuePairs.erase(key))
  {
    for (auto insertionLocation = std::begin(insertionOrder); insertionLocation != std::end(insertionOrder); ++insertionLocation)
      if (insertionLocation->compare(key) == 0)
      {
        insertionOrder.erase(insertionLocation);
        break;
      }
  }
}

void JsonObject::remove(const JsonString& key)
{
  if (keyValuePairs.erase(key))
  {
    for (auto insertionLocation = std::begin(insertionOrder); insertionLocation != std::end(insertionOrder); ++insertionLocation)
      if (insertionLocation->compare(key) == 0)
      {
        insertionOrder.erase(insertionLocation);
        break;
      }
  }
}

JsonObject::iterator JsonObject::remove(JsonObject::iterator pos)
{
  std::string key((*pos).first);
  std::vector<std::string>::iterator insertionIterator(std::end(insertionOrder));
  keyValuePairs.erase(key);
  for (auto insertionLocation = std::begin(insertionOrder); insertionLocation != std::end(insertionOrder); ++insertionLocation)
    if (insertionLocation->compare(key) == 0)
    {
      insertionIterator = insertionLocation;
      break;
    }
  return iterator(&keyValuePairs, insertionOrder.erase(insertionIterator));
}

JsonObject::iterator JsonObject::remove(JsonObject::const_iterator pos)
{
  std::string key((*pos).first);
  std::vector<std::string>::iterator insertionIterator(std::end(insertionOrder));
  keyValuePairs.erase(key);
  for (auto insertionLocation = std::begin(insertionOrder); insertionLocation != std::end(insertionOrder); ++insertionLocation)
    if (insertionLocation->compare(key) == 0)
    {
      insertionIterator = insertionLocation;
      break;
    }
  return iterator(&keyValuePairs, insertionOrder.erase(insertionIterator));
}

JsonValue::JsonValue(JsonValue&& other) : type(other.type), integerValue(0)
{
  // Perform the assignment based on type
  switch (type)
  {
    case JsonValueType::Boolean:
      booleanValue = other.booleanValue;
      break;
    case JsonValueType::Float:
      floatValue = other.floatValue;
      break;
    case JsonValueType::Integer:
      integerValue = other.integerValue;
      break;
    case JsonValueType::String:
      new (&stringValue) JsonString(std::move(other.stringValue));
      break;
    case JsonValueType::Object:
      new (&objectValue) JsonObject(std::move(other.objectValue));
      break;
    case JsonValueType::Array:
      new (&arrayValue) JsonArray(std::move(other.arrayValue));
      break;
    default:
      break;
  }
}

JsonValue& JsonValue::operator=(JsonValue&& other)
{
  // Handle non-trivial type assignments
  switch (type)
  {
    case JsonValueType::String:
    {
      if (other.type == JsonValueType::String)
      {
        stringValue = std::move(other.stringValue);
        return *this;
      }
      stringValue.~JsonString();
      break;
    }
    case JsonValueType::Object:
    {
      if (other.type == JsonValueType::Object)
      {
        objectValue = std::move(other.objectValue);
        return *this;
      }
      objectValue.~JsonObject();
      break;
    }
    case JsonValueType::Array:
    {
      if (other.type == JsonValueType::Array)
      {
        arrayValue = std::move(other.arrayValue);
        return *this;
      }
      arrayValue.~JsonArray();
      break;
    }
    default:
      break;
  }

  // Perform the actual assignment
  switch (other.type)
  {
    case JsonValueType::Boolean:
      booleanValue = other.booleanValue;
      break;
    case JsonValueType::Float:
      floatValue = other.floatValue;
      break;
    case JsonValueType::Integer:
      integerValue = other.integerValue;
      break;
    case JsonValueType::String:
      new (&stringValue) JsonString(std::move(other.stringValue));
      break;
    case JsonValueType::Object:
      new (&objectValue) JsonObject(std::move(other.objectValue));
      break;
    case JsonValueType::Array:
      new (&arrayValue) JsonArray(std::move(other.arrayValue));
      break;
    default:
      break;
  }
  *const_cast<JsonValueType*>(&type) = other.type;
  return *this;
}

JsonValue::~JsonValue(void)
{
  // Clean up memory based on the type
  switch (type)
  {
    case JsonValueType::String:
      stringValue.~JsonString();
      break;
    case JsonValueType::Object:
      objectValue.~JsonObject();
      break;
    case JsonValueType::Array:
      arrayValue.~JsonArray();
      break;
    default:
      break;
  }
}

std::string JsonValue::asString(bool prettyPrint, bool stringifyAllValues) const
{
  switch (type)
  {
    case JsonValueType::Boolean:
      return (booleanValue ? "true" : "false");
    case JsonValueType::Float:
    {
      char buf[32];
      snprintf(buf, sizeof(buf), "%.15g", floatValue);
      return buf;
    }
    case JsonValueType::Integer:
      return std::to_string(integerValue);
    case JsonValueType::Null:
      return "null";
    case JsonValueType::Object:
      return JsonParser::printCompositeString(objectValue, true, prettyPrint, stringifyAllValues, 0);
    case JsonValueType::String:
      return stringValue;
    case JsonValueType::Array:
      return JsonParser::printArrayString(arrayValue, true, prettyPrint, stringifyAllValues, 0);
    default:
      return std::string();
  }
}

int JsonValue::asInteger(void) const
{
  switch (type)
  {
    case JsonValueType::Boolean:
      return (booleanValue ? 1 : 0);
    case JsonValueType::Float:
      return static_cast<int>(floatValue);
    case JsonValueType::Integer:
      return static_cast<int>(integerValue);
    case JsonValueType::Null:
      return 0;
    case JsonValueType::Object:
      return 1;
    case JsonValueType::String:
      return static_cast<int>(strtol(stringValue.c_str(), nullptr, 0));
    case JsonValueType::Array:
      return (arrayValue.size() > 0) ? arrayValue.front().asInteger() : 0;
    default:
      return 0;
  }
}

unsigned int JsonValue::asUnsigned(void) const
{
  switch (type)
  {
    case JsonValueType::Boolean:
      return (booleanValue ? 1u : 0u);
    case JsonValueType::Float:
      return static_cast<unsigned int>(floatValue);
    case JsonValueType::Integer:
      return static_cast<unsigned int>(integerValue);
    case JsonValueType::Null:
      return 0u;
    case JsonValueType::Object:
      return 1u;
    case JsonValueType::String:
      return static_cast<unsigned int>(strtoul(stringValue.c_str(), nullptr, 0));
    case JsonValueType::Array:
      return (arrayValue.size() > 0) ? arrayValue.front().asUnsigned() : 0u;
    default:
      return 0u;
  }
}

double JsonValue::asFloat(void) const
{
  switch (type)
  {
    case JsonValueType::Boolean:
      return (booleanValue ? 1.0 : 0.0);
    case JsonValueType::Float:
      return floatValue;
    case JsonValueType::Integer:
      return static_cast<double>(integerValue);
    case JsonValueType::Null:
      return 0.0;
    case JsonValueType::Object:
      return 1.0;
    case JsonValueType::String:
      return strtod(stringValue.c_str(), nullptr);
    case JsonValueType::Array:
      return (arrayValue.size() > 0) ? arrayValue.front().asFloat() : 0.0;
    default:
      return 0.0;
  }
}

bool JsonValue::asBoolean(void) const
{
  switch (type)
  {
    case JsonValueType::Boolean:
      return booleanValue;
    case JsonValueType::Float:
      return (floatValue > 0.000000001);
    case JsonValueType::Integer:
      return (integerValue != 0);
    case JsonValueType::Null:
      return false;
    case JsonValueType::Object:
      return true;
    case JsonValueType::String:
      return (strcasecmp(stringValue.c_str(), "true") == 0);
    case JsonValueType::Array:
      return (arrayValue.size() > 0) ? arrayValue.front().asBoolean() : false;
    default:
      return false;
  }
}

JsonObject* JsonValue::asObject(void) const
{
  switch (type)
  {
    case JsonValueType::Boolean:
      return nullptr;
    case JsonValueType::Float:
      return nullptr;
    case JsonValueType::Integer:
      return nullptr;
    case JsonValueType::Null:
      return nullptr;
    case JsonValueType::Object:
      return const_cast<JsonObject*>(&objectValue);
    case JsonValueType::String:
      return nullptr;
    case JsonValueType::Array:
      return nullptr;
    default:
      return nullptr;
  }
}

std::vector<std::string> JsonValue::asStringArray(bool prettyPrint, bool stringifyAllValues) const
{
  switch (type)
  {
    case JsonValueType::Boolean:
      return std::vector<std::string>{(booleanValue ? "true" : "false")};
    case JsonValueType::Float:
    {
      char buf[32];
      snprintf(buf, sizeof(buf), "%.15g", floatValue);
      return std::vector<std::string>{buf};
    }
    case JsonValueType::Integer:
      return std::vector<std::string>{std::to_string(integerValue)};
    case JsonValueType::Null:
      return std::vector<std::string>{};
    case JsonValueType::Object:
      return std::vector<std::string>{JsonParser::printCompositeString(objectValue, true, prettyPrint, stringifyAllValues, 0)};
    case JsonValueType::String:
      return std::vector<std::string>{stringValue};
    case JsonValueType::Array:
    {
      std::vector<std::string> retArray;
      for (const auto& val : arrayValue) retArray.emplace_back(val.asString());
      return retArray;
    }
    default:
      return std::vector<std::string>{};
  }
}

std::vector<int> JsonValue::asIntegerArray(void) const
{
  switch (type)
  {
    case JsonValueType::Boolean:
      return std::vector<int>{booleanValue ? 1 : 0};
    case JsonValueType::Float:
      return std::vector<int>{static_cast<int>(floatValue)};
    case JsonValueType::Integer:
      return std::vector<int>{static_cast<int>(integerValue)};
    case JsonValueType::Null:
      return std::vector<int>{};
    case JsonValueType::Object:
      return std::vector<int>{};
    case JsonValueType::String:
      return std::vector<int>{static_cast<int>(strtol(stringValue.c_str(), nullptr, 0))};
    case JsonValueType::Array:
    {
      std::vector<int> retArray;
      for (const auto& val : arrayValue) retArray.emplace_back(val.asInteger());
      return retArray;
    }
    default:
      return std::vector<int>{};
  }
}

std::vector<unsigned int> JsonValue::asUnsignedArray(void) const
{
  switch (type)
  {
    case JsonValueType::Boolean:
      return std::vector<unsigned int>{booleanValue ? 1u : 0u};
    case JsonValueType::Float:
      return std::vector<unsigned int>{static_cast<unsigned int>(floatValue)};
    case JsonValueType::Integer:
      return std::vector<unsigned int>{static_cast<unsigned int>(integerValue)};
    case JsonValueType::Null:
      return std::vector<unsigned int>{};
    case JsonValueType::Object:
      return std::vector<unsigned int>{};
    case JsonValueType::String:
      return std::vector<unsigned int>{static_cast<unsigned int>(strtoul(stringValue.c_str(), nullptr, 0))};
    case JsonValueType::Array:
    {
      std::vector<unsigned int> retArray;
      for (const auto& val : arrayValue) retArray.emplace_back(val.asUnsigned());
      return retArray;
    }
    default:
      return std::vector<unsigned int>{};
  }
}

std::vector<double> JsonValue::asFloatArray(void) const
{
  switch (type)
  {
    case JsonValueType::Boolean:
      return std::vector<double>{booleanValue ? 1.0 : 0.0};
    case JsonValueType::Float:
      return std::vector<double>{floatValue};
    case JsonValueType::Integer:
      return std::vector<double>{static_cast<double>(integerValue)};
    case JsonValueType::Null:
      return std::vector<double>{};
    case JsonValueType::Object:
      return std::vector<double>{};
    case JsonValueType::String:
      return std::vector<double>{strtod(stringValue.c_str(), nullptr)};
    case JsonValueType::Array:
    {
      std::vector<double> retArray;
      for (const auto& val : arrayValue) retArray.emplace_back(val.asFloat());
      return retArray;
    }
    default:
      return std::vector<double>{};
  }
}

std::vector<bool> JsonValue::asBooleanArray(void) const
{
  switch (type)
  {
    case JsonValueType::Boolean:
      return std::vector<bool>{booleanValue};
    case JsonValueType::Float:
      return std::vector<bool>{floatValue > 0.000000001};
    case JsonValueType::Integer:
      return std::vector<bool>{integerValue != 0};
    case JsonValueType::Null:
      return std::vector<bool>{};
    case JsonValueType::Object:
      return std::vector<bool>{};
    case JsonValueType::String:
      return std::vector<bool>{strcasecmp(stringValue.c_str(), "true") == 0};
    case JsonValueType::Array:
    {
      std::vector<bool> retArray;
      for (const auto& val : arrayValue) retArray.push_back(val.asBoolean());
      return retArray;
    }
    default:
      return std::vector<bool>{};
  }
}

std::vector<const JsonObject*> JsonValue::asObjectArray(void) const
{
  switch (type)
  {
    case JsonValueType::Boolean:
      return std::vector<const JsonObject*>{};
    case JsonValueType::Float:
      return std::vector<const JsonObject*>{};
    case JsonValueType::Integer:
      return std::vector<const JsonObject*>{};
    case JsonValueType::Null:
      return std::vector<const JsonObject*>{};
    case JsonValueType::Object:
      return std::vector<const JsonObject*>{&objectValue};
    case JsonValueType::String:
      return std::vector<const JsonObject*>{};
    case JsonValueType::Array:
    {
      std::vector<const JsonObject*> retArray;
      for (const auto& val : arrayValue) retArray.emplace_back(val.asObject());
      return retArray;
    }
    default:
      return std::vector<const JsonObject*>{};
  }
}

std::vector<const JsonValue*> JsonValue::asMultidimensionalArray(void) const
{
  switch (type)
  {
    case JsonValueType::Boolean:
      return std::vector<const JsonValue*>{this};
    case JsonValueType::Float:
      return std::vector<const JsonValue*>{this};
    case JsonValueType::Integer:
      return std::vector<const JsonValue*>{this};
    case JsonValueType::Null:
      return std::vector<const JsonValue*>{this};
    case JsonValueType::Object:
      return std::vector<const JsonValue*>{this};
    case JsonValueType::String:
      return std::vector<const JsonValue*>{this};
    case JsonValueType::Array:
    {
      std::vector<const JsonValue*> retArray;
      for (const auto& val : arrayValue) retArray.emplace_back(&val);
      return retArray;
    }
    default:
      return std::vector<const JsonValue*>{this};
  }
}

void JsonValue::setValue(const char* value)
{
  switch (type)
  {
    case JsonValueType::Boolean:
      booleanValue = (strcasecmp(value, "true") == 0);
      break;
    case JsonValueType::Float:
      floatValue = strtod(value, nullptr);
      break;
    case JsonValueType::Integer:
      integerValue = static_cast<JsonInteger>(strtol(value, nullptr, 0));
      break;
    case JsonValueType::Null:
    {
      new (&stringValue) JsonString(value);
      *const_cast<JsonValueType*>(&type) = JsonValueType::String;
      break;
    }
    case JsonValueType::Object:
      break;
    case JsonValueType::String:
      stringValue.assign(value);
      break;
    case JsonValueType::Array:
      break;
    default:
      break;
  }
}

void JsonValue::setValue(const std::vector<const char*>& value)
{
  switch (type)
  {
    case JsonValueType::Boolean:
      booleanValue = true;
      break;
    case JsonValueType::Float:
      floatValue = 1.0;
      break;
    case JsonValueType::Integer:
      integerValue = 1;
      break;
    case JsonValueType::Null:
    {
      new (&arrayValue) JsonArray();
      for (const auto& val : value) arrayValue.emplace_back(val);
      *const_cast<JsonValueType*>(&type) = JsonValueType::Array;
      break;
    }
    case JsonValueType::Object:
      break;
    case JsonValueType::String:
    {
      stringValue.assign("[");
      for (const auto& val : value) stringValue.append(val).append(",");
      if (stringValue.back() == ',') stringValue.pop_back();
      stringValue.append("]");
      break;
    }
    case JsonValueType::Array:
    {
      arrayValue.clear();
      for (const auto& val : value) arrayValue.emplace_back(val);
      break;
    }
    default:
      break;
  }
}

void JsonValue::setValue(const std::vector<std::string>& value)
{
  switch (type)
  {
    case JsonValueType::Boolean:
      booleanValue = true;
      break;
    case JsonValueType::Float:
      floatValue = 1.0;
      break;
    case JsonValueType::Integer:
      integerValue = 1;
      break;
    case JsonValueType::Null:
    {
      new (&arrayValue) JsonArray();
      for (const auto& val : value) arrayValue.emplace_back(val);
      *const_cast<JsonValueType*>(&type) = JsonValueType::Array;
      break;
    }
    case JsonValueType::Object:
      break;
    case JsonValueType::String:
    {
      stringValue.assign("[");
      for (const auto& val : value) stringValue.append(val).append(",");
      if (stringValue.back() == ',') stringValue.pop_back();
      stringValue.append("]");
      break;
    }
    case JsonValueType::Array:
    {
      arrayValue.clear();
      for (const auto& val : value) arrayValue.emplace_back(val);
      break;
    }
    default:
      break;
  }
}

void JsonValue::setValue(int value)
{
  switch (type)
  {
    case JsonValueType::Boolean:
      booleanValue = value;
      break;
    case JsonValueType::Float:
      floatValue = static_cast<JsonFloat>(value);
      break;
    case JsonValueType::Integer:
      integerValue = static_cast<JsonInteger>(value);
      break;
    case JsonValueType::Null:
    {
      integerValue = static_cast<JsonInteger>(value);
      *const_cast<JsonValueType*>(&type) = JsonValueType::Integer;
      break;
    }
    case JsonValueType::Object:
      break;
    case JsonValueType::String:
      stringValue = std::to_string(value);
      break;
    case JsonValueType::Array:
      break;
    default:
      break;
  }
}

void JsonValue::setValue(const std::vector<int>& value)
{
  switch (type)
  {
    case JsonValueType::Boolean:
      booleanValue = true;
      break;
    case JsonValueType::Float:
      floatValue = 1.0;
      break;
    case JsonValueType::Integer:
      integerValue = 1;
      break;
    case JsonValueType::Null:
    {
      new (&arrayValue) JsonArray();
      for (const auto& val : value) arrayValue.emplace_back(val);
      *const_cast<JsonValueType*>(&type) = JsonValueType::Array;
      break;
    }
    case JsonValueType::Object:
      break;
    case JsonValueType::String:
    {
      stringValue.assign("[");
      for (const auto& val : value) stringValue.append(std::to_string(val)).append(",");
      if (stringValue.back() == ',') stringValue.pop_back();
      stringValue.append("]");
      break;
    }
    case JsonValueType::Array:
    {
      arrayValue.clear();
      for (const auto& val : value) arrayValue.emplace_back(val);
      break;
    }
    default:
      break;
  }
}

void JsonValue::setValue(unsigned int value)
{
  switch (type)
  {
    case JsonValueType::Boolean:
      booleanValue = value;
      break;
    case JsonValueType::Float:
      floatValue = static_cast<JsonFloat>(value);
      break;
    case JsonValueType::Integer:
      integerValue = static_cast<JsonInteger>(value);
      break;
    case JsonValueType::Null:
    {
      integerValue = static_cast<JsonInteger>(value);
      *const_cast<JsonValueType*>(&type) = JsonValueType::Integer;
      break;
    }
    case JsonValueType::Object:
      break;
    case JsonValueType::String:
      stringValue = std::to_string(value);
      break;
    case JsonValueType::Array:
      break;
    default:
      break;
  }
}

void JsonValue::setValue(const std::vector<unsigned int>& value)
{
  switch (type)
  {
    case JsonValueType::Boolean:
      booleanValue = true;
      break;
    case JsonValueType::Float:
      floatValue = 1.0;
      break;
    case JsonValueType::Integer:
      integerValue = 1;
      break;
    case JsonValueType::Null:
    {
      new (&arrayValue) JsonArray();
      for (const auto& val : value) arrayValue.emplace_back(val);
      *const_cast<JsonValueType*>(&type) = JsonValueType::Array;
      break;
    }
    case JsonValueType::Object:
      break;
    case JsonValueType::String:
    {
      stringValue.assign("[");
      for (const auto& val : value) stringValue.append(std::to_string(val)).append(",");
      if (stringValue.back() == ',') stringValue.pop_back();
      stringValue.append("]");
      break;
    }
    case JsonValueType::Array:
    {
      arrayValue.clear();
      for (const auto& val : value) arrayValue.emplace_back(val);
      break;
    }
    default:
      break;
  }
}

void JsonValue::setValue(double value)
{
  switch (type)
  {
    case JsonValueType::Boolean:
      booleanValue = (value > 0.000000001);
      ;
      break;
    case JsonValueType::Float:
      floatValue = value;
      break;
    case JsonValueType::Integer:
      integerValue = static_cast<JsonInteger>(value);
      break;
    case JsonValueType::Null:
    {
      floatValue = value;
      *const_cast<JsonValueType*>(&type) = JsonValueType::Float;
      break;
    }
    case JsonValueType::Object:
      break;
    case JsonValueType::String:
      stringValue = std::to_string(value);
      break;
    case JsonValueType::Array:
      break;
    default:
      break;
  }
}

void JsonValue::setValue(const std::vector<double>& value)
{
  switch (type)
  {
    case JsonValueType::Boolean:
      booleanValue = true;
      break;
    case JsonValueType::Float:
      floatValue = 1.0;
      break;
    case JsonValueType::Integer:
      integerValue = 1;
      break;
    case JsonValueType::Null:
    {
      new (&arrayValue) JsonArray();
      for (const auto& val : value) arrayValue.emplace_back(val);
      *const_cast<JsonValueType*>(&type) = JsonValueType::Array;
      break;
    }
    case JsonValueType::Object:
      break;
    case JsonValueType::String:
    {
      stringValue.assign("[");
      for (const auto& val : value) stringValue.append(std::to_string(val)).append(",");
      if (stringValue.back() == ',') stringValue.pop_back();
      stringValue.append("]");
      break;
    }
    case JsonValueType::Array:
    {
      arrayValue.clear();
      for (const auto& val : value) arrayValue.emplace_back(val);
      break;
    }
    default:
      break;
  }
}

void JsonValue::setValue(float value)
{
  switch (type)
  {
    case JsonValueType::Boolean:
      booleanValue = (value > 0.000000001);
      ;
      break;
    case JsonValueType::Float:
      floatValue = value;
      break;
    case JsonValueType::Integer:
      integerValue = static_cast<JsonInteger>(value);
      break;
    case JsonValueType::Null:
    {
      floatValue = value;
      *const_cast<JsonValueType*>(&type) = JsonValueType::Float;
      break;
    }
    case JsonValueType::Object:
      break;
    case JsonValueType::String:
      stringValue = std::to_string(value);
      break;
    case JsonValueType::Array:
      break;
    default:
      break;
  }
}

void JsonValue::setValue(const std::vector<float>& value)
{
  switch (type)
  {
    case JsonValueType::Boolean:
      booleanValue = true;
      break;
    case JsonValueType::Float:
      floatValue = 1.0;
      break;
    case JsonValueType::Integer:
      integerValue = 1;
      break;
    case JsonValueType::Null:
    {
      new (&arrayValue) JsonArray();
      for (const auto& val : value) arrayValue.emplace_back(val);
      *const_cast<JsonValueType*>(&type) = JsonValueType::Array;
      break;
    }
    case JsonValueType::Object:
      break;
    case JsonValueType::String:
    {
      stringValue.assign("[");
      for (const auto& val : value) stringValue.append(std::to_string(val)).append(",");
      if (stringValue.back() == ',') stringValue.pop_back();
      stringValue.append("]");
      break;
    }
    case JsonValueType::Array:
    {
      arrayValue.clear();
      for (const auto& val : value) arrayValue.emplace_back(val);
      break;
    }
    default:
      break;
  }
}

void JsonValue::setValue(bool value)
{
  switch (type)
  {
    case JsonValueType::Boolean:
      booleanValue = value;
      break;
    case JsonValueType::Float:
      floatValue = value ? 1.0 : 0.0;
      break;
    case JsonValueType::Integer:
      integerValue = value ? 1 : 0;
      break;
    case JsonValueType::Null:
    {
      booleanValue = value;
      *const_cast<JsonValueType*>(&type) = JsonValueType::Boolean;
      break;
    }
    case JsonValueType::Object:
      break;
    case JsonValueType::String:
      stringValue.assign(value ? "true" : "false");
      break;
    case JsonValueType::Array:
      break;
    default:
      break;
  }
}

void JsonValue::setValue(const std::vector<bool>& value)
{
  switch (type)
  {
    case JsonValueType::Boolean:
      booleanValue = true;
      break;
    case JsonValueType::Float:
      floatValue = 1.0;
      break;
    case JsonValueType::Integer:
      integerValue = 1;
      break;
    case JsonValueType::Null:
    {
      new (&arrayValue) JsonArray();
      for (const auto& val : value) arrayValue.emplace_back(val);
      *const_cast<JsonValueType*>(&type) = JsonValueType::Array;
      break;
    }
    case JsonValueType::Object:
      break;
    case JsonValueType::String:
    {
      stringValue.assign("[");
      for (const auto& val : value) stringValue.append(val ? "true" : "false").append(",");
      if (stringValue.back() == ',') stringValue.pop_back();
      stringValue.append("]");
      break;
    }
    case JsonValueType::Array:
    {
      arrayValue.clear();
      for (const auto& val : value) arrayValue.emplace_back(val);
      break;
    }
    default:
      break;
  }
}

void JsonValue::setValue(JsonObject&& value)
{
  switch (type)
  {
    case JsonValueType::Boolean:
      break;
    case JsonValueType::Float:
      break;
    case JsonValueType::Integer:
      break;
    case JsonValueType::Null:
    {
      new (&objectValue) JsonObject(std::move(value));
      *const_cast<JsonValueType*>(&type) = JsonValueType::Object;
      break;
    }
    case JsonValueType::Object:
      objectValue = std::move(value);
      break;
    case JsonValueType::String:
      break;
    case JsonValueType::Array:
      break;
    default:
      break;
  }
}

void JsonValue::setValue(JsonObject*&& value, size_t numObjects)
{
  switch (type)
  {
    case JsonValueType::Boolean:
      break;
    case JsonValueType::Float:
      break;
    case JsonValueType::Integer:
      break;
    case JsonValueType::Null:
    {
      new (&arrayValue) JsonArray();
      for (size_t i = 0; i < numObjects; ++i) arrayValue.emplace_back(std::move(value[i]));
      *const_cast<JsonValueType*>(&type) = JsonValueType::Array;
      break;
    }
    case JsonValueType::Object:
      break;
    case JsonValueType::String:
      break;
    case JsonValueType::Array:
    {
      arrayValue.clear();
      for (size_t i = 0; i < numObjects; ++i) arrayValue.emplace_back(std::move(value[i]));
      break;
    }
    default:
      break;
  }
}

void JsonValue::setValue(JsonValue*&& value, size_t numValues)
{
  switch (type)
  {
    case JsonValueType::Boolean:
      break;
    case JsonValueType::Float:
      break;
    case JsonValueType::Integer:
      break;
    case JsonValueType::Null:
    {
      new (&arrayValue) JsonArray();
      for (size_t i = 0; i < numValues; ++i) arrayValue.emplace_back(std::move(value[i]));
      *const_cast<JsonValueType*>(&type) = JsonValueType::Array;
      break;
    }
    case JsonValueType::Object:
      break;
    case JsonValueType::String:
      break;
    case JsonValueType::Array:
    {
      arrayValue.clear();
      for (size_t i = 0; i < numValues; ++i) arrayValue.emplace_back(std::move(value[i]));
      break;
    }
    default:
      break;
  }
}

JsonObject JsonParser::parseJsonString(const char* jsonString)
{
  // Parse this string as a JSON composite object
  std::deque<char> workingString(jsonString, jsonString + strlen(jsonString));
  return parseCompositeJsonValue(workingString).objectValue;
}

std::string JsonParser::printJsonString(const JsonObject& jsonStructure, bool prettyPrint, bool stringifyAllValues)
{
  // Create this string as if the JSON structure is a composite value
  return printCompositeString(jsonStructure, true, prettyPrint, stringifyAllValues, 0);
}

JsonString JsonParser::parseJsonKey(std::deque<char>& workingString)
{
  // Find the beginning of the JSON key
  while (!workingString.empty() && (workingString.front() != '"')) workingString.pop_front();

  // Parse and return the JSON key
  JsonString jsonKey;
  parseQuotedJsonString(workingString, jsonKey, true);
  return jsonKey;
}

JsonValue JsonParser::parseJsonValue(std::deque<char>& workingString)
{
  // Find the beginning of the JSON value
  while (!workingString.empty() && (invalidJsonValueStartingChars.find_first_of(workingString.front()) != std::string::npos)) workingString.pop_front();

  // See if this value is a composite, an array, or a literal value
  return (workingString.front() == '{')                                                                         ? parseCompositeJsonValue(workingString)
         : (workingString.front() == '[')                                                                       ? parseArrayJsonValue(workingString)
         : ((workingString.front() == ',') || (workingString.front() == ']') || (workingString.front() == '}')) ? JsonValue()
                                                                                                                : parseLiteralJsonValue(workingString);
}

JsonValue JsonParser::parseLiteralJsonValue(std::deque<char>& workingString)
{
  // Parse the JSON value as a simple string
  std::string jsonStringValue;
  if (workingString.front() == '"')
    parseQuotedJsonString(workingString, jsonStringValue, true);
  else
  {
    // Search for the first invalid JSON value character
    while (!workingString.empty() && (validJsonValueChars.find_first_of(workingString.front()) != std::string::npos))
    {
      jsonStringValue += workingString.front();
      workingString.pop_front();
    }
  }
  return JsonValue(jsonStringValue);
}

JsonValue JsonParser::parseCompositeJsonValue(std::deque<char>& workingString)
{
  // Find the beginning of the JSON composite object
  while (!workingString.empty() && (workingString.front() != '{')) workingString.pop_front();
  if (!workingString.empty()) workingString.pop_front();

  // Iterate through the string until the end of the JSON composite object is found
  JsonObject jsonEntries;
  while (!workingString.empty() && (workingString.front() != '}'))
  {
    // Parse the next key/value pair and add it to the list
    JsonString key(parseJsonKey(workingString));
    if (key.empty()) break;
    while (!workingString.empty() && (workingString.front() != ':')) workingString.pop_front();
    if (!workingString.empty()) workingString.pop_front();
    jsonEntries.emplace(key, parseJsonValue(workingString));
    while (!workingString.empty() && (workingString.front() != ',') && (workingString.front() != '}')) workingString.pop_front();
  }
  if (!workingString.empty()) workingString.pop_front();

  // Return the JSON composite object
  return JsonValue(std::move(jsonEntries));
}

JsonValue JsonParser::parseArrayJsonValue(std::deque<char>& workingString)
{
  // Find the beginning of the JSON array
  while (!workingString.empty() && (workingString.front() != '[')) workingString.pop_front();
  if (!workingString.empty()) workingString.pop_front();

  // Iterate through the string until the end of the JSON array is found
  std::vector<JsonValue> jsonArrayValues;
  while (!workingString.empty() && (workingString.front() != ']'))
  {
    // Find the beginning of the next value
    while (!workingString.empty() && (invalidJsonValueStartingChars.find_first_of(workingString.front()) != std::string::npos)) workingString.pop_front();
    if ((workingString.front() == ']') || (workingString.front() == ',')) break;

    // Parse the next value and add it to the list
    jsonArrayValues.emplace_back(parseJsonValue(workingString));
    while (!workingString.empty() && (workingString.front() != ',') && (workingString.front() != ']')) workingString.pop_front();
    if (workingString.front() == ',') workingString.pop_front();
  }
  if (!workingString.empty()) workingString.pop_front();
  return JsonValue(std::move(jsonArrayValues));
}

std::string JsonParser::printJsonKey(const JsonString& key, bool prettyPrint, int levelsFromRoot)
{
  // Print spaces if pretty-printing is enabled
  std::string jsonKey;
  if (prettyPrint)
    for (int i = 0; i < (2 * levelsFromRoot); ++i) jsonKey += ' ';

  // Print the JSON key
  return jsonKey.append("\"").append(key).append(prettyPrint ? "\": " : "\":");
}

std::string JsonParser::printJsonValue(const JsonValue& value, bool lastValue, bool prettyPrint, bool stringifyAllValues, int levelsFromRoot)
{
  // Print differently depending on value type
  std::string jsonValue;
  switch (value.type)
  {
    case JsonValueType::Object:
      jsonValue = printCompositeString(value.objectValue, lastValue, prettyPrint, stringifyAllValues, levelsFromRoot);
      break;
    case JsonValueType::Array:
      jsonValue = printArrayString(value.arrayValue, lastValue, prettyPrint, stringifyAllValues, levelsFromRoot);
      break;
    default:
      if (stringifyAllValues || (value.type == JsonValueType::String))
        jsonValue.append("\"").append(value.asString()).append("\"");
      else
        jsonValue.append(value.asString());
      if (!lastValue) jsonValue += ',';
      if (prettyPrint) jsonValue += '\n';
      break;
  }
  return jsonValue;
}

std::string JsonParser::printCompositeString(const JsonObject& value, bool lastValue, bool prettyPrint, bool stringifyAllValues, int levelsFromRoot)
{
  // Iterate through the entire structure, adding to the string along the way
  std::string jsonString(prettyPrint ? "{\n" : "{");
  for (auto entry = std::begin(value); entry != std::end(value);)
  {
    //  Print the next key/value pair and add it to the string
    jsonString += printJsonKey((*entry).first, prettyPrint, levelsFromRoot + 1);
    auto currentEntry = (*entry).second.get();
    jsonString += printJsonValue(*currentEntry, ++entry == std::end(value), prettyPrint, stringifyAllValues, levelsFromRoot + 1);
  }

  // Close the composite JSON string
  if (prettyPrint)
    for (int i = 0; i < (2 * levelsFromRoot); ++i) jsonString += ' ';
  jsonString += (lastValue ? "}" : "},");
  if (prettyPrint) jsonString += '\n';
  return jsonString;
}

std::string JsonParser::printArrayString(const JsonArray& value, bool lastValue, bool prettyPrint, bool stringifyAllValues, int levelsFromRoot)
{
  // Iterate through the entire structure, adding to the string along the way
  std::string jsonString("[");
  for (auto entry = std::begin(value); entry != std::end(value);)
  {
    auto currentEntry = entry;
    jsonString += printJsonValue(*currentEntry, ++entry == std::end(value), prettyPrint, stringifyAllValues, levelsFromRoot + 1);
    while (jsonString.back() == '\n') jsonString.pop_back();
  }

  // Close the array JSON string
  if ((jsonString.back() == '}') && prettyPrint)
  {
    jsonString += '\n';
    for (int i = 0; i < (2 * levelsFromRoot); ++i) jsonString += ' ';
  }
  jsonString += (lastValue ? "]" : "],");
  if (prettyPrint) jsonString += '\n';
  return jsonString;
}

int JsonParser::hexDigitValue(char c)
{
  if ((c >= '0') && (c <= '9')) return c - '0';
  if ((c >= 'a') && (c <= 'f')) return 10 + (c - 'a');
  if ((c >= 'A') && (c <= 'F')) return 10 + (c - 'A');
  return -1;
}

bool JsonParser::popHex4(std::deque<char>& in, uint32_t& value)
{
  value = 0;
  if (in.size() < 4) return false;
  for (int i = 0; i < 4; ++i)
  {
    const int h = hexDigitValue(in.front());
    if (h < 0) return false;
    value = (value << 4) | static_cast<uint32_t>(h);
    in.pop_front();
  }
  return true;
}

void JsonParser::appendUtf8(std::string& out, uint32_t cp)
{
  if (cp <= 0x7F)
    out.push_back(static_cast<char>(cp));
  else if (cp <= 0x7FF)
  {
    out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
  else if (cp <= 0xFFFF)
  {
    out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
  else
  {
    out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
}

bool JsonParser::decodeJsonEscape(std::deque<char>& in, std::string& out, bool decodeEscapes)
{
  if (in.empty()) return false;
  const char esc = in.front();
  in.pop_front();

  if (!decodeEscapes)
  {
    out.push_back('\\');
    out.push_back(esc);
    return true;
  }

  switch (esc)
  {
    case '"':
      out.push_back('"');
      return true;
    case '\\':
      out.push_back('\\');
      return true;
    case '/':
      out.push_back('/');
      return true;
    case 'b':
      out.push_back('\b');
      return true;
    case 'f':
      out.push_back('\f');
      return true;
    case 'n':
      out.push_back('\n');
      return true;
    case 'r':
      out.push_back('\r');
      return true;
    case 't':
      out.push_back('\t');
      return true;
    case 'u':
    {
      uint32_t cp = 0;
      if (!popHex4(in, cp)) return false;

      // Surrogate pair handling: \uD800..\uDBFF followed by \uDC00..\uDFFF
      if ((cp >= 0xD800) && (cp <= 0xDBFF))
      {
        if ((in.size() >= 6) && (in[0] == '\\') && (in[1] == 'u'))
        {
          in.pop_front();  // '\'
          in.pop_front();  // 'u'
          uint32_t low = 0;
          if (!popHex4(in, low)) return false;
          if ((low >= 0xDC00) && (low <= 0xDFFF))
            cp = 0x10000 + (((cp - 0xD800) << 10) | (low - 0xDC00));
          else
            return false;
        }
        else
          return false;
      }
      else if ((cp >= 0xDC00) && (cp <= 0xDFFF))
        return false;

      appendUtf8(out, cp);
      return true;
    }
    default:
      return false;
  }
}

bool JsonParser::parseQuotedJsonString(std::deque<char>& in, std::string& out, bool decodeEscapes)
{
  if (in.empty() || (in.front() != '"')) return false;
  in.pop_front();  // opening quote

  while (!in.empty())
  {
    const char c = in.front();
    in.pop_front();

    if (c == '"') return true;  // closing quote
    if (c == '\\')
    {
      if (!decodeJsonEscape(in, out, decodeEscapes)) return false;
    }
    else
      out.push_back(c);
  }
  return false;
}
