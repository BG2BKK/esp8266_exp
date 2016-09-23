#include "user_sensor.h"

void user_sensor(void *pvParameters)
{
	printf("start user sensor\n");
	while(1) {
		printf("This is user sensor task: %u\n", system_get_time());
		vTaskDelay(5000 / portTICK_RATE_MS);
	}
	vTaskDelete(NULL);
}


