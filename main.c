// Smart Toybox main application loop
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

#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "network.h"
#include "circ_buff.h"
#include "audiohw.h"
#include "audioplay.h"
#include "hx711.h"
#include "debug.h"
#include "common.h"
#include "circ_buff.h"
#include "files.h"
#include "mstime.h"
#include "accel.h"
#include "button.h"
#include "evqueue.h"
#include "http.h"
#include "game.h"
#include "settings.h"
#include "led.h"
#include "upgrade.h"
#include "power.h"
#include "watchdog.h"


#ifdef DEBUG
#define MUSIC_VOLUME 90
#define EFFECT_VOLUME 100
#define MASTER_VOLUME 100
#else
#define MUSIC_VOLUME 90
#define EFFECT_VOLUME 100
#define MASTER_VOLUME 100
#endif


static _u32 smarttoyboxip;

void print_client_side(ConnDesc *cd, char *buf, int len)
{
	if(!cd->woffs) {
		UART_PRINT("--http response (code=%d, len=%d)\r\n",cd->fd,cd->contlen);
	}
	//UART_PRINT(buf);
}


int main()
{
	platform_init();
	watchdog_init();

	unsigned long reset_cause = PRCMSysResetCauseGet();
#ifdef DEBUG
	UART_PRINT("*** Starting DEBUG %s, reset reason = %d\n\r", VERSION_DATE, reset_cause);
#else
	UART_PRINT("*** Starting RELEASE %s! reset reason = %d\n\r", VERSION_DATE, reset_cause);
#endif

	if (reset_cause == PRCM_WDT_RESET) reboot();

	evqueue_init();
	time_init();
	button_init();
	led_init();

	if (network_async_init())
		UART_PRINT("wlan connect failed\n\r");

	i2c_init(TRUE);

	// Initialize the audio hardware module
	if(audiohw_init()) {
		UART_PRINT("audio hw fail\n\r");
		LOOP_FOREVER();
	}
#ifdef AUDIO_SOFTWARE_MONO
	audiohw_set_channel_volume(0, 0);
#else
	audiohw_set_channel_volume(0, MUSIC_VOLUME);
#endif
	audiohw_set_channel_volume(1, EFFECT_VOLUME);
	audiohw_set_master_volume(MASTER_VOLUME);
	UART_PRINT("audio hardware initialized\n\r");

	if(accel_init()) {
		UART_PRINT("accel fail\n\r");
	}

	network_async_finish();
	hx711_init();
	file_init(); // note: should be called after simplelink init
	audioplay_init();

	for (;;_SlNonOsMainLoopTask()) {
		watchdog_ack();

		audioplay_process();
		http_process();

		struct event ev;
		char fn[2] = { 0 };

		while (evqueue_read(&ev)) {
			switch(ev.id) {
			case EV_ACCEL_TAP:
				UART_PRINT("accel tap\n\r");
				//http_connect(smarttoyboxip, "GET", "smarttoybox.com", "/", 0, 0, print_client_side);
				break;
			case EV_ACCEL_FLAT:
				UART_PRINT("accel flat: %s\n\r", ev.arg1?"yes":"no");
				if (game_started()) {
					game_process_accel_flat(ev.arg1);
				} else {
					if (!ev.arg1) { // flat
						UART_PRINT("play gardian!\n\r");
						if (audioplay_playmusic(1, GUARDIAN, 0))
							UART_PRINT("can't play GUARDIAN effect!\n\r");
					} else { // not flat
						UART_PRINT("play gardian_off!\n\r");
						if (audioplay_playmusic(1, GUARDIAN_OFF, 0))
							UART_PRINT("can't play GUARDIAN_OFF effect!\n\r");
					}
				}
				break;
			case EV_BUTTON:
				// 0-1s game start/stop
				// 1-2s get network status
				// 2-3s fw upgrade
				// 4-5s reboot
				// 5-6s smart config
				// 20+s factory reset
				if (ev.arg1 != BTN_RELEASE) led_blink_cnt(LED_GREEN, 1);
				if(ev.arg1 == BTN_PRESS) UART_PRINT("button press\n\r");
				else if(ev.arg1 == BTN_RELEASE) {
					UART_PRINT("button released after %d ms\n\r",ev.arg2);
					// if game or fw upgrade is in progress btn led will destroy their led scheme, so we should restore it
					if (game_started()) game_set_mood_led();
					if (upgrade_in_progress()) upgrade_set_led();
					if (ev.arg2 > 50) {
						if (ev.arg2 < 1000) {
							// start/stop game
							if (!game_started()) {
								if (upgrade_in_progress() || !accel_get_flat() || !game_start()) {
									led_blink_cnt(LED_RED, 4);
									UART_PRINT("game could not be started\n\r");
								}
							} else {
								game_stop(); // TODO cancel instead of stop
							}
						} else if (ev.arg2 < 2000) {
							// network status
							if (!game_started()) {
								short role = network_get_role();
								network_status_led(role);
								UART_PRINT("network status is %s\n\r", role == ROLE_STA ? "STA" : (role == ROLE_AP ? "AP" : "not connected"));
							}
						} else if (ev.arg2 < 3000) {
							// software update
							if (game_started()) game_stop();
							upgrade();
						} else if (ev.arg2 >= 4000 && ev.arg2 < 5000) {
							// reboot
							if (game_started()) game_stop();
							reboot();
						} else if (ev.arg2 < 6000) {
							// smart config
							if (!game_started() && network_start_smartcfg()) {
								led_blink_cnt(LED_RED, 4);
								UART_PRINT("could not start smart cfg\n\r");
							}
						} else if (ev.arg2 >= 20000) {
							// factory reset (TODO only fact image for now, but settings should be reset also)
							if (game_started()) game_stop();
							restore_to_factory_img();
						}
					}
				}
				else {
					UART_PRINT("button repeat %d\n\r", ev.arg1);
				}
				break;
			case EV_WEIGHT_START:
				//UART_PRINT("EV_WEIGHT_START: grams=%ld count = %d\n\r", ev.arg1, ev.arg2);
				if (game_started()) game_process_weight_start(ev.arg1, ev.arg2);
				break;
			case EV_WEIGHT:
				//UART_PRINT("EV_WEIGHT: grams=%ld count = %d\n\r", ev.arg1, ev.arg2);
				if (game_started()) game_process_weight(ev.arg1, ev.arg2);
				break;
			case EV_WEIGHT_STOP:
				//UART_PRINT("EV_WEIGHT_STOP: grams=%ld count = %d\n\r", ev.arg1, ev.arg2);
				if (game_started()) game_process_weight_stop(ev.arg1, ev.arg2);
				break;
			case EV_HTTP_SERVER:
				switch (ev.arg1) {
				case HTTPSERV_TEST:
					UART_PRINT("http server event: %c %d\n\r", ev.arg2, ev.arg2);
					break;
				default:
					UART_PRINT("unknown http server event: %d %d\n\r", ev.arg1, ev.arg2);
				}
				break;
			case EV_NEW_HTTP_FILE:
				fn[0] = (char)ev.arg1;
				if (fn[0] == 't') {
					UART_PRINT("new theme!\n\r");
					int err = stb_open(THEME_FILE);
					if (err) UART_PRINT("stb_open returned %d\n\r", err);
				} else
					audioplay_playfile(1, fn);
				break;
			case EV_TIMER_TICK:
				timer_process();
				button_process(ev.arg2);
				{
//					static unsigned count = 0;
//					if(++count % 20 == 0) UART_PRINT("sec\n\r");
				}
				break;
			case EV_TIMER_EXPIRED:
				//UART_PRINT("timer %d expired!\n\r", ev.arg1);
				if (game_started()) game_process_timer_expired(ev.arg1);
				break;
			case EV_NETWORK_CONNECTED:
				UART_PRINT("network connected as %s\n\r", ev.arg1 == ROLE_STA ? "Station" : "AP");
				network_status_led(ev.arg1);
				network_restart_http();
				http_init_server(8080);
				//smarttoyboxip = http_resolve_host("smarttoybox.com");
				break;
			case EV_NETWORK_DISCONNECTED:
				network_status_led(-1);
				UART_PRINT("network disconnected\n\r");
				http_deinit_server();
				break;
			case EV_NETWORK_SMARTCONFIG_COMPLETE:
				network_smartconfig_finish();
				break;
			case EV_FWUPGRADE_FINISHED:
				UART_PRINT("fw upgrade has finished with %s\n\r", ev.arg1? "success" : "error");
				reboot();
				break;
			default:
				UART_PRINT("unhandled event (%d, %d, %d, %d)\n\r", ev.id, ev.arg1, ev.arg2, ev.time);
			}
		}
	}
}
