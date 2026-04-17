#include "Common.h"
#include "Logger.h"
#include "NoaaWeatherAPI.h"
#include "OpenWeatherMapAPI.h"
#include "TomorrowIoAPI.h"
#include "WeatherDataRetriever.h"
#include "WeatherRetriever.h"
#include "secrets.h"

// Global application logger
Logger logger("/tmp/civicalert.log", Logger::DEBUG);

void test_noaa(void* curl)
{
  NoaaWeatherAPI noaa(PublicWeatherAPI::TemperatureUnits::CELSIUS, PublicWeatherAPI::PressureUnits::MILLIBAR, PublicWeatherAPI::WindSpeedUnits::METERS_PER_SECOND);
  WeatherData data = noaa.getCurrentWeatherData(curl, 36.1465, -86.7937);  // Nashville, TN coordinates
  logger.log(Logger::INFO, "NOAA Weather - Temp: %.2f C, Humidity: %.2f, Pressure: %.3f hPa\n", data.temperature, data.relativeHumidity, data.atmosphericPressure);
}

void test_openweathermap(void* curl, const char* apiKey)
{
  OpenWeatherMapAPI owm(apiKey, PublicWeatherAPI::TemperatureUnits::CELSIUS, PublicWeatherAPI::PressureUnits::MILLIBAR, PublicWeatherAPI::WindSpeedUnits::METERS_PER_SECOND);
  WeatherData data = owm.getCurrentWeatherData(curl, 36.1465, -86.7937);  // Nashville, TN coordinates
  logger.log(Logger::INFO, "OpenWeatherMap Weather - Temp: %.2f C, Humidity: %.2f, Pressure: %.3f hPa\n", data.temperature, data.relativeHumidity, data.atmosphericPressure);
}

void test_tomorrowio(void* curl, const char* apiKey)
{
  TomorrowIoAPI tomorrowIo(apiKey, PublicWeatherAPI::TemperatureUnits::CELSIUS, PublicWeatherAPI::PressureUnits::MILLIBAR, PublicWeatherAPI::WindSpeedUnits::METERS_PER_SECOND);
  WeatherData data = tomorrowIo.getCurrentWeatherData(curl, 36.1465, -86.7937);  // Nashville, TN coordinates
  logger.log(Logger::INFO, "Tomorrow.io Weather - Temp: %.2f C, Humidity: %.2f, Pressure: %.2f hPa\n", data.temperature, data.relativeHumidity, data.atmosphericPressure);
}

void test_weather_data_retriever(const char* owmApiKey, const char* tomorrowIoApiKey)
{
  WeatherDataRetriever retriever;
  retriever.addPublicApiService(WeatherDataRetriever::PublicAPI::NOAA, PublicWeatherAPI::TemperatureUnits::CELSIUS, PublicWeatherAPI::PressureUnits::MILLIBAR,
                                PublicWeatherAPI::WindSpeedUnits::METERS_PER_SECOND, nullptr, 2);
  retriever.addPublicApiService(WeatherDataRetriever::PublicAPI::OPEN_WEATHER_MAP, PublicWeatherAPI::TemperatureUnits::CELSIUS, PublicWeatherAPI::PressureUnits::MILLIBAR,
                                PublicWeatherAPI::WindSpeedUnits::METERS_PER_SECOND, owmApiKey, 1);
  retriever.addPublicApiService(WeatherDataRetriever::PublicAPI::TOMORROW_IO, PublicWeatherAPI::TemperatureUnits::CELSIUS, PublicWeatherAPI::PressureUnits::MILLIBAR,
                                PublicWeatherAPI::WindSpeedUnits::METERS_PER_SECOND, tomorrowIoApiKey, 3);
  WeatherData data = retriever.getCurrentWeatherData(36.1465, -86.7937);  // Nashville, TN coordinates
  logger.log(Logger::INFO, "WeatherDataRetriever - Temp: %.2f C, Humidity: %.2f, Pressure: %.3f hPa\n", data.temperature, data.relativeHumidity, data.atmosphericPressure);
}

void test_weather_retriever(const char* owmApiKey, const char* tomorrowIoApiKey)
{
  WeatherRetriever retriever;
  retriever.addPublicApiService(WeatherDataRetriever::PublicAPI::NOAA, PublicWeatherAPI::TemperatureUnits::CELSIUS, PublicWeatherAPI::PressureUnits::MILLIBAR,
                                PublicWeatherAPI::WindSpeedUnits::METERS_PER_SECOND, nullptr, 2);
  retriever.addPublicApiService(WeatherDataRetriever::PublicAPI::OPEN_WEATHER_MAP, PublicWeatherAPI::TemperatureUnits::CELSIUS, PublicWeatherAPI::PressureUnits::MILLIBAR,
                                PublicWeatherAPI::WindSpeedUnits::METERS_PER_SECOND, owmApiKey, 1);
  retriever.addPublicApiService(WeatherDataRetriever::PublicAPI::TOMORROW_IO, PublicWeatherAPI::TemperatureUnits::CELSIUS, PublicWeatherAPI::PressureUnits::MILLIBAR,
                                PublicWeatherAPI::WindSpeedUnits::METERS_PER_SECOND, tomorrowIoApiKey, 3);
  for (int i = 0; i < 5; ++i)  // Test multiple retrievals to ensure caching works correctly
  {
    WeatherData data = retriever.getCurrentWeatherData(36.1465, -86.7937);  // Nashville, TN coordinates
    logger.log(Logger::INFO, "WeatherRetriever - Temp: %.2f C, Humidity: %.2f, Pressure: %.3f hPa\n", data.temperature, data.relativeHumidity, data.atmosphericPressure);
    switch (i)
    {
      case 0:
        retriever.removePublicApiService(WeatherDataRetriever::PublicAPI::NOAA);
        break;
      case 1:
        retriever.removePublicApiService(WeatherDataRetriever::PublicAPI::TOMORROW_IO);
        break;
      case 2:
        retriever.removePublicApiService(WeatherDataRetriever::PublicAPI::OPEN_WEATHER_MAP);
        break;
      default:
        break;
    }
  }
}

int main(void)
{
  // Initialize the necessary SDKs
  AWS::initialize();
  WeatherDataRetriever::initialize();
  auto curl = WeatherDataRetriever::getCurlInstance();

  // Retrieve the necessary API keys from AWS Secrets Manager
  AwsSecrets secretManager;
  std::string openWeatherMapAPI = secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_WEATHER_OPENWEATHERMAP_API_ID);
  std::string tomorrowIoAPI = secretManager.getParameter(CivicAlert::AWS_PARAMETER_KEY_WEATHER_TOMORROWIO_API_ID);

  // Test the various weather API implementations
  test_noaa(curl);
  test_openweathermap(curl, openWeatherMapAPI.c_str());
  test_tomorrowio(curl, tomorrowIoAPI.c_str());

  // Test the automated weather retrieval implementations
  test_weather_data_retriever(openWeatherMapAPI.c_str(), tomorrowIoAPI.c_str());
  test_weather_retriever(openWeatherMapAPI.c_str(), tomorrowIoAPI.c_str());

  // Uninitialize all SDKs
  WeatherDataRetriever::uninitialize();
  AWS::uninitialize();
  return 0;
}
