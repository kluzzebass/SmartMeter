
/*
 *
 * Copyright (c) 2017 Jan Fredrik Leversund
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef _SmartMeter_h_
#define _SmartMeter_h_

#include <Arduino.h>
#include <Debouncer.h>
#include <Blinker.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include <ArduinoJson.h>  
#include <PubSubClient.h>
#include <lwip/inet.h>


#define LED D2
#define BTN D6
#define LDR D7

// Microseconds
#define DEBOUNCE_TIME 1000

// Used when the WiFiManager enters configuration mode and as the client name with MQTT.
#define AP_NAME "SmartMeter"

// Name of the file SPIFFS file containing the configuration.
#define CONFIG_FILE "/config.json"

#define RECONNECT_INTERVAL 5000
#define PUBLISH_INTERVAL 10000
#define MQTT_TOPIC_MAX_LEN 64
#define MQTT_PAYLOAD_MAX_LEN 64
#define MQTT_TOPIC_Wh "smartmeter/%s/Wh"

// Various variable max lengths
#define CFG_MQTT_SERVER_LEN 40
#define CFG_MQTT_PORT_LEN 6
#define CFG_METER_ID_LEN 10


class SmartMeter
{
private:
	unsigned long counter = 0;
	unsigned long lastPublish = 0;
	unsigned long lastReconnect = 0;

    WiFiManager wifiManager;

	Debouncer ldr;
	Blinker blinker;

	// MQTT config
	char mqttServer[CFG_MQTT_SERVER_LEN] = "192.168.2.2";
	char mqttPort[CFG_MQTT_PORT_LEN] = "1883";
	char meterId[CFG_METER_ID_LEN] = "104564";

	uint32_t mqttIp;

	WiFiClient wifiClient;
	PubSubClient psClient;

	char topic[MQTT_TOPIC_MAX_LEN];


	void readConfig();
	void setupWifi();
	void publish();

public:

	void setup();
	void loop();

	bool shouldSaveConfig = false;


};


extern SmartMeter smartmeter;


#endif // _SmartMeter_h_
