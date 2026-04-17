#ifndef __NOAA_WEATHER_API_HEADER_H__
#define __NOAA_WEATHER_API_HEADER_H__

#include <string>
#include "WeatherDataRetriever.h"

class NoaaWeatherAPI final : public PublicWeatherAPI
{
public:

  // Constructor/destructor
  NoaaWeatherAPI(TemperatureUnits unitsOfTemperature, PressureUnits unitsOfPressure, WindSpeedUnits unitsOfWindSpeed);
  ~NoaaWeatherAPI(void);

  // Weather data retrieval functions
  WeatherData getCurrentWeatherData(void* curl, float latitude, float longitude) const override;

private:

  // Private member variables
  std::string coordinatesUrlBegin, stationUrlEnd;
};

#endif  // #ifndef __NOAA_WEATHER_API_HEADER_H__
