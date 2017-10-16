// watchdog timer interface
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

#pragma once

#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "prcm.h"
#include "wdt.h"
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "mstime.h"
#include "debug.h"

#define WATCHDOG_PERIOD 10000

inline void watchdog_ack()
{
    MAP_WatchdogIntClear(WDT_BASE);
}

inline void watchdog_deinit()
{
    // Unlock to be able to configure the registers
    MAP_WatchdogUnlock(WDT_BASE);

    // Disable stalling of the watchdog timer during debug events
    MAP_WatchdogStallDisable(WDT_BASE);

    MAP_WatchdogIntClear(WDT_BASE);
    MAP_WatchdogIntUnregister(WDT_BASE);
}

inline void watchdog_init()
{
    // Enable the peripherals used by this example.
    MAP_PRCMPeripheralClkEnable(PRCM_WDT, PRCM_RUN_MODE_CLK);

    // Unlock to be able to configure the registers
    MAP_WatchdogUnlock(WDT_BASE);

    MAP_WatchdogStallEnable(WDT_BASE);

	MAP_IntPrioritySet(INT_WDT, INT_PRIORITY_LVL_1);
	MAP_WatchdogIntRegister(WDT_BASE, 0);
    MAP_WatchdogReloadSet(WDT_BASE, MILLISECONDS_TO_TICKS(WATCHDOG_PERIOD));

    // Start the timer. Once the timer is started, it cannot be disable.
    MAP_WatchdogEnable(WDT_BASE);
    if(!MAP_WatchdogRunning(WDT_BASE)) {
    	UART_PRINT("Warning: could not start watchdog!\n\r");
    	watchdog_deinit();
    }
}
