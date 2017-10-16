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

#include "platform.h"
#include "string.h"
#include "mstime.h"
#include "debug.h"
#include "evqueue.h"

#define EVQUEUE_SIZE 32 // must be power of 2
static struct event evqueue[EVQUEUE_SIZE];
static unsigned char nextread = 0, nextwrite = 0;
static BOOL empty = 1;

void evqueue_init()
{
	nextread = nextwrite = 0;
	empty = 1;
}

BOOL evqueue_read(struct event *ev)
{
	if (empty) return 0;
	if (ev) *ev = *(evqueue + nextread);
	int_disable_all();
	nextread++; nextread &= EVQUEUE_SIZE - 1;
	if (nextread == nextwrite) empty = 1;
	int_enable_all();
	return 1;
}

BOOL evqueue_write(unsigned char id, unsigned short arg1, unsigned long arg2)
{
	ASSERT(id < EV_LAST);

	int_disable_all();
	if (!empty && nextread == nextwrite) { // queue is full
		int_enable_all();
		return 0;
	}
	(evqueue + nextwrite)->id = id;
	(evqueue + nextwrite)->arg1 = arg1;
	(evqueue + nextwrite)->arg2 = arg2;
	(evqueue + nextwrite)->time = time_get();
	nextwrite++; nextwrite &= EVQUEUE_SIZE - 1;
	empty = 0;
	int_enable_all();
	return 1;
}
