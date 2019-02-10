#include <Arduino.h>
#include <avr/wdt.h>
#include <Wire.h>
#include <inttypes.h>
#include "DHT.h"
#include "OneWire.h"
#include "DallasTemperature.h"

#define DEBUG_MODE 1
#define HYDRO_DEBUG 1
#define ONE_WIRE_BUS 5
#define DHT_ROOM_PIN 7
#define CO2_PIN A0
#define WATER0_PIN A1
#define WATER1_PIN A2

DHT roomDHT(DHT_ROOM_PIN, DHT22);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float roomTemp, roomHumidity, roomHeatIndex,
      pod0Temp, pod1Temp, CO2PPM;

int waterLevel0, waterLevel1;

void readRoomTempHumidity();
void readPodTemps();
void co2ppm();
void readWaterLevels();
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
  Serial.println("Initializing room controller...");

  analogReference(DEFAULT);

  Wire.begin();
  roomDHT.begin();

  Serial.println("Starting...");
}

void loop() {

  digitalWrite(LED_BUILTIN, HIGH);

  sensors.requestTemperatures();
  readPodTemps();
  readRoomTempHumidity();
  co2ppm();
  readWaterLevels();

  digitalWrite(LED_BUILTIN, LOW);

  delay(2000);
}

void readPodTemps() {
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

void readRoomTempHumidity() {

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

void co2ppm() {

  int sensorValue = analogRead(CO2_PIN);

  // The analog signal is converted to a voltage
  float voltage = sensorValue*(5100/1024.0);
  if(voltage == 0)
  {
    Serial.println("CO2 Fault");
  }
  else if(voltage < 400)
  {
    Serial.println("Preheating CO2 sensor");
  }
  else
  {
    int voltage_diference = voltage-400;
    float concentration = voltage_diference*50.0/16.0;
    Serial.print("co2 voltage:");
    Serial.print(voltage);
    Serial.println("mv");
    Serial.print(concentration);
    Serial.println("ppm");
    CO2PPM = concentration;
  }
}

void readWaterLevels() {

	waterLevel0 = analogRead(WATER0_PIN) * 100 / 1024;
	waterLevel1 = analogRead(WATER1_PIN) * 100 / 1024;

	if(HYDRO_DEBUG) {
	 Serial.print("Water Level 0: " );
	 Serial.print(waterLevel0);
	 Serial.println("%");

	 Serial.print("Water Level 1: " );
	 Serial.print(waterLevel1);
	 Serial.println("%");
	}
}

void debug(String msg) {
  if(HYDRO_DEBUG) {
    Serial.println(msg);
  }
}
