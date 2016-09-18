#include "ets_sys.h"
#include "osapi.h"
#include "mqtt.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"

extern SYSCFG sysCfg;

void wifiConnectCb(uint8_t status);
void mqttConnectedCb(uint32_t *args);
void mqttDisconnectedCb(uint32_t *args);
void mqttPublishedCb(uint32_t *args);
void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len);
void ICACHE_FLASH_ATTR user_mqtt_init(void);
