#ifndef __JSON_PARSER_HEADER_H__
#define __JSON_PARSER_HEADER_H__

#include <cstddef>
#include <cstdint>
#include <deque>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

// JSON Types and Typedefs
class JsonValue;
enum class JsonValueType
{
  String,
  Integer,
  Float,
  Object,
  Array,
  Boolean,
  Null
};
typedef std::string JsonString;
typedef int64_t JsonInteger;
typedef double JsonFloat;
typedef std::vector<JsonValue> JsonArray;
typedef bool JsonBoolean;
class JsonObject final
{
public:

  // Constructors
  JsonObject(void) : keyValuePairs(), insertionOrder() {};
  JsonObject(JsonObject&& other) : keyValuePairs(std::move(other.keyValuePairs)), insertionOrder(std::move(other.insertionOrder)) {};
  JsonObject& operator=(JsonObject&& other)
  {
    keyValuePairs = std::move(other.keyValuePairs);
    insertionOrder = std::move(other.insertionOrder);
    return *this;
  };

  // Forward iterator classes and functions
  class iterator
  {
  private:

    std::unordered_map<std::string, std::unique_ptr<JsonValue>>* values;
    std::vector<std::string>::iterator currentLocation;

  public:

    typedef std::forward_iterator_tag iterator_category;
    typedef ptrdiff_t difference_type;
    typedef std::pair<const std::string&, std::unordered_map<std::string, std::unique_ptr<JsonValue>>::mapped_type&> value_type;
    typedef std::pair<const std::string&, std::unordered_map<std::string, std::unique_ptr<JsonValue>>::mapped_type&>& reference;
    typedef std::pair<const std::string&, std::unordered_map<std::string, std::unique_ptr<JsonValue>>::mapped_type&>* pointer;
    explicit iterator(std::unordered_map<std::string, std::unique_ptr<JsonValue>>* keyValues, std::vector<std::string>::iterator pos) : values(keyValues), currentLocation(pos) {};
    iterator& operator++(void)
    {
      ++currentLocation;
      return *this;
    };
    bool operator==(const iterator& other) const { return currentLocation == other.currentLocation; };
    bool operator!=(const iterator& other) const { return !operator==(other); };
    value_type operator*(void) { return value_type(*currentLocation, values->at(*currentLocation)); };
  };
  class const_iterator
  {
  private:

    const std::unordered_map<std::string, std::unique_ptr<JsonValue>>* values;
    std::vector<std::string>::const_iterator currentLocation;

  public:

    typedef std::forward_iterator_tag iterator_category;
    typedef ptrdiff_t difference_type;
    typedef std::pair<const std::string&, const std::unordered_map<std::string, std::unique_ptr<JsonValue>>::mapped_type&> value_type;
    typedef std::pair<const std::string&, const std::unordered_map<std::string, std::unique_ptr<JsonValue>>::mapped_type&>& reference;
    typedef std::pair<const std::string&, const std::unordered_map<std::string, std::unique_ptr<JsonValue>>::mapped_type&>* pointer;
    explicit const_iterator(const std::unordered_map<std::string, std::unique_ptr<JsonValue>>* keyValues, std::vector<std::string>::const_iterator pos)
        : values(keyValues), currentLocation(pos) {};
    const_iterator& operator++(void)
    {
      ++currentLocation;
      return *this;
    };
    bool operator==(const const_iterator& other) const { return currentLocation == other.currentLocation; };
    bool operator!=(const const_iterator& other) const { return !operator==(other); };
    const value_type operator*(void) const { return value_type(*currentLocation, values->at(*currentLocation)); };
  };
  iterator begin(void) { return iterator(&keyValuePairs, std::begin(insertionOrder)); };
  iterator end(void) { return iterator(&keyValuePairs, std::end(insertionOrder)); };
  const_iterator begin(void) const { return const_iterator(&keyValuePairs, std::begin(insertionOrder)); };
  const_iterator end(void) const { return const_iterator(&keyValuePairs, std::end(insertionOrder)); };

