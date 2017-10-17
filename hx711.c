// Smart Toybox scale interface (HX711)
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
#include "board.h"
#include "evqueue.h"
#include "debug.h"
#include "hx711.h"
#include "settings.h"
#include "led.h"

// gain: 128 or 64 for channel A; channel B works with 32 gain factor only
// channel selection is made by passing the appropriate gain
#define GAIN 128
#define GAIN_BITS 1 // 1 bit for gain 128, 3 bits for 64, 2 bits for 32

#define CALIB_STEPS	80
#define CALIB_STABILIZATION_STEPS	30
#define CALIB_WEIGHT 1510

long WEIGHT_THRESHOLD = 0;
long SCALE_OFFSET = 0;
long SCALE_FACTOR = 0;
signed char SCALE_ORIENTATION = 0;
char g_hx711calibration = 0;

// writes value to clock gpio
inline void write_clock(BOOL val)
{
	gpio_write(GPIO_HX_CLK, val);
}

// reads value from data gpio
inline BOOL read_data()
{
	return gpio_read(GPIO_HX_DATA);
}

static void power_down() {
	write_clock(0);
	delay_us(1);
	write_clock(1);
	delay_us(70); // must be > 60us
}

static void power_up() {
	write_clock(0);
}

long hx711_read() {
	unsigned i, data = 0;

//	MAP_UtilsDelay(80);

	// pulse the clock pin 24 times to read the data
	for (i = 0; i < 24; i++) {
		write_clock(1);
		delay_us(1);
		data <<= 1;
		data |= read_data();
		write_clock(0);
		delay_us(1);
	}

	// set the channel and the gain factor for the next reading using the clock pin
	for (i = 0; i < GAIN_BITS; i++) {
		write_clock(1);
		delay_us(1);
		write_clock(0);
		delay_us(1);
	}

	if (data & 0x800000) data |= 0xff000000;
	return (long)data;
}

static int first = 0;

long scalemin, scalemax;
static void ScaleCalibIntReq()
{
	if (first < CALIB_STEPS) {
		if (!first) {
			UART_PRINT("hx711 calibration\n\r");
			scalemin = scalemax = hx711_read();
			first = 1;
			gpio_clear_irq(GPIO_HX_DATA);
			return;
		}
		long val = hx711_read();
		UART_PRINT("%d: read val %x\n\r",first,val);
		if (val < scalemin) scalemin = val;
		else if (val > scalemax) scalemax = val;
		first++;
		gpio_clear_irq(GPIO_HX_DATA);
	}
	else {
		int cnt = first - CALIB_STEPS;
		if (!cnt) {
			long diff = (scalemax - scalemin) / 2;
			WEIGHT_THRESHOLD = labs(diff)+1;
			SCALE_OFFSET = scalemax - diff;
			UART_PRINT("min: %x max: %x diff: %d scaleoffset=%x\n\r",scalemin,scalemax,diff,SCALE_OFFSET);
			UART_PRINT("Please put %d gram weight on scale...\n\r", CALIB_WEIGHT);
			first++;
		}
		long val = hx711_read();
		gpio_clear_irq(GPIO_HX_DATA);
		if(!cnt) return;
		if(cnt == 1) {
			// waiting for weight
			//UART_PRINT("waiting\n\r");
			if (labs(val - SCALE_OFFSET) < ((CALIB_WEIGHT/10) * WEIGHT_THRESHOLD)) return;
			scalemin = scalemax = val;
			UART_PRINT("weight detected\n\r");
			first++;
			return;
		}
		first++;
		if (cnt < CALIB_STABILIZATION_STEPS) {
			UART_PRINT("stabilizing cnt=%d\n\r",cnt);
			return; //wait to stabilize
		}
		cnt -= CALIB_STABILIZATION_STEPS;
		if (cnt < CALIB_STEPS) {
			UART_PRINT("%d: read val %x\n\r",cnt,val);
			if (val < scalemin) scalemin = val;
			else if (val > scalemax) scalemax = val;
			return;
		}
		if (cnt == CALIB_STEPS) {
			long scalehigh = scalemax - ((scalemax - scalemin) / 2);
			long diff = scalehigh - SCALE_OFFSET;
			UART_PRINT("min: %x max: %x diff: %d scaleh=%x\n\r",scalemin,scalemax,diff,scalehigh);
			if (diff < 0) {
				SCALE_ORIENTATION = -1;
				diff = -diff;
			}
			else SCALE_ORIENTATION = 1;
			SCALE_FACTOR = diff / CALIB_WEIGHT;
			UART_PRINT("\n\r\n\rCALIBRATION FINISHED!\n\rWrite down these settings:\n\r------\n\r");
			UART_PRINT("scaleoffset=%d\n\rscalefactor=%d\n\rscaleorientation=%d\n\rscalethreshold=%d\n\r------\n\rRemove weight to write settings and reboot to normal operation\n\r", SCALE_OFFSET, SCALE_FACTOR, SCALE_ORIENTATION, WEIGHT_THRESHOLD);
		}
		if(cnt > CALIB_STEPS) {
			first--;
			if (labs(val - SCALE_OFFSET) > ((CALIB_WEIGHT/10) * WEIGHT_THRESHOLD)) return;
			evqueue_write(EV_CALIBRATION_FINISHED,0,0);
		}
		// ended
	}
}

static void ScaleIntReq()
{
	if (!first) UART_PRINT("hx711 normal operation\n\r");
	first = 1;
	static unsigned char cnt = 0;

	long val = hx711_read();
	// int w = weight_in_grams(val); if (w) UART_PRINT("HX711: raw=%ld grams = %d count = %d\n\r", val, w, cnt);
	if (labs(val - previous) > WEIGHT_THRESHOLD) {
		evqueue_write(cnt ? EV_WEIGHT : EV_WEIGHT_START, weight_in_grams(val), cnt);
		if (cnt < 255) cnt++;
	}
	else if (cnt) {
		evqueue_write(EV_WEIGHT_STOP, weight_in_grams(val), cnt);
		cnt = 0;
	}
	previous = val;
	gpio_clear_irq(GPIO_HX_DATA);
}

void hx711_init()
{
	gpio_mode(GPIO_HX_DATA, GPIO_MODE_INPUT);
	gpio_mode(GPIO_HX_CLK, GPIO_MODE_OUTPUT);

	power_down();
	power_up();

	SCALE_OFFSET = settings_get_int("scaleoffset", 0);
	SCALE_FACTOR = settings_get_int("scalefactor", 0);
	SCALE_ORIENTATION = settings_get_int("scaleorientation", 0);
	WEIGHT_THRESHOLD = settings_get_int("scalethreshold", 0);

	if(!SCALE_OFFSET && !SCALE_FACTOR && !SCALE_ORIENTATION && !WEIGHT_THRESHOLD) {
		g_hx711calibration = 1;
		led_blink_cnt(LED_YELLOW, 255);
		UART_PRINT("hx711 calibration\n\r");
	    // set interrupt for data pin to clibration
		gpio_attach_interrupt(GPIO_HX_DATA, ScaleCalibIntReq, GPIO_IRQMODE_FALLING_EDGE);
	} else {
		UART_PRINT("read hx711 settings:\n\rscaleoffset=%d\n\rscalefactor=%d\n\rscaleorientation=%d\n\rscalethreshold=%d\n\r", SCALE_OFFSET, SCALE_FACTOR, SCALE_ORIENTATION, WEIGHT_THRESHOLD);
	    // set interrupt for data pin
		gpio_attach_interrupt(GPIO_HX_DATA, ScaleIntReq, GPIO_IRQMODE_FALLING_EDGE);
	}
}
