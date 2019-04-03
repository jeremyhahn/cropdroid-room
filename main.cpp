#include <stdio.h>
#include <Arduino.h>
#include <Ethernet.h>
#include "DHT.h"
#include "OneWire.h"
#include "DallasTemperature.h"
#include "EEPROM.h"

#define DEBUG 0
#define EEPROM_DEBUG 1
#define BUFSIZE 100

#define ONE_WIRE_BUS 2
#define DHT_ROOM_PIN 3
#define CO2_PIN A0
#define WATER0_PIN A1
#define WATER1_PIN A2
#define PHOTO_PIN A3

extern int  __bss_end;
extern int  *__brkval;

const int channel_size = 10;
const int valid_channels[channel_size] = {
	4, 5, 6, 7, 8, 9,
	A4, A5, A6, A7
};

const char json_bracket_open[] = "{";
const char json_bracket_close[] = "}";

const char string_initializing[] PROGMEM = "Initializing room controller...";
const char string_dhcp_failed[] PROGMEM = "DHCP Failed";
const char string_http_200[] PROGMEM = "HTTP/1.1 200 OK";
const char string_http_404[] PROGMEM = "HTTP/1.1 404 Not Found";
const char string_http_500[] PROGMEM = "HTTP/1.1 500 Internal Server Error";
const char string_http_content_type_json[] PROGMEM = "Content-Type: application/json";
const char string_http_xpowered_by[] PROGMEM = "X-Powered-By: CropDroid v1.0";
const char string_rest_address[] PROGMEM = "REST service listening on: ";
const char string_switch_on[] PROGMEM = "Switching on";
const char string_switch_off[] PROGMEM = "Switching off";
const char string_json_key_mem[] PROGMEM = "\"mem\":";
const char string_json_key_tempF[] PROGMEM = ",\"tempF\":";
const char string_json_key_tempC[] PROGMEM = ",\"tempC\":";
const char string_json_key_humidity[] PROGMEM = ",\"humidity\":";
const char string_json_key_heatIndex[] PROGMEM = ",\"heatIndex\":";
const char string_json_key_vpd[] PROGMEM = ",\"vpd\":";
const char string_json_key_pod0[] PROGMEM = ",\"pod0\":";
const char string_json_key_pod1[] PROGMEM = ",\"pod1\":";
const char string_json_key_co2[] PROGMEM = ",\"co2\":";
const char string_json_key_water0[] PROGMEM = ",\"water0\":";
const char string_json_key_water1[] PROGMEM = ",\"water1\":";
const char string_json_key_photo[] PROGMEM = ",\"photo\":";
const char string_json_key_channels[] PROGMEM = ",\"channels\":{";
const char string_json_key_pin[] PROGMEM = "\"pin\":";
const char string_json_key_position[] PROGMEM =  ",\"position\":";
const char string_json_key_value[] PROGMEM =  ",\"value\":";
const char string_json_key_address[] PROGMEM =  "\"address\":";
const char string_json_bracket_open[] PROGMEM = "{";
const char string_json_bracket_close[] PROGMEM = "}";
const char string_json_error_invalid_channel[] PROGMEM = "{\"error\":\"Invalid channel\"}";
const char string_json_reboot_true PROGMEM = "{\"reboot\":true}";
const char string_json_reset_true PROGMEM = "{\"reset\":true}";
const char * const string_table[] PROGMEM = {
  string_initializing,
  string_dhcp_failed,
  string_http_200,
  string_http_404,
  string_http_500,
  string_http_content_type_json,
  string_http_xpowered_by,
  string_rest_address,
  string_switch_on,
  string_switch_off,
  string_json_key_mem,
  string_json_key_tempF,
  string_json_key_tempC,
  string_json_key_humidity,
  string_json_key_heatIndex,
  string_json_key_vpd,
  string_json_key_pod0,
  string_json_key_pod1,
  string_json_key_co2,
  string_json_key_water0,
  string_json_key_water1,
  string_json_key_photo,
  string_json_key_channels,
  string_json_key_pin,
  string_json_key_position,
  string_json_key_value,
  string_json_key_address,
  string_json_bracket_open,
  string_json_bracket_close,
  string_json_error_invalid_channel,
  string_json_reboot_true,
  string_json_reset_true
};
int idx_initializing = 0,
    idx_dhcp_failed = 1,
	idx_http_200 = 2,
	idx_http_404 = 3,
	idx_http_500 = 4,
	idx_http_content_type_json = 5,
	idx_http_xpowered_by = 6,
	idx_rest_address = 7,
	idx_switch_on = 8,
	idx_switch_off = 9,
	idx_json_key_mem = 10,
	idx_json_key_tempF = 11,
	idx_json_key_tempC = 12,
	idx_json_key_humidity = 13,
	idx_json_key_heatIndex = 14,
	idx_json_key_vpd = 15,
	idx_json_key_pod0 = 16,
	idx_json_key_pod1 = 17,
	idx_json_key_co2 = 18,
	idx_json_key_water0 = 19,
	idx_json_key_water1 = 20,
	idx_json_key_photo = 21,
	idx_json_key_channels = 22,
	idx_json_key_pin = 23,
	idx_json_key_position = 24,
	idx_json_key_value = 25,
	idx_json_key_address = 26,
	idx_json_key_bracket_open = 27,
	idx_json_key_bracket_close = 28,
	idx_json_error_invalid_channel = 29,
	idx_json_reboot_true = 30,
	idx_json_reset_true = 31;