  // Insertion and retrieval operators
  inline bool empty(void) const { return keyValuePairs.empty(); };
  inline bool contains(const char* key) const { return keyValuePairs.count(key); };
  inline bool contains(const JsonString& key) const { return keyValuePairs.count(key); };
  JsonValue& operator[](const char* key);
  JsonValue& operator[](const JsonString& key);
  JsonValue& operator[](JsonString&& key);
  inline JsonValue& at(const char* key) { return *keyValuePairs.at(key); };
  inline JsonValue& at(const JsonString& key) { return *keyValuePairs.at(key); };
  inline const JsonValue& at(const JsonString& key) const { return *keyValuePairs.at(key); };
  JsonValue& emplace(const char* key, JsonValue&& value);
  JsonValue& emplace(const JsonString& key, JsonValue&& value);
  JsonValue& emplace(JsonString&& key, JsonValue&& value);
  void remove(const char* key);
  void remove(const JsonString& key);
  iterator remove(iterator pos);
  iterator remove(const_iterator pos);

private:

  // Private member variables
  std::unordered_map<std::string, std::unique_ptr<JsonValue>> keyValuePairs;
  std::vector<std::string> insertionOrder;
};

// JSON Value Master Class
class JsonValue final
{
public:

  // Constructors to automatically create a JsonValue from pretty much any data type
  JsonValue(void) : type(JsonValueType::Null), integerValue(0) {};
  JsonValue(const char* jsonValue) : type(JsonValueType::String), stringValue(jsonValue) {};
  JsonValue(const std::string& jsonValue) : type(JsonValueType::String), stringValue(jsonValue) {};
  JsonValue(std::string&& jsonValue) : type(JsonValueType::String), stringValue(std::move(jsonValue)) {};
  JsonValue(const char** jsonValues, size_t numStrings) : type(JsonValueType::Array), arrayValue(jsonValues, jsonValues + numStrings) {};
  JsonValue(const std::string* jsonValues, size_t numStrings) : type(JsonValueType::Array), arrayValue(jsonValues, jsonValues + numStrings) {};
  JsonValue(std::initializer_list<const char*> jsonValues) : type(JsonValueType::Array), arrayValue(std::begin(jsonValues), std::end(jsonValues)) {};
  JsonValue(std::initializer_list<std::string> jsonValues) : type(JsonValueType::Array), arrayValue(std::begin(jsonValues), std::end(jsonValues)) {};
  JsonValue(int jsonValue) : type(JsonValueType::Integer), integerValue(static_cast<JsonInteger>(jsonValue)) {};
  JsonValue(int* jsonValues, size_t numInts) : type(JsonValueType::Array), arrayValue(jsonValues, jsonValues + numInts) {};
  JsonValue(std::initializer_list<int> jsonValues) : type(JsonValueType::Array), arrayValue(std::begin(jsonValues), std::end(jsonValues)) {};
  JsonValue(unsigned int jsonValue) : type(JsonValueType::Integer), integerValue(static_cast<JsonInteger>(jsonValue)) {};
  JsonValue(unsigned int* jsonValues, size_t numInts) : type(JsonValueType::Array), arrayValue(jsonValues, jsonValues + numInts) {};
  JsonValue(std::initializer_list<unsigned int> jsonValues) : type(JsonValueType::Array), arrayValue(std::begin(jsonValues), std::end(jsonValues)) {};
  JsonValue(double jsonValue) : type(JsonValueType::Float), floatValue(jsonValue) {};
  JsonValue(double* jsonValues, size_t numFloats) : type(JsonValueType::Array), arrayValue(jsonValues, jsonValues + numFloats) {};
  JsonValue(std::initializer_list<double> jsonValues) : type(JsonValueType::Array), arrayValue(std::begin(jsonValues), std::end(jsonValues)) {};
  JsonValue(bool jsonValue) : type(JsonValueType::Boolean), booleanValue(jsonValue) {};
  JsonValue(bool* jsonValues, size_t numBools) : type(JsonValueType::Array), arrayValue(jsonValues, jsonValues + numBools) {};
  JsonValue(std::initializer_list<bool> jsonValues) : type(JsonValueType::Array), arrayValue(std::begin(jsonValues), std::end(jsonValues)) {};
  JsonValue(JsonObject&& jsonValue) : type(JsonValueType::Object), objectValue(std::move(jsonValue)) {};
  JsonValue(JsonObject*&& jsonValues, size_t numObjects) : type(JsonValueType::Array), arrayValue()
  {
    for (size_t i = 0; i < numObjects; ++i) arrayValue.emplace_back(std::move(jsonValues[i]));
  };
  JsonValue(std::vector<JsonValue>&& jsonValues) : type(JsonValueType::Array), arrayValue(std::move(jsonValues)) {};
  JsonValue(JsonValue*&& jsonValues, size_t numValues) : type(JsonValueType::Array), arrayValue()
  {
    for (size_t i = 0; i < numValues; ++i) arrayValue.emplace_back(std::move(jsonValues[i]));
  };
  JsonValue(JsonValue&& other);
  JsonValue& operator=(JsonValue&& other);
  ~JsonValue(void);

