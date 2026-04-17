#include <curl/curl.h>
#include "NoaaWeatherAPI.h"
#include "OpenWeatherMapAPI.h"
#include "TomorrowIoAPI.h"
#include "WeatherDataRetriever.h"

void* WeatherDataRetriever::curl = nullptr;
int WeatherDataRetriever::referenceCount = 0;
std::mutex WeatherDataRetriever::retrievalMutex;
std::mutex WeatherDataRetriever::initializationMutex;

void WeatherDataRetriever::initialize(void)
{
  // Only allow one complete initialization
  std::lock_guard<std::mutex> lock(initializationMutex);
  if (referenceCount++ > 0) return;

  // Perform global and local initializations
  curl_global_init(CURL_GLOBAL_ALL);
  if (curl == nullptr) curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "WeatherDataRetriever/1.0");
  curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "deflate");
}

void WeatherDataRetriever::uninitialize(void)
{
  // Only allow one complete cleanup
  std::lock_guard<std::mutex> lock(initializationMutex);
  if (--referenceCount > 0) return;
  if (referenceCount < 0)
  {
    // Disallow unmatched uninitializations
    referenceCount = 0;
    return;
  }

  // Peform global and local cleanups
  if (curl != nullptr)
  {
    curl_easy_cleanup(curl);
    curl = nullptr;
  }
  curl_global_cleanup();
}

void* WeatherDataRetriever::getCurlInstance(void) { return curl; }

PublicWeatherAPI::PublicWeatherAPI(TemperatureUnits unitsOfTemperature, PressureUnits unitsOfPressure, WindSpeedUnits unitsOfWindSpeed)
    : temperatureUnits(unitsOfTemperature), pressureUnits(unitsOfPressure), windSpeedUnits(unitsOfWindSpeed)
{
}

size_t PublicWeatherAPI::writeCurlDataString(void* downloadedData, size_t wordSize, size_t numWords, void* outputStringPtr)
{
  const auto outputSize = wordSize * numWords;
  static_cast<std::string*>(outputStringPtr)->append(static_cast<char*>(downloadedData), outputSize);
  return outputSize;
}

WeatherDataRetriever::WeatherDataRetriever(void) : weatherAPIs() {}
WeatherDataRetriever::~WeatherDataRetriever(void) {}

bool WeatherDataRetriever::addPublicApiService(PublicAPI apiType, PublicWeatherAPI::TemperatureUnits unitsOfTemperature, PublicWeatherAPI::PressureUnits unitsOfPressure,
                                               PublicWeatherAPI::WindSpeedUnits unitsOfWindSpeed, const char* apiKey, int8_t priority)
{
  // If a weather API already exists with the desired priority, return false
  if ((priority >= 0) && weatherAPIs.count(priority))
    return false;
  else if (priority == -1)
    priority = weatherAPIs.empty() ? 0 : weatherAPIs.rbegin()->first + 1;

  // Create and add a concrete implementation for the desired weather API
  switch (apiType)
  {
    case PublicAPI::NOAA:
      weatherAPIs.emplace(priority, std::unique_ptr<PublicWeatherAPI>(new NoaaWeatherAPI(unitsOfTemperature, unitsOfPressure, unitsOfWindSpeed)));
      return true;
    case PublicAPI::OPEN_WEATHER_MAP:
      if (apiKey)
      {
        weatherAPIs.emplace(priority, std::unique_ptr<PublicWeatherAPI>(new OpenWeatherMapAPI(apiKey, unitsOfTemperature, unitsOfPressure, unitsOfWindSpeed)));
        return true;
      }
      break;
    case PublicAPI::TOMORROW_IO:
      if (apiKey)
      {
        weatherAPIs.emplace(priority, std::unique_ptr<PublicWeatherAPI>(new TomorrowIoAPI(apiKey, unitsOfTemperature, unitsOfPressure, unitsOfWindSpeed)));
        return true;
      }
      break;
    default:
      break;
  }

  return false;
}

void WeatherDataRetriever::removePublicApiService(PublicAPI apiType)
{
  // Search for the weather API with the referenced type and remove it
  for (auto it = std::cbegin(weatherAPIs); it != std::cend(weatherAPIs); ++it)
  {
    bool found = false;
    switch (apiType)
    {
      case PublicAPI::NOAA:
        if (dynamic_cast<NoaaWeatherAPI*>(it->second.get()))
        {
          found = true;
          weatherAPIs.erase(it);
        }
        break;
      case PublicAPI::OPEN_WEATHER_MAP:
        if (dynamic_cast<OpenWeatherMapAPI*>(it->second.get()))
        {
          found = true;
          weatherAPIs.erase(it);
        }
        break;
      case PublicAPI::TOMORROW_IO:
        if (dynamic_cast<TomorrowIoAPI*>(it->second.get()))
        {
          found = true;
          weatherAPIs.erase(it);
        }
        break;
      default:
        break;
    }
    if (found) break;
  }
}

PublicWeatherAPI* WeatherDataRetriever::getPublicApiService(PublicAPI apiType)
{
  // Search for the weather API with the referenced type and return a reference to it
  for (auto& weatherAPI : weatherAPIs) switch (apiType)
    {
      case PublicAPI::NOAA:
        if (dynamic_cast<NoaaWeatherAPI*>(weatherAPI.second.get())) return weatherAPI.second.get();
        break;
      case PublicAPI::OPEN_WEATHER_MAP:
        if (dynamic_cast<OpenWeatherMapAPI*>(weatherAPI.second.get())) return weatherAPI.second.get();
        break;
      case PublicAPI::TOMORROW_IO:
        if (dynamic_cast<TomorrowIoAPI*>(weatherAPI.second.get())) return weatherAPI.second.get();
        break;
      default:
        break;
    }
  return nullptr;
}

WeatherData WeatherDataRetriever::getCurrentWeatherData(float latitude, float longitude) const
{
  // Ensure that the cURL library has been properly initialized
  std::lock_guard<std::mutex> lock(retrievalMutex);
  if (referenceCount <= 0) initialize();

  // Attempt to fetch the current weather from all weather APIs in order of priority until one succeeds
  WeatherData weatherData;
  for (const auto& apiService : weatherAPIs)
  {
    weatherData = apiService.second->getCurrentWeatherData(curl, latitude, longitude);
    if (weatherData.isValid) break;
  }
  return weatherData;
}
