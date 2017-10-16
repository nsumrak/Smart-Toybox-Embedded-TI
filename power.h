// Power related functions interface
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

#include "prcm.h"
#include "rom_map.h"
#include <utils.h>

#include "debug.h"

// Reboot the MCU by requesting hibernate for a short duration
inline void reboot()
{
	UART_PRINT("Rebooting...\n\r");
	// Configure hibernate RTC wakeup
	PRCMHibernateWakeupSourceEnable(PRCM_HIB_SLOW_CLK_CTR);

	MAP_UtilsDelay(8000000);

	// Set wake up time
	PRCMHibernateIntervalSet(330);
	PRCMHibernateEnter();

	// Control should never reach here
	while(1) {}
}
