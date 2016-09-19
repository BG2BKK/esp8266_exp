#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"
#include "mem.h"
#include "espconn.h"
#include "uart.h"

#include "user_main.h"
#include "config.h"
#include "sc.h"
#include "cJSON.h"
#include "_mqtt.h"

LOCAL os_timer_t check_ip_timer;
MQTT_Client mqttClient;
SYSCFG sysCfg;

static volatile os_timer_t user_timer;
static void user_timerfunc(os_event_t *events);

static volatile os_timer_t info_timer;
static void info_timerfunc(os_event_t *events);

static volatile os_timer_t mqtt_timer;
static void mqtt_timerfunc(os_event_t *events);

static volatile os_timer_t mqtt_pub_timer;
static void mqtt_pub_timerfunc(os_event_t *events);

volatile u8 MQTT_CONNECTED = 0;

void ICACHE_FLASH_ATTR user_timer_config(void);

void ICACHE_FLASH_ATTR user_check_ip(void *t)
{
	struct ip_info ipconfig;
	u32 recon_interval = 0;
	u8 status;

	//disarm timer first
	os_timer_disarm(&check_ip_timer);

	//get ip info of ESP8266 station
	wifi_get_ip_info(STATION_IF, &ipconfig);

	status = wifi_station_get_connect_status();

//	os_printf("station status: %d\n", wifi_station_get_connect_status() );
	if (status == STATION_GOT_IP && ipconfig.ip.addr != 0) 
	{
		os_printf("got ip !!!"IPSTR" \r\n", IP2STR(&ipconfig.ip));
		if(sysCfg.cfg_holder != CFG_HOLDER) {
			sysCfg.cfg_holder = CFG_HOLDER;
			CFG_Save(&sysCfg);
		}
	}
	else 
	{
		if ((wifi_station_get_connect_status() == STATION_WRONG_PASSWORD ||
					wifi_station_get_connect_status() == STATION_NO_AP_FOUND ||
					wifi_station_get_connect_status() == STATION_CONNECT_FAIL)) 
		{
			os_printf("connect fail !!! \r\n");
			recon_interval = 3000;
		} 
		else 
		{
			recon_interval = 300;
		}
		os_timer_setfn(&check_ip_timer, (os_timer_func_t *)user_check_ip, NULL);
		os_timer_arm(&check_ip_timer, 300, 0);
	}
}

void ICACHE_FLASH_ATTR user_set_station_config(SYSCFG *sysCfg)
{
	struct station_config stationConf; 
	os_memset(stationConf.ssid, 0, 32);
	os_memset(stationConf.password, 0, 64);

	//need not mac address
	stationConf.bssid_set = 0; 

	//Set ap settings 
	os_memcpy(&stationConf.ssid, sysCfg->sta_ssid, 32); 
	os_memcpy(&stationConf.password, sysCfg->sta_pwd, 64); 
	wifi_station_set_config(&stationConf); 

	//set a timer to check whether got ip from router succeed or not.
	os_timer_disarm(&check_ip_timer);
	os_timer_setfn(&check_ip_timer, (os_timer_func_t *)user_check_ip, NULL);
	os_timer_arm(&check_ip_timer, 1000, 0);
}

static void info_timerfunc(os_event_t *events) 
{
	/*
	 * use to send heartbeat
	 */
	struct ip_info ipconfig;
	uint32 timestamp = system_get_time();
	uint8 status = wifi_station_get_connect_status();
	wifi_get_ip_info(STATION_IF, &ipconfig);

	os_printf_plus("SSID: %s, PWD: %s, IP: "IPSTR", connect_status: %d, timestamp: %u\n", sysCfg.sta_ssid, sysCfg.sta_pwd, IP2STR(&(ipconfig.ip)), status, timestamp);

}

// 与网络有关的，比如tcp发送心跳、mqtt连接等，用wifi连接和断开事件来触发
// 与用户任务有关的，比如显示oled数据、读取传感器信息，通过定时任务实现

static void mqtt_timerfunc(os_event_t *events)
{
	os_timer_disarm(&mqtt_timer);

//	if(mqttClient.connState == TCP_CONNECTED) {
//		os_printf("mqtt_client state: %d %d\n", mqttClient.mqtt_state, TCP_CONNECTED);
//	} else {

		u8 status = wifi_station_get_connect_status();
		if(status == STATION_GOT_IP) {
			os_printf("Mqtt service start\n");
			MQTT_Connect(&mqttClient);
		} else {
			MQTT_Disconnect(&mqttClient);
			os_timer_setfn(&mqtt_timer, (os_timer_func_t *)mqtt_timerfunc, NULL);
			os_timer_arm(&mqtt_timer, 5000, 0);
		}
//	}
}

static void mqtt_pub_timerfunc(os_event_t *events)
{
	if(MQTT_CONNECTED == 1) {
		struct ip_info ipconfig;
		uint8 ip[16];
		wifi_get_ip_info(STATION_IF, &ipconfig);
		os_sprintf(ip, IPSTR, IP2STR(&(ipconfig.ip)));

		uint8 macaddr[18], mac[18];
		wifi_get_macaddr(STATION_IF, macaddr);
		os_sprintf(mac, MACSTR, MAC2STR(macaddr));
	
		uint32 timestamp = system_get_time();
		uint32 heapsize  = system_get_free_heap_size();

		cJSON *fmt=cJSON_CreateObject();
		cJSON_AddNumberToObject(fmt,"timestamp", timestamp);
		cJSON_AddNumberToObject(fmt,"heapsize", heapsize);
		cJSON_AddStringToObject(fmt,"ip", ip);
		cJSON_AddStringToObject(fmt,"mac", mac);
		char *msg = cJSON_Print(fmt);
		cJSON_Delete(fmt);
		
		MQTT_Publish(&mqttClient, "/mqtt/pub/topic/0", msg, os_strlen(msg), 0, 0);
		cJSON_free(msg);
	}
}

void ICACHE_FLASH_ATTR user_timer_config(void )
{
	os_timer_disarm(&info_timer);
	os_timer_setfn(&info_timer, (os_timer_func_t *)info_timerfunc, NULL);
	os_timer_arm(&info_timer, 30000, 1);

	os_timer_disarm(&mqtt_timer);
	os_timer_setfn(&mqtt_timer, (os_timer_func_t *)mqtt_timerfunc, NULL);
	os_timer_arm(&mqtt_timer, 5000, 0);

	os_timer_disarm(&mqtt_pub_timer);
	os_timer_setfn(&mqtt_pub_timer, (os_timer_func_t *)mqtt_pub_timerfunc, NULL);
	os_timer_arm(&mqtt_pub_timer, 5000, 1);


	os_printf("\n\n==================================\n\n");
}

void user_init(void)
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_printf("SDK version:%s\n", system_get_sdk_version());
	os_printf("CPU Frequency: %d MHz\n", system_get_cpu_freq());

	wifi_set_opmode(STATION_MODE);

	s8 flag = CFG_Load(&sysCfg);
	if(flag != 0) {
		user_start_smartconfig(&check_ip_timer, user_check_ip);
	} else {
		os_printf("SSID: %s  PWD: %s\n", sysCfg.sta_ssid, sysCfg.sta_pwd);
		user_set_station_config(&sysCfg);
	}
	user_mqtt_init();
	user_timer_config();
}
