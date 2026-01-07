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
	@if [ -n "$(DNF_EXISTS)" ]; then \
		sudo dnf install -y make cmake gcc-c++ opus-devel libogg-devel zlib-devel libcurl-devel rapidjson-devel libpq-devel; \
		git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp.git && cd aws-sdk-cpp && mkdir build && cd build; \
		cmake .. -DCMAKE_BUILD_TYPE=Release -DCPP_STANDARD=17 -DENABLE_RTTI=OFF -DENABLE_TESTING=OFF -DENABLE_VIRTUAL_OPERATIONS=OFF -DENABLE_UNITY_BUILD=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_ONLY="s3;sqs;ssm;secretsmanager;iot"; \
		make && sudo make install && cd ../.. && rm -rf aws-sdk-cpp; \
		wget https://github.com/xiph/libopusenc/releases/download/v0.3/libopusenc-0.3.tar.gz && tar -xzf libopusenc-0.3.tar.gz && cd libopusenc-0.3; \
		./configure && make && sudo make install && cd .. && rm -rf libopusenc-0.3 libopusenc-0.3.tar.gz; \
	elif [ -n "$(APT_EXISTS)" ]; then \
		sudo apt install -y make cmake gcc-c++ libopus-dev libogg-dev zlib1g-dev libcurl4-openssl-dev rapidjson-dev libpq-dev; \
		git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp.git && cd aws-sdk-cpp && mkdir build && cd build; \
		cmake .. -DCMAKE_BUILD_TYPE=Release -DCPP_STANDARD=17 -DENABLE_RTTI=OFF -DENABLE_TESTING=OFF -DENABLE_VIRTUAL_OPERATIONS=OFF -DENABLE_UNITY_BUILD=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_ONLY="s3;sqs;ssm;secretsmanager;iot"; \
		make && sudo make install && cd ../.. && rm -rf aws-sdk-cpp; \
		wget https://github.com/xiph/libopusenc/releases/download/v0.3/libopusenc-0.3.tar.gz && tar -xzf libopusenc-0.3.tar.gz && cd libopusenc-0.3; \
		./configure && make && sudo make install && cd .. && rm -rf libopusenc-0.3 libopusenc-0.3.tar.gz; \
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
