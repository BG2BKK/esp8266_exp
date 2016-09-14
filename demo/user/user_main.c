#include "ets_sys.h"
#include "osapi.h"

#include "user_interface.h"

#include "user_webserver.h"

#include "config.h"
#include "espconn.h"
#include "mem.h"

#include "smartconfig.h"
#include "airkiss.h"

#define DEVICE_TYPE 		"gh_9e2cff3dfa51" //wechat public number
#define DEVICE_ID 			"122475" //model ID

#define DEFAULT_LAN_PORT 	12476

LOCAL esp_udp ssdp_udp;
LOCAL struct espconn pssdpudpconn;
LOCAL os_timer_t ssdp_time_serv;

uint8_t  lan_buf[200];
uint16_t lan_buf_len;
uint8 	 udp_sent_cnt = 0;


//#define NET_DOMAIN "cn.bing.com"
#define NET_DOMAIN "localhost"
#define pheadbuffer "GET /test HTTP/1.1\r\nUser-Agent: curl/7.37.0\r\nHost: %s\r\nAccept: */*\r\n\r\n"

#define packet_size   (2 * 1024)

LOCAL os_timer_t test_timer;
LOCAL os_timer_t check_ip_timer;
LOCAL struct espconn user_tcp_conn;
LOCAL struct _esp_tcp user_tcp;
ip_addr_t tcp_server_ip;
ip_addr_t esp_server_ip;

	SYSCFG sysCfg;
	SAVE_FLAG saveFlag;


