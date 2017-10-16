// Smart Toybox game control interface
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

#include "stdlib.h"
#include "game.h"
#include "audioplay.h"
#include "mstime.h"
#include "STBreader.h"
#include "debug.h"
#include "string.h"
#include "hx711.h"
#include "led.h"

#ifdef DEBUG
#define GAME_DURATION 60000
#define GIMME_TIMER_DURATION 10000
#define GRAND_GULP_THRESHOLD_WEIGHT 200
#define GRAND_CHEW_THRESHOLD_WEIGHT 200
#define WIN_WEIGHT 1000 // in grams // TODO read from settings, detect while inactive
#else
#define GAME_DURATION 180000
#define GIMME_TIMER_DURATION 20000
#define GRAND_GULP_THRESHOLD_WEIGHT 500
#define GRAND_CHEW_THRESHOLD_WEIGHT 500
#define WIN_WEIGHT 1000 // in grams // TODO read from settings, detect while inactive
#endif

#define CHEW_THRESHOLD 1
#define GULP_THRESHOLD 3
#define COMPLAIN_THRESHOLD 3
#define CHEW_THRESHOLD_COUNT 2
#define GRAND_GULP_THRESHOLD_COUNT 30
#define CHANGE_TEMPO (7 * (GAME_DURATION) / 8) // x miliseconds untill game tempo will change
//#define MOOD_DELAY 2000 // time between changing mood and playing its sound

enum {
	GAME_GIMME_TIMER = 0,
	GAME_STOP_TIMER,
	GAME_TEMPO_TIMER
};

#define MIN_MOOD -3
#define MAX_MOOD +3

static unsigned short last_weight = 0;
BOOL pause = 0, started = 0;
static unsigned char tempo = 0;
static signed char mood = 0; // 0 is neutral, negative is bad mood, positive is good mood
//static long start_time = 0;

// if num is -1, random music of the given type is played
static void play_effect(unsigned char type, int num, BOOL now)
{
	//UART_PRINT("time from start: %d\n\r", time_get() - start_time);
	//if (((time_get() - start_time) < 2000) && type != GAME_START) {
	//	UART_PRINT("not playing effect in first 2 seconds! start_time = %d, now = %d\n\r", start_time, time_get());
	//	return;
	//}
	if(num == -1) num = rand() % stb_get_music_num(type);
	UART_PRINT("playing effect %d (num = %d now = %d count = %d)!\n\r", type, num, now, stb_get_music_num(type));
	if (now) {
		if (audioplay_playmusic(1, type, num)) {
			UART_PRINT("can't play effect %d!\n\r", type);
		}
	} else {
		if (audioplay_appendmusic(1, type, num)) {
			UART_PRINT("can't play effect %d!\n\r", type);
		}
	}
}

void game_set_mood_led()
{
	if (pause) {
		led_blink_cnt(LED_YELLOW, 255);
	} else {
		leds_off();
		switch (mood) {
		case -3: case -2:
			led_on(LED_RED, 1);
			break;
		case -1: case 0: case 1:
			led_on(LED_YELLOW, 1);
			break;
		case 2: case 3:
			led_on(LED_GREEN, 1);
			break;
		}
	}
}

static void change_mood_by(signed char factor)
{
	// TODO change mood based on weight!
	UART_PRINT("change_mood by factor = %d old mood = %d\n\r", mood, factor);
	if (factor) {
		signed char newmood = mood;
		newmood += factor;
		if (newmood >= MAX_MOOD) newmood = MAX_MOOD;
		if (newmood <= MIN_MOOD) newmood = MIN_MOOD;
		UART_PRINT("mood changed from %d to %d\n\r", mood, newmood);
//		if (newmood > 0 && newmood > mood)
//			timer_set(GAME_MOOD_TIMER, MOOD_DELAY, 0, play_mood);
//			// TODO create theme with 3 sounds for both pos and negative mood and than use lower play_effect call
//			// for now use 1 sound for pos & 1 sound for neg mood
//			// play_effect(GAME_GOOD_PROGRESS, newmood - 1, 0);
//		else if (newmood < 0 && newmood < mood)
//			play_effect(GAME_BAD_PROGRESS, -1, 0);
//			// play_effect(GAME_BAD_PROGRESS, -newmood - 1, 0);
		mood = newmood;
	}
	game_set_mood_led();
}

void game_pause()
{
	ASSERT(started);
	if (pause) {
		UART_PRINT("game pause: game already paused!\n\r");
		return;
	}

	pause = 1;
	UART_PRINT("game is paused!\n\r");

	audioplay_stop_all();
	timer_pause(GAME_GIMME_TIMER);
	timer_pause(GAME_STOP_TIMER);
	timer_pause(GAME_TEMPO_TIMER);

	game_set_mood_led();
}

void game_resume()
{
	ASSERT(started);
	if (!pause) {
		UART_PRINT("game resume: game was not paused!\n\r");
		return;
	}

	pause = 0;
	UART_PRINT("game is now resumed!\n\r");

	game_set_mood_led();

	audioplay_stop(1);

	if (audioplay_playmusic(0, GAME_MUSIC, tempo)) {
		UART_PRINT("can't play music!\n\r");
		return;
	}

	timer_resume(GAME_GIMME_TIMER);
	timer_resume(GAME_STOP_TIMER);
	timer_resume(GAME_TEMPO_TIMER);
}

