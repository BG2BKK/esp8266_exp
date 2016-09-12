#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"

#include "user_devicefind.h"
#include "user_webserver.h"

#if ESP_PLATFORM
#include "user_esp_platform.h"
#endif

#include "espconn.h"
#include "mem.h"


//#define NET_DOMAIN "cn.bing.com"
#define NET_DOMAIN "localhost"
#define pheadbuffer "GET /test HTTP/1.0\r\nUser-Agent: curl/7.37.0\r\nHost: %s\r\nAccept: */*\r\n\r\n"

#define packet_size   (2 * 1024)

LOCAL os_timer_t test_timer;
LOCAL struct espconn user_tcp_conn;
LOCAL struct _esp_tcp user_tcp;
ip_addr_t tcp_server_ip;
ip_addr_t esp_server_ip;

static volatile os_timer_t user_timer;
static volatile os_timer_t info_timer;

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
	debug_connection_info(arg, "recv_cb");
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

	os_printf("connect succeed !!! \r\n");

	espconn_regist_recvcb(pespconn, user_tcp_recv_cb);
	espconn_regist_sentcb(pespconn, user_tcp_sent_cb);
	espconn_regist_disconcb(pespconn, user_tcp_discon_cb);

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
	//error occured , tcp connection broke. user can try to reconnect here. 

	os_printf("reconnect callback, error code %d !!! \r\n",err);

	debug_connection_info(arg, "reconn_cb");
}

/******************************************************************************
 * FunctionName : user_check_ip
 * Description  : check whether get ip addr or not
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
	void ICACHE_FLASH_ATTR
user_check_ip(void)
{
	struct ip_info ipconfig;

	//disarm timer first
	os_timer_disarm(&test_timer);

	//get ip info of ESP8266 station
	wifi_get_ip_info(STATION_IF, &ipconfig);

	os_printf("station status: %d\n", wifi_station_get_connect_status() );
	if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) 
	{
		os_printf("got ip !!!"IPSTR" \r\n", IP2STR(&ipconfig.ip));
	} 
	else 
	{
		if ((wifi_station_get_connect_status() == STATION_WRONG_PASSWORD ||
					wifi_station_get_connect_status() == STATION_NO_AP_FOUND ||
					wifi_station_get_connect_status() == STATION_CONNECT_FAIL)) 
		{
			os_printf("connect fail !!! \r\n");
		} 
		else 
		{
			//re-arm timer to check ip
			os_timer_setfn(&test_timer, (os_timer_func_t *)user_check_ip, NULL);
			os_timer_arm(&test_timer, 100, 0);
		}
	}
}


/******************************************************************************
 * FunctionName : user_set_station_config
 * Description  : set the router info which ESP8266 station will connect to 
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
	void ICACHE_FLASH_ATTR
user_set_station_config(void)
{

	char ssid[32] = "Sina Plaza Office"; 
	char password[64] = "urtheone"; 

	struct station_config stationConf; 

	os_memset(stationConf.ssid, 0, 32);
	os_memset(stationConf.password, 0, 64);

	//need not mac address
	stationConf.bssid_set = 0; 

	//Set ap settings 
	os_memcpy(&stationConf.ssid, ssid, 32); 
	os_memcpy(&stationConf.password, password, 64); 
	wifi_station_set_config(&stationConf); 

	//set a timer to check whether got ip from router succeed or not.
	os_timer_disarm(&test_timer);
	os_timer_setfn(&test_timer, (os_timer_func_t *)user_check_ip, NULL);
	os_timer_arm(&test_timer, 100, 0);

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
}
static void user_timerfunc(os_event_t *events) 
{
	struct ip_info ipconfig;
	wifi_get_ip_info(STATION_IF, &ipconfig);

	if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0) 
	{
		// Connect to tcp server as NET_DOMAIN
		user_tcp_conn.proto.tcp = &user_tcp;
		user_tcp_conn.type = ESPCONN_TCP;
		user_tcp_conn.state = ESPCONN_NONE;

		//		const char esp_tcp_server_ip[4] = {10,237,36,18}; // remote IP of TCP server
		const char esp_tcp_server_ip[4] = {45,78,38,250}; // remote IP of TCP server

		os_memcpy(user_tcp_conn.proto.tcp->remote_ip, esp_tcp_server_ip, 4);

		user_tcp_conn.proto.tcp->remote_port = 8080;  // remote port

//		user_tcp_conn.proto.tcp->local_port = espconn_port(); //local port of ESP8266
		user_tcp_conn.proto.tcp->local_port = 9999; //local port of ESP8266

		espconn_regist_connectcb(&user_tcp_conn, user_tcp_connect_cb); // register connect callback
		espconn_regist_reconcb(&user_tcp_conn, user_tcp_recon_cb); // register reconnect callback as error handler
		espconn_set_opt(&user_tcp_conn, ESPCONN_KEEPALIVE);
		espconn_set_opt(&user_tcp_conn, ESPCONN_NODELAY);
		espconn_connect(&user_tcp_conn); 

	} 
}

	void ICACHE_FLASH_ATTR
user_timer_config(void )
{
	os_timer_disarm(&user_timer);
	os_timer_setfn(&user_timer, (os_timer_func_t *)user_timerfunc, NULL);
	os_timer_arm(&user_timer, 20000, 1);

	os_timer_disarm(&info_timer);
	os_timer_setfn(&info_timer, (os_timer_func_t *)info_timerfunc, NULL);
	os_timer_arm(&info_timer, 5000, 1);

	os_printf("\n\n==================================\n\n");
}

	LOCAL void ICACHE_FLASH_ATTR
user_esp_platform_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
	struct espconn *pespconn = (struct espconn *)arg;
	if (ipaddr != NULL)
		os_printf("user_esp_platform_dns_found %d.%d.%d.%d\n",
				*((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1),
				*((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));
}
void dns_test(void) {
	struct espconn pespconn ;
	espconn_gethostbyname(&pespconn,"iot.espressif.cn", &esp_server_ip,
			user_esp_platform_dns_found);
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void)
{
	uart_init(115200, 115200);
	os_printf("SDK version:%s\n", system_get_sdk_version());
	os_printf("CPU Frequency: %d MHz\n", system_get_cpu_freq());
//	system_update_cpu_freq(160);
//	os_printf("CPU Frequency: %d MHz\n", system_get_cpu_freq());

	//Set softAP + station mode 
	wifi_set_opmode(STATIONAP_MODE); 

//	wifi_station_set_reconnect_policy(true);

	//ESP8266 connect to router
	user_set_station_config();

	user_timer_config();
	dns_test();
}


