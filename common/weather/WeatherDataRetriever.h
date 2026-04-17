#ifndef __WEATHER_DATA_RETRIEVER_HEADER_H__
#define __WEATHER_DATA_RETRIEVER_HEADER_H__

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>

// Weather data structure
struct WeatherData final
{
  WeatherData(void) : isValid(false), temperature(0.0), relativeHumidity(-1.0), atmosphericPressure(-1.0), windSpeed(-1.0) {};
  bool isValid;
  double temperature, relativeHumidity;
  double atmosphericPressure, windSpeed;
};

// Abstract base class for Public API implementations
class PublicWeatherAPI
{
public:

  // Allowable units of measurement
  enum class TemperatureUnits
  {
    FAHRENHEIT,
    CELSIUS,
    KELVIN
  };
  enum class PressureUnits
  {
    ATMOSPHERE,
    MILLIBAR,
    PASCAL,
    MM_MERCURY,
    INCH_MERCURY,
    PSI
  };
  enum class WindSpeedUnits
  {
    METERS_PER_SECOND,
    KILOMETERS_PER_HOUR,
    MILES_PER_HOUR
  };

  // Virtual destructor to enable deletion by base class
  virtual ~PublicWeatherAPI(void) = default;

  // Weather data retrieval functions
  virtual WeatherData getCurrentWeatherData(void* curl, float latitude, float longitude) const = 0;

protected:

  // Can only be instantiated by inherited classes
  PublicWeatherAPI(TemperatureUnits unitsOfTemperature, PressureUnits unitsOfPressure, WindSpeedUnits unitsOfWindSpeed);

  // cURL HTTP callback
  static size_t writeCurlDataString(void* downloadedData, size_t wordSize, size_t numWords, void* outputStringPtr);

  // Inherited member variables
  TemperatureUnits temperatureUnits;
  PressureUnits pressureUnits;
  WindSpeedUnits windSpeedUnits;
};

// Weather data retrieval master class
class WeatherDataRetriever final
{
public:

  // Public API implementations
  enum class PublicAPI
  {
    NOAA,
    OPEN_WEATHER_MAP,
    TOMORROW_IO
  };

  // Initialization/cleanup
  static void initialize(void);
  static void uninitialize(void);
  static void* getCurlInstance(void);

  // Constructor/destructor
  WeatherDataRetriever(void);
  ~WeatherDataRetriever(void);

  // Weather API service management functionality
  bool addPublicApiService(PublicAPI apiType, PublicWeatherAPI::TemperatureUnits unitsOfTemperature, PublicWeatherAPI::PressureUnits unitsOfPressure,
                           PublicWeatherAPI::WindSpeedUnits unitsOfWindSpeed, const char* apiKey = nullptr, int8_t priority = -1);
  void removePublicApiService(PublicAPI apiType);
  PublicWeatherAPI* getPublicApiService(PublicAPI apiType);
  WeatherData getCurrentWeatherData(float latitude, float longitude) const;

private:

  // Static member variables
  static void* curl;
  static int referenceCount;
  static std::mutex retrievalMutex;
  static std::mutex initializationMutex;

  // Private member variables
  std::map<uint8_t, std::unique_ptr<PublicWeatherAPI>> weatherAPIs;
};

#endif  // #ifndef __WEATHER_DATA_RETRIEVER_HEADER_H__
