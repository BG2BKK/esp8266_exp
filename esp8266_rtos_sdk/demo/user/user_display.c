#include "user_display.h"

bool bootup() 
{
	i2c_init();
	bool flag = oled_init(1);
	if(!flag) {
		printf("oled display boot failed\n");
		return flag;
	}
	oled_select_font(1, 0);
	printf("display begin %dx%d\n", oled_get_width(1), oled_get_height(1));
	return true;
}

void welcome()
{
	uint8 pos = 0;
	char *greeting = "bg2bkk: hello world";

	oled_clear(1);
   	for(pos=0; pos < 20; pos++)
		oled_draw_hline(1, 0, pos, 90, 1);
	oled_draw_string(1, 0, 30, greeting, 1);
	oled_refresh(1);
}

void user_display(void *pvParameters)
{

	uint8 len = 20;
	char time[len];

	bootup();
	welcome();

	while(1) {
		vTaskDelay(1000 / portTICK_RATE_MS);
		uint32 timestamp = system_get_time() / 1000000;
		sprintf(time, "time:%12ds", timestamp);
		time[len-1] = '\0';
		oled_clear(1);
		oled_draw_string(1, 0, 40, time, 1);
		oled_refresh(1);
	}
	vTaskDelete(NULL);
}
