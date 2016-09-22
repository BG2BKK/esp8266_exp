##SDK Version : ESP8266_RTOS_SDK_V1.4.2_16_07_04
##Platform : ESP-LAUNCHER BAOARD

##Operation Steps:

1. Enter path:/home/esp8266/Share, clone ESP8266 RTOS SDK to lubuntu environment by command: 
       
		git clone https://github.com/espressif/esp8266-rtos-sample-code.git
	   
2. Enter SDK folder:/home/esp8266/Share/ESP8266_RTOS_SDK, Copy example folder "ssl_server_demo" next to bin/ folder in the SDK folder. The SDK folder should have folders inside it like : bin, examples, third party...with the lib - openssl.a and mbedtls.a,and inpress your own WIFI SSID and key to the defination "SSID" and "PASSWORD", int the file "SDK\ssl_server_demon\include\user_config.h"

3. If the SDK path is not updated in gen_misc.sh, right click the script and edit the path to bin folder and SDK folder. for the current SDK, the gen_mish.sh would have a path like:
       
		export SDK_PATH="/home/esp8266/Share/ESP8266_RTOS_SDK"
		export BIN_PATH="/home/esp8266/Share/ESP8266_RTOS_SDK/bin"
	   
4. Enter example folder, run ./gen_misc.sh, and follow below steps to fininsh the sample code compile:
	
		Option 1 > Enter Y/y.
		Option 2 > Enter 1. 
		Option 3 > Enter Default(Just Press enter)
		Option 4 > Enter Default(Just Press enter)
		Option 5 > Enter 5.
	   
5. "user1.2048.new.5.bin" should be found in "/home/esp8266/Share/ESP8266_RTOS_SDK/bin/upgrade", Flash the Binaries with ESP Flashing tool at the instructed Locations. Download bin files to ESP-LAUNCHER as below sittings.
		
		Download address of each bin files:
		blank.bin				            0x1FE000
		esp_init_data_default.bin			    0x1FC000
		boot_v1.5.bin					    0x00000
		user1.2048.new.5.bin			            0x01000
		
		Flash download tool settings:
		CrystalFreq: 26M
		SPI SPEED: 40MHz
		SPID MODE: QIO
		FLASH SIZE: 16Mbit-C11
			
##FOR VERIFY: 
On ESP8266 Dev Board, connect a serial terminal at 74880 Baud Rate, start your IE browser of microsoft. 
You should input the context of "https://192.168.17.128", the IP of your module may not be 192.168.17.128, you should input your module's IP
You may see that it shows the website is not able to be trusted, that you should select that "go on to visit it"
You should wait for a moment until your see the "hello word" in your IE page

