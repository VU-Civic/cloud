#### Target and Hardware Details ####

BUILD := bin
TARGET := EvidenceCollector

SRC := main.cpp AwsServices.cpp EvidenceProcessor.cpp PacketReceiver.cpp
COMMON_SRC := Common.cpp aws.cpp mqtt.cpp s3.cpp secrets.cpp sqs.cpp Logger.cpp PostgreSQL.cpp ProcessUtils.cpp
VPATH := . ../common ../common/aws ../common/general src
OBJECTS := $(COMMON_SRC:%.cpp=$(BUILD)/%.o) $(SRC:%.cpp=$(BUILD)/%.o)

APT_EXISTS := $(shell command -v apt 2> /dev/null)
DNF_EXISTS := $(shell command -v dnf 2> /dev/null)
BREW_EXISTS := $(shell command -v brew 2> /dev/null)
USER := $(shell whoami)
GROUP := $(shell id -gn)

CXX := g++
MKDIR := @mkdir -p
RM := @rm -rf

CXXFLAGS := -O3 -march=native -std=c++17 -flto -fno-rtti -ffunction-sections -fdata-sections -Wall -Wextra -I../common -I../common/aws -I../common/general -I/usr/include -I/usr/local/include -I/usr/include/opus -I/usr/include/postgresql -I/opt/homebrew/opt/libpq/include -I/opt/homebrew/include -I/opt/homebrew/include/opus
LDFLAGS := -Wl,-rpath,/usr/local/lib -Wl,--gc-sections -L/usr/local/lib -L/usr/lib -L/opt/homebrew/lib -L/opt/homebrew/opt/libpq/lib -laws-cpp-sdk-iot -laws-cpp-sdk-ssm -laws-cpp-sdk-secretsmanager -laws-cpp-sdk-s3 -laws-cpp-sdk-sqs -laws-cpp-sdk-core -laws-crt-cpp -laws-c-sdkutils -laws-c-s3 -laws-c-mqtt -laws-c-http -laws-c-compression -laws-c-event-stream -laws-c-auth -laws-checksums -laws-c-common -laws-c-io -laws-c-cal -ls2n -lpq -lopusenc -lopus -lcurl -lcrypto -lz -lpthread


#### Build Targets ####

all: $(BUILD) $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(BUILD)/$(TARGET) $(OBJECTS) $(LDFLAGS)