char string_buffer[50];
char float_buffer[10];

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DHT roomDHT(DHT_ROOM_PIN, DHT22);

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
void switchOn(int pin);
void switchOff(int pin);
void resetDefaults();
int availableMemory();
void(* resetFunc) (void) = 0;

byte defaultMac[] = { 0x04, 0x02, 0x00, 0x00, 0x00, 0x01 };

byte mac[] = {
	EEPROM.read(0),
	EEPROM.read(1),
	EEPROM.read(2),
	EEPROM.read(3),
	EEPROM.read(4),
	EEPROM.read(5)
};
IPAddress ip(
  EEPROM.read(6),
  EEPROM.read(7),
  EEPROM.read(8),
  EEPROM.read(9));

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

  //EEPROM.write(0, 0x04);

  analogReference(DEFAULT);

  for(int i=0; i<channel_size; i++) {
	  pinMode(i, OUTPUT);
	  digitalWrite(i, LOW);
  }

  #if DEBUG || EEPROM_DEBUG
    Serial.begin(115200);

    strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_initializing])));
    Serial.println(string_buffer);
  #endif

  byte macByte1 = EEPROM.read(0);

  #if DEBUG || EEPROM_DEBUG
    Serial.print("EEPROM.read(0): ");
    Serial.println(macByte1);
  #endif

  if(macByte1 == 255) {
    if(Ethernet.begin(defaultMac) == 0) {
	  #if DEBUG
	    strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_dhcp_failed])));
		Serial.println(string_buffer);
	  #endif
	  return;
	}
  }
  else {
    Ethernet.begin(mac, ip);
  }

  #if DEBUG || EEPROM_DEBUG
    Serial.print("MAC: ");
    for(int i=0; i<6; i++) {
      Serial.print(mac[i], HEX);
    }
    Serial.println();

    strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_rest_address])));
    Serial.print(string_buffer);
    Serial.println(Ethernet.localIP());
  #endif

  httpServer.begin();
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

  delay(2000); // DHT22 requires min 2 seconds between polls
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

  if(voltage == 0) {
	#if DEBUG
      Serial.println("CO2 Fault");
	#endif
  }
  else if(voltage < 400) {
	#if DEBUG
      Serial.println("Preheating CO2 sensor");
	#endif
  }
  else {
    int voltage_diference = voltage - 400;
    float concentration = voltage_diference *50.0 / 16.0;
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

  itoa(waterLevel0, float_buffer, 10);
  strcpy(string_buffer, "Water0: ");
  strcat(string_buffer, float_buffer);
  strcat(string_buffer, "%");
  Serial.println(string_buffer);

  itoa(waterLevel1, float_buffer, 10);
  strcpy(string_buffer, "Water1: ");
  strcat(string_buffer, float_buffer);
  strcat(string_buffer, "%");
  Serial.println(string_buffer);

#endif
}

void switchOn(int pin) {
#if DEBUG
  char sPin[3];
  itoa(pin, sPin, 10);
  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_switch_on])));
  strcat(string_buffer, " pin ");
  strcat(string_buffer, sPin);
  Serial.println(string_buffer);
#endif
  digitalWrite(pin, HIGH);
}

void switchOff(int pin) {
#if DEBUG
  char sPin[3];
  itoa(pin, sPin, 10);
  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_switch_off])));
  strcat(string_buffer, " pin ");
  strcat(string_buffer, sPin);
  Serial.println(string_buffer);
#endif
  digitalWrite(pin, LOW);
}

