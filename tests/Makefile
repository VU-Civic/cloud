#### Target and Hardware Details ####

BUILD := bin

SRC := 
COMMON_SRC := aws.cpp mqtt.cpp s3.cpp secrets.cpp sqs.cpp GeoMath.cpp JsonParser.cpp Logger.cpp PostgreSQL.cpp ProcessUtils.cpp Transcoding.cpp NoaaWeatherAPI.cpp OpenWeatherMapAPI.cpp TomorrowIoAPI.cpp WeatherDataRetriever.cpp WeatherRetriever.cpp
VPATH := . ../common ../common/aws ../common/general ../common/weather ../fusion/src
OBJECTS := $(COMMON_SRC:%.cpp=$(BUILD)/%.o) $(SRC:%.cpp=$(BUILD)/%.o)

CXX := g++
MKDIR := @mkdir -p
RM := @rm -rf

CXXFLAGS := -std=c++17 -Wall -Wextra -I../fusion/src -I../common -I../common/aws -I../common/general -I../common/weather -I/usr/include -I/usr/local/include -I/usr/include/postgresql -I/opt/homebrew/opt/libpq/include -I/opt/homebrew/include
LDFLAGS := -L/usr/local/lib -L/usr/lib -L/opt/homebrew/lib -L/opt/homebrew/opt/libpq/lib -laws-c-common -laws-crt-cpp -laws-cpp-sdk-core -laws-cpp-sdk-ssm -laws-cpp-sdk-secretsmanager -laws-cpp-sdk-s3 -laws-cpp-sdk-sqs -lpq -lcurl -lcrypto -lssl -lpthread


#### Build Targets ####

all:
	$(error Please specify a target to build)

AWS_SRC := test_aws.cpp
AWS_OBJ := $(AWS_SRC:%.cpp=$(BUILD)/%.o) $(OBJECTS)
aws: $(BUILD) $(AWS_OBJ)
	$(CXX) $(CXXFLAGS) -o $(BUILD)/aws $(AWS_OBJ) $(LDFLAGS)

GEOMATH_SRC := test_geomath.cpp
GEOMATH_OBJ := $(GEOMATH_SRC:%.cpp=$(BUILD)/%.o) $(OBJECTS)
geomath: $(BUILD) $(GEOMATH_OBJ)
	$(CXX) $(CXXFLAGS) -o $(BUILD)/geomath $(GEOMATH_OBJ) $(LDFLAGS)

TRANSCODING_SRC := test_transcoding.cpp
TRANSCODING_OBJ := $(TRANSCODING_SRC:%.cpp=$(BUILD)/%.o) $(OBJECTS)
transcoding: $(BUILD) $(TRANSCODING_OBJ)
	$(CXX) $(CXXFLAGS) -o $(BUILD)/transcoding $(TRANSCODING_OBJ) $(LDFLAGS)

WEATHER_SRC := test_weather.cpp
WEATHER_OBJ := $(WEATHER_SRC:%.cpp=$(BUILD)/%.o) $(OBJECTS)
weather: $(BUILD) $(WEATHER_OBJ)
	$(CXX) $(CXXFLAGS) -o $(BUILD)/weather $(WEATHER_OBJ) $(LDFLAGS)

$(BUILD):
	$(MKDIR) $@

$(BUILD)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-$(RM) $(BUILD)

.PHONY: all directories clean
