#### Target and Hardware Details ####

BUILD := bin
TARGET := EvidenceCollector

SRC := main.cpp AwsServices.cpp EvidenceProcessor.cpp PacketReceiver.cpp
COMMON_SRC := Common.cpp aws.cpp mqtt.cpp s3.cpp secrets.cpp sqs.cpp Logger.cpp PostgreSQL.cpp CivicAlertDB.cpp ProcessUtils.cpp
VPATH := . ../common ../common/aws ../common/general src
OBJECTS := $(COMMON_SRC:%.cpp=$(BUILD)/%.o) $(SRC:%.cpp=$(BUILD)/%.o)

APT_EXISTS := $(shell command -v apt 2> /dev/null)
DNF_EXISTS := $(shell command -v dnf 2> /dev/null)
BREW_EXISTS := $(shell command -v brew 2> /dev/null)

CXX := g++
MKDIR := @mkdir -p
RM := @rm -rf

CXXFLAGS := -O3 -march=native -flto -std=c++17 -Wall -Wextra -I../common -I../common/aws -I../common/general -I/usr/include -I/usr/local/include -I/usr/include/opus -I/usr/include/postgresql -I/opt/homebrew/opt/libpq/include -I/opt/homebrew/include -I/opt/homebrew/include/opus
LDFLAGS := -L/usr/local/lib -L/usr/lib -L/opt/homebrew/lib -L/opt/homebrew/opt/libpq/lib -laws-c-common -laws-crt-cpp -laws-cpp-sdk-core -laws-cpp-sdk-ssm -laws-cpp-sdk-secretsmanager -laws-cpp-sdk-s3 -laws-cpp-sdk-sqs -lpq -lopus -lopusenc -lpthread


#### Build Targets ####

all: $(BUILD) $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(BUILD)/$(TARGET) $(OBJECTS) $(LDFLAGS)

setup:
	@if [ -n "$(DNF_EXISTS)" ] || [ -n "$(APT_EXISTS)" ] && [ ! -d "vcpkg" ]; then \
		git clone https://github.com/Microsoft/vcpkg.git && cd vcpkg && ./bootstrap-vcpkg.sh; \
	fi
	@if [ -n "$(DNF_EXISTS)" ]; then \
		sudo dnf install -y make gcc-c++ kernel-headers glibc-devel opus-devel libogg-devel zlib-devel libcurl-devel rapidjson-devel libpq-devel; \
		./vcpkg/vcpkg install aws-sdk-cpp[s3,ec2,iot] --recurse; \
	elif [ -n "$(APT_EXISTS)" ]; then \
		sudo apt install -y make gcc-c++ linux-libc-dev libopus-dev libogg-dev zlib1g-dev libcurl-dev rapidjson-dev libpq-dev; \
		./vcpkg/vcpkg install aws-sdk-cpp[s3,ec2,iot] --recurse; \
	elif [ -n "$(BREW_EXISTS)" ]; then \
		brew install make g++ opus libogg rapidjson libpq aws-sdk-cpp; \
	fi

$(BUILD):
	$(MKDIR) $@

$(BUILD)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-$(RM) $(BUILD)

.PHONY: all setup clean
