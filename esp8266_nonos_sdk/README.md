
ESP8266 NONOS SDK experiment
--------------------------------

* scan_ap
* join_ap
* tcp_client
* uart_tcp
* esp_mqtt_proj

* [echo server](github.com/tobyzxj/goecho)
	* 注意测试keepalive功能时，先将go server的keepalive设置大一些
* [flasher]
	* sudo ../tools/esptool.py --port /dev/ttyUSB1 --baud 460800 write_flash -fm dio -fs 32m -ff 40m 0x00000 ../bin/eagle.flash.bin 0x10000 ../bin/eagle.irom0text.bin 0x3fc000 ../bin/esp_init_data_default.bin 0x3fe000 ../bin/blank.bin

* [fota]
	* sudo ../tools/esptool.py --port /dev/ttyUSB1 --baud 460800 write_flash -fm dio -fs 32m -ff 40m 0x00000  ../../esp8266_nonos_sdk/bin/boot_v1.6.bin 0x01000 ../bin/upgrade/user1.4096.new.6.bin 0xfe000 ~/Downloads/19d773a0d0af444671e878c215a2ae2efcde976e.bin 0x3fc000 ../bin/esp_init_data_default.bin 0x3fe000 ../bin/blank.bin  0x3fb000 ../bin/blank.bin 


* 产品设计方案
	* 第一次启动时，开启AP+Station模式，Station模式肯定是连不上网的，因为出厂时没有预设AP+Passwd; AP模式可以开启tcp server，用于手机配置它联网；Station能连上网后，将模式设置为STATION模式，并提示用户，用户设备即将从8266发出的AP断开，原因是该AP消失；Station模式连上网后，将配置写入指定SPI Flash地址中，下次启动将从这个地址读取数据；再次启动时，读取指定地址的配置，如果没有，则等待从AP获取。
	* 中途连接时，所有状态通过oled显示
	* 第二种方案是，初始化启动时，首先从指定SPI Flash读取之前的配置，并连接路由器，如果能连接成功，则开始正常工作；如果未能读取SPI Flash，或者连接路由器不成功，则开启ESP touch或者ESP Airkiss进行smartconfig；配置成功并连接路由器成功后，将新的配置写入SPI FLash中，提示用户连接成功，即将重启；第二次启动时，从指定地址读取之前配置，然后连接路由器

* [demo]
	* 实现以上产品方案

关心的技术点
----------------

* mqtt
	* nonos sdk ---- esp_mqtt_proj || demo
	* rtos sdk ---- esp_rtos_paho

* cjson
	* nonos sdk	---- demo
	* rtos	sdk ---- 04protocol/json_demo

* 在线升级 X

* ssl

* i2c_master
	* i2c oled

* esp now
	* 设备不需要联网，可以通过手机直接通信、控制

* tcp server
* websocket
* 深度睡眠


