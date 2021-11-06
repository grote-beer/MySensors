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

#include "MyConfig.h"
#include "MyProtocol.h"
#include "cjv_iot.h"
#include "MyGatewayTransport.h"
#include "MyMessage.h"
#include "MyProtocol.h"

// global variables
extern MyMessage _msgTmp;
extern cjv_iot cjv;

char _serialInputString[MY_GATEWAY_MAX_RECEIVE_LENGTH];    // A buffer for incoming commands from serial interface
uint8_t _serialInputPos;
MyMessage _CJV_msg;
static bool _CJV_available = false;

// Topic structure: MY_MQTT_PUBLISH_TOPIC_PREFIX/NODE-ID/SENSOR-ID/CMD-TYPE/ACK-FLAG/SUB-TYPE
// cppcheck-suppress constParameter
bool gatewayTransportSend(MyMessage &message)
{
		
	setIndication(INDICATION_GW_TX);
	char *topic = protocolMyMessage2MQTT(MY_MQTT_PUBLISH_TOPIC_PREFIX, message);
	GATEWAY_DEBUG(PSTR("GWT:TPS:TOPIC=%s,MSG SENT\n"), topic);

	cjv.publish(topic, message.getString(_convBuffer));
	return true;
}

bool gatewayTransportInit(void)
{
	(void)gatewayTransportSend(buildGw(_msgTmp, I_GATEWAY_READY).set(MSG_GW_STARTUP_COMPLETE));
	// Send presentation of locally attached sensors (and node if applicable)
	presentNode();
	return true;
}

void my_sensors_mqtt_callback(char *_topic, byte *_payload, unsigned int length){
	GATEWAY_DEBUG(PSTR("GWT:IMQ:TOPIC=%s, MSG RECEIVED\n"), _topic);
	_CJV_available = protocolMQTT2MyMessage(_CJV_msg, _topic, _payload, length);
	setIndication(INDICATION_GW_RX);
	
	#ifdef CJV_DEBUG
    cjv.error("Mysensors callback");
#endif
}

bool gatewayTransportAvailable(void)
{
	if(!cjv.connected()) return false;

	// check if connected
	// check if message is in buffer
	return _CJV_available;
}

MyMessage & gatewayTransportReceive(void)
{
	// Return the last parsed message
	// set buffer flag to false
	return _CJV_msg;
}
