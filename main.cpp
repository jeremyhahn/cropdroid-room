#include <stdio.h>
#include <Arduino.h>
#include <Ethernet.h>
#include "DHT.h"
#include "OneWire.h"
#include "DallasTemperature.h"

#define DEBUG 1
#define BUFSIZE 100

#define ONE_WIRE_BUS 2
#define DHT_ROOM_PIN 5
#define DEHU_PIN 6
#define FAN_PIN 7
#define CO2_PIN A0
#define WATER0_PIN A1
#define WATER1_PIN A2
#define PHOTO_PIN A3

const char string_initializing[] PROGMEM = "Initializing...";
const char string_dhcp_failed[] PROGMEM = "DHCP Failed";
const char string_http_200[] PROGMEM = "HTTP/1.1 200 OK";
const char string_http_404[] PROGMEM = "HTTP/1.1 404 Not Found";
const char string_http_500[] PROGMEM = "HTTP/1.1 500 Internal Server Error";
const char string_http_xpowered_by[] PROGMEM = "X-Powered-By: Harvest Room Controller v1.0";
const char string_rest_address[] PROGMEM = "REST service listening on: ";
const char string_dehu_start[] PROGMEM = "Starting dehumidifier...";
const char string_dehu_stop[] PROGMEM = "Stopping dehumidifier...";
const char string_fan_start[] PROGMEM = "Starting fan...";
const char string_fan_stop[] PROGMEM = "Stopping fan...";
const char * const string_table[] PROGMEM = {
  string_initializing,
  string_dhcp_failed,
  string_http_200,
  string_http_404,
  string_http_500,
  string_http_xpowered_by,
  string_rest_address,
  string_dehu_start,
  string_dehu_stop,
  string_fan_start,
  string_fan_stop
};
int idx_initializing = 0,
    idx_dhcp_failed = 1,
	idx_http_200 = 2,
	idx_http_404 = 3,
	idx_http_500 = 4,
	idx_http_xpowered_by = 5,
	idx_rest_address = 6,
	idx_dehu_start = 7,
	idx_dehu_stop = 8,
	idx_fan_start = 9,
	idx_fan_stop = 10;
char string_buffer[50];

DHT roomDHT(DHT_ROOM_PIN, DHT22);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float roomTempF, roomTempC, roomHumidity, roomHeatIndex,
      pod0Temp, pod1Temp, CO2PPM, VPD = 0.0;

int waterLevel0, waterLevel1, photo = 0;

void readRoomTempHumidity();
void calculateVPD(float temp, float rh);
void readPodTemps();
void readPhoto();
void co2ppm();
void readWaterLevels();
void handleWebRequest();
void send404();
void startDehu();
void stopDehu();
void startFan();
void stopFan();
void(* resetFunc) (void) = 0;

byte mac[] = { 0x04, 0x02, 0x00, 0x00, 0x00, 0x01 };
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

  analogReference(DEFAULT);
  pinMode(DEHU_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);

#if DEBUG
  Serial.begin(115200);

  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_initializing])));
  Serial.println(string_buffer);
#endif

  if(Ethernet.begin(mac) == 0) {
    strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_dhcp_failed])));
    Serial.println(string_buffer);
    Ethernet.begin(mac, ip);
  }

  httpServer.begin();

  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_rest_address])));
  Serial.println(string_buffer);
  Serial.println(Ethernet.localIP());

  roomDHT.begin();
}

void loop() {

  digitalWrite(LED_BUILTIN, HIGH);

  sensors.requestTemperatures();
  readPodTemps();
  readRoomTempHumidity();
  readPhoto();
  calculateVPD(roomTempC, roomHumidity);
  co2ppm();
  readWaterLevels();

  handleWebRequest();

  digitalWrite(LED_BUILTIN, LOW);

  delay(2000);
}

