// Smart Toybox milisecond time services
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
#include "debug.h"

#define TIMER_MS_RESOLUTION		50

#define SYS_CLK				    80000000
#define MILLISECONDS_TO_TICKS(ms)   ((SYS_CLK/1000) * (ms))

#define TIMER_COUNT 8


typedef void (*timer_callback)(unsigned char id);

struct timer_desc {
	BOOL active;
	BOOL paused;
	unsigned char loopcnt; // loop count, 255 is infinite
	unsigned long time;
	unsigned long rest; // ms till timer expires
	timer_callback cb;
};

extern struct timer_desc timers[TIMER_COUNT];
extern unsigned long g_timercount;

void time_init();
void timer_process();

inline void timer_set_cb(unsigned char id, unsigned long time, unsigned char loopcnt, timer_callback cb)
{
	ASSERT(id < TIMER_COUNT);
	timers[id].time = timers[id].rest = time;
	timers[id].loopcnt = loopcnt;
	timers[id].cb = cb;
	timers[id].paused = 0;
	timers[id].active = loopcnt ? 1 : 0;
}

inline void timer_set(unsigned char id, unsigned long time, unsigned char loopcnt)
{
	timer_set_cb(id,time,loopcnt,0);
}

inline void timer_reset(unsigned char id)
{
	ASSERT(id < TIMER_COUNT);
	timers[id].rest = timers[id].time;
	timers[id].paused = 0;
}

inline void timer_start(unsigned char id)
{
	ASSERT(id < TIMER_COUNT);
	timers[id].active = 1;
	timers[id].paused = 0;
	timers[id].rest = timers[id].time;
	if(!timers[id].loopcnt) timers[id].loopcnt = 1;
}

inline void timer_stop(unsigned char id)
{
	ASSERT(id < TIMER_COUNT);
	timers[id].active = 0;
}

inline void timer_pause(unsigned char id)
{
	ASSERT(id < TIMER_COUNT);
	timers[id].paused = 1;
}

inline void timer_resume(unsigned char id)
{
	ASSERT(id < TIMER_COUNT);
	timers[id].paused = 0;
}

inline BOOL timer_active(unsigned char id)
{
	ASSERT(id < TIMER_COUNT);
	return (timers[id].active && !timers[id].paused);
}

inline void timer_delay(unsigned char id, unsigned long delay)
{
	ASSERT(id < TIMER_COUNT);
	if (timers[id].rest < delay)
		timers[id].rest = delay;
}

inline unsigned long time_get()
{
	return g_timercount;
}
