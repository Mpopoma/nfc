#ifndef _WS2812_H
#define _WS2812_H

#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "gpio.h"
#include "osapi.h"

#define LEDS 1

#define WSGPIO 5
#define IO_MUX PERIPHS_IO_MUX_GPIO5_U
#define IO_FUNC FUNC_GPIO5

char outbuffer[LEDS*3];

os_timer_t led_timer;

void WS2812OutBuffer( uint8_t * buffer, uint16_t length );
void led_reset(void *arg);
void set_led(int r, int g, int b, long delay);
#endif

