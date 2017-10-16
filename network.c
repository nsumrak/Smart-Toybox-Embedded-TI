// Smart Toybox Network Interface
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

#define DNS_NAME			"SmartToybox"
#define SERVICE_DESC		"SmartToybox connection service"
#define SERVICE_NAME		"SmartToybox._toybox._tcp.local"
#define SERVICE_PORT		8080

#include <stdio.h>
#include <ctype.h>

// Simplelink includes
#include "simplelink.h"

// driverlib includes
#include "hw_types.h"
#include "rom.h"
#include "rom_map.h"
#include "utils.h"

// App Includes
#include "network.h"
#include "audiohw.h"
#include "debug.h"
#include "common.h"
//#include "httpclient.h"
#include "evqueue.h"
#include "mstime.h"
#include "button.h"
#include "led.h"
#include "watchdog.h"

static volatile unsigned long  status = 0;//SimpleLink Status
static unsigned long ip = 0; //Device IP address
static unsigned long gateway = 0; //Device IP address
static unsigned char ssid[MAXIMAL_SSID_LENGTH + 1]; //Connection SSID
static unsigned char bssid[SL_BSSID_LENGTH]; //Connection BSSID
static _i16 g_role = -1;

#define HTTP_GET_UNAME "__SL_G_UNM"
#define HTTP_POST_VOLUME "__SL_P_UAV"
#define HTTP_POST_TEST "__SL_P_UTT"

short network_get_role()
{
	return CONNECTED_AND_HAS_IP() ? g_role : -1;
}

// turn on green if network connected in station mode, yellow for ap mode, red if not connected
void network_status_led(short role)
{
	if (role >= 0)
		led_on_time(role == ROLE_STA ? LED_GREEN : LED_YELLOW, 3000);
	else {
		led_blink_cnt(LED_YELLOW, 4);
	}
}

// SimpleLink Asynchronous Event Handlers
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pSlHttpServerEvent, SlHttpServerResponse_t *pSlHttpServerResponse)
{
	if (!pSlHttpServerEvent || !pSlHttpServerResponse) {
		UART_PRINT("SimpleLinkHttpServerCallback param is null\n\r");
		return;
	}

//	UART_PRINT("SimpleLinkHttpServerCallback %d\n\r", pSlHttpServerEvent->Event);
	switch (pSlHttpServerEvent->Event) {
	case SL_NETAPP_HTTPGETTOKENVALUE_EVENT: {
		if (!memcmp(pSlHttpServerEvent->EventData.httpTokenName.data, HTTP_GET_UNAME, 10)) {
			memcpy(pSlHttpServerResponse->ResponseData.token_value.data, "petra", 5);
			pSlHttpServerResponse->ResponseData.token_value.len = 5;
		}
		break;
	}
	case SL_NETAPP_HTTPPOSTTOKENVALUE_EVENT: {
		if (!memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, HTTP_POST_VOLUME, 10)) {
			unsigned char *ptr = pSlHttpServerEvent->EventData.httpPostData.token_value.data;
			unsigned char len = pSlHttpServerEvent->EventData.httpPostData.token_value.len;
			if (ptr && len && isdigit(*ptr)) {
				unsigned char volume = 0;
				do {
					volume = volume * 10 + (*ptr++ - '0');
				} while (--len && isdigit(*ptr) && volume <= 100);
				if (volume) {
					audiohw_speaker_mute(0);
					audiohw_set_master_volume(volume);
				} else {
					audiohw_speaker_mute(1);
				}
			}
		} else if (!memcmp(pSlHttpServerEvent->EventData.httpPostData.token_name.data, HTTP_POST_TEST, 10)) {
			unsigned char *ptr = pSlHttpServerEvent->EventData.httpPostData.token_value.data;
			if (pSlHttpServerEvent->EventData.httpPostData.token_value.len && ptr) {
				evqueue_write(EV_HTTP_SERVER, HTTPSERV_TEST, (unsigned short)(*ptr));
			}
		}
		break;
	}
	default:
		break;
	}
}


void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    if(!pNetAppEvent) {
        UART_PRINT("SimpleLinkNetAppEventHandler pNetAppEvent is null\n\r");
        //LOOP_FOREVER();
        return;
    }

	switch (pNetAppEvent->Event) {
	case SL_NETAPP_IPV4_IPACQUIRED_EVENT: {
		SET_STATUS_BIT(status, STATUS_BIT_IP_AQUIRED);

		timer_stop(TIMER_NETWORK);
		ip = pNetAppEvent->EventData.ipAcquiredV4.ip;
		UNUSED(ip);
		gateway = pNetAppEvent->EventData.ipAcquiredV4.gateway;
		UNUSED(gateway);
		evqueue_write(EV_NETWORK_CONNECTED, g_role, ip);

		UART_PRINT("[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , Gateway=%d.%d.%d.%d\n\r",
				SL_IPV4_BYTE(ip, 3), SL_IPV4_BYTE(ip, 2), SL_IPV4_BYTE(ip, 1), SL_IPV4_BYTE(ip, 0),
				SL_IPV4_BYTE(gateway, 3), SL_IPV4_BYTE(gateway, 2), SL_IPV4_BYTE(gateway, 1), SL_IPV4_BYTE(gateway, 0));
		break;
	}
	default:
		UART_PRINT("[NETAPP EVENT] Unexpected event [0x%x] \n\r", pNetAppEvent->Event);
	}
}