void calculateVPD(float temp, float rh) {
  //float es = 0.6108 * exp(17.27 * temp / (temp + 237.3));
  //float ea = rh / 100 * es;
  //VPD = ea - es;

  float es = 0.6107 * exp(7.5 * temp / (temp + 237.3));
  float ea = rh / 100 * es;
  VPD = ea - es;


  //float svp = 610.78 * exp(temp / (temp + 238.3) * 17.2694);
  //VPD = svp * (1 - rh / 100);

#if DEBUG
  Serial.print("VPD: ");
  Serial.println(VPD);
#endif
}

void readPodTemps() {
  pod0Temp = sensors.getTempFByIndex(0);
  pod1Temp = sensors.getTempFByIndex(1);

#if DEBUG
  Serial.print("Pod 0 Temp: ");
  Serial.println(pod0Temp);
  Serial.print("Pod 1 Temp: ");
  Serial.println(pod1Temp);
#endif
}

void readPhoto() {
  photo = analogRead(PHOTO_PIN);
#if DEBUG
  Serial.print("Photo: ");
  Serial.println(photo);
#endif
}

void readRoomTempHumidity() {

  roomTempC = roomDHT.readTemperature(false);
  roomTempF = roomDHT.convertCtoF(roomTempC);
  roomHumidity = roomDHT.readHumidity();
  roomHeatIndex = roomDHT.computeHeatIndex(roomTempF, roomHumidity);

#if DEBUG
  Serial.print("TempF: ");
  Serial.println(roomTempF);
  Serial.print("TempC: ");
  Serial.println(roomTempC);
  Serial.print("Humidity: ");
  Serial.println(roomHumidity);
  Serial.print("HeatIndex: ");
  Serial.println(roomHeatIndex);
#endif
}

void co2ppm() {

  int sensorValue = analogRead(CO2_PIN);

  float voltage = sensorValue * (5000 / 1024.0);
  if(voltage == 0)
  {
	#if DEBUG
      Serial.println("CO2 Fault");
	#endif
  }
  else if(voltage < 400)
  {
	#if DEBUG
      Serial.println("Preheating CO2 sensor");
	#endif
  }
  else
  {
    int voltage_diference = voltage-400;
    float concentration = voltage_diference*50.0/16.0;

	#if DEBUG
	  Serial.print("co2 voltage:");
	  Serial.print(voltage);
	  Serial.println("mv");
	  Serial.print(concentration);
	  Serial.println("ppm");
	#endif
    CO2PPM = concentration;
  }
}

void readWaterLevels() {

  waterLevel0 = analogRead(WATER0_PIN) * 100 / 1024;
  waterLevel1 = analogRead(WATER1_PIN) * 100 / 1024;

#if DEBUG
  Serial.print("Water0: " );
  Serial.print(waterLevel0);
  Serial.println("%");

  Serial.print("Water1: " );
  Serial.print(waterLevel1);
  Serial.println("%");
#endif
}

void startDehu() {
#if DEBUG
  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_dehu_start])));
  Serial.println(string_buffer);
#endif
  digitalWrite(DEHU_PIN, HIGH);
}

void stopDehu() {
#if DEBUG
  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_dehu_stop])));
  Serial.println(string_buffer);
#endif
  digitalWrite(DEHU_PIN, LOW);
}

void startFan() {
#if DEBUG
  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_fan_start])));
  Serial.println(string_buffer);
#endif
  digitalWrite(FAN_PIN, HIGH);
}

void stopFan() {
#if DEBUG
  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_fan_stop])));
  Serial.println(string_buffer);
#endif
  digitalWrite(FAN_PIN, LOW);
}

