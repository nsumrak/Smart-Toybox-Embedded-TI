// Smart Toybox button services
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

#include <stdio.h>
#include "platform.h"
#include "board.h"
#include "mstime.h"
#include "debug.h"
#include "evqueue.h"
#include "button.h"

volatile unsigned long button_pressed_at = 0;
static volatile BOOL button_state = 0;
static unsigned char button_repeat_sec = 0;


static void BtnIntReq()
{
	button_state = gpio_read(GPIO_BUTTON);
	gpio_clear_irq(GPIO_BUTTON);
}

void button_process(unsigned long now)
{
	if(button_pressed_at) {
		if(button_state) { // repeat support
			now -= button_pressed_at;
			now /= BUTTON_REPEAT_TIME; // repeat on second
			if(button_repeat_sec < BTN_PRESS-1 && now > button_repeat_sec) {
				evqueue_write(EV_BUTTON, (unsigned short)now, now-button_pressed_at);
				button_repeat_sec++;
			}
		} else { // released
			evqueue_write(EV_BUTTON, BTN_RELEASE, now-button_pressed_at);
			button_pressed_at = 0;
			return;
		}
	} else if(button_state) { // pressed
		button_pressed_at = now;
		button_repeat_sec = 0;
		evqueue_write(EV_BUTTON, BTN_PRESS, button_pressed_at);
	}
}

void button_init() {
	button_state = gpio_read(GPIO_BUTTON);
	gpio_attach_interrupt(GPIO_BUTTON, BtnIntReq, GPIO_BOTH_EDGES);
}