void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
	UART_PRINT("SimpleLinkWlanEventHandler pWlanEvent: %d\n\r", pWlanEvent->Event);

	if (!pWlanEvent) {
		UART_PRINT("SimpleLinkWlanEventHandler pWlanEvent is null\n\r");
		//LOOP_FOREVER();
		return;
	}

	switch (pWlanEvent->Event) {
	case SL_WLAN_CONNECT_EVENT: {
		SET_STATUS_BIT(status, STATUS_BIT_CONNECTION);

		// Copy new connection SSID and BSSID to global parameters
		memcpy(ssid, pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_name, pWlanEvent->EventData.STAandP2PModeWlanConnected.ssid_len);
		memcpy(bssid, pWlanEvent->EventData.STAandP2PModeWlanConnected.bssid, SL_BSSID_LENGTH);

		UART_PRINT("[WLAN EVENT] STA Connected to the AP: %s , "
				"BSSID: %x:%x:%x:%x:%x:%x\n\r", ssid, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

		break;
	}
	case SL_WLAN_DISCONNECT_EVENT: {
		CLR_STATUS_BIT(status, STATUS_BIT_CONNECTION);
		CLR_STATUS_BIT(status, STATUS_BIT_IP_AQUIRED);

		evqueue_write(EV_NETWORK_DISCONNECTED, 0, ip);

		// If the user has initiated 'Disconnect' request,
		//'reason_code' is SL_USER_INITIATED_DISCONNECTION
		if (pWlanEvent->EventData.STAandP2PModeDisconnected.reason_code == SL_USER_INITIATED_DISCONNECTION) {
			UART_PRINT("[WLAN EVENT]Device disconnected from the AP: %s, "
					"BSSID: %x:%x:%x:%x:%x:%x on application's "
					"request \n\r", ssid, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
		} else {
			UART_PRINT("[WLAN ERROR]Device disconnected from the AP AP: %s, "
					"BSSID: %x:%x:%x:%x:%x:%x on an ERROR..!! \n\r", ssid, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
		}
		memset(ssid, 0, sizeof(ssid));
		memset(bssid, 0, sizeof(bssid));
		break;
	}
	case SL_WLAN_SMART_CONFIG_COMPLETE_EVENT:
		SET_STATUS_BIT(status, STATUS_BIT_SMARTCONFIG_START);
		UART_PRINT("[WLAN EVENT] Smart config complete\n\r");
		evqueue_write(EV_NETWORK_SMARTCONFIG_COMPLETE, 0, ip);
//		UART_PRINT("sl_WlanSmartConfigStop returned %d\n\r", sl_WlanSmartConfigStop());
		break;
	case SL_WLAN_SMART_CONFIG_STOP_EVENT:
		CLR_STATUS_BIT(status, STATUS_BIT_SMARTCONFIG_START);
		UART_PRINT("[WLAN EVENT] Smart config stop\n\r");
	default:
		UART_PRINT("[WLAN EVENT] Unexpected event [0x%x]\n\r", pWlanEvent->Event);
	}
}


void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
	// This application doesn't work w/ socket - Events are not expected
	switch (pSock->Event) {
	case SL_SOCKET_TX_FAILED_EVENT:
		switch (pSock->socketAsyncEvent.SockTxFailData.status) {
		case SL_ECLOSE:
			UART_PRINT("[SOCK ERROR] - close socket (%d) operation "
					"failed to transmit all queued packets\n\n", pSock->socketAsyncEvent.SockAsyncData.sd);
			break;
		default:
			UART_PRINT("[SOCK ERROR] - TX FAILED : socket %d , reason"
					"(%d) \n\n", pSock->socketAsyncEvent.SockAsyncData.sd, pSock->socketAsyncEvent.SockTxFailData.status);
		}
		break;

	default:
		UART_PRINT("[SOCK EVENT] - Unexpected Event [%x0x]\n\n", pSock->Event);
	}
}


static _i16 ConfigureMode(int mode)
{
	_i16 ret = sl_WlanSetMode(mode);
	if (ret < 0) {
		ERR_PRINT(ret);
		return -1;
	}

    // Restart Network processor
    ret = sl_Stop(SL_STOP_TIMEOUT);

    // reset status bits
    CLR_STATUS_BIT_ALL(status);

    ret = g_role = sl_Start(NULL,NULL,NULL);

   	SET_STATUS_BIT(status, STATUS_BIT_NWP_INIT);
	sl_NetAppSet(SL_NET_APP_DEVICE_CONFIG_ID, NETAPP_SET_GET_DEV_CONF_OPT_DEVICE_URN, strlen(DNS_NAME), (_u8 *) DNS_NAME);
	sl_NetAppMDNSUnRegisterService(SERVICE_NAME, (unsigned char)strlen(SERVICE_NAME));
	sl_NetAppMDNSRegisterService(SERVICE_NAME, (unsigned char)strlen(SERVICE_NAME), SERVICE_DESC, (unsigned char)strlen(SERVICE_DESC), SERVICE_PORT, 2000, 0); // last 0 - service is unique
	// what if fail???

	return ret;
}


void network_smartconfig_finish()
{
	unsigned char policyVal;
	sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(1,0,0,0,0), &policyVal, 1 /*PolicyValLen*/);
	CLR_STATUS_BIT(status, STATUS_BIT_SMARTCONFIG_START);
	UART_PRINT("smart config finish\n\r");
}


