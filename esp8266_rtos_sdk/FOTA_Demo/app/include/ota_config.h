#ifndef __OTA_CONFIG_H__
#define __OTA_CONFIG_H__

#define DEMO_SERVER "192.168.1.101"
#define DEMO_SERVER_PORT 8080

#define DEMO_WIFI_SSID     "TP-LINK_A0E338"
#define DEMO_WIFI_PASSWORD  "huanghuang"

#define OTA_TIMEOUT 120000  //120000 ms

// #define pheadbuffer "Connection: keep-alive\r\n\
// Cache-Control: no-cache\r\n\
// User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
// Accept: */*\r\n\
// Accept-Encoding: gzip,deflate,sdch\r\n\
// Accept-Language: zh-CN,zh;q=0.8\r\n\r\n"

#define pheadbuffer "Accept-Encoding: gzip,deflate,sdch\r\n\r\n"

#endif

