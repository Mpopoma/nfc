#include "ets_sys.h"
#include "espmissingincludes.h"

#define PERIPHS_IO_MUX_PULLDWN          BIT6
#define PIN_PULLDWN_DIS(PIN_NAME)             CLEAR_PERI_REG_MASK(PIN_NAME, PERIPHS_IO_MUX_PULLDWN)
#include "spi.h"
#include "espmissingincludes.h"


void ICACHE_FLASH_ATTR spi_init()
{
	//miso to input
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTDI_U);
	GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 0x1 << MISO);

	//mosi to input
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTCK_U);
	GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 0x1 << MOSI);
	
	//clock to output
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTMS_U);
	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 0x1 << CLK);

	// SS pin to output
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTDO_U);
	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 0x1 << SS);
	GPIO_OUTPUT_SET(SS, 1);
}

uint8_t ICACHE_FLASH_ATTR platform_spi_send_recv(int spi_periph, uint8_t val, bool close)
{
	uint8_t i;
	uint8_t polarity = 0; 
	uint8_t phase = 0;
	uint8_t buffer = 0;
	uint8_t mask = 0x80;

	// slave select low
	GPIO_OUTPUT_SET(SS, 0);

	// clock
	GPIO_OUTPUT_SET(CLK, polarity);
	
	//mosi to output
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
	PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTCK_U);
	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 0x1 << MOSI);

	if (phase == 0)
	{
		if(val & mask)
		{
			GPIO_OUTPUT_SET(MOSI, 1);
		}
		else
		{
			GPIO_OUTPUT_SET(MOSI, 0);
		}

		val <<= 1;
	}

	os_delay_us(2);

	for (i = 0; i < 8; ++i) 
	{
		GPIO_OUTPUT_SET(CLK, !polarity);

		if(phase == 1)
		{
			if(val & mask)
			{
				GPIO_OUTPUT_SET(MOSI, 1);
			}
			else
			{
				GPIO_OUTPUT_SET(MOSI, 0);
			}

			val <<= 1;
		}
		else
		{
				buffer |= GPIO_INPUT_GET(MISO);
				if (i != 7)
				{
					buffer <<= 1;
				}
		}
		os_delay_us(2);

		GPIO_OUTPUT_SET(CLK, polarity);

		if(phase == 0)
		{
			if(val & mask)
			{
				GPIO_OUTPUT_SET(MOSI, 1);
			}
			else
			{
				GPIO_OUTPUT_SET(MOSI, 0);
			}
			val <<= 1;
		}
		else
		{
				buffer |= GPIO_INPUT_GET(MISO);
				if (i != 7)
				{
					buffer <<= 1;
				}
		}
		os_delay_us(2);
	}

	if(close)
	{
		//mosi to input
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
		PIN_PULLUP_DIS(PERIPHS_IO_MUX_MTCK_U);
		GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 0x1 << MOSI);
	
		GPIO_OUTPUT_SET(SS, 1);
	}

	return buffer;
}
