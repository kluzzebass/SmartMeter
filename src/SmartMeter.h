
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






#define LED D2
// #define LED 4 

#define BTN D6
// #define BTN 12

#define LDR D7
// #define LDR 13

// Microseconds
#define DEBOUNCE_TIME 1000

// Used when the WiFiManager enters configuration mode.
#define AP_NAME "SmartMeter AP"


//////////////////

/*

#include <SmartMeter.h>

void setup()
{
	smartmeter.setup();
}

void loop()
{
	smartmeter.loop();
}

*/
///////////////




class SmartMeter
{
private:
	unsigned long counter = 0;

    WiFiManager wifiManager;

	Debouncer ldr;
	Blinker blinker;

	// MQTT config
	char mqtt_server[40];
	char mqtt_port[6] = "1883";

	void readConfig();
	void setupWifi();


public:

	void setup();
	void loop();

	bool shouldSaveConfig = false;


};


extern SmartMeter smartmeter;


#endif // _SmartMeter_h_
