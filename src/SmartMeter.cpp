
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

#include <SmartMeter.h>


SmartMeter smartmeter;

// Callback notifying us of the need to save config.
void saveConfigCallback ()
{
  Serial.println("Flagging config for save.");
  smartmeter.shouldSaveConfig = true;
}

void SmartMeter::setup()
{
	Serial.begin(115200);
	Serial.println("Starting up.");

	pinMode(BTN, INPUT_PULLUP);
	pinMode(LDR, INPUT);

	blinker.setPin(LED);
	ldr.delay = DEBOUNCE_TIME;

	readConfig();
	setupWifi();
}

void SmartMeter::loop()
{
	blinker.check();
	ldr.update(digitalRead(LDR));

	if (ldr.changed && ldr.state)
	{
		counter++;
		Serial.println(counter);
		blinker.setOnForTime(10);
	}

	// Button is pressed, let's restart.
	if (!digitalRead(BTN))
	{
		Serial.println("Button is pressed, restarting.");
		ESP.restart();
	}

}


void SmartMeter::setupWifi()
{
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);


    wifiManager.setSaveConfigCallback(saveConfigCallback);

    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);

    wifiManager.setConnectTimeout(60);

	analogWrite(LED, 50);
	if (!digitalRead(BTN))
	{
		Serial.println("Entering configuration mode by force.");
		wifiManager.startConfigPortal(AP_NAME);
	}
	else
	{
		if (!wifiManager.autoConnect(AP_NAME))
		{
			Serial.println("Failed to connect and hit timeout.");
			delay(3000);
			ESP.restart();
			delay(5000);
		}
	}
	analogWrite(LED, 0);

    //if you get here you have connected to the WiFi
    Serial.println("Successfully connected.");

    //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());

    //save the custom parameters to FS
    if (shouldSaveConfig)
    {
		Serial.println("Saving config.");
		DynamicJsonBuffer jsonBuffer;
		JsonObject &json = jsonBuffer.createObject();
		json["mqtt_server"] = mqtt_server;
		json["mqtt_port"] = mqtt_port;

		File configFile = SPIFFS.open("/config.json", "w");
		if (!configFile)
		{
			Serial.println("Failed to open config file for writing.");
		}
		else
		{
			json.printTo(Serial);
			Serial.println();
			json.printTo(configFile);
			configFile.close();
			//end save
		}

    }
	Serial.print("Local IP: ");
	Serial.println(WiFi.localIP());
}



void SmartMeter::readConfig()
{
    //read configuration from FS json
    Serial.print("Mounting file system...");

    if (!SPIFFS.begin())
    {
		Serial.println(" failed!");
		return;
	}

	Serial.println(" success.");
	if (!SPIFFS.exists("/config.json"))
	{
		Serial.println("No config file found.");
		return;
	}

	//file exists, reading and loading
	Serial.print("Reading config file...");
	File configFile = SPIFFS.open("/config.json", "r");

	if (!configFile)
	{
		Serial.println(" failed!");
		return;
	}

	Serial.println(" success.");
	size_t size = configFile.size();
	// Allocate a buffer to store contents of the file.
	std::unique_ptr<char[]> buf(new char[size]);

	configFile.readBytes(buf.get(), size);
	DynamicJsonBuffer jsonBuffer;
	JsonObject &json = jsonBuffer.parseObject(buf.get());

	if (!json.success())
	{
		Serial.println("Failed to load json config!");
		return;
	}

	Serial.println("\nParsed json.");
	json.printTo(Serial);

	strcpy(mqtt_server, json["mqtt_server"]);
	strcpy(mqtt_port, json["mqtt_port"]);

}