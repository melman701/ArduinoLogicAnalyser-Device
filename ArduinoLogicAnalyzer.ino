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

  if (tryToReadConfiguration()) {
    
  }

  int count = buffer.size();
  if (count > 0 && (!config.enabled || count >= config.samplesNumber)) {
    const ChannelState chState = buffer.shift();
    sendChState(chState);
  }
}

bool tryToReadConfiguration() {
  if (Serial.available()) {
    DynamicJsonDocument doc(jsonDocSize);
    DeserializationError err = deserializeJson(doc, Serial);
    if (err == DeserializationError::Ok) {
      config.enabled = doc["enabled"].as<byte>();
      config.samplesNumber = doc["samplesNumber"].as<byte>();
      JsonArray arr = doc["chConfig"];
      if (!arr.isNull()) {
        for (int i = 0; i < arr.size() && i < chNumber; ++i) {
          ChannelConfig& chConfig = config.chConfig[i];
          chConfig.enabled = arr[i]["enabled"].as<byte>();
        }
      }
      return true;
    } else if (err == DeserializationError::NoMemory) {
      Serial.println("{\"err\":\"Not enough memory\"}");
    } else {
      Serial.println("{\"err\":\"Not parsed\"}");
    }
  }
  return false;
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

void sendChState(const ChannelState& chState) {
  Serial.print("{\"ch\":\"");
  Serial.print(chState.channel);
  Serial.print("\", \"state\":\"");
  Serial.print(chState.state);
  Serial.print("\", \"time\":\"");
  Serial.print(chState.timestamp);
  Serial.println("\"}");
}
