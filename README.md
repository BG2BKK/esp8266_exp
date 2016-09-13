
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

