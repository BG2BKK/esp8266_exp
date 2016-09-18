#ifndef __MQTT_CONFIG_H__
#define __MQTT_CONFIG_H__

#define CFG_HOLDER	0x00FF55A4	/* Change this value to load default configurations */
#define CFG_LOCATION	0x79	/* Please don't change or if you know what you doing */
#define MQTT_SSL_ENABLE

/*DEFAULT CONFIGURATIONS*/

#define MQTT_HOST			"192.168.2.163" //or "mqtt.yourdomain.com"
//#define MQTT_HOST			"192.168.1.101" //or "mqtt.yourdomain.com"
//#define MQTT_HOST			"45.78.38.250" //or "mqtt.yourdomain.com"
//#define MQTT_HOST			"10.237.36.18" //or "mqtt.yourdomain.com"
#define MQTT_PORT			1883
#define MQTT_BUF_SIZE		1024
#define MQTT_KEEPALIVE		120	 /*second*/

#define MQTT_CLIENT_ID		"DVES_%08X"
#define MQTT_USER			"DVES_USER"
#define MQTT_PASS			"DVES_PASS"

// #define STA_SSID "Sina Plaza Mobile"
// #define STA_PASS "urtheone"

//#define STA_SSID "huang"
//#define STA_PASS "sh19901222"

#define STA_SSID "TP-LINK_A0E338"
#define STA_PASS "huanghuang"

#define STA_TYPE AUTH_WPA2_PSK

#define MQTT_RECONNECT_TIMEOUT 	20	/*second*/

#define DEFAULT_SECURITY	0
#define QUEUE_BUFFER_SIZE	256

#define PROTOCOL_NAMEv31	/*MQTT version 3.1 compatible with Mosquitto v0.15*/
//PROTOCOL_NAMEv311			/*MQTT version 3.11 compatible with https://eclipse.org/paho/clients/testing/*/

#endif // __MQTT_CONFIG_H__
