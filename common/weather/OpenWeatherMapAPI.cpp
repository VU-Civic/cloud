#include <curl/curl.h>
#include "OpenWeatherMapAPI.h"
#include "JsonParser.h"

OpenWeatherMapAPI::OpenWeatherMapAPI(const char* openWeatherMapApiKey, TemperatureUnits unitsOfTemperature, PressureUnits unitsOfPressure, WindSpeedUnits unitsOfWindSpeed)
    : PublicWeatherAPI(unitsOfTemperature, unitsOfPressure, unitsOfWindSpeed),
      apiKey(openWeatherMapApiKey),
      weatherUrlBegin("https://api.openweathermap.org/data/3.0/onecall?lat="),
      weatherUrlEnd(std::string("&units=")
                        .append((unitsOfTemperature == TemperatureUnits::CELSIUS) ? "metric" : "imperial")
                        .append("&exclude=minutely,hourly,daily,alerts")
                        .append("&appid=")
                        .append(openWeatherMapApiKey))
{
}
OpenWeatherMapAPI::~OpenWeatherMapAPI(void) {}

WeatherData OpenWeatherMapAPI::getCurrentWeatherData(void* curl, float latitude, float longitude) const
{
  // Ask the cURL library to download the current weather information
  std::string output;
  WeatherData weatherData;
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &PublicWeatherAPI::writeCurlDataString);
  curl_easy_setopt(curl, CURLOPT_URL, (weatherUrlBegin + std::to_string(latitude) + std::string("&lon=") + std::to_string(longitude) + weatherUrlEnd).c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
  if (curl_easy_perform(curl) != CURLE_OK) return weatherData;

  // Parse the relevant weather information from the output
  const auto weatherObject = JsonParser::parseJsonString(output.c_str());
  if (weatherObject.contains("current") && weatherObject.at("current").asObject())
  {
    const auto rootWeatherObject = weatherObject.at("current").asObject();
    weatherData.isValid = true;
    if (rootWeatherObject->contains("temp")) weatherData.temperature = rootWeatherObject->at("temp").asFloat();
    if (rootWeatherObject->contains("pressure")) switch (pressureUnits)
      {
        case PressureUnits::ATMOSPHERE:
          weatherData.atmosphericPressure = 9.8692326671601283000246730816679e-4 * rootWeatherObject->at("pressure").asFloat();
          break;
        case PressureUnits::MILLIBAR:
          weatherData.atmosphericPressure = rootWeatherObject->at("pressure").asFloat();
          break;
        case PressureUnits::INCH_MERCURY:
          weatherData.atmosphericPressure = 0.02952874414014310387367382186035 * rootWeatherObject->at("pressure").asFloat();
          break;
        case PressureUnits::MM_MERCURY:
          weatherData.atmosphericPressure = 0.75006168270416975080187515420676 * rootWeatherObject->at("pressure").asFloat();
          break;
        case PressureUnits::PASCAL:
          weatherData.atmosphericPressure = 100.0 * rootWeatherObject->at("pressure").asFloat();
          break;
        case PressureUnits::PSI:
          weatherData.atmosphericPressure = 0.01450382432765852454971625956082 * rootWeatherObject->at("pressure").asFloat();
          break;
        default:
          break;
      }
    if (rootWeatherObject->contains("humidity")) weatherData.relativeHumidity = 0.01 * rootWeatherObject->at("humidity").asFloat();
    if (rootWeatherObject->contains("wind_speed")) switch (windSpeedUnits)
      {
        case WindSpeedUnits::METERS_PER_SECOND:
          if (temperatureUnits == TemperatureUnits::FAHRENHEIT)
            weatherData.windSpeed = 0.44703888888888888888888888888889 * rootWeatherObject->at("wind_speed").asFloat();
          else
            weatherData.windSpeed = rootWeatherObject->at("wind_speed").asFloat();
          break;
        case WindSpeedUnits::MILES_PER_HOUR:
          if (temperatureUnits == TemperatureUnits::FAHRENHEIT)
            weatherData.windSpeed = rootWeatherObject->at("wind_speed").asFloat();
          else
            weatherData.windSpeed = 2.2369418519393043110840468763592 * rootWeatherObject->at("wind_speed").asFloat();
          break;
        case WindSpeedUnits::KILOMETERS_PER_HOUR:
          if (temperatureUnits == TemperatureUnits::FAHRENHEIT)
            weatherData.windSpeed = 1.609344 * rootWeatherObject->at("wind_speed").asFloat();
          else
            weatherData.windSpeed = 3.6 * rootWeatherObject->at("wind_speed").asFloat();
          break;
        default:
          break;
      }
  }
  return weatherData;
}