  // Converting getters
  std::string asString(bool prettyPrint = false, bool stringifyAllValues = false) const;
  int asInteger(void) const;
  unsigned int asUnsigned(void) const;
  double asFloat(void) const;
  bool asBoolean(void) const;
  JsonObject* asObject(void) const;
  std::vector<std::string> asStringArray(bool prettyPrint = false, bool stringifyAllValues = false) const;
  std::vector<int> asIntegerArray(void) const;
  std::vector<unsigned int> asUnsignedArray(void) const;
  std::vector<double> asFloatArray(void) const;
  std::vector<bool> asBooleanArray(void) const;
  std::vector<const JsonObject*> asObjectArray(void) const;
  std::vector<const JsonValue*> asMultidimensionalArray(void) const;

  // Converting setters
  void setValue(const char* value);
  inline void setValue(const std::string& value) { return setValue(value.c_str()); };
  void setValue(const std::vector<const char*>& value);
  void setValue(const std::vector<std::string>& value);
  void setValue(int value);
  void setValue(const std::vector<int>& value);
  void setValue(unsigned int value);
  void setValue(const std::vector<unsigned int>& value);
  void setValue(float value);
  void setValue(const std::vector<float>& value);
  void setValue(double value);
  void setValue(const std::vector<double>& value);
  void setValue(bool value);
  void setValue(const std::vector<bool>& value);
  void setValue(JsonObject&& value);
  void setValue(JsonObject*&& value, size_t numObjects);
  void setValue(JsonValue*&& value, size_t numValues);

  // Publicly-accessible JSON type
  const JsonValueType type;

private:

  // Allow parser to directly access the union fields
  friend class JsonParser;

  // Union of possible value types
  union
  {
    JsonString stringValue;
    JsonInteger integerValue;
    JsonFloat floatValue;
    JsonObject objectValue;
    JsonArray arrayValue;
    JsonBoolean booleanValue;
  };
};

// JSON Parser Class
class JsonParser final
{
public:

  // Parser functionality
  static JsonObject parseJsonString(const char* jsonString);
  static std::string printJsonString(const JsonObject& jsonStructure, bool prettyPrint, bool stringifyAllValues);

private:

  // Private parsing functions
  static JsonString parseJsonKey(std::deque<char>& workingString);
  static JsonValue parseJsonValue(std::deque<char>& workingString);
  static JsonValue parseLiteralJsonValue(std::deque<char>& workingString);
  static JsonValue parseCompositeJsonValue(std::deque<char>& workingString);
  static JsonValue parseArrayJsonValue(std::deque<char>& workingString);

  // Private printing functions
  static std::string printJsonKey(const JsonString& key, bool prettyPrint, int levelsFromRoot);
  static std::string printJsonValue(const JsonValue& value, bool lastValue, bool prettyPrint, bool stringifyAllValues, int levelsFromRoot);
  static std::string printCompositeString(const JsonObject& value, bool lastValue, bool prettyPrint, bool stringifyAllValues, int levelsFromRoot);
  static std::string printArrayString(const JsonArray& value, bool lastValue, bool prettyPrint, bool stringifyAllValues, int levelsFromRoot);

  // Private unescaping functions
  static int hexDigitValue(char c);
  static bool popHex4(std::deque<char>& in, uint32_t& value);
  static void appendUtf8(std::string& out, uint32_t cp);
  static bool decodeJsonEscape(std::deque<char>& in, std::string& out, bool decodeEscapes);
  static bool parseQuotedJsonString(std::deque<char>& in, std::string& out, bool decodeEscapes);

  // Allow JSON Value class to access these parsers
  friend class JsonValue;
};

#endif  // #ifndef __JSON_PARSER_HEADER_H__
