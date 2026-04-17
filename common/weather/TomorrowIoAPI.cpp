#include <curl/curl.h>
#include "JsonParser.h"
#include "TomorrowIoAPI.h"

TomorrowIoAPI::TomorrowIoAPI(const char* tomorrowIoApiKey, TemperatureUnits unitsOfTemperature, PressureUnits unitsOfPressure, WindSpeedUnits unitsOfWindSpeed)
    : PublicWeatherAPI(unitsOfTemperature, unitsOfPressure, unitsOfWindSpeed),
      apiKey(tomorrowIoApiKey),
      weatherUrlBegin("https://api.tomorrow.io/v4/weather/realtime?location="),
      weatherUrlEnd(std::string("&units=").append((unitsOfTemperature == TemperatureUnits::FAHRENHEIT) ? "imperial" : "metric").append("&apikey=").append(tomorrowIoApiKey))
{
}
TomorrowIoAPI::~TomorrowIoAPI(void) {}

WeatherData TomorrowIoAPI::getCurrentWeatherData(void* curl, float latitude, float longitude) const
{
  // Ask the cURL library to download the current weather information
  std::string output;
  WeatherData weatherData;
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &PublicWeatherAPI::writeCurlDataString);
  curl_easy_setopt(curl, CURLOPT_URL, (weatherUrlBegin + std::to_string(latitude) + std::string("%2C") + std::to_string(longitude) + weatherUrlEnd).c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
  if (curl_easy_perform(curl) != CURLE_OK) return weatherData;

  // Parse the relevant weather information from the output
  const auto weatherObject = JsonParser::parseJsonString(output.c_str());
  if (weatherObject.contains("data") && weatherObject.at("data").asObject() && weatherObject.at("data").asObject()->contains("values") &&
      weatherObject.at("data").asObject()->at("values").asObject())
  {
    weatherData.isValid = true;
    const auto rootWeatherObject = weatherObject.at("data").asObject()->at("values").asObject();
    if (rootWeatherObject->contains("temperature")) weatherData.temperature = rootWeatherObject->at("temperature").asFloat();
    if (rootWeatherObject->contains("pressureSeaLevel")) switch (pressureUnits)
      {
        case PressureUnits::ATMOSPHERE:
          weatherData.atmosphericPressure = 9.8692326671601283000246730816679e-4 * rootWeatherObject->at("pressureSeaLevel").asFloat();
          break;
        case PressureUnits::MILLIBAR:
          weatherData.atmosphericPressure = rootWeatherObject->at("pressureSeaLevel").asFloat();
          break;
        case PressureUnits::INCH_MERCURY:
          weatherData.atmosphericPressure = 0.02952874414014310387367382186035 * rootWeatherObject->at("pressureSeaLevel").asFloat();
          break;
        case PressureUnits::MM_MERCURY:
          weatherData.atmosphericPressure = 0.75006168270416975080187515420676 * rootWeatherObject->at("pressureSeaLevel").asFloat();
          break;
        case PressureUnits::PASCAL:
          weatherData.atmosphericPressure = 100.0 * rootWeatherObject->at("pressureSeaLevel").asFloat();
          break;
        case PressureUnits::PSI:
          weatherData.atmosphericPressure = 0.01450382432765852454971625956082 * rootWeatherObject->at("pressureSeaLevel").asFloat();
          break;
        default:
          break;
      }
    if (rootWeatherObject->contains("humidity")) weatherData.relativeHumidity = 0.01 * rootWeatherObject->at("humidity").asFloat();
    if (rootWeatherObject->contains("windSpeed")) switch (windSpeedUnits)
      {
        case WindSpeedUnits::METERS_PER_SECOND:
          if (temperatureUnits == TemperatureUnits::FAHRENHEIT)
            weatherData.windSpeed = 0.44703888888888888888888888888889 * rootWeatherObject->at("windSpeed").asFloat();
          else
            weatherData.windSpeed = rootWeatherObject->at("windSpeed").asFloat();
          break;
        case WindSpeedUnits::MILES_PER_HOUR:
          if (temperatureUnits == TemperatureUnits::FAHRENHEIT)
            weatherData.windSpeed = rootWeatherObject->at("windSpeed").asFloat();
          else
            weatherData.windSpeed = 2.2369418519393043110840468763592 * rootWeatherObject->at("windSpeed").asFloat();
          break;
        case WindSpeedUnits::KILOMETERS_PER_HOUR:
          if (temperatureUnits == TemperatureUnits::FAHRENHEIT)
            weatherData.windSpeed = 1.609344 * rootWeatherObject->at("windSpeed").asFloat();
          else
            weatherData.windSpeed = 3.6 * rootWeatherObject->at("windSpeed").asFloat();
          break;
        default:
          break;
      }
  }
  return weatherData;
}
