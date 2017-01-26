
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
  Serial.println(F("Flagging config for save."));
  smartmeter.shouldSaveConfig = true;
}

void SmartMeter::setup()
{
	Serial.begin(115200);
	Serial.println(F("Starting up."));

	pinMode(BTN, INPUT_PULLUP);
	pinMode(LDR, INPUT);

	blinker.setPin(LED);
	ldr.delay = DEBOUNCE_TIME;

	readConfig();
	setupWifi();



	IPAddress addr(inet_addr(mqttServer));
	uint16_t port = atoi(mqttPort);


	Serial.print("MQTT address: ");
	addr.printTo(Serial);
	Serial.print(":");
	Serial.println(port);

	psClient.setClient(wifiClient);
	psClient.setServer(addr, port);

	snprintf(topic, MQTT_TOPIC_MAX_LEN, MQTT_TOPIC_Wh, meterId);
	Serial.print(F("Publishing to topic "));
	Serial.println(topic);
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
		Serial.println(F("Button is pressed, restarting."));
		ESP.restart();
	}

	psClient.loop();
	publish();

}


void SmartMeter::setupWifi()
{
    WiFiManagerParameter customMQTTServer("server", "mqtt server", mqttServer, CFG_MQTT_SERVER_LEN);
    WiFiManagerParameter customMQTTPort("port", "mqtt port", mqttPort, CFG_MQTT_PORT_LEN);
    WiFiManagerParameter customMeterId("id", "meter id", meterId, CFG_METER_ID_LEN);

    wifiManager.setSaveConfigCallback(saveConfigCallback);

    wifiManager.addParameter(&customMQTTServer);
    wifiManager.addParameter(&customMQTTPort);
    wifiManager.addParameter(&customMeterId);

    wifiManager.setConnectTimeout(60);

	// Indicate that we're in setup/connect mode.
	analogWrite(LED, 50);
	if (!digitalRead(BTN))
	{
		Serial.println(F("Entering configuration mode by force."));
		wifiManager.startConfigPortal(AP_NAME);
	}
	else
	{
		if (!wifiManager.autoConnect(AP_NAME))
		{
			Serial.println(F("Failed to connect and hit timeout."));
			delay(3000);
			ESP.restart();
			delay(5000);
		}
	}
	analogWrite(LED, 0);

    // If you get here you have connected to the WiFi
    Serial.println(F("Successfully connected."));

    // Read updated parameters
    strcpy(mqttServer, customMQTTServer.getValue());
    strcpy(mqttPort, customMQTTPort.getValue());
    strcpy(meterId, customMeterId.getValue());

    // Save the custom parameters to FS
    if (shouldSaveConfig)
    {
		Serial.println(F("Saving config."));
		DynamicJsonBuffer jsonBuffer;
		JsonObject &json = jsonBuffer.createObject();
		json["mqtt_server"] = mqttServer;
		json["mqtt_port"] = mqttPort;
		json["meter_id"] = meterId;

		File configFile = SPIFFS.open(CONFIG_FILE, "w");
		if (!configFile)
		{
			Serial.println(F("Failed to open config file for writing."));
		}
		else
		{
			json.printTo(Serial);
			Serial.println();
			json.printTo(configFile);
			configFile.close();
		}

    }
	Serial.print(F("Local IP: "));
	Serial.println(WiFi.localIP());
}



void SmartMeter::readConfig()
{
    Serial.print(F("Mounting file system..."));

    if (!SPIFFS.begin())
    {
		Serial.println(F(" failed!"));
		return;
	}

	Serial.println(F(" success."));
	if (!SPIFFS.exists(CONFIG_FILE))
	{
		Serial.println(F("No config file found."));
		return;
	}

	Serial.print(F("Reading config file..."));
	File configFile = SPIFFS.open(CONFIG_FILE, "r");

	if (!configFile)
	{
		Serial.println(F(" failed."));
		return;
	}

	Serial.println(F(" success!"));

	size_t size = configFile.size();
	std::unique_ptr<char[]> buf(new char[size]);

	configFile.readBytes(buf.get(), size);
	DynamicJsonBuffer jsonBuffer;
	JsonObject &json = jsonBuffer.parseObject(buf.get());

	if (!json.success())
	{
		Serial.println(F("Failed to load json config!"));
		return;
	}

	Serial.println(F("\nParsed json."));
	json.printTo(Serial);

	strcpy(mqttServer, json["mqtt_server"]);
	strcpy(mqttPort, json["mqtt_port"]);
	strcpy(meterId, json["meter_id"]);
}


void SmartMeter::publish()
{
	unsigned long now = millis();

	// Check if the client's still connected.
	if (!psClient.connected())
	{
		if (now - lastReconnect > RECONNECT_INTERVAL)
		{
			Serial.print(F("Attempting to connect to MQTT server..."));
			lastReconnect = now;
			if (psClient.connect(AP_NAME))
			{
				Serial.println(F(" success!"));
			}
			else
			{
				Serial.println(F(" failed."));
				return;
			}
		}
		else
			return;
	}

	// If we got this far, we're connected.
	if (now - lastPublish > PUBLISH_INTERVAL)
	{
		// All right, let's publish something!
		lastPublish = now;
		char payload[MQTT_PAYLOAD_MAX_LEN];
		snprintf(payload, MQTT_PAYLOAD_MAX_LEN, "%lu", counter);
		Serial.print(F("Publishing: "));
		Serial.println(payload);		
		psClient.publish(topic, payload, true);
	}

}




