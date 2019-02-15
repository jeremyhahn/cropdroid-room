#include <Arduino.h>
#include <Ethernet.h>
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
#define BUFSIZE 100

//const char PROGMEM_initController[] PROGMEM  = {"Initializing controller..."};

DHT roomDHT(DHT_ROOM_PIN, DHT22);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float roomTempF, roomTempC, roomHumidity, roomHeatIndex,
      pod0Temp, pod1Temp, CO2PPM, VPD;

int pod0WaterLevel, pod1WaterLevel;

void readRoomTempHumidity();
void calculateVPD(float temp, float rh);
void readPodTemps();
void co2ppm();
void readWaterLevels();
void handleWebRequest();
void sendHtmlHeader();
void send404();

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xFE, 0x41 };
IPAddress ip(192,168,0,200);

EthernetServer httpServer(80);
EthernetClient httpClient;

int main(void) {
  init();
  setup();;

  for (;;)
    loop();

  return 0;
}

void setup(void) {

  Serial.begin(115200);
  Serial.println("Initializing controller...");

  analogReference(DEFAULT);

  if(Ethernet.begin(mac) == 0) {
    Serial.println("DHCP failed");
    Ethernet.begin(mac, ip);
  }

  httpServer.begin();

  Serial.print("REST server listening on ");
  Serial.println(Ethernet.localIP());

  roomDHT.begin();

  Serial.println("Starting...");
}

void loop() {

  digitalWrite(LED_BUILTIN, HIGH);

  sensors.requestTemperatures();
  readPodTemps();
  readRoomTempHumidity();
  calculateVPD(roomTempC, roomHumidity);
  co2ppm();
  readWaterLevels();

  handleWebRequest();

  digitalWrite(LED_BUILTIN, LOW);

  delay(2000);
}

void calculateVPD(float temp, float rh) {
  float es = 0.6108 * exp(17.27 * temp / (temp + 237.3));
  float ea = rh / 100 * es;
  VPD = ea - es;
    Serial.print("VPD: ");
    Serial.println(VPD);
}

void readPodTemps() {
  pod0Temp = sensors.getTempFByIndex(0);
  pod1Temp = sensors.getTempFByIndex(1);

    Serial.print("Pod 0 Temp: ");
    Serial.println(pod0Temp);
    Serial.print("Pod 1 Temp: ");
    Serial.println(pod1Temp);
}

void readRoomTempHumidity() {

  roomTempC = roomDHT.readTemperature(false);
  roomTempF = roomDHT.convertCtoF(roomTempC);
  roomHumidity = roomDHT.readHumidity();
  roomHeatIndex = roomDHT.computeHeatIndex(roomTempF, roomHumidity);

    Serial.print("Room Temp F: ");
    Serial.println(roomTempF);
    Serial.print("Room Temp C: ");
    Serial.println(roomTempC);
    Serial.print("Room Humidity: ");
    Serial.println(roomHumidity);
    Serial.print("Room Heat Index: ");
    Serial.println(roomHeatIndex);
}

void co2ppm() {

  int sensorValue = analogRead(CO2_PIN);

  // The analog signal is converted to a voltage
  float voltage = sensorValue * (5000 / 1024.0);
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

  pod0WaterLevel = analogRead(WATER0_PIN) * 100 / 1024;
  pod1WaterLevel = analogRead(WATER1_PIN) * 100 / 1024;

   Serial.print("Water Level 0: " );
   Serial.print(pod0WaterLevel);
   Serial.println("%");

   Serial.print("Water Level 1: " );
   Serial.print(pod1WaterLevel);
   Serial.println("%");
}

void handleWebRequest() {

	httpClient = httpServer.available();

	char clientline[BUFSIZE];
	int index = 0;

	if (httpClient) {

		// reset input buffer
		index = 0;

		while (httpClient.connected()) {

			if (httpClient.available()) {

				char c = httpClient.read();
				if (c != '\n' && c != '\r' && index < BUFSIZE) {
					clientline[index++] = c;
					continue;
				}

				httpClient.flush();

				String urlString = String(clientline);
				String op = urlString.substring(0, urlString.indexOf(' '));
				urlString = urlString.substring(urlString.indexOf('/'), urlString.indexOf(' ', urlString.indexOf('/')));
				urlString.toCharArray(clientline, BUFSIZE);

				//char *key = strtok(clientline, "/");
				//char *resource = strtok(NULL, "/");

				char *resource = strtok(clientline, "/");

				Serial.print("HTTP Resource: ");
				Serial.println(resource);

				char *param1 = strtok(NULL, "/");
				char *param2 = strtok(NULL, "/");

				String jsonOut = String();
				int address = atoi(param1);

				// /status
				if (strncmp(resource, "status", 6) == 0) {

					jsonOut += "{";
						jsonOut += "\"roomTempF\":\"" + String(roomTempF) + "\", ";
						jsonOut += "\"roomTempC\":\"" + String(roomTempC) + "\", ";
						jsonOut += "\"roomHumidity\":\"" + String(roomHumidity) + "\", ";
						jsonOut += "\"roomHeatIndex\":\"" + String(roomHeatIndex) + "\", ";
						jsonOut += "\"VPD\":\"" + String(VPD) + "\", ";
						jsonOut += "\"pod0Temp\":\"" + String(pod0Temp) + "\", ";
						jsonOut += "\"pod1Temp\":\"" + String(pod1Temp) + "\", ";
						jsonOut += "\"CO2PPM\":\"" + String(CO2PPM) + "\", ";
						jsonOut += "\"pod0WaterLevel\":\"" + String(pod0WaterLevel) + "\", ";
						jsonOut += "\"pod1WaterLevel\":\"" + String(pod1WaterLevel) + "\" ";
					jsonOut += "}";
				}
				else {
					send404();
					break;
				}

				httpClient.println("HTTP/1.1 200 OK");
				//httpClient.println("Content-Type: text/html");
				//httpClient.println("Access-Control-Allow-Origin: *");
				//httpClient.println("X-Powered-By: Harvest Room Controller v1.0");
				httpClient.println();
				httpClient.println(jsonOut);

				break;
			}
		}
	}

	// give the web browser time to receive the data
	delay(100);

	// close the connection:
	httpClient.stop();
}

void sendHtmlHeader() {

	httpClient.println("<h5>Harvest Room Controller v1.0</h5>");
}

void send404() {

	httpClient.println("HTTP/1.1 404 Not Found");
	//httpClient.println("Content-Type: text/html");
	//httpClient.println("X-Powered-By: Harvest Room Controller v1.0");
	httpClient.println();

	//sendHtmlHeader();
	//httpClient.println("<h1>Not Found</h1>");
}