void handleWebRequest() {

	httpClient = httpServer.available();

	char clientline[BUFSIZE];
	int index = 0;

	bool reboot = false;
	char json[275];
	char sPin[3];
	char state[3];

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

				#if DEBUG
				  Serial.print("Resource: ");
				  Serial.println(resource);

				  Serial.print("Parm1: ");
				  Serial.println(param1);

				  Serial.print("Param2: ");
				  Serial.println(param2);
				#endif

				// /status
				if (strncmp(resource, "status", 6) == 0) {

					strcpy(json, json_bracket_open);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_mem])));
					  strcat(json, string_buffer);
					  itoa(availableMemory(), float_buffer, 10);
					  strcat(json, float_buffer);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_tempF])));
					  strcat(json, string_buffer);
					  dtostrf(roomTempF, 4, 2, float_buffer);
					  strcat(json, float_buffer);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_tempC])));
					  strcat(json, string_buffer);
                      dtostrf(roomTempC, 4, 2, float_buffer);
                      strcat(json, float_buffer);

                      strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_humidity])));
					  strcat(json, string_buffer);
                      dtostrf(roomHumidity, 4, 2, float_buffer);
					  strcat(json, float_buffer);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_heatIndex])));
					  strcat(json, string_buffer);
					  dtostrf(roomHeatIndex, 4, 2, float_buffer);
					  strcat(json, float_buffer);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_vpd])));
					  strcat(json, string_buffer);
					  dtostrf(VPD, 4, 2, float_buffer);
					  strcat(json, float_buffer);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_pod0])));
					  strcat(json, string_buffer);
					  dtostrf(pod0Temp, 4, 2, float_buffer);
					  strcat(json, float_buffer);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_pod1])));
					  strcat(json, string_buffer);
					  dtostrf(pod1Temp, 4, 2, float_buffer);
					  strcat(json, float_buffer);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_co2])));
					  strcat(json, string_buffer);
					  dtostrf(CO2PPM, 4, 2, float_buffer);
					  strcat(json, float_buffer);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_water0])));
					  strcat(json, string_buffer);
					  itoa(waterLevel0, float_buffer, 10);
					  strcat(json, float_buffer);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_water1])));
					  strcat(json, string_buffer);
					  itoa(waterLevel1, float_buffer, 10);
					  strcat(json, float_buffer);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_photo])));
					  strcat(json, string_buffer);
					  itoa(photo, float_buffer, 10);
					  strcat(json, float_buffer);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_channels])));
					  strcat(json, string_buffer);
					    for(int i=0; i<channel_size; i++) {
						  itoa(valid_channels[i], sPin, 10);
						  itoa(digitalRead(valid_channels[i]), state, 10);
						  strcat(json, "\"c");
						  strcat(json, sPin);
						  strcat(json, "\":");
						  strcat(json, state);
						  if(i + 1 < channel_size) {
							  strcat(json, ",");
						  }
					    }
					  strcat(json, json_bracket_close);

					strcat(json, json_bracket_close);
				}

				// /switch/?     1 = on, else off
				else if (strncmp(resource, "switch", 7) == 0) {

					int pin = atoi(param1);
					int position = atoi(param2);

					bool valid = false;
					for(int i=0; i<channel_size; i++) {
						if(pin == valid_channels[i]) {
							valid = true;
							break;
						}
					}
					if(valid) {

						strcpy(json, json_bracket_open);

						  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_pin])));
					      strcat(json, string_buffer);
					      strcat(json, param1);

					      strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_position])));
						  strcat(json, string_buffer);
						  strcat(json, param2);

						strcat(json, json_bracket_close);

						#if DEBUG
							Serial.print("/switch: ");
							Serial.println(json);
						#endif

						position == 1 ? switchOn(pin) : switchOff(pin);
					}
					else {
						strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_error_invalid_channel])));
					    strcat(json, string_buffer);
					}
				}

				// /eeprom
				else if (strncmp(resource, "eeprom", 6) == 0) {
					#if EEPROM_DEBUG
						Serial.println("/eeprom");
						Serial.println(param1);
						Serial.println(param2);
					#endif

					EEPROM.write(atoi(param1), atoi(param2));

					strcpy(json, json_bracket_open);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_address])));
					  strcat(json, string_buffer);
					  strcat(json, param1);

					  strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_key_value])));
					  strcat(json, string_buffer);
					  strcat(json, param2);

					strcat(json, json_bracket_close);
				}

				// /reboot
				else if (strncmp(resource, "reboot", 6) == 0) {
					#if DEBUG
						Serial.println("/reboot");
					#endif
					strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_reboot_true])));
					strcat(json, string_buffer);
					reboot = true;
				}

				// /reset
				else if (strncmp(resource, "reset", 5) == 0) {
					#if DEBUG
						Serial.println("/reset");
					#endif

					resetDefaults();

					strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_json_reset_true])));
					strcat(json, string_buffer);
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

				strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_http_content_type_json])));
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

	if(reboot)  {
	  resetFunc();
	}
}

void send404() {

	strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_http_404])));
	httpClient.println(string_buffer);

	#if DEBUG
	  Serial.println(string_buffer);
	#endif

	strcpy_P(string_buffer, (char*)pgm_read_word(&(string_table[idx_http_xpowered_by])));
	httpClient.println(string_buffer);

	httpClient.println();
}

void resetDefaults() {
	EEPROM.write(0, defaultMac[0]);
	EEPROM.write(1, defaultMac[1]);
	EEPROM.write(2, defaultMac[2]);
	EEPROM.write(3, defaultMac[3]);
	EEPROM.write(4, defaultMac[4]);
	EEPROM.write(5, defaultMac[5]);
	EEPROM.write(6, 192);
	EEPROM.write(7, 168);
	EEPROM.write(8, 0);
	EEPROM.write(9, 91);
}

int availableMemory() {
  int free_memory;
  return ((int) __brkval == 0) ? ((int) &free_memory) - ((int) &__bss_end) :
      ((int) &free_memory) - ((int) __brkval);
}