BOOL game_start()
{
	ASSERT(!started); // TODO or ignore or restart?

	UART_PRINT("game start\n\r");

	// TODO what if the game is started with "full" toybox?
	last_weight = hx711_get_weight();

	tempo = 0;
	mood = 0;


	pause = 0;
	game_set_mood_led(); // init led

	//start_time = time_get();
	if (audioplay_playmusic(0, GAME_MUSIC, tempo)) {
		UART_PRINT("can't play music! (tempo = %d)\n\r", tempo);
		//return 0;
	}

	play_effect(GAME_START, 0, 1);
	// TODO allow weight processing and other game events only after starting sound has finished

	timer_set(GAME_GIMME_TIMER, GIMME_TIMER_DURATION, 255);
	timer_set(GAME_STOP_TIMER, GAME_DURATION, 1);
	timer_set(GAME_TEMPO_TIMER, CHANGE_TEMPO, 1);

	started = 1;

	return 1;
}

void game_stop()
{
	ASSERT(started);

	UART_PRINT("game stop\n\r");
	// TODO what if game was paused? finish game or delay?

	started = 0;

	leds_off();

	audioplay_stop_all();

	if (last_weight >= WIN_WEIGHT) {
		play_effect(GAME_WIN, -1, 0);
		// TODO other stuff for win?
	} else {
		play_effect(GAME_LOSE, -1, 0);
		// TODO other stuff for lose?
	}

	timer_stop(GAME_GIMME_TIMER);
	timer_stop(GAME_STOP_TIMER);
	timer_stop(GAME_TEMPO_TIMER);
}

static BOOL gulpnow = 1;
void game_process_weight_start(unsigned short weight, unsigned short count)
{
	ASSERT(started);

	gulpnow = 1;

	UART_PRINT("EV_WEIGHT_START: grams=%ld count = %d\n\r", weight, count);
	if (pause) {
		UART_PRINT("game_process_weight_start: game paused!\n\r");
		return;
	}
}

void game_process_weight(unsigned short weight, unsigned short count)
{
	ASSERT(started);
//	UART_PRINT("EV_WEIGHT: grams=%ld count = %d\n\r", weight, count);
	if (pause) {
		UART_PRINT("game_process_weight: game paused!\n\r");
		return;
	}

	static BOOL chewing = 0;
	if (count <= 1) chewing = 0;
	if (count >= CHEW_THRESHOLD_COUNT && weight > last_weight + CHEW_THRESHOLD) { // weight rising
		// UART_PRINT("====weight rising, count = %d\n\r", count);
		timer_reset(GAME_GIMME_TIMER);
		timer_delay(GAME_STOP_TIMER, 2000);
		// if (count >= CHEW_THRESHOLD_COUNT && !chewing) {
		if (!chewing) {
			chewing = 1;
			UART_PRINT("====start chewing\n\r");
			// simple version without grand events
			//play_effect(weight > last_weight + GRAND_CHEW_THRESHOLD_WEIGHT ? GAME_CHEW_GRAND : GAME_CHEW, -1, 1);
			play_effect(GAME_CHEW, -1, 1);
		}
	}
	if (count > 7) gulpnow = 0; // if chewing lasts long enough, it is better to start gulp once chewing (single) sound has finished
}

void game_process_weight_stop(unsigned short weight, unsigned short count)
{
	ASSERT(started);
	UART_PRINT("EV_WEIGHT_STOP: grams=%ld count = %d\n\r", weight, count);
	if (pause) {
		UART_PRINT("game_process_stop: game paused!\n\r");
		return;
	}

	if (count >= CHEW_THRESHOLD_COUNT) {
		//UART_PRINT("EV_WEIGHT_STOP: grams=%ld count = %d\n\r", weight, count);
		if (weight > last_weight + GULP_THRESHOLD) {
			timer_reset(GAME_GIMME_TIMER);
			UART_PRINT("====gulp\n\r");
			// simple version without grand events
//			BOOL grand = count > GRAND_GULP_THRESHOLD_COUNT || weight > last_weight + GRAND_GULP_THRESHOLD_WEIGHT;
//			if (grand) {
//				play_effect(GAME_GULP_GRAND, -1, gulpnow);
//				change_mood_by(2);
//			} else {
				play_effect(GAME_GULP, -1, gulpnow);
				change_mood_by(1);
//			}
		} else {
			//UART_PRINT("****** PROCESS WEIGHT STOP last weight = %d\n\r", last_weight);
			//if (time_get() - start_time >= 2000)
				audioplay_stop(1);
			UART_PRINT("weight <= last_weight\n\r");
			if (weight < last_weight - COMPLAIN_THRESHOLD) {
				UART_PRINT("====au\n\r");
				timer_delay(GAME_GIMME_TIMER, 1000);
				play_effect(GAME_COMPLAIN, -1, 1);
				change_mood_by(-1);
			}
		}
	}

	last_weight = weight;
}

void game_process_timer_expired(unsigned short timer)
{
	if(!game_active()) return;

	switch (timer) {
	case GAME_GIMME_TIMER:
		play_effect(GAME_GIMME, -1, 0);
		change_mood_by(-1);
		break;
	case GAME_TEMPO_TIMER:
		UART_PRINT("change tempo\n\r");
		//ASSERT(++tempo <= MAX_TEMPO);
		tempo = 1;
		if (audioplay_appendmusic(0, GAME_MUSIC, tempo))
			UART_PRINT("can't play music! (tempo = %d)\n\r", tempo);
		break;
	case GAME_STOP_TIMER:
		game_stop();
		break;
	};
}

void game_process_accel_flat(BOOL flat)
{
	ASSERT(started);

	UART_PRINT("game_process_accel_flat()\n\r");

	if (!flat) {
		UART_PRINT("play game_gardian!\n\r");
		game_pause();
		play_effect(GAME_GUARDIAN, -1, 1);
	} else {
		UART_PRINT("play game_gardian_off!\n\r");
		game_resume();
		play_effect(GAME_GUARDIAN_OFF, -1, 1);
	}
}
