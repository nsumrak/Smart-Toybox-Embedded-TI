// Smart Toybox Event Queue implementation
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

enum {
	EV_ACCEL_TAP,
	EV_ACCEL_FLAT,
	EV_BUTTON,
	EV_HTTP_SERVER,
	EV_WEIGHT_START,
	EV_WEIGHT,
	EV_WEIGHT_STOP,
	EV_NEW_HTTP_FILE,
	EV_TIMER_TICK,
	EV_TIMER_EXPIRED,
	EV_NETWORK_CONNECTED,
	EV_NETWORK_DISCONNECTED,
	EV_NETWORK_SMARTCONFIG_COMPLETE,
	EV_FWUPGRADE_FINISHED,
	EV_CALIBRATION_FINISHED,
	EV_LAST
};

struct event {
	unsigned char id;
	unsigned short arg1;
	unsigned long arg2;
	unsigned long time;
};

void evqueue_init();
BOOL evqueue_read(struct event *ev);
BOOL evqueue_write(unsigned char id, unsigned short arg1, unsigned long arg2);
