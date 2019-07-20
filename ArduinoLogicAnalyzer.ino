#include <ArduinoJson.h>
#include <CircularBuffer.h>

const byte chPins[] = { 2, 3 };
const byte chNumber = 2;
const short bufferSize = 100;
const int jsonDocSize = 2 * (JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(chNumber) + chNumber * JSON_OBJECT_SIZE(1));

struct ChannelConfig {
  bool enabled;
};

struct Config {
  bool enabled;
  byte samplesNumber;
  ChannelConfig chConfig[chNumber];
};

struct ChannelState {
  byte channel;
  byte state;
  long timestamp;
};

Config config;
CircularBuffer<ChannelState, bufferSize> buffer;
//StaticJsonDocument<jsonDocSize> doc;

void setup() {
  // put your setup code here, to run once:
  for (int i = 0; i < chNumber; ++i) {
    pinMode(chPins[i], INPUT_PULLUP);
  }

  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (config.enabled) {
    readInputsState();
  }

  readDataFromSerial();

  int count = buffer.size();
  if (count > 0 && (!config.enabled || count >= config.samplesNumber)) {
    const ChannelState chState = buffer.shift();
    sendChState(chState);
  }
}

void readDataFromSerial() {
  const byte GetInfoCmd = 1;
  const byte SetConfig = 2;
  
  if (Serial.available()) {
    DynamicJsonDocument doc(jsonDocSize);
    DeserializationError err = deserializeJson(doc, Serial);
    if (err == DeserializationError::Ok) {
      byte cmd = doc["cmd"].as<byte>();
      if (cmd == GetInfoCmd) {
        sendInfo();
      } else if (cmd == SetConfig) {
        config.enabled = doc["enabled"].as<byte>();
        config.samplesNumber = doc["samplesNumber"].as<byte>();
        JsonArray arr = doc["chConfig"];
        if (!arr.isNull()) {
          for (int i = 0; i < arr.size() && i < chNumber; ++i) {
            ChannelConfig& chConfig = config.chConfig[i];
            chConfig.enabled = arr[i]["enabled"].as<byte>();
          }
        }
      }
    } else if (err == DeserializationError::NoMemory) {
      Serial.println("{\"err\":\"Not enough memory\"}");
    } else {
      Serial.println("{\"err\":\"Not parsed\"}");
    }
  }
}

void readInputsState() {
  for (int i = 0; i < chNumber && !buffer.isFull(); ++i) {
    if (config.chConfig[i].enabled) {
      ChannelState chState;
      chState.channel = i;
      chState.state = digitalRead(chPins[i]);
      chState.timestamp = micros();
      buffer.push(chState);
    }
  }
}

void sendInfo() {
  Serial.print("{\"type\":\"info\",\"channels\":");
  Serial.print(chNumber);
  Serial.println("}");
}

void sendChState(const ChannelState& chState) {
  Serial.print("{\"type\":\"data\",\"ch\":\"");
  Serial.print(chState.channel);
  Serial.print("\", \"state\":\"");
  Serial.print(chState.state);
  Serial.print("\", \"time\":\"");
  Serial.print(chState.timestamp);
  Serial.println("\"}");
}
