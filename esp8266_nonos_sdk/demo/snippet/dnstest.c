
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


