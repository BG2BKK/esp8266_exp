##SDK Version : ESP8266_RTOS_SDK_V1.4.2_16_07_04
##Platform : ESP-LAUNCHER BOARD

##Operation Steps:

1. Enter path:/home/esp8266/Share, clone ESP8266 RTOS SDK to lubuntu environment by command: 
       
		git clone https://github.com/espressif/ESP8266_RTOS_SDK.git 
	   
2. Enter SDK folder:/home/esp8266/Share/ESP8266_RTOS_SDK, Copy example folder "app" next to bin/ folder in the SDK folder; copy folder "openssl" to include/; copy "libmbedtls.a" and "libopenssl.a" to lib/. The SDK folder should have folders inside it like : bin, examples, third party... 

	If you want to test the openSSL client demo: you should use the defination "#define DEMO_CLIENT" int the file "ESP8266_RTOS_SDK\openssl_demo\programs\openssl_demo.c". 

	If you want to test the openSSL client demo: You should not use the defination "#define DEMO_CLIENT" int the file "ESP8266_RTOS_SDK\openssl_demo\programs\openssl_demo.c".
	   
3. Enter example folder, run ./gen_misc.sh, and follow below steps to finish the sample code compile:
	
		Option confirm SDK PATH > Enter y
		Option 1 will be automatically selected,
		Option 2 > Enter 0. 
		Option 3 > Enter Default(Just Press enter)
		Option 4 > Enter Default(Just Press enter)
		Option 5 > Enter 0.
	   
4. "eagle.flash.bin, eagle.irom0text.bin" should be found in "/home/esp8266/Share/ESP8266_RTOS_SDK/bin/", Flash the Binaries with ESP Flashing tool at the instructed Locations. Download bin files to ESP-LAUNCHER as below sittings.
		
		Download address of each bin files:
		blank.bin				           		 0x7E000
		esp_init_data_default.bin			  	 0x7C000
		eagle.flash.bin				   			 0x00000
		eagle.irom0text.bin			          	 0x20000
		
		Flash download tool settings:
		CrystalFreq: 26M
		SPI SPEED: 40MHz
		SPID MODE: QIO
		FLASH SIZE: 4M
			
##FOR VERIFY: 
On ESP8266 Dev Board, connect a serial terminal at 74880 Baud Rate. 

IF you test the openSSL client demo:you can see it will download the "www.baidu.com" main page and print the context

IF you want to test the openSSL client demo: 

1. You should input the context of "https://192.168.17.128", the IP of your module may not be 192.168.17.128, you should input your module's IP
	
2. You may see that it shows the website is not able to be trusted, that you should select that "go on visit it again"
	
3. You should wait for a moment until your see the "hello word" in your IE page.
	 
	