long network_smartconfig()
{
	unsigned char policyVal;
	long ret = -1;

 	UART_PRINT("starting smart config\r\n");
	//sl_WlanDisconnect();

	// Clear all profiles
	// This is of course not a must, it is used in this example to make sure
	// we will connect to the new profile added by SmartConfig
	ret = sl_WlanProfileDel(0xff);
	if (ret < 0) {
		ERR_PRINT(ret);
		return -1;
	}

	//set AUTO policy
	ret = sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(1, 0, 0, 0, 1), &policyVal, 1 /*PolicyValLen*/);
	if (ret < 0) {
		ERR_PRINT(ret);
		return -1;
	}

	// Start SmartConfig
	// This example uses the unsecured SmartConfig method
	//
	ret = sl_WlanSmartConfigStart(0, SMART_CONFIG_CIPHER_NONE, 0, 0, 0, NULL, NULL, NULL);
	if (ret < 0) {
		ERR_PRINT(ret);
		return -1;
	}

	UART_PRINT("smartconfig started\n\r");
	return 0;
}

static void network_connect_expired(unsigned char id)
{
	if (CONNECTED_AND_HAS_IP()) return;
	// Couldn't connect Using Auto Profile
	UART_PRINT("Couldn't connect Using Auto Profile, going AP\n\r");

	g_role = ConfigureMode(ROLE_AP);
	if(g_role != ROLE_AP) {
		UART_PRINT("AP mode fail!\n\r");
		g_role = -1;
	}
	return;
}

long network_start_smartcfg()
{
	UART_PRINT("board enters smart config\n\r");
	if(g_role == ROLE_AP) {
		UART_PRINT("AP mode, waiting for IP\n\r");
		// somewhere said that in AP mode it must wait for IP
		while (!GET_STATUS_BIT(status, STATUS_BIT_IP_AQUIRED)) {
			_SlNonOsMainLoopTask();
			watchdog_ack();
		}
		UART_PRINT("got IP, changing to STA\n\r");
		g_role = ConfigureMode(ROLE_STA);
		if(g_role != ROLE_STA) {
			UART_PRINT("STA mode fail!\n\r");
			return -1;
		}
	}
	return network_smartconfig();
}

void network_restart_http()
{
	_i16 ret;
	// Restart Internal HTTP Server
	if ((ret = sl_NetAppStop(SL_NET_APP_HTTP_SERVER_ID)) < 0 || (ret = sl_NetAppStart(SL_NET_APP_HTTP_SERVER_ID))) {
		ERR_PRINT(ret);
//		return -1;
	}
}

static BOOL network_is_profile_saved()
{
	_i8 i;
	_i8 name[32];
	_u8 mac[6];
 	SlSecParams_t security;
 	SlGetSecParamsExt_t sec_ent;
 	_u32 prio;
	_i16 namelen;

	for(i=0; i<7; i++) {
		if(sl_WlanProfileGet(i, name, &namelen, mac, &security, &sec_ent, &prio) >= 0)
			return TRUE;
	}
	return FALSE;
}

