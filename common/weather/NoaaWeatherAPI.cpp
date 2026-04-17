#include <curl/curl.h>
#include <vector>
#include "GeoMath.h"
#include "MathUtilities.h"
#include "NoaaWeatherAPI.h"
#include "JsonParser.h"

NoaaWeatherAPI::NoaaWeatherAPI(TemperatureUnits unitsOfTemperature, PressureUnits unitsOfPressure, WindSpeedUnits unitsOfWindSpeed)
    : PublicWeatherAPI(unitsOfTemperature, unitsOfPressure, unitsOfWindSpeed), coordinatesUrlBegin("https://api.weather.gov/points/"), stationUrlEnd("/observations/latest")
{
}
NoaaWeatherAPI::~NoaaWeatherAPI(void) {}

WeatherData NoaaWeatherAPI::getCurrentWeatherData(void* curl, float latitude, float longitude) const
{
  // Ask the cURL library to download a list of stations for the indicated coordinates
  std::string output;
  WeatherData weatherData;
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &PublicWeatherAPI::writeCurlDataString);
  curl_easy_setopt(curl, CURLOPT_URL, (coordinatesUrlBegin + std::to_string(latitude) + std::string("%2C") + std::to_string(longitude)).c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
  if (curl_easy_perform(curl) != CURLE_OK) return weatherData;

  // Parse the URL for local station information from the output
  const auto coordinatesObject = JsonParser::parseJsonString(output.c_str());
  if (!coordinatesObject.contains("properties") || !coordinatesObject.at("properties").asObject() ||
      !coordinatesObject.at("properties").asObject()->contains("observationStations"))
    return weatherData;
  const auto stationsURL = coordinatesObject.at("properties").asObject()->at("observationStations").asString();

  // Ask the cURL library to download a list of stations using the returned station URL
  output.clear();
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &PublicWeatherAPI::writeCurlDataString);
  curl_easy_setopt(curl, CURLOPT_URL, stationsURL.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
  if (curl_easy_perform(curl) != CURLE_OK) return weatherData;

  // Search the returned stations list for the closest station to the current location
  std::string bestStationURL;
  const auto stationsList = JsonParser::parseJsonString(output.c_str());
  if (!stationsList.contains("features")) return weatherData;
  for (const auto& station : stationsList.at("features").asObjectArray())
    if (station->contains("id"))
    {
      bestStationURL.assign(station->at("id").asString());
      break;
    }
  if (bestStationURL.empty()) return weatherData;

  // Ask the cURL library to download the station observation data using the closest station URL
  output.clear();
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &PublicWeatherAPI::writeCurlDataString);
  curl_easy_setopt(curl, CURLOPT_URL, (bestStationURL + stationUrlEnd).c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
  if (curl_easy_perform(curl) != CURLE_OK) return weatherData;

  // Parse the current weather information from the returned output
  const auto weatherOutput = JsonParser::parseJsonString(output.c_str());
  if (weatherOutput.contains("properties") && weatherOutput.at("properties").asObject())
  {
    const auto rootWeatherObject = weatherOutput.at("properties").asObject();
    if (rootWeatherObject->contains("temperature") && rootWeatherObject->at("temperature").asObject() && rootWeatherObject->at("temperature").asObject()->contains("value") &&
        rootWeatherObject->at("temperature").asObject()->at("value").asString().compare("null"))
    {
      weatherData.isValid = true;
      switch (temperatureUnits)
      {
        case TemperatureUnits::CELSIUS:
          weatherData.temperature = rootWeatherObject->at("temperature").asObject()->at("value").asFloat();
          break;
        case TemperatureUnits::KELVIN:
          weatherData.temperature = 273.15 + rootWeatherObject->at("temperature").asObject()->at("value").asFloat();
          break;
        case TemperatureUnits::FAHRENHEIT:
          weatherData.temperature = 32.0 + (rootWeatherObject->at("temperature").asObject()->at("value").asFloat() * 9.0 / 5.0);
          break;
        default:
          break;
      }
    }
    if (rootWeatherObject->contains("barometricPressure") && rootWeatherObject->at("barometricPressure").asObject() &&
        rootWeatherObject->at("barometricPressure").asObject()->contains("value") && rootWeatherObject->at("temperature").asObject()->at("value").asString().compare("null"))
      switch (pressureUnits)
      {
        case PressureUnits::ATMOSPHERE:
          weatherData.atmosphericPressure = 9.8692326671601283000246730816679e-6 * rootWeatherObject->at("barometricPressure").asObject()->at("value").asFloat();
          break;
        case PressureUnits::MILLIBAR:
          weatherData.atmosphericPressure = 0.01 * rootWeatherObject->at("barometricPressure").asObject()->at("value").asFloat();
          break;
        case PressureUnits::INCH_MERCURY:
          weatherData.atmosphericPressure = 2.952874414014310387367382186035e-4 * rootWeatherObject->at("barometricPressure").asObject()->at("value").asFloat();
          break;
        case PressureUnits::MM_MERCURY:
          weatherData.atmosphericPressure = 7.5006168270416975080187515420676e-3 * rootWeatherObject->at("barometricPressure").asObject()->at("value").asFloat();
          break;
        case PressureUnits::PASCAL:
          weatherData.atmosphericPressure = rootWeatherObject->at("barometricPressure").asObject()->at("value").asFloat();
          break;
        case PressureUnits::PSI:
          weatherData.atmosphericPressure = 1.450382432765852454971625956082e-4 * rootWeatherObject->at("barometricPressure").asObject()->at("value").asFloat();
          break;
        default:
          break;
      }
    if (rootWeatherObject->contains("relativeHumidity") && rootWeatherObject->at("relativeHumidity").asObject() &&
        rootWeatherObject->at("relativeHumidity").asObject()->contains("value") && rootWeatherObject->at("temperature").asObject()->at("value").asString().compare("null"))
      weatherData.relativeHumidity = 0.01 * rootWeatherObject->at("relativeHumidity").asObject()->at("value").asFloat();
    if (rootWeatherObject->contains("windSpeed") && rootWeatherObject->at("windSpeed").asObject() && rootWeatherObject->at("windSpeed").asObject()->contains("value") &&
        rootWeatherObject->at("temperature").asObject()->at("value").asString().compare("null"))
      switch (windSpeedUnits)
      {
        case WindSpeedUnits::METERS_PER_SECOND:
          weatherData.windSpeed = rootWeatherObject->at("windSpeed").asObject()->at("value").asFloat();
          break;
        case WindSpeedUnits::MILES_PER_HOUR:
          weatherData.windSpeed = 2.2369418519393043110840468763592 * rootWeatherObject->at("windSpeed").asObject()->at("value").asFloat();
          break;
        case WindSpeedUnits::KILOMETERS_PER_HOUR:
          weatherData.windSpeed = 3.6 * rootWeatherObject->at("windSpeed").asObject()->at("value").asFloat();
          break;
        default:
          break;
      }
  }
  return weatherData;
}
