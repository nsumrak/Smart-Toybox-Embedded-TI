// Smart Toybox Dual-color LED interface
//
// Copyright(C) 2016. Nebojsa Sumrak and Jelena (Petra) Markovic
//
//   This program is free software; you can redistribute it and / or modify
//	 it under the terms of the GNU General Public License as published by
//	 the Free Software Foundation; either version 2 of the License, or
//	 (at your option) any later version.
//
//	 This program is distributed in the hope that it will be useful,
//	 but WITHOUT ANY WARRANTY; without even the implied warranty of
//	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	 GNU General Public License for more details.
//
//	 You should have received a copy of the GNU General Public License along
//	 with this program; if not, write to the Free Software Foundation, Inc.,
//	 51 Franklin Street, Fifth Floor, Boston, MA 02110 - 1301 USA.

#pragma once

#include "platform.h"
#include "board.h"
#include "mstime.h"
#include "debug.h"

#define BLINK_DURATION 200

enum {
	LED_RED = 1,
	LED_GREEN = 2,
	LED_YELLOW = LED_RED | LED_GREEN,
};

inline void led_on(unsigned char led, BOOL on)
{
//	UART_PRINT("led on: LED = 0x%02x on = %d\n\r", led, on);
	// TODO turn off other led before? implement for yellow?
	if(led&LED_RED) gpio_write(GPIO_LED_RED, on);
	if(led&LED_GREEN) gpio_write(GPIO_LED_GREEN, on);
}

inline void leds_off()
{
	timer_stop(5);
	led_on(LED_YELLOW, 0);
}

inline unsigned char *get_blink_led()
{
	static unsigned char blink = LED_YELLOW;
	return &blink;
}

static unsigned char loopcnt = 0;
static unsigned char currcnt = 0;
inline void led_blink_loop(unsigned char id)
{
	currcnt++; // overflow is ok since it will go back to 0 and can only happen when loopcnt is 255
	if (loopcnt != 255 && currcnt >= loopcnt) {
		leds_off();
		return;
	}
//	UART_PRINT("blinking led 0x%02x, on = %d\n\r", *(get_blink_led()), val);
	unsigned char led =*(get_blink_led());
	if (led == 255) {
		led_on(currcnt & 1? LED_RED : LED_GREEN, 1);
		led_on(currcnt & 1? LED_GREEN : LED_RED, 0);
	} else
		led_on(led, currcnt & 1);
}

// blink led for count number of times / 2
// led = 255 : red & green leds will blink alternately
inline void led_blink_cnt(unsigned char led, unsigned char cnt)
{
	ASSERT(led <= LED_YELLOW || led == 255);
	*(get_blink_led()) = led;
	leds_off();
	// TODO use constant for timer 5
	loopcnt = cnt != 255 ? cnt * 2 : 255;
	currcnt = 0;
	timer_set_cb(5, BLINK_DURATION, loopcnt, led_blink_loop);
}

// blink led for given time
inline void led_blink_time(unsigned char led, unsigned long duration)
{
	led_blink_cnt(led, duration / BLINK_DURATION / 2);
}

// turn led on for a given time
inline void led_on_time(unsigned char led, unsigned long duration)
{
	ASSERT(led <= LED_YELLOW);
	leds_off();
	led_on(led, 1);
	timer_set_cb(5, duration, 1, (timer_callback)leds_off);
}

inline void led_init()
{
	gpio_mode(GPIO_LED_GREEN, GPIO_MODE_OUTPUT);
	gpio_mode(GPIO_LED_RED, GPIO_MODE_OUTPUT);
}
