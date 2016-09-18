#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"
#include "mem.h"
#include "espconn.h"
#include "uart.h"

#include "config.h"
#include "sc.h"
#include "_mqtt.h"

//#define NET_DOMAIN "cn.bing.com"
#define NET_DOMAIN "localhost"
#define pheadbuffer "GET /test HTTP/1.1\r\nUser-Agent: curl/7.37.0\r\nHost: %s\r\nAccept: */*\r\n\r\n"

#define packet_size   (2 * 1024)

LOCAL os_timer_t check_ip_timer;
LOCAL struct espconn user_tcp_conn;
LOCAL struct _esp_tcp user_tcp;
ip_addr_t tcp_server_ip;
ip_addr_t esp_server_ip;

MQTT_Client mqttClient;
SYSCFG sysCfg;
SAVE_FLAG saveFlag;

static volatile os_timer_t user_timer;
static void user_timerfunc(os_event_t *events);

static volatile os_timer_t info_timer;
static void info_timerfunc(os_event_t *events);

static volatile os_timer_t mqtt_timer;
static void mqtt_timerfunc(os_event_t *events);

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
 *******************************************************************************/
	uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
	enum flash_size_map size_map = system_get_flash_size_map();
	uint32 rf_cal_sec = 0;

	switch (size_map) {
		case FLASH_SIZE_4M_MAP_256_256:
			rf_cal_sec = 128 - 5;
			break;

		case FLASH_SIZE_8M_MAP_512_512:
			rf_cal_sec = 256 - 5;
			break;

		case FLASH_SIZE_16M_MAP_512_512:
		case FLASH_SIZE_16M_MAP_1024_1024:
			rf_cal_sec = 512 - 5;
			break;

		case FLASH_SIZE_32M_MAP_512_512:
		case FLASH_SIZE_32M_MAP_1024_1024:
			rf_cal_sec = 1024 - 5;
			break;

		default:
			rf_cal_sec = 0;
			break;
	}

	return rf_cal_sec;
}

	void ICACHE_FLASH_ATTR
user_rf_pre_init(void)
{
}

void ICACHE_FLASH_ATTR
debug_connection_info(void *arg, char *conn)
{
	struct espconn *pesp_conn = arg;
	remot_info *pcon_info = NULL; 
	sint8 flag = espconn_get_connection_info(pesp_conn, &pcon_info, 0);
	if (flag == ESPCONN_OK) {
		os_printf(IPSTR":%d\n", IP2STR(pcon_info->remote_ip), pcon_info->remote_port);
	} else {
		os_printf("%s not connected: %d\n", conn, flag);
	}
}

/******************************************************************************
 * FunctionName : user_tcp_recv_cb
 * Description  : receive callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
	LOCAL void ICACHE_FLASH_ATTR
user_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
	//received some data from tcp connection

	os_printf("tcp recv !!! %s \r\n", pusrdata);
	debug_connection_info((struct espconn*)arg, "recv_cb");
//	espconn_disconnect((struct espconn*)arg);
}
/******************************************************************************
 * FunctionName : user_tcp_sent_cb
 * Description  : data sent callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
	LOCAL void ICACHE_FLASH_ATTR
user_tcp_sent_cb(void *arg)
{
	//data sent successfully

	os_printf("tcp sent succeed !!! \r\n");
}
/******************************************************************************
 * FunctionName : user_tcp_discon_cb
 * Description  : disconnect callback.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
	LOCAL void ICACHE_FLASH_ATTR
user_tcp_discon_cb(void *arg)
{
	//tcp disconnect successfully

	os_printf("tcp disconnect succeed !!! \r\n");
}
/******************************************************************************
 * FunctionName : user_esp_platform_sent
 * Description  : Processing the application data and sending it to the host
 * Parameters   : pespconn -- the espconn used to connetion with the host
 * Returns      : none
 *******************************************************************************/
	LOCAL void ICACHE_FLASH_ATTR
user_sent_data(struct espconn *pespconn)
{
	char *pbuf = (char *)os_zalloc(packet_size);

	os_sprintf(pbuf, pheadbuffer, NET_DOMAIN);

	espconn_send(pespconn, pbuf, os_strlen(pbuf));

	os_free(pbuf);

}

