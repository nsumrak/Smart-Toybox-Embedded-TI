// Smart Toybox Network Interface
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

#include "mstime.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TIMER_NETWORK	(TIMER_COUNT-1)

#define SL_STOP_TIMEOUT 200
#define AUTO_CONNECTION_TIMEOUT_COUNT 30000 // 30 Sec
#define SMART_CONFIG_BUTTON_TIMEOUT_COUNT 4000 // 30 Sec


// Status bits - These are used to set/reset the corresponding bits in given variable
typedef enum {
	STATUS_BIT_NWP_INIT = 0, // If this bit is set: Network Processor is
							 // powered up
	STATUS_BIT_CONNECTION,   // If this bit is set: the device is connected to
							 // the AP or client is connected to device (AP)
	STATUS_BIT_IP_LEASED,    // If this bit is set: the device has leased IP to
							 // any connected client
	STATUS_BIT_IP_AQUIRED,   // If this bit is set: the device has acquired an IP
	STATUS_BIT_SMARTCONFIG_START, // If this bit is set: the SmartConfiguration
								  // process is started from SmartConfig app
	STATUS_BIT_P2P_DEV_FOUND,    // If this bit is set: the device (P2P mode)
								 // found any p2p-device in scan
	STATUS_BIT_P2P_REQ_RECEIVED, // If this bit is set: the device (P2P mode)
								 // found any p2p-negotiation request
	STATUS_BIT_CONNECTION_FAILED, // If this bit is set: the device(P2P mode)
								  // connection to client(or reverse way) is failed
	STATUS_BIT_PING_DONE         // If this bit is set: the device has completed
								 // the ping operation
} e_StatusBits;



#define CLR_STATUS_BIT_ALL(status_variable)   (status_variable &= 1)
#define SET_STATUS_BIT(status_variable, bit) status_variable |= (1<<(bit))
#define CLR_STATUS_BIT(status_variable, bit) status_variable &= ~(1<<(bit))
#define GET_STATUS_BIT(status_variable, bit) (0 != (status_variable & (1<<(bit))))

#define CONNECTED_AND_HAS_IP() (GET_STATUS_BIT(status, STATUS_BIT_CONNECTION) && GET_STATUS_BIT(status, STATUS_BIT_IP_AQUIRED))


enum {
	HTTPSERV_TEST,
};

long network_smartconfig();
int network_init();
void network_restart_http();
long network_start_smartcfg();
void network_smartconfig_finish();

void network_async_finish();
int network_async_init();

// returns role when network connected, -1 otherwise
short network_get_role();
void network_status_led(short role);

#ifdef __cplusplus
}
#endif
