#### Target and Hardware Details ####

BUILD := bin

SRC := 
COMMON_SRC := Common.cpp aws.cpp mqtt.cpp s3.cpp secrets.cpp sqs.cpp Logger.cpp PostgreSQL.cpp
VPATH := . ../common ../common/aws ../common/general
OBJECTS := $(COMMON_SRC:%.cpp=$(BUILD)/%.o) $(SRC:%.cpp=$(BUILD)/%.o)

CXX := g++
MKDIR := @mkdir -p
RM := @rm -rf

CXXFLAGS := -std=c++17 -Wall -Wextra -I../common -I../common/aws -I../common/general -I/usr/include -I/usr/local/include -I/usr/include/postgresql -I/opt/homebrew/opt/libpq/include -I/opt/homebrew/include
LDFLAGS := -L/usr/local/lib -L/usr/lib -L/opt/homebrew/lib -L/opt/homebrew/opt/libpq/lib -laws-c-common -laws-crt-cpp -laws-cpp-sdk-core -laws-cpp-sdk-ssm -laws-cpp-sdk-secretsmanager -laws-cpp-sdk-s3 -laws-cpp-sdk-sqs -lpq -lpthread


#### Build Targets ####

all:
	$(error Please specify a target to build)

AWS_SRC := test_aws.cpp
AWS_OBJ := $(AWS_SRC:%.cpp=$(BUILD)/%.o) $(OBJECTS)
aws: $(BUILD) $(AWS_OBJ)
	$(CXX) $(CXXFLAGS) -o $(BUILD)/aws $(AWS_OBJ) $(LDFLAGS)

$(BUILD):
	$(MKDIR) $@

$(BUILD)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-$(RM) $(BUILD)

.PHONY: all directories clean