/******************************************************************************
 * FunctionName : user_tcp_connect_cb
 * Description  : A new incoming tcp connection has been connected.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
	LOCAL void ICACHE_FLASH_ATTR
user_tcp_connect_cb(void *arg)
{
	struct espconn *pespconn = arg;
	uint32 keepidle = 30;
	uint32 keepintvl = 10;
	uint32 keepcnt = 3;

	os_printf("connect succeed !!! \r\n");

	espconn_regist_recvcb(pespconn, user_tcp_recv_cb);
	espconn_regist_sentcb(pespconn, user_tcp_sent_cb);
	espconn_regist_disconcb(pespconn, user_tcp_discon_cb);

	espconn_set_opt(pespconn, ESPCONN_KEEPALIVE);		// before espconn_set_keepalive
	os_printf("%d\n", espconn_set_keepalive(pespconn, ESPCONN_KEEPIDLE, &keepidle));
	os_printf("%d\n", espconn_set_keepalive(pespconn, ESPCONN_KEEPINTVL, &keepintvl));
	os_printf("%d\n", espconn_set_keepalive(pespconn, ESPCONN_KEEPCNT, &keepcnt));

	os_printf(IPSTR":%d -> "IPSTR":%d\n", IP2STR(pespconn->proto.tcp->local_ip), pespconn->proto.tcp->local_port,
		IP2STR(pespconn->proto.tcp->remote_ip), pespconn->proto.tcp->remote_port);

	debug_connection_info(arg, "connect_cb");
	user_sent_data(pespconn);
}

/******************************************************************************
 * FunctionName : user_tcp_recon_cb
 * Description  : reconnect callback, error occured in TCP connection.
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
	LOCAL void ICACHE_FLASH_ATTR
user_tcp_recon_cb(void *arg, sint8 err)
{
	os_printf("reconnect callback, error code %d !!! \r\n",err);
	os_timer_disarm(&user_timer);
	os_timer_setfn(&user_timer, (os_timer_func_t *)user_timerfunc, NULL);
	os_timer_arm(&user_timer, 1000, 0);
}

/******************************************************************************
 * FunctionName : user_check_ip
 * Description  : check whether get ip addr or not
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR user_check_ip(void *t)
{
	struct ip_info ipconfig;
	u32 recon_interval = 0;

	//disarm timer first
	os_timer_disarm(&check_ip_timer);

	//get ip info of ESP8266 station
	wifi_get_ip_info(STATION_IF, &ipconfig);

//	os_printf("station status: %d\n", wifi_station_get_connect_status() );
	if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) 
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

/******************************************************************************
 * FunctionName : user_set_station_config
 * Description  : set the router info which ESP8266 station will connect to 
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
	void ICACHE_FLASH_ATTR
user_set_station_config(SYSCFG *sysCfg)
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
	os_timer_arm(&check_ip_timer, 100, 0);
}

static void info_timerfunc(os_event_t *events) 
{
	struct ip_info ipconfig;
	uint8 macaddr[6];
	uint32 timestamp = system_get_time();
	sint8 rssi = wifi_station_get_rssi();

	wifi_get_macaddr(STATION_IF, macaddr);
	wifi_get_ip_info(STATION_IF, &ipconfig);
	os_printf_plus("Mac: "MACSTR" local IP: "IPSTR" rssi: %d timestamp: %d\n", MAC2STR(macaddr), IP2STR(&(ipconfig.ip)), rssi, timestamp);
	os_printf("%d %d %d\n", user_tcp_conn.type, user_tcp_conn.state, ESPCONN_CLOSE );
	os_printf("connect_status: %d\n", wifi_station_get_connect_status());
}

static void user_timerfunc(os_event_t *events) 
{
	struct ip_info ipconfig;
	wifi_get_ip_info(STATION_IF, &ipconfig);

	if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) 
	{
//	os_printf("%d\n", user_tcp_conn.proto.tcp->remote_port);

//		if (user_tcp_conn.proto.tcp->remote_port == 0) {

			// Connect to tcp server as NET_DOMAIN
			user_tcp_conn.proto.tcp = &user_tcp;
			user_tcp_conn.type = ESPCONN_TCP;
			user_tcp_conn.state = ESPCONN_NONE;

//			const char esp_tcp_server_ip[4] = {192,168,2,207}; // remote IP of TCP server
			const char esp_tcp_server_ip[4] = {10,237,35,104}; // remote IP of TCP server

			os_memcpy(user_tcp_conn.proto.tcp->remote_ip, esp_tcp_server_ip, 4);

			user_tcp_conn.proto.tcp->remote_port = 8020;  // remote port

			user_tcp_conn.proto.tcp->local_port = espconn_port(); //local port of ESP8266

			espconn_regist_connectcb(&user_tcp_conn, user_tcp_connect_cb); // register connect callback
			espconn_regist_reconcb(&user_tcp_conn, user_tcp_recon_cb); // register reconnect callback as error handler
//		}
		espconn_connect(&user_tcp_conn); 
	} else {
		os_printf("status: %d "IPSTR"\n", wifi_station_get_connect_status(), IP2STR(&ipconfig.ip));
	}
}

static void mqtt_timerfunc(os_event_t *events)
{
	s8 status = 0;
	os_timer_disarm(&mqtt_timer);
	status = wifi_station_get_connect_status();
	if(status == STATION_GOT_IP) {
		MQTT_Connect(&mqttClient);
		os_printf("Mqtt service start\n");
	} else {
		MQTT_Disconnect(&mqttClient);
		os_timer_setfn(&mqtt_timer, (os_timer_func_t *)mqtt_timerfunc, NULL);
		os_timer_arm(&mqtt_timer, 5000, 0);
	}
}

void ICACHE_FLASH_ATTR user_timer_config(void )
{
	os_timer_disarm(&user_timer);
	os_timer_setfn(&user_timer, (os_timer_func_t *)user_timerfunc, NULL);
	os_timer_arm(&user_timer, 5000, 0);

	os_timer_disarm(&info_timer);
	os_timer_setfn(&info_timer, (os_timer_func_t *)info_timerfunc, NULL);
	os_timer_arm(&info_timer, 5000, 1);

	os_timer_disarm(&mqtt_timer);
	os_timer_setfn(&mqtt_timer, (os_timer_func_t *)mqtt_timerfunc, NULL);
	os_timer_arm(&mqtt_timer, 5000, 0);

	os_printf("\n\n==================================\n\n");
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
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
	CFG_config(&sysCfg);
	user_mqtt_init();
	user_timer_config();
}
