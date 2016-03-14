#ifndef SPI_APP_H
#define SPI_APP_H

#include "spi_register.h"
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "c_types.h"
#include "user_interface.h"
#include "gpio.h"

#define MISO 12
#define MOSI 13
#define CLK	 14
#define SS  15 
#define SPI_DELAY 2

void spi_init();
uint8_t platform_spi_send_recv(int spi_periph, uint8_t val, bool close);

#endif 
