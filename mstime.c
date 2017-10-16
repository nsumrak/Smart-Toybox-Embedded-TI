// Smart Toybox milisecond time services
// TI PLATFORM implementation
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

#include "mstime.h"
#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_ints.h"
#include "interrupt.h"
#include "timer.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "utils.h"
#include "evqueue.h"
#include "common.h"


struct timer_desc timers[TIMER_COUNT];
unsigned long g_timercount = 0;


static void TimerHandler(void)
{
	// Clear all pending interrupts from the timer we are currently using.
	unsigned long ulInts = MAP_TimerIntStatus(TIMERA0_BASE, true);
	MAP_TimerIntClear(TIMERA0_BASE, ulInts);
	g_timercount += TIMER_MS_RESOLUTION;
	evqueue_write(EV_TIMER_TICK, 0, g_timercount);
}


void time_init()
{
	MAP_PRCMPeripheralClkEnable(PRCM_TIMERA0, PRCM_RUN_MODE_CLK);
	MAP_PRCMPeripheralReset(PRCM_TIMERA0);
	MAP_TimerConfigure(TIMERA0_BASE,TIMER_CFG_PERIODIC);
	MAP_TimerPrescaleSet(TIMERA0_BASE,TIMER_A,0);

	MAP_IntPrioritySet(INT_TIMERA0A, INT_PRIORITY_LVL_1);
    MAP_TimerIntRegister(TIMERA0_BASE, TIMER_A, TimerHandler);
    MAP_TimerIntEnable(TIMERA0_BASE, TIMER_TIMA_TIMEOUT);

	MAP_TimerLoadSet(TIMERA0_BASE,TIMER_A,MILLISECONDS_TO_TICKS(TIMER_MS_RESOLUTION));
	MAP_TimerEnable(TIMERA0_BASE,TIMER_A);
}

void timer_process()
{
	unsigned char i;
	for (i = 0; i < TIMER_COUNT; i++) {
		if (timers[i].active && !timers[i].paused) {
			if (timers[i].rest > TIMER_MS_RESOLUTION)
				timers[i].rest -= TIMER_MS_RESOLUTION;
			else {
				if (timers[i].loopcnt && (timers[i].loopcnt == 255 || --timers[i].loopcnt)) {
					timers[i].rest = timers[i].time;
				}
				else {
					timers[i].rest = 0;
					timers[i].active = 0;
				}
				evqueue_write(EV_TIMER_EXPIRED, i, 0);
				if(timers[i].cb) timers[i].cb(i);
			}
		}
	}
}