void handleWebRequest() {

	httpClient = httpServer.available();

	char clientline[BUFSIZE];
	int index = 0;
	bool reset = false;

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

				char *resource = strtok(clientline, "/");
				char *param1 = strtok(NULL, "/");
				char *param2 = strtok(NULL, "/");

				//Serial.print("Resource: ");
				//Serial.println(resource);

				char json[255];
				char float_buffer[7];

				int address = atoi(param1);

				// /status
				if (strncmp(resource, "status", 6) == 0) {

					strcpy(json, "{");

					  strcat(json, "\"tempF\":");
					  dtostrf(roomTempF, 4, 2, float_buffer);
					  strcat(json, float_buffer);
					  strcat(json, ", ");

					  strcat(json, "\"tempC\":");
                      dtostrf(roomTempC, 4, 2, float_buffer);
                      strcat(json, float_buffer);
                      strcat(json, ", ");

                      strcat(json, "\"humidity\":");
                      dtostrf(roomHumidity, 4, 2, float_buffer);
					  strcat(json, float_buffer);
					  strcat(json, ", ");

					  strcat(json, "\"heatIndex\":");
					  dtostrf(roomHeatIndex, 4, 2, float_buffer);
					  strcat(json, float_buffer);
					  strcat(json, ", ");

					  strcat(json, "\"vpd\":");
					  dtostrf(VPD, 4, 2, float_buffer);
					  strcat(json, float_buffer);
					  strcat(json, ", ");

					  strcat(json, "\"pod0\":");
					  dtostrf(pod0Temp, 4, 2, float_buffer);
					  strcat(json, float_buffer);
					  strcat(json, ", ");

					  strcat(json, "\"pod1\":");
					  dtostrf(roomHeatIndex, 4, 2, float_buffer);
					  strcat(json, float_buffer);
					  strcat(json, ", ");

					  strcat(json, "\"co2\":");
					  dtostrf(CO2PPM, 4, 2, float_buffer);
					  strcat(json, float_buffer);
					  strcat(json, ", ");

					  strcat(json, "\"water0\":");
					  itoa(waterLevel0, float_buffer, 10);
					  strcat(json, float_buffer);
					  strcat(json, ", ");

					  strcat(json, "\"water1\":");
					  itoa(waterLevel1, float_buffer, 10);
					  strcat(json, float_buffer);
					  strcat(json, ", ");

					  strcat(json, "\"photo\":");
					  itoa(photo, float_buffer, 10);
					  strcat(json, float_buffer);

					strcat(json, "}");
				}
				// /dehu/?     1 = on, else off
				else if (strncmp(resource, "dehu", 4) == 0) {
					#if DEBUG
						Serial.print("Dehu: ");
						Serial.println(param1);
					#endif
					if(strncmp(param1, "1", 1) == 0) {
						startDehu();
						strcpy(json, "{\"dehu\":true}");
					}
					else {
						stopDehu();
						strcpy(json, "{\"dehu\":false}");
					}
				}
				// /fan/?     1 = on, else off
				else if (strncmp(resource, "fan", 4) == 0) {
					#if DEBUG
						Serial.print("Fan: ");
						Serial.println(param1);
					#endif
					if(strncmp(param1, "1", 1) == 0) {
						startFan();
						strcpy(json, "{\"fan\":true}");
					}
					else {
						stopFan();
						strcpy(json, "{\"fan\":false}");
					}
				}
				// /reboot
				else if (strncmp(resource, "reboot", 6) == 0) {
					#if DEBUG
						Serial.println("Rebooting...");
					#endif
					strcpy(json, "{\"reboot\":true}");
					reset = true;
				}
				else {
					send404();
					break;
				}

				strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_http_200])));
				httpClient.println(string_buffer);

				//httpClient.println("Access-Control-Allow-Origin: *");

				strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_http_xpowered_by])));
				httpClient.println(string_buffer);

				httpClient.println();
				httpClient.println(json);

				break;
			}
		}
	}

	// give the web browser time to receive the data
	delay(100);

	// close the connection:
	httpClient.stop();

	if(reset)  {
	  resetFunc();
	}
}

void send404() {

	strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_http_404])));
	httpClient.println(string_buffer);

	strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_http_xpowered_by])));
	httpClient.println(string_buffer);

	httpClient.println();
}
