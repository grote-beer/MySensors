/*
* The MySensors Arduino library handles the wireless radio link and protocol
* between your home built sensors/actuators and HA controller of choice.
* The sensors forms a self healing radio network with optional repeaters. Each
* repeater and gateway builds a routing tables in EEPROM which keeps track of the
* network topology allowing messages to be routed to nodes.
*
* Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
* Copyright (C) 2013-2020 Sensnology AB
* Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
*
* Documentation: http://www.mysensors.org
* Support Forum: http://forum.mysensors.org
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*/


// Topic structure: MY_MQTT_PUBLISH_TOPIC_PREFIX/NODE-ID/SENSOR-ID/CMD-TYPE/ACK-FLAG/SUB-TYPE

#include "MyGatewayTransport.h"



#if defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_ESP32)
#if !defined(MY_WIFI_SSID)
#error ESP8266/ESP32 MQTT gateway: MY_WIFI_SSID not defined!
#endif
#endif



static PubSubClient _MQTT_client(_MQTT_ethClient);
static bool _MQTT_connecting = true;
static bool _MQTT_available = false;
static MyMessage _MQTT_msg;

// cppcheck-suppress constParameter
bool gatewayTransportSend(MyMessage &message)
{
	if (!_MQTT_client.connected()) {
		return false;
	}
	setIndication(INDICATION_GW_TX);
	char *topic = protocolMyMessage2MQTT(MY_MQTT_PUBLISH_TOPIC_PREFIX, message);
	GATEWAY_DEBUG(PSTR("GWT:TPS:TOPIC=%s,MSG SENT\n"), topic);
#if defined(MY_MQTT_CLIENT_PUBLISH_RETAIN)
	const bool retain = message.getCommand() == C_SET ||
	                    (message.getCommand() == C_INTERNAL && message.getType() == I_BATTERY_LEVEL);
#else
	const bool retain = false;
#endif /* End of MY_MQTT_CLIENT_PUBLISH_RETAIN */
	return _MQTT_client.publish(topic, message.getString(_convBuffer), retain);
}

void incomingMQTT(char *topic, uint8_t *payload, unsigned int length)
{
	GATEWAY_DEBUG(PSTR("GWT:IMQ:TOPIC=%s, MSG RECEIVED\n"), topic);
	_MQTT_available = protocolMQTT2MyMessage(_MQTT_msg, topic, payload, length);
	setIndication(INDICATION_GW_RX);
}

bool reconnectMQTT(void)
{
	// Attempt to connect
	if (_MQTT_client.connect(MY_MQTT_CLIENT_ID, MY_MQTT_USER, MY_MQTT_PASSWORD)) {
		GATEWAY_DEBUG(PSTR("GWT:RMQ:OK\n"));
		// Send presentation of locally attached sensors (and node if applicable)
		presentNode();
		// Once connected, publish subscribe
		char inTopic[strlen(MY_MQTT_SUBSCRIBE_TOPIC_PREFIX) + strlen("/+/+/+/+/+")];
		(void)strncpy(inTopic, MY_MQTT_SUBSCRIBE_TOPIC_PREFIX, strlen(MY_MQTT_SUBSCRIBE_TOPIC_PREFIX) + 1);
		(void)strcat(inTopic, "/+/+/+/+/+");
		_MQTT_client.subscribe(inTopic);

		return true;
	}
	delay(1000);
	GATEWAY_DEBUG(PSTR("!GWT:RMQ:FAIL\n"));
	return false;
}



bool gatewayTransportInit(void)
{
	_MQTT_client.setCallback(incomingMQTT);


	return true;
}

bool gatewayTransportAvailable(void)
{
	if (_MQTT_connecting) {
		return false;
	}
#if defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_ESP32)
	if (WiFi.status() != WL_CONNECTED) {
#if defined(MY_GATEWAY_ESP32)
		(void)gatewayTransportInit();
#endif
		return false;
	}
#endif
	if (!_MQTT_client.connected()) {
		//reinitialise client
		if (gatewayTransportConnect()) {
			reconnectMQTT();
		}
		return false;
	}
	_MQTT_client.loop();
	return _MQTT_available;
}

MyMessage & gatewayTransportReceive(void)
{
	// Return the last parsed message
	_MQTT_available = false;
	return _MQTT_msg;
}
