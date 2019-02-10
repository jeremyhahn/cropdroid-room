#include <Arduino.h>
#include <avr/wdt.h>
#include "DHT.h"
#include <Wire.h>
#include <inttypes.h>

#include "OneWire.h"
#include "DallasTemperature.h"

#define DEBUG_MODE 1
#define HYDRO_DEBUG 1
#define ONE_WIRE_BUS 5
#define DHT_ROOM_PIN 7

DHT roomDHT(DHT_ROOM_PIN, DHT22);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float roomTemp, roomHumidity, roomHeatIndex,
      pod0Temp, pod1Temp;

void readTempHumidity();
void readDallasTemps();
void debug(String msg);

int main(void) {
  init();
  setup();

  for (;;)
    loop();

  return 0;
}

void setup(void) {

  Serial.begin(115200);
  Serial.println("Initializing controller...");

  Wire.begin();
  roomDHT.begin();

  readTempHumidity();
}

void loop() {

  digitalWrite(LED_BUILTIN, HIGH);

  sensors.requestTemperatures();
  readDallasTemps();
  readTempHumidity();

  digitalWrite(LED_BUILTIN, LOW);
}


void readDallasTemps() {
  debug("Reading OneWire temperatures");
  pod0Temp = sensors.getTempFByIndex(0);
  pod1Temp = sensors.getTempFByIndex(1);

  if(HYDRO_DEBUG) {
    Serial.print("Pod 0 Temp: ");
    Serial.println(pod0Temp);
    Serial.print("Pod 1 Temp: ");
    Serial.println(pod1Temp);
  }
}

void readTempHumidity() {

  debug("Reading room temp, humidity, & heat index");
  roomTemp = roomDHT.readTemperature(true);
  roomHumidity = roomDHT.readHumidity();
  roomHeatIndex = roomDHT.computeHeatIndex(roomTemp, roomHumidity);

  if(HYDRO_DEBUG) {
    Serial.print("Room Temp: ");
    Serial.println(roomTemp);
    Serial.print("Room Humidity: ");
    Serial.println(roomHumidity);
    Serial.print("Room Heat Index: ");
    Serial.println(roomHeatIndex);
  }
}

void debug(String msg) {
  if(HYDRO_DEBUG) {
    Serial.println(msg);
  }
}
