#include "u8g_esp8266.h"

void u8g_Delay(uint16_t msec)
{
	os_delay_us( msec * 1000 );
}
void u8g_MicroDelay(void)
{
	os_delay_us( 1 );
}
void u8g_10MicroDelay(void)
{
	os_delay_us( 10 );
}

uint8_t u8g_com_hw_i2c_fn(u8g_t *u8g, uint8_t msg, uint8_t arg_val, void *arg_ptr)
{

	switch(msg)
	{
		case U8G_COM_MSG_STOP:
			//STOP THE DEVICE
			break;

		case U8G_COM_MSG_INIT:
			//INIT HARDWARE INTERFACES, TIMERS, GPIOS...
			break;

		case U8G_COM_MSG_ADDRESS:  
			//SWITCH FROM DATA TO COMMAND MODE (arg_val == 0 for command mode)
			break;

		case U8G_COM_MSG_CHIP_SELECT:
			if ( arg_val == 0 )
			{
				/* disable chip, send stop condition */
				i2c_stop();
			}
			else
			{
				/* enable, do nothing: any byte writing will trigger the i2c start */
			}
			break;


		case U8G_COM_MSG_RESET:
			//TOGGLE THE RESET PIN ON THE DISPLAY BY THE VALUE IN arg_val
			break;

		case U8G_COM_MSG_WRITE_BYTE:
			_data(((struct _u8g_t *)u8g)->i2c_addr, arg_val);
			//WRITE BYTE TO DEVICE
			break;

		case U8G_COM_MSG_WRITE_SEQ:
		case U8G_COM_MSG_WRITE_SEQ_P:
			//WRITE A SEQUENCE OF BYTES TO THE DEVICE
			break;

	}
	return 1;
}