setup:
	@sudo mkdir -p /var/log/civicalert/to_upload && sudo chown -R $(USER):$(GROUP) /var/log/civicalert
	@if [ -n "$(DNF_EXISTS)" ]; then \
		sudo dnf install -y make cmake gcc-c++ opus-devel libogg-devel zlib-devel libcurl-devel rapidjson-devel libpq-devel postgresql17 amazon-cloudwatch-agent; \
		git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp.git && cd aws-sdk-cpp && mkdir build && cd build; \
		cmake .. -DCMAKE_BUILD_TYPE=Release -DCPP_STANDARD=17 -DENABLE_RTTI=OFF -DENABLE_TESTING=OFF -DENABLE_VIRTUAL_OPERATIONS=OFF -DENABLE_UNITY_BUILD=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_ONLY="s3;sqs;ssm;secretsmanager;iot"; \
		make && sudo make install && cd ../.. && rm -rf aws-sdk-cpp; \
		wget https://github.com/xiph/libopusenc/releases/download/v0.3/libopusenc-0.3.tar.gz && tar -xzf libopusenc-0.3.tar.gz && cd libopusenc-0.3; \
		./configure && make && sudo make install && cd .. && rm -rf libopusenc-0.3 libopusenc-0.3.tar.gz; \
		echo -e "function civicalertdb() {\n   DB_URL=\$(aws ssm get-parameter --name DbEndpoint --query Parameter --output json | jq -r '.Value')\n   SECRETS=\$(aws secretsmanager get-secret-value --secret-id 'rds"'!'"db-bfd5b244-8d9b-45c6-be4e-9d6482fedb5a' --query SecretString --output text)\n   UNAME=\$(echo \$SECRETS | jq -r '.username')\n   PASS=\$(echo \$SECRETS | jq -r '.password')\n   PGPASSWORD=\$PASS psql -h \$DB_URL -U \$UNAME -d civicalert\n}" >> ~/.bashrc; \
		echo -e '{\n  "agent": { "metrics_collection_interval": 60 },\n  "logs": {\n    "logs_collected": {\n      "files": {\n        "collect_list": [\n          {\n            "file_path": "/var/log/civicalert/to_upload/*.log",\n            "log_group_name": "/civicalert/evidence-collector.log",\n            "log_stream_name": "{instance_id}",\n            "auto_removal": true,\n            "retention_in_days": 30,\n            "timestamp_format": "%Y-%m-%d %H:%M:%S",\n            "timezone": "UTC"\n          }\n        ]\n      }\n    }\n  },\n  "metrics": {\n    "metrics_collected": {\n      "cpu": { "measurement": [ "usage_active" ] },\n      "disk": { "measurement": [ "used_percent" ] },\n      "mem": { "measurement": [ "used_percent" ] },\n      "net": { "measurement": [ "bytes_sent", "bytes_recv" ] }\n    }\n  }\n}\n' | sudo tee /opt/aws/amazon-cloudwatch-agent/etc/amazon-cloudwatch-agent.json > /dev/null; \
		sudo amazon-cloudwatch-agent-ctl -a fetch-config -m ec2 -s -c file:/opt/aws/amazon-cloudwatch-agent/etc/amazon-cloudwatch-agent.json; \
	elif [ -n "$(APT_EXISTS)" ]; then \
		sudo apt install -y make cmake gcc-c++ libopus-dev libogg-dev zlib1g-dev libcurl4-openssl-dev rapidjson-dev libpq-dev postgresql amazon-cloudwatch-agent; \
		git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp.git && cd aws-sdk-cpp && mkdir build && cd build; \
		cmake .. -DCMAKE_BUILD_TYPE=Release -DCPP_STANDARD=17 -DENABLE_RTTI=OFF -DENABLE_TESTING=OFF -DENABLE_VIRTUAL_OPERATIONS=OFF -DENABLE_UNITY_BUILD=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_ONLY="s3;sqs;ssm;secretsmanager;iot"; \
		make && sudo make install && cd ../.. && rm -rf aws-sdk-cpp; \
		wget https://github.com/xiph/libopusenc/releases/download/v0.3/libopusenc-0.3.tar.gz && tar -xzf libopusenc-0.3.tar.gz && cd libopusenc-0.3; \
		./configure && make && sudo make install && cd .. && rm -rf libopusenc-0.3 libopusenc-0.3.tar.gz; \
		echo -e "function civicalertdb() {\n   DB_URL=\$(aws ssm get-parameter --name DbEndpoint --query Parameter --output json | jq -r '.Value')\n   SECRETS=\$(aws secretsmanager get-secret-value --secret-id 'rds"'!'"db-bfd5b244-8d9b-45c6-be4e-9d6482fedb5a' --query SecretString --output text)\n   UNAME=\$(echo \$SECRETS | jq -r '.username')\n   PASS=\$(echo \$SECRETS | jq -r '.password')\n   PGPASSWORD=\$PASS psql -h \$DB_URL -U \$UNAME -d civicalert\n}" >> ~/.bashrc; \
		echo -e '{\n  "agent": { "metrics_collection_interval": 60 },\n  "logs": {\n    "logs_collected": {\n      "files": {\n        "collect_list": [\n          {\n            "file_path": "/var/log/civicalert/to_upload/*.log",\n            "log_group_name": "/civicalert/evidence-collector.log",\n            "log_stream_name": "{instance_id}",\n            "auto_removal": true,\n            "retention_in_days": 30,\n            "timestamp_format": "%Y-%m-%d %H:%M:%S",\n            "timezone": "UTC"\n          }\n        ]\n      }\n    }\n  },\n  "metrics": {\n    "metrics_collected": {\n      "cpu": { "measurement": [ "usage_active" ] },\n      "disk": { "measurement": [ "used_percent" ] },\n      "mem": { "measurement": [ "used_percent" ] },\n      "net": { "measurement": [ "bytes_sent", "bytes_recv" ] }\n    }\n  }\n}\n' | sudo tee /opt/aws/amazon-cloudwatch-agent/etc/amazon-cloudwatch-agent.json > /dev/null; \
		sudo amazon-cloudwatch-agent-ctl -a fetch-config -m ec2 -s -c file:/opt/aws/amazon-cloudwatch-agent/etc/amazon-cloudwatch-agent.json; \
	elif [ -n "$(BREW_EXISTS)" ]; then \
		brew install make g++ opus libogg rapidjson libpq aws-sdk-cpp postgresql; \
	fi

$(BUILD):
	$(MKDIR) $@

$(BUILD)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

install:
	@sudo cp $(BUILD)/$(TARGET) /usr/local/bin/$(TARGET)
	@echo -e "[Unit]\nDescription=CivicAlert Evidence Collector\nAfter=network.target\n\n[Service]\nExecStart=/usr/local/bin/$(TARGET)\nRestart=always\nRestartSec=5s\nUser=ec2-user\nGroup=ec2-user\n\n[Install]\nWantedBy=multi-user.target\n" | sudo tee /usr/lib/systemd/system/civicalert.service > /dev/null
	@sudo systemctl daemon-reload
	@sudo systemctl enable civicalert.service
	@sudo systemctl start civicalert.service

clean:
	-$(RM) $(BUILD)

.PHONY: all setup install clean
