//#define NET_DOMAIN "cn.bing.com"
#define NET_DOMAIN "localhost"
#define pheadbuffer "GET /test HTTP/1.1\r\nUser-Agent: curl/7.37.0\r\nHost: %s\r\nAccept: */*\r\n\r\n"

#define packet_size   (2 * 1024)



LOCAL struct espconn user_tcp_conn;
LOCAL struct _esp_tcp user_tcp;
ip_addr_t tcp_server_ip;
ip_addr_t esp_server_ip;

	
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