static volatile os_timer_t user_timer;
static volatile os_timer_t info_timer;
static void user_timerfunc(os_event_t *events);

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
		} 
		else 
		{
			//re-arm timer to check ip
			os_timer_setfn(&test_timer, (os_timer_func_t *)user_check_ip, NULL);
			os_timer_arm(&test_timer, 300, 0);
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
	os_printf("%d %d %d\n", user_tcp_conn.type, user_tcp_conn.state, ESPCONN_CLOSE );
//	system_print_meminfo();
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

	void ICACHE_FLASH_ATTR
user_timer_config(void )
{
	os_timer_disarm(&user_timer);
	os_timer_setfn(&user_timer, (os_timer_func_t *)user_timerfunc, NULL);
	os_timer_arm(&user_timer, 5000, 0);

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

const airkiss_config_t akconf =
{
	(airkiss_memset_fn)&memset,
	(airkiss_memcpy_fn)&memcpy,
	(airkiss_memcmp_fn)&memcmp,
	0,
};

LOCAL void ICACHE_FLASH_ATTR
airkiss_wifilan_time_callback(void)
{
	uint16 i;
	airkiss_lan_ret_t ret;
	
	if ((udp_sent_cnt++) >30) {
		udp_sent_cnt = 0;
		os_timer_disarm(&ssdp_time_serv);//s
		//return;
	}

	ssdp_udp.remote_port = DEFAULT_LAN_PORT;
	ssdp_udp.remote_ip[0] = 255;
	ssdp_udp.remote_ip[1] = 255;
	ssdp_udp.remote_ip[2] = 255;
	ssdp_udp.remote_ip[3] = 255;
	lan_buf_len = sizeof(lan_buf);
	ret = airkiss_lan_pack(AIRKISS_LAN_SSDP_NOTIFY_CMD,
		DEVICE_TYPE, DEVICE_ID, 0, 0, lan_buf, &lan_buf_len, &akconf);
	if (ret != AIRKISS_LAN_PAKE_READY) {
		os_printf("Pack lan packet error!");
		return;
	}
	
	ret = espconn_sendto(&pssdpudpconn, lan_buf, lan_buf_len);
	if (ret != 0) {
		os_printf("UDP send error!");
	}
	os_printf("Finish send notify!\n");
}

LOCAL void ICACHE_FLASH_ATTR
airkiss_wifilan_recv_callbk(void *arg, char *pdata, unsigned short len)
{
	uint16 i;
	remot_info* pcon_info = NULL;
		
	airkiss_lan_ret_t ret = airkiss_lan_recv(pdata, len, &akconf);
	airkiss_lan_ret_t packret;
	
	switch (ret){
	case AIRKISS_LAN_SSDP_REQ:
		espconn_get_connection_info(&pssdpudpconn, &pcon_info, 0);
		os_printf("remote ip: %d.%d.%d.%d \r\n",pcon_info->remote_ip[0],pcon_info->remote_ip[1],
			                                    pcon_info->remote_ip[2],pcon_info->remote_ip[3]);
		os_printf("remote port: %d \r\n",pcon_info->remote_port);
      
        pssdpudpconn.proto.udp->remote_port = pcon_info->remote_port;
		os_memcpy(pssdpudpconn.proto.udp->remote_ip,pcon_info->remote_ip,4);
		ssdp_udp.remote_port = DEFAULT_LAN_PORT;
		
		lan_buf_len = sizeof(lan_buf);
		packret = airkiss_lan_pack(AIRKISS_LAN_SSDP_RESP_CMD,
			DEVICE_TYPE, DEVICE_ID, 0, 0, lan_buf, &lan_buf_len, &akconf);
		
		if (packret != AIRKISS_LAN_PAKE_READY) {
			os_printf("Pack lan packet error!");
			return;
		}

		os_printf("\r\n\r\n");
		for (i=0; i<lan_buf_len; i++)
			os_printf("%c",lan_buf[i]);
		os_printf("\r\n\r\n");
		
		packret = espconn_sendto(&pssdpudpconn, lan_buf, lan_buf_len);
		if (packret != 0) {
			os_printf("LAN UDP Send err!");
		}
		
		break;
	default:
		os_printf("Pack is not ssdq req!%d\r\n",ret);
		break;
	}
}

void ICACHE_FLASH_ATTR
airkiss_start_discover(void)
{
	ssdp_udp.local_port = DEFAULT_LAN_PORT;
	pssdpudpconn.type = ESPCONN_UDP;
	pssdpudpconn.proto.udp = &(ssdp_udp);
	espconn_regist_recvcb(&pssdpudpconn, airkiss_wifilan_recv_callbk);
	espconn_create(&pssdpudpconn);

	os_timer_disarm(&ssdp_time_serv);
	os_timer_setfn(&ssdp_time_serv, (os_timer_func_t *)airkiss_wifilan_time_callback, NULL);
	os_timer_arm(&ssdp_time_serv, 1000, 1);//1s
}

void ICACHE_FLASH_ATTR
smartconfig_onLink(void *pdata)
{
	struct station_config *sta_conf = pdata;
	os_printf("SSID: %s\n", sta_conf->ssid);
	os_printf("PWD: %s\n", sta_conf->password);

	os_strncpy(sysCfg.sta_ssid, sta_conf->ssid, sizeof(sysCfg.sta_ssid) - 1);
	os_strncpy(sysCfg.sta_pwd, sta_conf->password, sizeof(sysCfg.sta_pwd) - 1);
	sysCfg.sta_type = STA_TYPE;

    wifi_station_set_config(sta_conf);
    wifi_station_disconnect();
    wifi_station_connect();

	os_timer_disarm(&check_ip_timer);
	os_timer_setfn(&check_ip_timer, (os_timer_func_t *)user_check_ip, NULL);
	os_timer_arm(&check_ip_timer, 300, 0);
}

void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata)
{
    switch(status) {
        case SC_STATUS_WAIT:
            os_printf("SC_STATUS_WAIT\n");
            break;
        case SC_STATUS_FIND_CHANNEL:
            os_printf("SC_STATUS_FIND_CHANNEL\n");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            os_printf("SC_STATUS_GETTING_SSID_PSWD\n");
			sc_type *type = pdata;
            if (*type == SC_TYPE_ESPTOUCH) {
                os_printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
            } else {
                os_printf("SC_TYPE:SC_TYPE_AIRKISS\n");
            }
            break;
        case SC_STATUS_LINK:
//            os_printf("SC_STATUS_LINK\n");
			os_printf("Got SSID and Passwd\n");
			smartconfig_onLink(pdata);
            break;
        case SC_STATUS_LINK_OVER:
            os_printf("SC_STATUS_LINK_OVER\n");
            if (pdata != NULL) {
				//SC_TYPE_ESPTOUCH
                uint8 phone_ip[4] = {0};

                os_memcpy(phone_ip, (uint8*)pdata, 4);
                os_printf("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
//            } else {
//            	//SC_TYPE_AIRKISS - support airkiss v2.0
//				airkiss_start_discover();
//	先不做airkiss
            }
            smartconfig_stop();
            break;
    }
	
}

void ICACHE_FLASH_ATTR
user_start_smartconfig(void) 
{
	smartconfig_set_type(SC_TYPE_ESPTOUCH); //SC_TYPE_ESPTOUCH,SC_TYPE_AIRKISS,SC_TYPE_ESPTOUCH_AIRKISS
	smartconfig_start(smartconfig_done);
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

	wifi_set_opmode(STATION_MODE);

	s8 flag = CFG_Load(&sysCfg);
	if(flag != 0) {
		user_start_smartconfig();

	} else {
		os_printf("SSID: %s\n", sysCfg.sta_ssid);
		os_printf("PWD: %s\n", sysCfg.sta_pwd);
		user_set_station_config(&sysCfg);
	}
	//	user_timer_config();
	//	dns_test();
}

