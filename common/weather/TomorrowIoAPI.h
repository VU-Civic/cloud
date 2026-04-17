#ifndef __TOMORROW_IO_API_HEADER_H__
#define __TOMORROW_IO_API_HEADER_H__

#include <string>
#include "WeatherDataRetriever.h"

class TomorrowIoAPI final : public PublicWeatherAPI
{
public:

  // Constructor/destructor
  TomorrowIoAPI(const char* tomorrowIoApiKey, TemperatureUnits unitsOfTemperature, PressureUnits unitsOfPressure, WindSpeedUnits unitsOfWindSpeed);
  ~TomorrowIoAPI(void);

  // Weather data retrieval functions
  WeatherData getCurrentWeatherData(void* curl, float latitude, float longitude) const override;

private:

  // Private member variables
  const std::string apiKey;
  std::string weatherUrlBegin, weatherUrlEnd;
};

#endif  // #ifndef __TOMORROW_IO_API_HEADER_H__