static void network_do_config_finish()
{
	// check if there is profiles saved, if not - smart config
	if(!network_is_profile_saved())
	{
		network_connect_expired(TIMER_NETWORK);
		return;
	}

	if (g_role != ROLE_STA) {
		if (g_role == ROLE_AP) {
			// If the device is in AP mode, we need to wait for this event
			// before doing anything
			while (!GET_STATUS_BIT(status, STATUS_BIT_IP_AQUIRED)) {
				_SlNonOsMainLoopTask();
				watchdog_ack();
			}
		}

		g_role = ConfigureMode(ROLE_STA);
		if (g_role != ROLE_STA) {
			g_role = sl_Stop(SL_STOP_TIMEOUT);
			CLR_STATUS_BIT_ALL(status);
			g_role = -1;
			UART_PRINT("Unable to set STA mode...\n\r");
			return;
		}
	}

	// wait for the device to Auto Connect
	timer_set_cb(TIMER_NETWORK, AUTO_CONNECTION_TIMEOUT_COUNT, 1, network_connect_expired);
}

static void network_do_started(_u32 stat)
{
	UART_PRINT("netproc started status %d\n\r",stat);
	SET_STATUS_BIT(status, STATUS_BIT_NWP_INIT);
	g_role = stat;
}

int network_async_init()
{
	//Start Simplelink Device
	g_role = sl_Start(0, 0, network_do_started);

	if (g_role < 0) {
		ERR_PRINT(g_role);
		return g_role;
	}
	return 0;
}

void network_async_finish()
{
	while (!GET_STATUS_BIT(status, STATUS_BIT_NWP_INIT)) {
		_SlNonOsMainLoopTask();
		watchdog_ack();
	}

	UART_PRINT("net init done role=%d\n\r", g_role);
	network_do_config_finish();
	UART_PRINT("net init finished role=%d\n\r", g_role);
}

int network_init()
{
	//Start Simplelink Device
	_i16 ret = g_role = sl_Start(0, 0, 0);

	if (ret < 0) {
		ERR_PRINT(ret);
		return ret;
	}
	UART_PRINT("simple link started\n\r");
	SET_STATUS_BIT(status, STATUS_BIT_NWP_INIT);
	_SlNonOsMainLoopTask();
	watchdog_ack();

	network_do_config_finish();
	return 0;
}

//#define PING_INTERVAL       1000    /* In msecs */
//#define PING_TIMEOUT        3000    /* In msecs */
//#define PING_PKT_SIZE       20      /* In bytes */
//#define NO_OF_ATTEMPTS      3
//#define HOST_NAME               "www.ti.com"
//unsigned long  g_ulPingPacketsRecv = 0; //Number of Ping Packets received
//static void SimpleLinkPingReport(SlPingReport_t *pPingReport)
//{
//	UART_PRINT("SimpleLinkPingReport: %d\n\r", pPingReport->PacketsReceived);
//	SET_STATUS_BIT(status, STATUS_BIT_PING_DONE);
//	g_ulPingPacketsRecv = pPingReport->PacketsReceived;
//}
//static int CheckInternetConnection()
//{
//    SlPingStartCommand_t pingParams = {0};
//    SlPingReport_t pingReport = {0};
//
//    unsigned long ulIpAddr = 0;
//    long lRetVal = -1;
//
//    CLR_STATUS_BIT(status, STATUS_BIT_PING_DONE);
//    g_ulPingPacketsRecv = 0;
//
//    // Set the ping parameters
//    pingParams.PingIntervalTime = PING_INTERVAL;
//    pingParams.PingSize = PING_PKT_SIZE;
//    pingParams.PingRequestTimeout = PING_TIMEOUT;
//    pingParams.TotalNumberOfAttempts = NO_OF_ATTEMPTS;
//    pingParams.Flags = 0;
//    pingParams.Ip = gateway;
//
//    // Get external host IP address
//    lRetVal = sl_NetAppDnsGetHostByName((signed char*)HOST_NAME, sizeof(HOST_NAME), &ulIpAddr, SL_AF_INET);
//    if (lRetVal < 0) {
//    	ERR_PRINT(lRetVal);
//    	return -1;
//    }
//
//    // Replace the ping address to match HOST_NAME's IP address
//    pingParams.Ip = ulIpAddr;
//
//    // Try to ping HOST_NAME
//    lRetVal = sl_NetAppPingStart((SlPingStartCommand_t*)&pingParams, SL_AF_INET, (SlPingReport_t*)&pingReport, SimpleLinkPingReport);
//    if (lRetVal < 0) {
//    	ERR_PRINT(lRetVal);
//    	return -1;
//    }
//
//    // Wait
//    while(!GET_STATUS_BIT(status, STATUS_BIT_PING_DONE))
//    {
//      // Wait for Ping Event
//#ifndef SL_PLATFORM_MULTI_THREADED
//        _SlNonOsMainLoopTask();
//#endif
//    }
//
//    if (!g_ulPingPacketsRecv)
//    {
//        // Problem with internet connection
//        UART_PRINT("INTERNET_CONNECTION_FAILED\n\r");
//        return -1;
//    }
//
//    // Internet connection is successful
//    return 0;
//}
