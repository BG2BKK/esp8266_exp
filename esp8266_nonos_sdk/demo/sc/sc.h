
#ifndef __SC_H__
#define __SC_H__

#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "smartconfig.h"
#include "airkiss.h"
#include "espconn.h"

#include "config.h"

#define DEVICE_TYPE 		"gh_9e2cff3dfa51" //wechat public number
#define DEVICE_ID 			"122475" //model ID

#define DEFAULT_LAN_PORT 	12476
void ICACHE_FLASH_ATTR airkiss_wifilan_time_callback(void);
void ICACHE_FLASH_ATTR airkiss_wifilan_recv_callbk(void *arg, char *pdata, unsigned short len);
void ICACHE_FLASH_ATTR airkiss_start_discover(void);
void ICACHE_FLASH_ATTR smartconfig_onLink(void *pdata);
void ICACHE_FLASH_ATTR smartconfig_done(sc_status status, void *pdata);
void ICACHE_FLASH_ATTR user_start_smartconfig(os_timer_t *, os_timer_func_t *);

#endif
