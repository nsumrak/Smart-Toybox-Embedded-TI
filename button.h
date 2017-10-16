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

#pragma once

#include "common.h"
#include "mstime.h"

enum {
	BTN_PRESS = 0xffff,
	BTN_RELEASE = 0,
	BTN_REPEAT_1s,
	BTN_REPEAT_2s,
	BTN_REPEAT_3s,
	BTN_REPEAT_4s,
	BTN_REPEAT_5s
};

#define BUTTON_REPEAT_TIME 1000

extern volatile unsigned long button_pressed_at;

void button_init();
void button_process(unsigned long now);

inline BOOL button_get()
{
	if(button_pressed_at)
		return time_get()-button_pressed_at;
	else
		return 0;
}
